#include "MipsAssemblyGenPass.h"

using namespace llvm;

#include <map>

namespace {

const int tab32[32] = {0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
                       8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31};

int log2_32(uint32_t value)
{
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return tab32[(uint32_t)(value * 0x07C4ACDD) >> 27];
}

bool isPowerOf2(uint32_t x)
{
    return x && !(x & (x - 1));
}

struct MipsAssemblyGenPass : public ModulePass
{
    enum Mips32Reg {
        ZERO,
        AT,
        V0,
        V1,
        A0,
        A1,
        A2,
        A3,
        T0,
        T1,
        T2,
        T3,
        T4,
        T5,
        T6,
        T7,
        S0,
        S1,
        S2,
        S3,
        S4,
        S5,
        S6,
        S7,
        T8,
        T9,
        K0,
        K1,
        GP,
        SP,
        FP,
        RA
    };

    static char ID;
    MipsAssemblyGenPass() : ModulePass(ID), ss(assemblyText) {}
    bool runOnModule(Module &M) override;
    void print(raw_ostream &O, const Module *M) const override { O << assemblyText; }

private:
    // General purpose registers for register allocation
    static const Mips32Reg RegPool[20];
    static const Mips32Reg TempReg[12];

    std::string        assemblyText;
    raw_string_ostream ss;

    DenseMap<const Value *, Mips32Reg> regAlloc;
    DenseMap<const Value *, uint32_t>  stackAlloc;
    uint32_t                           stackTop;
    DenseMap<const Value *, uint32_t>  globalAlloc;
    uint32_t                           globalTop;

    std::multimap<const BasicBlock *, const PHINode *>               phiBlocks;
    std::map<Mips32Reg, const Value *>                               FRegs;
    std::map<const BasicBlock *, std::map<Mips32Reg, const Value *>> BBToRegs;

    enum CFGNodeStatus { UNVISITED, VISITED, PROCESSED };
    DenseMap<const BasicBlock *, SmallPtrSet<const Value *, 32>> live, defs;
    DenseMap<const BasicBlock *, CFGNodeStatus>                  BBStatus;

    void genFunctionAsm(const Function *F);
    void genBasicBlockAsm(const BasicBlock *BB);
    void genInstructionAsm(const Instruction *I);
    void globalAllocate(const Module *M);
    void stackAllocate(const Function *F);
    void registerAllocate(const Function *F);
    void recordPhiBlock(const Function *F);
    void analysisLiveness(const Function *F);
    void addLiveInToUserBlock(const Value *value, const User *user);
    bool dfsCFGLiveSet(const BasicBlock *BB);
    bool needRegister(const Instruction *I);

