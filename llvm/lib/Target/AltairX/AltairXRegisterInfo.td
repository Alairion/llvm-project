//===-- AltairXRegisterInfo.td - AltairX Register defs -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let Namespace = "AltairX" in
def AltairXRegAltNameIndex : RegAltNameIndex;

// Base class for register definition
class AltairXRegister<
  bits<6> enc, 
  string name, 
  list<string> alt,
  list<Register> subregs = [], 
  list<SubRegIndex> subregsi = []
> : Register<name, alt> {
  let Namespace = "AltairX";
  let HWEncoding{5-0} = enc;
  let SubRegs = subregs;
  let SubRegIndices = subregsi;
}

// Integer registers
class AltairXIntRegister<
  bits<6> enc, 
  string name, 
  list<string> alt, 
  list<Register> subregs = [],
  list<SubRegIndex> subregsi = []
> : AltairXRegister<enc, name, alt, subregs, subregsi> {
  let RegAltNameIndices = [AltairXRegAltNameIndex];
}

// Stack pointer
def R0 : AltairXIntRegister<0, "r0", ["sp"]>, DwarfRegNum<[0]>;

let Namespace = "AltairX" in {
  def ISubReg8 : SubRegIndex<8>;
  def ISubReg16 : SubRegIndex<16>;
  def ISubReg32 : SubRegIndex<32>;
}

// Args/ret
foreach i = 1...8 in {
  def R#i#b : AltairXIntRegister<i, "r"#i, ["a"#!sub(i, 1)]>, DwarfRegNum<[i]>;
  def R#i#w : AltairXIntRegister<i, "r"#i, ["a"#!sub(i, 1)], [!cast<Register>("R"#i#"b")], [ISubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXIntRegister<i, "r"#i, ["a"#!sub(i, 1)], [!cast<Register>("R"#i#"w")], [ISubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXIntRegister<i, "r"#i, ["a"#!sub(i, 1)], [!cast<Register>("R"#i#"d")], [ISubReg32]>, DwarfRegNum<[i]>;
}

// Non-volatile registers
foreach i = 9...20 in {
  def R#i#b : AltairXIntRegister<i, "r"#i, ["s"#!sub(i, 9)]>, DwarfRegNum<[i]>;
  def R#i#w : AltairXIntRegister<i, "r"#i, ["s"#!sub(i, 9)], [!cast<Register>("R"#i#"b")], [ISubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXIntRegister<i, "r"#i, ["s"#!sub(i, 9)], [!cast<Register>("R"#i#"w")], [ISubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXIntRegister<i, "r"#i, ["s"#!sub(i, 9)], [!cast<Register>("R"#i#"d")], [ISubReg32]>, DwarfRegNum<[i]>;
}

// Volatile registers
foreach i = 21...31 in {
  def R#i#b : AltairXIntRegister<i, "r"#i, ["t"#!sub(i, 21)]>, DwarfRegNum<[i]>;
  def R#i#w : AltairXIntRegister<i, "r"#i, ["t"#!sub(i, 21)], [!cast<Register>("R"#i#"b")], [ISubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXIntRegister<i, "r"#i, ["t"#!sub(i, 21)], [!cast<Register>("R"#i#"w")], [ISubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXIntRegister<i, "r"#i, ["t"#!sub(i, 21)], [!cast<Register>("R"#i#"d")], [ISubReg32]>, DwarfRegNum<[i]>;
}

// Volatile renamed registers
foreach i = 32...55 in {
  def R#i#b : AltairXIntRegister<i, "r"#i, ["n"#!sub(i, 32)]>, DwarfRegNum<[i]>;
  def R#i#w : AltairXIntRegister<i, "r"#i, ["n"#!sub(i, 32)], [!cast<Register>("R"#i#"b")], [ISubReg8]>, DwarfRegNum<[i]>;
  def R#i#d : AltairXIntRegister<i, "r"#i, ["n"#!sub(i, 32)], [!cast<Register>("R"#i#"w")], [ISubReg16]>, DwarfRegNum<[i]>;
  def R#i   : AltairXIntRegister<i, "r"#i, ["n"#!sub(i, 32)], [!cast<Register>("R"#i#"d")], [ISubReg32]>, DwarfRegNum<[i]>;
}

// Special registers
def R56 : AltairXIntRegister<56, "r56", ["acc"]>, DwarfRegNum<[56]>;
def R57 : AltairXIntRegister<57, "r57", ["bpq"]>, DwarfRegNum<[57]>;
def R58 : AltairXIntRegister<58, "r58", ["bf1"]>, DwarfRegNum<[58]>;
def R59 : AltairXIntRegister<59, "r59", ["bf2"]>, DwarfRegNum<[59]>;
def R60 : AltairXIntRegister<60, "r60", ["bl1"]>, DwarfRegNum<[60]>;
def R61 : AltairXIntRegister<61, "r61", ["bl2"]>, DwarfRegNum<[61]>;
def R62 : AltairXIntRegister<62, "r62", ["ba1"]>, DwarfRegNum<[62]>;

let isConstant = 1 in
def ZERO : AltairXIntRegister<63, "r63", ["zero"]>, DwarfRegNum<[63]>;

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order volatile renamed, volatile, non-volatile, others.
def GPIReg8 : RegisterClass<"AltairX", [i8], 8, (add
  (sequence "R%ub", 32, 55),
  (sequence "R%ub", 21, 31),
  (sequence "R%ub", 1, 8),
  (sequence "R%ub", 9, 20),
  R0, ZERO),
  AltairXRegAltNameIndex
