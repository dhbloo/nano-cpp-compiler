// Mips-r 32 Calling Convention

def RetCC_Mips : CallingConv<[
    // Promote i1/i8/i16 return values to i32.
    CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,

    // First 2 function return values are assigned to $v0(R2) and $v1(R3)
    CCIfType<[i32], CCAssignToReg<[R2, R3]>>,
    
    // Extra return values are assigned to stack, 4 bytes per argument
    // and 4 bytes alignment.
    CCIfType<[i32], CCAssignToStack<[4, 4]>>
]>;

def CC_Mips : CallingConv<[
    // Promote i1/i8/i16 return values to i32.
    CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,

    // First 4 arguments are assigned to $a0(R4) - $a3(R7)
    CCIfType<[i32], CCAssignToReg<[R4, R5, R6, R7]>>,

    // Extra arguments are assigned to stack, 4 bytes per argument
    // and 4 bytes alignment.
    CCIfType<[i32], CCAssignToStack<[4, 4]>>
]>;

def CSR_Mips : CalleeSavedRegs<(add
    // Saved registers $s0(R16) - $s7(R23)
    R16, R17, R18, R19, R20, R21, R22, R23,

    // Frame pointer $fp(R30)
    R30,

    // Return address $ra(R31)
    R31
)>;