//===-- AltairXRegisterInfo.td - AltairX Register defs -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let Namespace = "AltairX" in
def AltairXRegPrettyNameIndex : RegAltNameIndex;

// Base class for register definition
class AltairXRegister<
  bits<6> enc,
  string name,
  string prettyname,
  list<Register> subregs = [],
  list<SubRegIndex> subregsi = []
> : Register<name, [prettyname]> {
  let Namespace = "AltairX";
  let HWEncoding{5-0} = enc;
  let SubRegs = subregs;
  let SubRegIndices = subregsi;
  let RegAltNameIndices = [AltairXRegPrettyNameIndex];
}

// Stack pointer
def R0 : AltairXRegister<0, "r0", "sp">, DwarfRegNum<[0]>;

let Namespace = "AltairX" in {
  def SubReg8  : SubRegIndex<8>;
  def SubReg16 : SubRegIndex<16>;
  def SubReg32 : SubRegIndex<32>;
}

// Args/ret
foreach i = 1...8 in {
  def R#i#b : AltairXRegister<i, "r"#i, "a"#!sub(i, 1)>, DwarfRegNum<[i]>;
  def R#i#w : AltairXRegister<i, "r"#i, "a"#!sub(i, 1), [!cast<Register>("R"#i#"b")], [SubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXRegister<i, "r"#i, "a"#!sub(i, 1), [!cast<Register>("R"#i#"w")], [SubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXRegister<i, "r"#i, "a"#!sub(i, 1), [!cast<Register>("R"#i#"d")], [SubReg32]>, DwarfRegNum<[i]>;
}

// Non-volatile registers
foreach i = 9...19 in {
  def R#i#b : AltairXRegister<i, "r"#i, "s"#!sub(i, 9)>, DwarfRegNum<[i]>;
  def R#i#w : AltairXRegister<i, "r"#i, "s"#!sub(i, 9), [!cast<Register>("R"#i#"b")], [SubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXRegister<i, "r"#i, "s"#!sub(i, 9), [!cast<Register>("R"#i#"w")], [SubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXRegister<i, "r"#i, "s"#!sub(i, 9), [!cast<Register>("R"#i#"d")], [SubReg32]>, DwarfRegNum<[i]>;
}

// Volatile registers
foreach i = 20...30 in {
  def R#i#b : AltairXRegister<i, "r"#i, "t"#!sub(i, 20)>, DwarfRegNum<[i]>;
  def R#i#w : AltairXRegister<i, "r"#i, "t"#!sub(i, 20), [!cast<Register>("R"#i#"b")], [SubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXRegister<i, "r"#i, "t"#!sub(i, 20), [!cast<Register>("R"#i#"w")], [SubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXRegister<i, "r"#i, "t"#!sub(i, 20), [!cast<Register>("R"#i#"d")], [SubReg32]>, DwarfRegNum<[i]>;
}

// Link register
def R31 : AltairXRegister<31, "r31", "lr">, DwarfRegNum<[31]>;

// Volatile renamed registers
foreach i = 32...55 in {
  def R#i#b : AltairXRegister<i, "r"#i, "n"#!sub(i, 32)>, DwarfRegNum<[i]>;
  def R#i#w : AltairXRegister<i, "r"#i, "n"#!sub(i, 32), [!cast<Register>("R"#i#"b")], [SubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXRegister<i, "r"#i, "n"#!sub(i, 32), [!cast<Register>("R"#i#"w")], [SubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXRegister<i, "r"#i, "n"#!sub(i, 32), [!cast<Register>("R"#i#"d")], [SubReg32]>, DwarfRegNum<[i]>;
}

// Special registers
def R56 : AltairXRegister<56, "r56", "acc">, DwarfRegNum<[56]>;
def R57 : AltairXRegister<57, "r57", "ba1">, DwarfRegNum<[57]>;
def R58 : AltairXRegister<58, "r58", "ba2">, DwarfRegNum<[58]>;
def R59 : AltairXRegister<59, "r59", "bf1">, DwarfRegNum<[59]>;
def R60 : AltairXRegister<60, "r60", "bf2">, DwarfRegNum<[60]>;
def R61 : AltairXRegister<61, "r61", "bl1">, DwarfRegNum<[61]>;
def R62 : AltairXRegister<62, "r62", "bl2">, DwarfRegNum<[62]>;