>;

def GPIReg16 : RegisterClass<"AltairX", [i16], 16, (add
  (sequence "R%uw", 32, 55),
  (sequence "R%uw", 21, 31),
  (sequence "R%uw", 1, 8),
  (sequence "R%uw", 9, 20),
  R0, ZERO),
  AltairXRegAltNameIndex
>;

def GPIReg32 : RegisterClass<"AltairX", [i32], 32, (add
  (sequence "R%ud", 32, 55),
  (sequence "R%ud", 21, 31),
  (sequence "R%ud", 1, 8),
  (sequence "R%ud", 9, 20),
  R0, ZERO),
  AltairXRegAltNameIndex
>;

def GPIReg64 : RegisterClass<"AltairX", [i64], 64, (add
  (sequence "R%u", 32, 55),
  (sequence "R%u", 21, 31),
  (sequence "R%u", 1, 8),
  (sequence "R%u", 9, 20),
  R0, ZERO),
  AltairXRegAltNameIndex
>;

def GPIRegs : RegisterCategory<[GPIReg64, GPIReg32, GPIReg16, GPIReg8]>;

// Must contain only one register!
def SPReg64 : RegisterClass<"AltairX", [i64], 64, (add R0), AltairXRegAltNameIndex>;

// Quotien, remainder, and product (low), product (high)
def RQb : AltairXRegister<0, "Qb", []>, DwarfRegNum<[64]>;
def RQw : AltairXRegister<0, "Qw", [], [RQb], [ISubReg8]>, DwarfRegNum<[64]>;
def RQd : AltairXRegister<0, "Qd", [], [RQw], [ISubReg16]>, DwarfRegNum<[64]>;
def RQ  : AltairXRegister<0, "Q",  [], [RQd], [ISubReg32]>, DwarfRegNum<[64]>;

def RQRb : AltairXRegister<1, "QR", []>, DwarfRegNum<[65]>;
def RQRw : AltairXRegister<1, "QR", [], [RQRb], [ISubReg8]>, DwarfRegNum<[65]>;
def RQRd : AltairXRegister<1, "QR", [], [RQRw], [ISubReg16]>, DwarfRegNum<[65]>;
def RQR  : AltairXRegister<1, "QR", [], [RQRd], [ISubReg32]>, DwarfRegNum<[65]>;

def RPLb : AltairXRegister<2, "PL", []>, DwarfRegNum<[66]>;
def RPLw : AltairXRegister<2, "PL", [], [RPLb], [ISubReg8]>, DwarfRegNum<[66]>;
def RPLd : AltairXRegister<2, "PL", [], [RPLw], [ISubReg16]>, DwarfRegNum<[66]>;
def RPL  : AltairXRegister<2, "PL", [], [RPLd], [ISubReg32]>, DwarfRegNum<[66]>;

def RPHb : AltairXRegister<3, "PH", []>, DwarfRegNum<[67]>;
def RPHw : AltairXRegister<3, "PH", [], [RPHb], [ISubReg8]>, DwarfRegNum<[67]>;
def RPHd : AltairXRegister<3, "PH", [], [RPHw], [ISubReg16]>, DwarfRegNum<[67]>;
def RPH  : AltairXRegister<3, "PH", [], [RPHd], [ISubReg32]>, DwarfRegNum<[67]>;

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
def MDUReg8  : RegisterClass<"AltairX", [i8], 8, (add RQb, RQRb, RPLb, RPHb)>;

def MDURegs : RegisterCategory<[MDUReg64, MDUReg32, MDUReg16, MDUReg8]>;

// Comparison flag
def CmpFlag : AltairXRegister<0, "cmpflags", []>;

// CmpFlag.
def CmpFlagReg64 : RegisterClass<"AltairX", [i64], 64, (add CmpFlag)> {
  let CopyCost = -1;  // Don't allow copying of flag register
}

// internal regs
def LR : AltairXRegister<0, "LR", []>;
def BR : AltairXRegister<1, "BR", []>;

def RIReg32 : RegisterClass<"AltairX", [i32], 32, (add LR, BR)>;

// Registers that can be spilled one way or another.
// Some of them may need additional temp register to be stored in a GPIReg
def SpillableReg8 : RegisterClass<"AltairX", [i8], 8, (add GPIReg8, MDUReg8)>;
def SpillableReg16 : RegisterClass<"AltairX", [i16], 16, (add GPIReg16, MDUReg16)>;
def SpillableReg32 : RegisterClass<"AltairX", [i32], 32, (add GPIReg32, MDUReg32, RIReg32)>; // MDU regs
def SpillableReg64 : RegisterClass<"AltairX", [i64], 64, (add SPReg64, GPIReg64, MDUReg64)>;

// special regs...
// def AccumulatorRegister : RegisterClass<"AltairX", [i64], 64, (add R56)>:

//class AltairXF32Reg<bits<6> enc, bits<2> comp, string n, list<string> alt = [], list<Register> aliases> : AltairXReg<enc, n, alt> {
//  let VectorComp = comp;
//  let Aliases = aliases;
//}
//
//class AltairXV32Reg<bits<6> enc, string n, list<string> alt = [], list<Register> aliases> : AltairXReg<enc, n, alt> {
//}
//
//class AltairXF64Reg<bits<6> enc, string n, list<string> alt = [], list<Register> aliases> : AltairXReg<enc, n, alt> {
//}