    void                genFName(const Function *F);
    void                genBBName(const BasicBlock *BB);
    void                genJump(const Instruction *I, const BasicBlock *toBB);
    bool                genPhi(const BasicBlock *BB, const Value *cond);
    friend raw_ostream &operator<<(raw_ostream &os, Mips32Reg reg)
    {
        const char *RegNames[32] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
                                    "t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                                    "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7",
                                    "t8",   "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
        os << '$' << RegNames[reg];
        return os;
    }
};

char MipsAssemblyGenPass::ID = 0;

const MipsAssemblyGenPass::Mips32Reg MipsAssemblyGenPass::RegPool[20] = {
    S0, S1, T0, T1, S2, S3, T2, T3, S4, S5, T4, T5, S6, S7, T6, T7, T8, T9, K0, K1};

const MipsAssemblyGenPass::Mips32Reg MipsAssemblyGenPass::TempReg[12] =
    {T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, K0, K1};

bool MipsAssemblyGenPass::runOnModule(Module &M)
{
    assemblyText.clear();
    globalAllocate(&M);

    ss << "\n.text\n";

    auto funcMain = M.getFunction("main");
    if (funcMain && !funcMain->isDeclaration()) {
        ss << "\tjal ";
        genFName(funcMain);
        ss << "\n\taddiu " << V0 << ", " << ZERO << ", " << 10 << "\n\tsyscall\n\n";

        ss << "F_write:\n"
           << "\taddiu " << V0 << ", " << ZERO << ", " << 1 << '\n'
           << "\tsyscall\n"
           << "\tjr " << RA << "\n\n"
           << "F_putchar:\n"
           << "\taddiu " << V0 << ", " << ZERO << ", " << 11 << '\n'
           << "\tsyscall\n"
           << "\tjr " << RA << "\n\n";

        genFunctionAsm(funcMain);
    }

    for (const auto &F : M.functions()) {
        if (&F != funcMain && !F.isDeclaration())
            genFunctionAsm(&F);
    }

    ss.flush();
    return false;
}

void MipsAssemblyGenPass::genFunctionAsm(const Function *F)
{
    ss << "# function: " << F->getName() << '\n';

    analysisLiveness(F);

    // Allocate stack storage value
    stackAllocate(F);

    // Record argument register
    regAlloc.clear();
    int argIndex = 0;
    for (const auto &arg : F->args()) {
        if (argIndex < 4)
            regAlloc[&arg] = Mips32Reg(A0 + argIndex++);
        else {
            regAlloc[&arg] = ZERO;
            ss << "# unsupported argcount > 4, assigning " << arg.getName() << " to $0\n";
        }
    }

    // Allocate value into register using graph coloring algorithm
    registerAllocate(F);

    // Associate PHINode to its incoming BasicBlock
    recordPhiBlock(F);

    genFName(F);
    ss << ":\n";

    // Align to 4 bytes
    stackTop         = (stackTop + 3) & ~3;
    int saveRegCount = 2;
    // Save Stack Frame, Reg
    for (int saveReg = S0; saveReg <= S7; saveReg++) {
        if (FRegs.find(Mips32Reg(saveReg)) != FRegs.end())
            saveRegCount++;
    }

    ss << "\taddiu " << SP << ", " << SP << ", -" << (4 * saveRegCount + stackTop)
       << '\n';
    ss << "\tsw " << RA << ", " << stackTop << '(' << SP << ")\n";
    ss << "\tsw " << FP << ", " << stackTop + 4 << '(' << SP << ")\n";
    for (int saveReg = S0; saveReg <= S7; saveReg++) {
        if (FRegs.find(Mips32Reg(saveReg)) != FRegs.end()) {
            ss << "\tsw " << Mips32Reg(saveReg) << ", "
               << stackTop + 8 + 4 * (saveReg - S0) << '(' << SP << ")\n";
        }
    }
    ss << "\tmove " << FP << ", " << SP << '\n';

    for (const auto &BB : F->getBasicBlockList()) {
        genBasicBlockAsm(&BB);
    }

    ss << '_' << F->getName() << "_RET:\n";

    // Restore Stack Frame, Reg
    ss << "\tmove " << SP << ", " << FP << '\n';
    for (int saveReg = S0; saveReg <= S7; saveReg++) {
        if (FRegs.find(Mips32Reg(saveReg)) != FRegs.end()) {
            ss << "\tlw " << Mips32Reg(saveReg) << ", "
               << stackTop + 8 + 4 * (saveReg - S0) << '(' << SP << ")\n";
        }
    }
    ss << "\tlw " << FP << ", " << stackTop + 4 << '(' << SP << ")\n";
    ss << "\tlw " << RA << ", " << stackTop << '(' << SP << ")\n";
    ss << "\taddiu " << SP << ", " << SP << ", " << (4 * saveRegCount + stackTop) << '\n';

    ss << "\tjr " << RA << "\n\n";
}  // namespace

void MipsAssemblyGenPass::genBasicBlockAsm(const BasicBlock *BB)
{
    genBBName(BB);
    ss << ":\n";
    for (const auto &I : BB->getInstList()) {
        genInstructionAsm(&I);
    }
}

void MipsAssemblyGenPass::genInstructionAsm(const Instruction *I)
{
    if (isa<AllocaInst>(I) || isa<PHINode>(I)) {
        // pass
        return;
    }
    else if (auto GEPInst = dyn_cast<GetElementPtrInst>(I)) {
        auto       address = GEPInst->getPointerOperand();
        DataLayout dataLayout(I->getFunction()->getParent());
        APInt      offset(32, 0, true);

        if (GEPInst->accumulateConstantOffset(dataLayout, offset)) {
            ss << "\t #constant offset GEP not supported\n";
        }
        else {
            if (globalAlloc.find(address) != globalAlloc.end())
                ss << "\taddiu " << regAlloc[I] << ", " << GP << ", "
                   << globalAlloc[address] << '\n';
            else if (!regAlloc[address])
                ss << "\taddiu " << regAlloc[I] << ", " << FP << ", "
                   << stackAlloc[address] << '\n';
            else if (regAlloc[address] == V0)
                ss << "\tlw " << regAlloc[I] << ", " << stackAlloc[address] << '(' << FP
                   << ")\n";
            else
                ss << "\tmove " << regAlloc[I] << ", " << regAlloc[address] << '\n';

            auto elemType = GEPInst->getSourceElementType();
            for (auto indexIt = GEPInst->idx_begin();;) {
                uint32_t  elemSize   = dataLayout.getTypeAllocSize(elemType);
                Value *   indexValue = indexIt->get();
                Mips32Reg regIndex;

                if (auto constIdx = dyn_cast<ConstantInt>(indexValue)) {
                    if (constIdx->getValue().isNullValue()) {
                        goto next_index;
                    }
                    else {
                        regIndex = V1;
                        ss << "\taddiu " << regIndex << ", " << ZERO << ", "
                           << constIdx->getSExtValue() << '\n';
                    }
                }
                else {
                    regIndex = regAlloc[indexValue];
                    if (regIndex == V0) {
                        regIndex = V1;
                        ss << "\tlw " << regIndex << ", " << stackAlloc[indexValue] << '('
                           << FP << ")\n";
                    }
                }

                if (isPowerOf2(elemSize)) {
                    uint32_t sizeShift = log2_32(elemSize);
                    ss << "\tsll " << V1 << ", " << regIndex << ", " << sizeShift << '\n';
                }
                else {
                    ss << "\taddiu " << AT << ", " << ZERO << ", " << elemSize << '\n';
                    ss << "\tmul " << V1 << ", " << regIndex << ", " << AT << '\n';
                }

                ss << "\taddu " << regAlloc[I] << ", " << regAlloc[I] << ", " << V1
                   << '\n';

            next_index:
                if (++indexIt == GEPInst->idx_end())
                    break;

                if (auto arrayType = dyn_cast<ArrayType>(elemType)) {
                    elemType = arrayType->getArrayElementType();
                }
                else {
                    ss << "\t # unsupported type in GEP\n";
                    elemType->print(ss);
                    ss << '\n';
                    break;
                }
            }
        }

        if (regAlloc[I] == V0)
            ss << "\tsw " << V0 << ", " << stackAlloc[I] << '(' << FP << ")\n";
    }
    else if (auto loadInst = dyn_cast<LoadInst>(I)) {
        auto  address = loadInst->getOperand(0);
        APInt offset(32, 0, true);

        if (auto GEP = dyn_cast_or_null<GEPOperator>(dyn_cast<ConstantExpr>(address))) {
            auto       GEPAddress = GEP->getPointerOperand();
            DataLayout dataLayout(I->getFunction()->getParent());

            if (globalAlloc.find(GEPAddress) == globalAlloc.end())
                ss << "\t #wrong address operand\n";

            if (!GEP->accumulateConstantOffset(dataLayout, offset))
                ss << "\t #GEP not constant\n";

            address = GEPAddress;
        }

        // Is load from global variable?
        if (globalAlloc.find(address) != globalAlloc.end()) {
            ss << "\tlw " << regAlloc[I] << ", "
               << offset.getSExtValue() + globalAlloc[address] << '(' << GP << ")\n";

            if (regAlloc[I] == V0)
                ss << "\tsw " << V0 << ", " << stackAlloc[I] << '(' << FP << ")\n";
        }
        // Is load from address in register?
        else if (regAlloc[address]) {
            if (regAlloc[address] == V0)
                ss << "\tlw " << V0 << ", " << stackAlloc[address] << '(' << FP << ")\n";

            ss << "\tlw " << regAlloc[I] << ", 0(" << regAlloc[address] << ")\n";

            if (regAlloc[I] == V0)
                ss << "\tsw " << V0 << ", " << stackAlloc[I] << '(' << FP << ")\n";
        }
        // Load from stack address
        else {
            if (stackAlloc.find(address) == stackAlloc.end())
                ss << "\t# load address invalid\n";

            // Load destination spilled, delay load
            if (regAlloc[I] == V0) {
                stackAlloc[I] = stackAlloc[address];
            }
            else {
                ss << "\tlw " << regAlloc[I] << ", " << stackAlloc[address] << '(' << FP
                   << ")\n";
            }
        }
    }
    else if (auto storeInst = dyn_cast<StoreInst>(I)) {
        auto  value   = storeInst->getOperand(0);
        auto  address = storeInst->getOperand(1);
        APInt offset(32, 0, true);

        if (auto GEP = dyn_cast_or_null<GEPOperator>(dyn_cast<ConstantExpr>(address))) {
            auto       GEPAddress = GEP->getPointerOperand();
            DataLayout dataLayout(I->getFunction()->getParent());

            if (globalAlloc.find(GEPAddress) == globalAlloc.end())
                ss << "\t #wrong address operand\n";

            if (!GEP->accumulateConstantOffset(dataLayout, offset))
                ss << "\t #GEP not constant\n";

            address = GEPAddress;
        }

        if (auto constant = dyn_cast<ConstantInt>(value)) {
            ss << "\taddi " << V0 << ", " << ZERO << ", " << constant->getSExtValue()
               << '\n';
            ss << "\tsw " << V0 << ", ";
        }
        else {
            if (regAlloc[value] == V0)
                ss << "\tlw " << V0 << ", " << stackAlloc[value] << '(' << FP << ")\n";

            ss << "\tsw " << regAlloc[value] << ", ";
        }

        if (globalAlloc.find(address) != globalAlloc.end())
            ss << offset.getSExtValue() + globalAlloc[address] << '(' << GP << ")\n";
        else if (!regAlloc[address])
            ss << stackAlloc[address] << '(' << FP << ")\n";
        else {
            auto regAddr = regAlloc[address];
            if (regAddr == V0) {
                ss << "\tlw " << V1 << ", " << stackAlloc[address] << '(' << FP << ")\n";
                regAddr = V1;
            }

            ss << "0(" << regAddr << ")\n";
        }
    }
    else if (auto binaryOpInst = dyn_cast<BinaryOperator>(I)) {
        auto binaryOp = binaryOpInst->getOpcode();
        auto op1      = binaryOpInst->getOperand(0);
        auto op2      = binaryOpInst->getOperand(1);

        auto constant1 = dyn_cast<ConstantInt>(op1);
        auto constant2 = dyn_cast<ConstantInt>(op2);
        auto constant  = constant1 ? constant1 : constant2;

        if (constant1 && constant2) {
            ss << "\t # unsupported binary instruction with two constant operand\n";
        }
        else {
            auto reg1 = regAlloc[op1];
            auto reg2 = (regAlloc[op2] == V0 ? V1 : regAlloc[op2]);
            auto reg  = constant1 ? reg2 : reg1;

            if (!constant1 && reg1 == V0)
                ss << "\tlw " << reg1 << ", " << stackAlloc[op1] << '(' << FP << ")\n";
            if (!constant2 && reg2 == V1)
                ss << "\tlw " << reg2 << ", " << stackAlloc[op2] << '(' << FP << ")\n";

            switch (binaryOp) {
            case Instruction::Add:
                if (constant) {
                    ss << "\taddiu " << regAlloc[I] << ", " << reg << ", "
                       << constant->getSExtValue() << '\n';
                }
                else {
                    ss << "\taddu " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                break;
            case Instruction::Sub:
                if (constant1) {
                    if (constant1->getValue().isNullValue()) {
                        ss << "\tsubu " << regAlloc[I] << ", " << ZERO << ", " << reg2
                           << '\n';
                    }
                    else {
                        ss << "\taddiu " << V0 << ", " << ZERO << ", -"
                           << constant2->getSExtValue() << '\n';
                        ss << "\tsubu " << regAlloc[I] << ", " << V0 << ", " << reg2
                           << '\n';
                    }
                }
                else if (constant2) {
                    ss << "\taddu " << regAlloc[I] << ", " << reg1 << ", -"
                       << constant2->getSExtValue() << '\n';
                }
                else {
                    ss << "\tsubu " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                break;
            case Instruction::Mul:
                if (constant1) {
                    reg1 = V0;
                    ss << "\taddiu " << reg1 << ", " << ZERO << ", "
                       << constant1->getSExtValue() << '\n';
                }
                else if (constant2) {
                    reg2 = V1;
                    ss << "\taddiu " << reg2 << ", " << ZERO << ", "
                       << constant2->getSExtValue() << '\n';
                }

                ss << "\tmul " << regAlloc[I] << ", " << reg1 << ", " << reg2 << '\n';
                break;
            case Instruction::Shl:
                if (constant1) {
                    reg1 = V0;
                    ss << "\taddiu " << reg1 << ", " << ZERO << ", "
                       << constant1->getSExtValue() << '\n';
                    ss << "\tsllv " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                else if (constant2) {
                    ss << "\tsll " << regAlloc[I] << ", " << reg1 << ", "
                       << (constant2->getZExtValue() & 31) << '\n';
                }
                else {
                    ss << "\tsllv " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                break;
            case Instruction::LShr:
                if (constant1) {
                    reg1 = V0;
                    ss << "\taddiu " << reg1 << ", " << ZERO << ", "
                       << constant1->getSExtValue() << '\n';
                    ss << "\tsrlv " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                else if (constant2) {
                    ss << "\tsrl " << regAlloc[I] << ", " << reg1 << ", "
                       << (constant2->getZExtValue() & 31) << '\n';
                }
                else {
                    ss << "\tsrlv " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                break;
            case Instruction::AShr:
                if (constant1) {
                    reg1 = V0;
                    ss << "\taddiu " << reg1 << ", " << ZERO << ", "
                       << constant1->getSExtValue() << '\n';
                    ss << "\tsrav " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                else if (constant2) {
                    ss << "\tsra " << regAlloc[I] << ", " << reg1 << ", "
                       << (constant2->getZExtValue() & 31) << '\n';
                }
                else {
                    ss << "\tsrav " << regAlloc[I] << ", " << reg1 << ", " << reg2
                       << '\n';
                }
                break;
            case Instruction::And:
                if (constant) {
                    ss << "\tandi " << regAlloc[I] << ", " << reg << ", "
                       << constant->getSExtValue() << '\n';
                }
                else {
                    ss << "\tand " << regAlloc[I] << ", " << reg1 << ", " << reg2 << '\n';
                }
                break;
            case Instruction::Or:
                if (constant) {
                    ss << "\tori " << regAlloc[I] << ", " << reg << ", "
                       << constant->getSExtValue() << '\n';
                }
                else {
                    ss << "\tor " << regAlloc[I] << ", " << reg1 << ", " << reg2 << '\n';
                }
                break;

            default:
                ss << "\t # unsupported binary instruction\n";
            }

            if (regAlloc[I] == V0)
                ss << "\tsw " << V0 << ", " << stackAlloc[I] << '(' << FP << ")\n";
        }
    }
    else if (auto icmpInst = dyn_cast<ICmpInst>(I)) {
        auto op1 = icmpInst->getOperand(0);
        auto op2 = icmpInst->getOperand(1);

        auto constant1 = dyn_cast<ConstantInt>(op1);
        auto constant2 = dyn_cast<ConstantInt>(op2);

        if (constant1 && constant2) {
            ss << "\t # unsupported integer compare instruction with two constant "
                  "operand\n";
        }
        else {
            auto reg1 = regAlloc[op1];
            auto reg2 = (regAlloc[op2] == V0 ? V1 : regAlloc[op2]);
            auto reg  = constant1 ? reg2 : reg1;

            if (!constant1 && reg1 == V0)
                ss << "\tlw " << reg1 << ", " << stackAlloc[op1] << '(' << FP << ")\n";
            if (!constant2 && reg2 == V1)
                ss << "\tlw " << reg2 << ", " << stackAlloc[op2] << '(' << FP << ")\n";

            if (constant1) {
                reg1 = V0;
                ss << "\taddiu " << reg1 << ", " << ZERO << ", "
                   << constant1->getSExtValue() << '\n';
            }
            else if (constant2) {
                reg2 = V1;
                ss << "\taddiu " << reg2 << ", " << ZERO << ", "
                   << constant2->getSExtValue() << '\n';
            }

            const char *relation;
            switch (icmpInst->getPredicate()) {
            case ICmpInst::ICMP_SGT:
            case ICmpInst::ICMP_SGE:
            case ICmpInst::ICMP_SLT:
            case ICmpInst::ICMP_SLE:
                relation = "\tslt ";
                break;
            default:
                relation = "\tsltu ";
                break;
            }

            switch (icmpInst->getPredicate()) {
            case ICmpInst::ICMP_EQ:
                ss << "\tsub " << regAlloc[I] << ", " << reg1 << ", " << reg2 << '\n';
                ss << "\tsltu " << regAlloc[I] << ", " << ZERO << ", " << regAlloc[I]
                   << '\n';
                ss << "\tnegu " << regAlloc[I] << ", " << regAlloc[I] << '\n';
                ss << "\taddiu " << regAlloc[I] << ", " << regAlloc[I] << ", 1\n";
                break;
            case ICmpInst::ICMP_NE:
                ss << "\tsub " << regAlloc[I] << ", " << reg1 << ", " << reg2 << '\n';
                break;
            case ICmpInst::ICMP_UGT:
            case ICmpInst::ICMP_SGT:
                ss << relation << regAlloc[I] << ", " << reg2 << ", " << reg1 << '\n';
                break;
            case ICmpInst::ICMP_UGE:
            case ICmpInst::ICMP_SGE:
                ss << relation << regAlloc[I] << ", " << reg1 << ", " << reg2 << '\n';
                ss << "\tsltu " << regAlloc[I] << ", " << ZERO << ", " << regAlloc[I]
                   << '\n';
                ss << "\tnegu " << regAlloc[I] << ", " << regAlloc[I] << '\n';
                ss << "\taddiu " << regAlloc[I] << ", " << regAlloc[I] << ", 1\n";
                break;
            case ICmpInst::ICMP_ULT:
            case ICmpInst::ICMP_SLT:
                ss << relation << regAlloc[I] << ", " << reg1 << ", " << reg2 << '\n';
                break;
            case ICmpInst::ICMP_ULE:
            case ICmpInst::ICMP_SLE:
                ss << relation << regAlloc[I] << ", " << reg2 << ", " << reg1 << '\n';
                ss << "\tsltu " << regAlloc[I] << ", " << ZERO << ", " << regAlloc[I]
                   << '\n';
                ss << "\tnegu " << regAlloc[I] << ", " << regAlloc[I] << '\n';
                ss << "\taddiu " << regAlloc[I] << ", " << regAlloc[I] << ", 1\n";
                break;
            default:
                ss << "\t # unsupported integer compare instruction\n";
            }

            if (regAlloc[I] == V0)
                ss << "\tsw " << V0 << ", " << stackAlloc[I] << '(' << FP << ")\n";
        }
    }
    else if (auto branchInst = dyn_cast<BranchInst>(I)) {
        if (branchInst->isUnconditional()) {
            genPhi(I->getParent(), nullptr);
            genJump(I, branchInst->getSuccessor(0));
        }
        else {
            auto condition = branchInst->getCondition();

            if (isa<ConstantInt>(condition)) {
                ss << "\t # unsupported branch instruction with constant condition\n";
            }
            else {
                bool      condSaved = genPhi(I->getParent(), condition);
                Mips32Reg condReg   = condSaved ? V0 : regAlloc[condition];

                if (!condSaved && condReg == V0)
                    ss << "\tlw " << V0 << ", " << stackAlloc[condition] << '(' << FP
                       << ")\n";

                if (branchInst->getSuccessor(0) == I->getParent()->getNextNode()) {
                    ss << "\tbeq " << condReg << ", " << ZERO << ", ";
                    genBBName(branchInst->getSuccessor(1));
                    ss << '\n';
                    genJump(I, branchInst->getSuccessor(0));
                }
                else {
                    ss << "\tbne " << condReg << ", " << ZERO << ", ";
                    genBBName(branchInst->getSuccessor(0));
                    ss << '\n';
                    genJump(I, branchInst->getSuccessor(1));
                }
            }
        }
    }
    else if (auto selectInst = dyn_cast<SelectInst>(I)) {
        auto cond   = selectInst->getCondition();
        auto tValue = selectInst->getTrueValue();
        auto fValue = selectInst->getFalseValue();

        if (auto tConstant = dyn_cast<ConstantInt>(tValue))
            ss << "\taddiu " << regAlloc[I] << ", " << ZERO << ", "
               << tConstant->getZExtValue() << '\n';
        else if (regAlloc[tValue] == V0)
            ss << "\tlw " << regAlloc[I] << ", " << stackAlloc[tValue] << '(' << FP
               << ")\n";
        else
            ss << "\tmove " << regAlloc[I] << ", " << regAlloc[tValue] << '\n';

        auto regCond = regAlloc[cond];
        if (regCond == V0) {
            regCond = AT;
            ss << "\tlw " << regCond << ", " << stackAlloc[cond] << '(' << FP << ")\n";
        }

        // Jump to false branch if condition is zero
        ss << "\tbeq " << regCond << ", " << ZERO << ", ";
        genBBName(I->getParent());
        ss << '_' << reinterpret_cast<uint64_t>(fValue) << "_\n";
        // Jump to true branch
        ss << "\tj ";
        genBBName(I->getParent());
        ss << '_' << reinterpret_cast<uint64_t>(tValue) << "_\n";

        // False branch
        genBBName(I->getParent());
        ss << '_' << reinterpret_cast<uint64_t>(fValue) << "_:\n";

        if (auto fConstant = dyn_cast<ConstantInt>(fValue))
            ss << "\taddiu " << regAlloc[I] << ", " << ZERO << ", "
               << fConstant->getZExtValue() << '\n';
        else if (regAlloc[fValue] == V0)
            ss << "\tlw " << regAlloc[I] << ", " << stackAlloc[fValue] << '(' << FP
               << ")\n";
        else
            ss << "\tmove " << regAlloc[I] << ", " << regAlloc[fValue] << '\n';

        // True/merge branch
        genBBName(I->getParent());
        ss << '_' << reinterpret_cast<uint64_t>(tValue) << "_:\n";

        if (regAlloc[I] == V0)
            ss << "\tsw " << V0 << ", " << stackAlloc[I] << '(' << FP << ")\n";
    }
    else if (auto returnInst = dyn_cast<ReturnInst>(I)) {
        auto retValue = returnInst->getReturnValue();
        if (retValue) {
            if (auto constRV = dyn_cast<ConstantInt>(retValue)) {
                ss << "\taddi " << V0 << ", " << ZERO << ", " << constRV->getSExtValue()
                   << '\n';
            }
            else if (regAlloc[retValue] == V0) {
                ss << "\tlw " << V0 << ", " << stackAlloc[retValue] << '(' << FP << ")\n";
            }
            else {
                ss << "\tmove " << V0 << ", " << regAlloc[retValue] << '\n';
            }
        }
        genJump(I, nullptr);
    }
    else if (auto callInst = dyn_cast<CallInst>(I)) {
        if (callInst->arg_size() > 4) {
            ss << "\t # unsupported function call with more than 4 arguments\n";
        }
        else {
            int   callerArgcount = 0;
            int   tempRegCount   = 0;
            auto &BBRegs         = BBToRegs[I->getParent()];

            // Save temp registers
            for (auto tempReg : TempReg) {
                if (!callInst->getCalledFunction()->getReturnType()->isVoidTy()
                    && tempReg == regAlloc[I])
                    continue;

                if (BBRegs.find(tempReg) != BBRegs.end()) {
                    ss << "\tsw " << tempReg << ", -" << ++tempRegCount * 4 << '(' << SP
                       << ")\n";
                }
            }

            // Save argument registers
            for (int i = 0; i < callInst->getCaller()->arg_size(); i++) {
                if (BBRegs.find(Mips32Reg(A0 + i)) != BBRegs.end()) {
                    ss << "\tsw " << Mips32Reg(A0 + i) << ", -"
                       << (++callerArgcount + tempRegCount) * 4 << "(" << SP << ")\n";
                }
            }

            // Adjust stack
            if (callerArgcount + tempRegCount > 0)
                ss << "\taddiu " << SP << ", " << SP << ", -"
                   << (callerArgcount + tempRegCount) * 4 << '\n';

            // Set function arguments
            for (int i = 0; i < callInst->arg_size(); i++) {
                auto argOp = callInst->getArgOperand(i);
                if (auto constArg = dyn_cast<ConstantInt>(argOp)) {
                    ss << "\taddiu " << Mips32Reg(A0 + i) << ", " << ZERO << ", "
                       << constArg->getSExtValue() << '\n';
                }
                else if (regAlloc[argOp] == V0) {
                    ss << "\tlw " << Mips32Reg(A0 + i) << ", " << stackAlloc[argOp] << '('
                       << FP << ")\n";
                }
                else {
                    ss << "\tmove " << Mips32Reg(A0 + i) << ", " << regAlloc[argOp]
                       << '\n';
                }
            }

            ss << "\tjal ";
            genFName(callInst->getCalledFunction());
            ss << '\n';

            // Restore stack
            if (callerArgcount + tempRegCount > 0)
                ss << "\taddiu " << SP << ", " << SP << ", "
                   << (callerArgcount + tempRegCount) * 4 << '\n';

            // Restore temp registers
            tempRegCount = 0;
            for (auto tempReg : TempReg) {
                if (!callInst->getCalledFunction()->getReturnType()->isVoidTy()
                    && tempReg == regAlloc[I])
                    continue;

                if (BBRegs.find(tempReg) != BBRegs.end()) {
                    ss << "\tlw " << tempReg << ", -" << ++tempRegCount * 4 << '(' << SP
                       << ")\n";
                }
            }

            // Restore argument registers
            callerArgcount = 0;
            for (int i = 0; i < callInst->getCaller()->arg_size(); i++) {
                if (BBRegs.find(Mips32Reg(A0 + i)) != BBRegs.end()) {
                    ss << "\tlw " << Mips32Reg(A0 + i) << ", -"
                       << (++callerArgcount + tempRegCount) * 4 << "(" << SP << ")\n";
                }
            }

            // Save function return value
            if (!callInst->getCalledFunction()->getReturnType()->isVoidTy()) {
                if (regAlloc[I] == V0)
                    ss << "\tsw " << V0 << ", " << stackAlloc[I] << '(' << FP << ")\n";
                else
                    ss << "\tmove " << regAlloc[I] << ", " << V0 << '\n';
            }
        }
    }
    else if (isa<SExtInst>(I) || isa<ZExtInst>(I)) {
        regAlloc[I]   = regAlloc[I->getOperand(0)];
        stackAlloc[I] = stackAlloc[I->getOperand(0)];
    }
    else {
        ss << "\t # unsupported instruction\n";
    }
}

void MipsAssemblyGenPass::globalAllocate(const Module *M)
{
    globalAlloc.clear();
    globalTop = 0;

    DataLayout dataLayout(M);

    for (const auto &global : M->getGlobalList()) {
        // Skip exteral variable
        if (global.isDeclaration())
            continue;

        uint32_t size  = dataLayout.getTypeAllocSize(global.getType()->getElementType());
        uint32_t align = global.getAlignment();

        // Make global allocation aligned
        globalTop            = (globalTop + align - 1) & ~(align - 1);
        globalAlloc[&global] = globalTop;
        globalTop += size;

        ss << "# " << globalAlloc[&global] << ", size " << size << ", align " << align
           << " : " << global.getName() << '\n';
    }
}

void MipsAssemblyGenPass::stackAllocate(const Function *F)
{
    stackAlloc.clear();
    stackTop = 0;

    DataLayout dataLayout(F->getParent());

    for (const_inst_iterator It = inst_begin(F), E = inst_end(F); It != E; It++) {
        if (auto allocInst = dyn_cast<AllocaInst>(&*It)) {
            uint32_t size  = dataLayout.getTypeAllocSize(allocInst->getAllocatedType());
            uint32_t align = allocInst->getAlignment();

            // Make stack allocation aligned
            stackTop              = (stackTop + align - 1) & ~(align - 1);
            stackAlloc[allocInst] = stackTop;
            stackTop += size;

            // ss << "# stack alloc: " << allocInst->getName() << " = "
            //    << stackAlloc[allocInst] << '\n';
            ss << "# " << stackAlloc[allocInst] << ", size " << size << ", align "
               << align << " : " << allocInst->getName() << '\n';
        }
    }
}

void MipsAssemblyGenPass::registerAllocate(const Function *F)
{
    BBToRegs.clear();
    int maxRegUseCount = 0;
    int regSpillCount  = 0;

    for (const auto &BB : F->getBasicBlockList()) {
        int regUseCount = 0;

        auto &BBRegs = BBToRegs[&BB];
        for (auto value : live[&BB]) {
            if (regAlloc.find(value) != regAlloc.end() && regAlloc[value] != V0) {
                BBRegs[regAlloc[value]] = value;
                regUseCount++;
            }
            else if (auto arg = dyn_cast<Argument>(value)) {
                if (arg->getArgNo() < 4) {
                    BBRegs[Mips32Reg(A0 + arg->getArgNo())] = value;
                    regUseCount++;
                }
            }
        }

        for (const auto &Inst : BB.getInstList()) {
            auto I = &Inst;

            if (!needRegister(I))
                continue;

            for (Mips32Reg reg : RegPool) {
                if (BBRegs.find(reg) == BBRegs.end()) {
                    regAlloc[I] = reg;
                    BBRegs[reg] = I;
                    regUseCount++;
                    break;
                }
            }

            if (regAlloc[I])
                continue;

            // Allocate value on stack
            regAlloc[I] = V0;
            regSpillCount++;

            // Load instruction of constant address can be delayed
            if (auto loadInst = dyn_cast<LoadInst>(I)) {
                auto address = loadInst->getOperand(0);
                if (!regAlloc[address])
                    continue;
            }

            // Make stack allocation aligned
            uint32_t size = 4, align = 4;
            stackTop      = (stackTop + align - 1) & ~(align - 1);
            stackAlloc[I] = stackTop;
            stackTop += size;
        }

        // Merge basicblock registers uses into function registers
        for (auto regKV : BBRegs) {
            FRegs[regKV.first] = regKV.second;
        }

        maxRegUseCount = std::max(maxRegUseCount, regUseCount);

        ss << "# BB <" << BB.getName() << "> alive value:\n";
        for (auto value : live[&BB]) {
            ss << "#\t" << regAlloc[value] << ", ";
            value->print(ss);
            ss << '\n';
        }
    }

    ss << "# max register used: " << maxRegUseCount
       << ", register spill: " << regSpillCount << '\n';
}

void MipsAssemblyGenPass::recordPhiBlock(const Function *F)
{
    phiBlocks.clear();
    for (const_inst_iterator It = inst_begin(F), E = inst_end(F); It != E; It++) {
        if (auto PHIInst = dyn_cast<PHINode>(&*It)) {
            for (BasicBlock *inBlock : PHIInst->blocks()) {
                phiBlocks.insert({inBlock, PHIInst});
            }
        }
    }
}

bool MipsAssemblyGenPass::needRegister(const Instruction *I)
{
    // Skip instruction which returns void
    if (isa<AllocaInst>(I) || isa<StoreInst>(I) || isa<BranchInst>(I)
        || isa<ReturnInst>(I) || isa<SExtInst>(I) || isa<ZExtInst>(I))
        return false;

    // Skip call insturction of a function returning void
    if (auto callInst = dyn_cast<CallInst>(I)) {
        if (callInst->getCalledFunction()->getReturnType()->isVoidTy())
            return false;
    }

    // Skip constant GEP instruction
    if (auto GEPInst = dyn_cast<GetElementPtrInst>(I)) {
        // Constant GEP instruction result can be computed directly
        if (GEPInst->hasAllConstantIndices() && !regAlloc[GEPInst->getPointerOperand()])
            return false;
    }

    return true;
}

void MipsAssemblyGenPass::genFName(const Function *F)
{
    ss << "F_" << F->getName();
}

void MipsAssemblyGenPass::genBBName(const BasicBlock *BB)
{
    ss << '_' << BB->getParent()->getName() << "_BB_";
    for (char c : BB->getName()) {
        if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9'
            || c == '_')
            ss << c;
        else if (c == '.')
            ss << "__";
        else
            ss << '_' << (int)c;
    }
}

void MipsAssemblyGenPass::genJump(const Instruction *I, const BasicBlock *toBB)
{
    if (toBB) {
        if (I->getParent()->getNextNode() != toBB) {
            ss << "\tj ";
            genBBName(toBB);
            ss << '\n';
        }
    }
    else {
        if (I->getParent()->getNextNode() != toBB) {
            ss << "\tj _" << I->getFunction()->getName() << "_RET\n";
        }
    }
}

bool MipsAssemblyGenPass::genPhi(const BasicBlock *BB, const Value *cond)
{
    auto pbRange   = phiBlocks.equal_range(BB);
    bool condSaved = false;

    SmallPtrSet<const Value *, 32>     phiValues;
    DenseMap<const Value *, Mips32Reg> collideRegs;
    DenseMap<const Value *, int>       collideStack;
    int                                stackTop = 4;

    for (auto pbIt = pbRange.first; pbIt != pbRange.second; pbIt++) {
        phiValues.insert(pbIt->second);
    }

    for (auto pbIt = pbRange.first; pbIt != pbRange.second; pbIt++) {
        auto PHIInst    = pbIt->second;
        auto blockValue = PHIInst->getIncomingValueForBlock(BB);

        if (phiValues.find(blockValue) == phiValues.end())
            continue;

        if (regAlloc[PHIInst] == V0)
            continue;

        auto &BBRegs = BBToRegs[BB];
        for (Mips32Reg reg : RegPool) {
            if (BBRegs.find(reg) == BBRegs.end()) {
                collideRegs[PHIInst] = reg;
                BBRegs[reg]          = PHIInst;
                break;
            }
        }

        if (!collideRegs[PHIInst]) {
            collideRegs[PHIInst] = V0;
            // Make stack allocation aligned
            uint32_t size = 4, align = 4;
            stackTop              = (stackTop + align - 1) & ~(align - 1);
            collideStack[PHIInst] = stackTop;
            stackTop += size;
        }

        if (auto constant = dyn_cast<ConstantInt>(blockValue)) {
            ss << "\taddiu " << collideRegs[PHIInst] << ", " << ZERO << ", "
               << constant->getSExtValue() << '\n';

            if (collideRegs[PHIInst] == V0)
                ss << "\tsw " << V0 << ", " << collideStack[PHIInst] << '(' << FP
                   << ")\n";
        }
        else {
            if (regAlloc[blockValue] == V0)
                ss << "\tlw " << V0 << ", " << stackAlloc[blockValue] << '(' << FP
                   << ")\n";

            if (collideRegs[PHIInst] == V0)
                ss << "\tsw " << regAlloc[blockValue] << ", " << collideStack[PHIInst]
                   << '(' << FP << ")\n";
            else
                ss << "\tmove " << collideRegs[PHIInst] << ", " << regAlloc[blockValue]
                   << '\n';
        }
    }

    for (auto pbIt = pbRange.first; pbIt != pbRange.second; pbIt++) {
        auto PHIInst    = pbIt->second;
        auto blockValue = PHIInst->getIncomingValueForBlock(BB);

        // Save condition to temporary register when PHI collides with condition
        if (!condSaved && cond && regAlloc[cond] != V0
            && regAlloc[PHIInst] == regAlloc[cond]) {
            ss << "\tmove " << V0 << ", " << regAlloc[cond] << '\n';
            condSaved = true;
        }

        if (collideRegs[PHIInst]) {
            if (collideRegs[PHIInst] == V0)
                ss << "\tlw " << V0 << ", " << collideStack[blockValue] << '(' << FP
                   << ")\n";

            if (regAlloc[PHIInst] == V0)
                ss << "\tsw " << collideRegs[PHIInst] << ", " << stackAlloc[PHIInst]
                   << '(' << FP << ")\n";
            else
                ss << "\tmove " << regAlloc[PHIInst] << ", " << collideRegs[PHIInst]
                   << '\n';
        }
        else {
            if (auto constant = dyn_cast<ConstantInt>(blockValue)) {
                ss << "\taddiu " << regAlloc[PHIInst] << ", " << ZERO << ", "
                   << constant->getSExtValue() << '\n';

                if (regAlloc[PHIInst] == V0)
                    ss << "\tsw " << V0 << ", " << stackAlloc[PHIInst] << '(' << FP
                       << ")\n";
            }
            else {
                if (regAlloc[blockValue] == V0)
                    ss << "\tlw " << V0 << ", " << stackAlloc[blockValue] << '(' << FP
                       << ")\n";

                if (regAlloc[PHIInst] == V0)
                    ss << "\tsw " << regAlloc[blockValue] << ", " << stackAlloc[PHIInst]
                       << '(' << FP << ")\n";
                else
                    ss << "\tmove " << regAlloc[PHIInst] << ", " << regAlloc[blockValue]
                       << '\n';
            }
        }
    }

    return condSaved;
}

void MipsAssemblyGenPass::analysisLiveness(const Function *F)
{
    live.clear();
    defs.clear();

    // Function argument def-use chains
    for (const auto &arg : F->args()) {
        for (auto user : arg.users())
            addLiveInToUserBlock(&arg, user);
    }

    // Instruction result def-use chains
    for (const auto &BB : F->getBasicBlockList()) {
        for (const auto &I : BB) {
            if (!needRegister(&I))
                continue;

            defs[&BB].insert(&I);

            for (auto user : I.users())
                addLiveInToUserBlock(&I, user);
        }
    }

    // Back propagate variable live set until stable
    do {
        BBStatus.clear();
    } while (dfsCFGLiveSet(&F->getEntryBlock()));
}

void MipsAssemblyGenPass::addLiveInToUserBlock(const Value *value, const User *user)
{
    if (auto PHIInst = dyn_cast<PHINode>(user)) {
        for (const auto &incomeValue : PHIInst->incoming_values()) {
            if (value == incomeValue) {
                auto incomeBB = PHIInst->getIncomingBlock(incomeValue);
                // if (auto Inst = dyn_cast<Instruction>(value)) {
                //     if (Inst->getParent() == incomeBB)
                //         return;
                // }
                live[incomeBB].insert(value);
            }
        }
    }
    else {
        auto userInst = cast<Instruction>(user);
        // if (auto Inst = dyn_cast<Instruction>(value)) {
        //     if (Inst->getParent() == userInst->getParent())
        //         return;
        // }
        live[userInst->getParent()].insert(value);
    }
}

bool MipsAssemblyGenPass::dfsCFGLiveSet(const BasicBlock *BB)
{
    BBStatus[BB] = VISITED;

    bool changed = false;
    for (auto SuccBB : successors(BB)) {
        // Avoid processing back edge
        if (BBStatus[SuccBB] != UNVISITED)
            continue;

        changed |= dfsCFGLiveSet(SuccBB);
    }

    // Compute live for this BB using formula:
    // Live(BB) = union(Live(S) - Defs(S)) for S in succ(BB) + Defs(BB)
    for (auto SuccBB : successors(BB)) {
        changed |= set_union(live[BB], set_difference(live[SuccBB], defs[SuccBB]));
    }

    changed |= set_union(live[BB], defs[BB]);
    BBStatus[BB] = PROCESSED;
    return changed;
}

static RegisterPass<MipsAssemblyGenPass>
    X("MipsAsmGen", "MIPS Assembly Generation Pass", true, true);

}  // namespace

llvm::ModulePass *createMipsAssemblyGenPass()
{
    return new MipsAssemblyGenPass;
}