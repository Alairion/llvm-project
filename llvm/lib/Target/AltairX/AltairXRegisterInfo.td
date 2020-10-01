//===-- AltairXRegisterInfo.td - AltairX Register defs -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


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
> : AltairXRegister<enc, name, alt, subregs, subregsi> {}

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
def R63 : AltairXIntRegister<63, "r63", ["ba2"]>, DwarfRegNum<[63]>;

// Quotien, remainder, and product (low), product (high)
def RQ : AltairXRegister<0, "Q", []>, DwarfRegNum<[64]>;
def RQR : AltairXRegister<1, "QR", []>, DwarfRegNum<[65]>;
def RPL : AltairXRegister<2, "PL", []>, DwarfRegNum<[66]>;
def RPH : AltairXRegister<3, "PH", []>, DwarfRegNum<[67]>;

def CmpFlag : AltairXRegister<0, "cmpflags", []>;

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order volatile renamed, volatile, non-volatile, others.
def SPReg64 : RegisterClass<"AltairX", [i64], 64, (add R0)>;

def GPIReg8 : RegisterClass<"AltairX", [i8], 8, (add
  (sequence "R%ub", 32, 55),
  (sequence "R%ub", 1, 8),
  (sequence "R%ub", 21, 31),
  (sequence "R%ub", 9, 20)
)>;

def GPIReg16 : RegisterClass<"AltairX", [i16], 16, (add
  (sequence "R%uw", 32, 55),
  (sequence "R%uw", 1, 8),
  (sequence "R%uw", 21, 31),
  (sequence "R%uw", 9, 20)
)>;

def GPIReg32 : RegisterClass<"AltairX", [i32], 32, (add
  (sequence "R%ud", 32, 55),
  (sequence "R%ud", 1, 8),
  (sequence "R%ud", 21, 31),
  (sequence "R%ud", 9, 20)
)>;

def GPIReg64 : RegisterClass<"AltairX", [i64], 64, (add
  (sequence "R%u", 32, 55),
  (sequence "R%u", 1, 8),
  (sequence "R%u", 21, 31),
  (sequence "R%u", 9, 20)
)>;

// special regs...
// def AccumulatorRegister : RegisterClass<"AltairX", [i64], 64, (add R56)>:

// CmpFlag.
// def CmpFlagReg64 : RegisterClass<"AltairX", [i64], 64, (add CmpFlag)> {
//   let CopyCost = -1;  // Don't allow copying of flag register
//   let isAllocatable = 0;
// }

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