let isConstant = 1 in
def ZERO : AltairXRegister<63, "r63", "zero">, DwarfRegNum<[63]>;

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order volatile renamed, volatile, non-volatile, others.
def GPIReg8 : RegisterClass<"AltairX", [i8], 8, (add
  (sequence "R%ub", 32, 55),
  (sequence "R%ub", 21, 30),
  (sequence "R%ub", 1, 8),
  (sequence "R%ub", 9, 20),
  ZERO)
>;

def GPIReg16 : RegisterClass<"AltairX", [i16], 16, (add
  (sequence "R%uw", 32, 55),
  (sequence "R%uw", 21, 30),
  (sequence "R%uw", 1, 8),
  (sequence "R%uw", 9, 20),
  ZERO)
>;

def GPIReg32 : RegisterClass<"AltairX", [i32], 32, (add
  (sequence "R%ud", 32, 55),
  (sequence "R%ud", 21, 30),
  (sequence "R%ud", 1, 8),
  (sequence "R%ud", 9, 20),
  ZERO)
>;

def GPIReg64 : RegisterClass<"AltairX", [i64], 64, (add
  R0,
  (sequence "R%u", 32, 55),
  (sequence "R%u", 21, 30),
  R31,
  (sequence "R%u", 1, 8),
  (sequence "R%u", 9, 20),
  ZERO)
>;

def GPIRegs : RegisterCategory<[GPIReg64, GPIReg32, GPIReg16, GPIReg8]>;

// Must contain only one register!
// def SPReg64 : RegisterClass<"AltairX", [i64], 64, (add R0)>;
// def LRReg64 : RegisterClass<"AltairX", [i64], 64, (add R31)>;

// Quotien, remainder, and product (low), product (high)
def RQb : AltairXRegister<0, "Q", "Q">, DwarfRegNum<[64]>;
def RQw : AltairXRegister<0, "Q", "Q", [RQb], [SubReg8]>, DwarfRegNum<[64]>;
def RQd : AltairXRegister<0, "Q", "Q", [RQw], [SubReg16]>, DwarfRegNum<[64]>;
def RQ  : AltairXRegister<0, "Q", "Q", [RQd], [SubReg32]>, DwarfRegNum<[64]>;

def RQRb : AltairXRegister<1, "QR", "QR">, DwarfRegNum<[65]>;
def RQRw : AltairXRegister<1, "QR", "QR", [RQRb], [SubReg8]>, DwarfRegNum<[65]>;
def RQRd : AltairXRegister<1, "QR", "QR", [RQRw], [SubReg16]>, DwarfRegNum<[65]>;
def RQR  : AltairXRegister<1, "QR", "QR", [RQRd], [SubReg32]>, DwarfRegNum<[65]>;

def RPLb : AltairXRegister<2, "PL", "PL">, DwarfRegNum<[66]>;
def RPLw : AltairXRegister<2, "PL", "PL", [RPLb], [SubReg8]>, DwarfRegNum<[66]>;
def RPLd : AltairXRegister<2, "PL", "PL", [RPLw], [SubReg16]>, DwarfRegNum<[66]>;
def RPL  : AltairXRegister<2, "PL", "PL", [RPLd], [SubReg32]>, DwarfRegNum<[66]>;

def RPHb : AltairXRegister<3, "PH", "PH">, DwarfRegNum<[67]>;
def RPHw : AltairXRegister<3, "PH", "PH", [RPHb], [SubReg8]>, DwarfRegNum<[67]>;
def RPHd : AltairXRegister<3, "PH", "PH", [RPHw], [SubReg16]>, DwarfRegNum<[67]>;
def RPH  : AltairXRegister<3, "PH", "PH", [RPHd], [SubReg32]>, DwarfRegNum<[67]>;

def QReg64 : RegisterClass<"AltairX", [i64], 64, (add RQ)>;
def QReg32 : RegisterClass<"AltairX", [i32], 32, (add RQd)>;
def QReg16 : RegisterClass<"AltairX", [i16], 16, (add RQw)>;
def QReg8  : RegisterClass<"AltairX", [i8],  8,  (add RQb)>;

def QRReg64 : RegisterClass<"AltairX", [i64], 64, (add RQR)>;
def QRReg32 : RegisterClass<"AltairX", [i32], 32, (add RQRd)>;
def QRReg16 : RegisterClass<"AltairX", [i16], 16, (add RQRw)>;
def QRReg8  : RegisterClass<"AltairX", [i8],  8,  (add RQRb)>;

