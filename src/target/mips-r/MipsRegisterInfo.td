// Mips-r Register Infomation

class MipsReg<bits<16> Enc, string n> : Register<n> {
    let HWEncoding = Enc;
    let Namespace = "MIPS";
}

// MIPS 32-bit general purpose register
class MipsGPRReg<bits<16> Enc, string n> : MipsReg<Enc, n>;

// Registers
let Namespace = "MIPS" in {
    foreach i = 0-31 in {
        def R#i : MipsGPRReg<i, "$"#i>;
    }
}

// Register Classes
class GPR32Class : RegisterClass<"MIPS", [i32], 32, (add
    R0, R1, R2, R3, R4, R5, R6, R7,
    R8, R9, R10, R11, R12, R13, R14, R15,
    R16, R17, R18, R19, R20, R21, R22, R23, 
    R24, R25, R26, R27, R28, R29, R30, R31)>;

def GPR32Zero : RegisterClass<"Mips", [i32], 32, (add R0)>;

def GPR32NonZero : RegisterClass<"MIPS", [i32], 32, (add
    R1, R2, R3, R4, R5, R6, R7,
    R8, R9, R10, R11, R12, R13, R14, R15,
    R16, R17, R18, R19, R20, R21, R22, R23, 
    R24, R25, R26, R27, R28, R29, R30, R31)>;