def PLReg64 : RegisterClass<"AltairX", [i64], 64, (add RPL)>;
def PLReg32 : RegisterClass<"AltairX", [i32], 32, (add RPLd)>;
def PLReg16 : RegisterClass<"AltairX", [i16], 16, (add RPLw)>;
def PLReg8  : RegisterClass<"AltairX", [i8],  8,  (add RPLb)>;

def PHReg64 : RegisterClass<"AltairX", [i64], 64, (add RPH)>;
def PHReg32 : RegisterClass<"AltairX", [i32], 32, (add RPHd)>;
def PHReg16 : RegisterClass<"AltairX", [i16], 16, (add RPHw)>;
def PHReg8  : RegisterClass<"AltairX", [i8],  8,  (add RPHb)>;

def MDUReg64 : RegisterClass<"AltairX", [i64], 64, (add RQ, RQR, RPL, RPH)>;
def MDUReg32 : RegisterClass<"AltairX", [i32], 32, (add RQd, RQRd, RPLd, RPHd)>;
def MDUReg16 : RegisterClass<"AltairX", [i16], 16, (add RQw, RQRw, RPLw, RPHw)>;
def MDUReg8  : RegisterClass<"AltairX", [i8],  8, (add RQb, RQRb, RPLb, RPHb)>;

def MDURegs : RegisterCategory<[MDUReg64, MDUReg32, MDUReg16, MDUReg8]>;

// internal regs
// flag register
def FR : AltairXRegister<0, "fr", "fr">;
def RIReg32 : RegisterClass<"AltairX", [i32], 32, (add FR)>{
  let CopyCost = -1; // Disable implicit copy of these registers
  let isAllocatable = 0;
}

// Registers that can be spilled one way or another.
// Some of them may need additional temp register to be stored in a GPIReg
def SpillableReg8  : RegisterClass<"AltairX", [i8], 8, (add GPIReg8)>;
def SpillableReg16 : RegisterClass<"AltairX", [i16], 16, (add GPIReg16)>;
def SpillableReg32 : RegisterClass<"AltairX", [i32], 32, (add GPIReg32)>;
def SpillableReg64 : RegisterClass<"AltairX", [i64], 64, (add GPIReg64)>;

// Floating-point registers
// Base class for float register definition (simplier than int registers)

// We have 56 128-bits registers.
// So F0 may be a single float, a vector of float (1 to 4 values), or a single double-precision register.
// They can also hold a i64 value, this is needed for int <-> float conversions and bitcasts
foreach i = 0...56 in {
  def F#i : AltairXRegister<i, "v"#i, "v"#i>, DwarfRegNum<[i]>;
  def D#i : AltairXRegister<i, "v"#i, "v"#i, [!cast<Register>("F"#i)], [SubReg32]>, DwarfRegNum<[i]>;

  let Aliases = [!cast<Register>("F"#i)] in {
    def VI#i#b : AltairXRegister<i, "v"#i, "v"#i>, DwarfRegNum<[i]>;
  }
}

let isConstant = 1 in
def FZERO : AltairXRegister<63, "v63", "zero">, DwarfRegNum<[63]>;

def FReg32 : RegisterClass<"AltairX", [f32], 32, (add
  (sequence "F%u", 0, 55),
  FZERO
)>;

def FReg64 : RegisterClass<"AltairX", [f64], 64, (add
  (sequence "D%u", 0, 55),
  FZERO
)>;

// used by fcmove and fs[e|lt]
def VIReg8 : RegisterClass<"AltairX", [i8], 8, (add
  (sequence "VI%ub", 0, 55)
)>;

def FRegs : RegisterCategory<[FReg32, FReg64]>;

def VQs : AltairXRegister<0, "VQ", "Q">, DwarfRegNum<[64]>;

let Aliases = [VQs] in // aliases are bidirectional, so VQs also has VQd as an alias!
def VQd : AltairXRegister<0, "VQ", "Q">, DwarfRegNum<[64]>;

let isAllocatable = 0 in
def EFReg32 : RegisterClass<"AltairX", [f32], 32, (add VQs)>;

def EFReg64 : RegisterClass<"AltairX", [f64], 64, (add VQd)>;

//def VQ32 : AltairXRegister<0, "VQ", "Q">, DwarfRegNum<[64]>;
//def VQ64 : AltairXRegister<0, "VQ", "Q", [VQ32], [SubReg32]>, DwarfRegNum<[64]>;

//let isAllocatable = 0 in {
//  def EFReg32 : RegisterClass<"AltairX", [f32], 32, (add VQ32)>;
//  def EFReg64 : RegisterClass<"AltairX", [f64], 64, (add VQ64)>;
//}