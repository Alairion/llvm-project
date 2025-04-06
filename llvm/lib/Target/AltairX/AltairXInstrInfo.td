//== AltairXInstrInfo.td - Target Description for AltairX Target -*- tablegen -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This file contains the AltairX implementation of the TargetInstrInfo class.
//===----------------------------------------------------------------------===//

include "AltairXInstrFormats.td"

//===----------------------------------------------------------------------===//
// Summary:
// - Common defitions
// - Procedure return
// - Procedure calling
// - ALU basics operations
// - sext, zext and trunc
// - Constant move
// - Load
// - Store
// - Comparisons
// - Branch
//===----------------------------------------------------------------------===//

// Integer x-bits immediate
multiclass SizedSImm<int size> {
  def s64imm#size : ImmLeaf<i64, [{ return isInt<64>(Imm); }]>;
  def s32imm#size : ImmLeaf<i32, [{ return isInt<32>(Imm); }]>;
  def s16imm#size : ImmLeaf<i16, [{ return isInt<16>(Imm); }]>;
  def s8imm#size  : ImmLeaf<i8,  [{ return isInt<8>(Imm); }]>;
}

defm "" : SizedSImm<9>;
defm "" : SizedSImm<10>;

multiclass SizedUImm<int size> {
  def u64imm#size : ImmLeaf<i64, [{ return isUInt<64>(Imm); }]>;
  def u32imm#size : ImmLeaf<i32, [{ return isUInt<32>(Imm); }]>;
  def u16imm#size : ImmLeaf<i16, [{ return isUInt<16>(Imm); }]>;
  def u8imm#size  : ImmLeaf<i8,  [{ return isUInt<8>(Imm); }]>;
}

// Addressing mode
def addr : ComplexPattern<iPTR, 3, "selectAddr", [], []>;
def addrimm : ComplexPattern<iPTR, 2, "selectAddrImm", [], []>;
def addrimmsp : ComplexPattern<iPTR, 2, "selectAddrImmSP", [], []>;

//===----------------------------------------------------------------------===//
// Procedure calling
//===----------------------------------------------------------------------===//

// Procedure calling
// These are target-independent nodes, but have target-specific formats.
def SDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart, [SDNPHasChain, SDNPOutGlue]>;
def SDT_CallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END", SDT_CallSeqEnd, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

let Defs = [R0], Uses = [R0] in {
def ADJCALLSTACKDOWN : AltairXPseudo<
  (outs), 
  (ins i64imm:$amt1, i64imm:$amt2),
  [(callseq_start timm:$amt1, timm:$amt2)]
  >;

def ADJCALLSTACKUP : AltairXPseudo<
  (outs), 
  (ins i64imm:$amt1, i64imm:$amt2),
  [(callseq_end timm:$amt1, timm:$amt2)]
  >;
}

//===----------------------------------------------------------------------===//
// ALU basics operations
//===----------------------------------------------------------------------===//

class ALUInstRR<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode, RegisterClass regclass>
  : AltairXInstALURR<opcode, size,
    (outs regclass:$rd), 
    (ins regclass:$rs1, regclass:$rs2),
    [(set regclass:$rd, (opnode regclass:$rs1, regclass:$rs2))],
    name, "$rd, $rs1, $rs2"> {
}

class ALUInstRI<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode, RegisterClass regclass, Operand immop, ImmLeaf immpat>
  : AltairXInstALURI<opcode, size,
    (outs regclass:$rd), 
    (ins regclass:$rs1, immop:$imm),
    [(set regclass:$rd, (opnode regclass:$rs1, immpat:$imm))],
    name, "$rd, $rs1, $imm"> {
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
multiclass ALUInst<string name, AltairXOpcode opcode, SDNode opnode> {
  def RR  : ALUInstRR<!strconcat(name, ".q"),  3, opcode, opnode, GPIReg64>;
  def RI  : ALUInstRI<!strconcat(name, "i.q"), 3, opcode, opnode, GPIReg64, i64imm, s64imm9>;
  def RRd : ALUInstRR<!strconcat(name, ".d"),  2, opcode, opnode, GPIReg32>;
  def RId : ALUInstRI<!strconcat(name, "i.d"), 2, opcode, opnode, GPIReg32, i32imm, s32imm9>;
  def RRw : ALUInstRR<!strconcat(name, ".w"),  1, opcode, opnode, GPIReg16>;
  def RIw : ALUInstRI<!strconcat(name, "i.w"), 1, opcode, opnode, GPIReg16, i16imm, s16imm9>;
  def RRb : ALUInstRR<!strconcat(name, ".b"),  0, opcode, opnode, GPIReg8>;
  def RIb : ALUInstRI<!strconcat(name, "i.b"), 0, opcode, opnode, GPIReg8, i8imm, s8imm9>;
}

let isAdd = 1, isReMaterializable = 1 in
defm Add : ALUInst<"add", OPCODE_ADD, add>;
defm Sub : ALUInst<"sub", OPCODE_SUB, sub>;
defm Xor : ALUInst<"xor", OPCODE_XOR, xor>;
defm Or  : ALUInst<"or", OPCODE_OR, or>;
defm And : ALUInst<"and", OPCODE_AND, and>;

// Shifts always use 64-bits immediates
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
multiclass ALUInstShift<string name, AltairXOpcode opcode, SDNode opnode> {
  def RR  : ALUInstRR<!strconcat(name, ".q"),  3, opcode, opnode, GPIReg64>;
  def RI  : ALUInstRI<!strconcat(name, "i.q"), 3, opcode, opnode, GPIReg64, i64imm, s64imm9>;
  def RRd : ALUInstRR<!strconcat(name, ".d"),  2, opcode, opnode, GPIReg32>;
  def RId : ALUInstRI<!strconcat(name, "i.d"), 2, opcode, opnode, GPIReg32, i64imm, s64imm9>;
  def RRw : ALUInstRR<!strconcat(name, ".w"),  1, opcode, opnode, GPIReg16>;
  def RIw : ALUInstRI<!strconcat(name, "i.w"), 1, opcode, opnode, GPIReg16, i64imm, s64imm9>;
  def RRb : ALUInstRR<!strconcat(name, ".b"),  0, opcode, opnode, GPIReg8>;
  def RIb : ALUInstRI<!strconcat(name, "i.b"), 0, opcode, opnode, GPIReg8, i64imm, s64imm9>;
}

defm Lsl : ALUInstShift<"lsl", OPCODE_LSL, shl>;
defm Asr : ALUInstShift<"asr", OPCODE_ASR, sra>;
defm Lsr : ALUInstShift<"lsr", OPCODE_LSR, srl>;

//===----------------------------------------------------------------------===//
// sext, zext and trunc
//===----------------------------------------------------------------------===//

// sext (to i64)
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class SExt<RegisterClass intype, string sizename, bits<2> size> 
  : AltairXInstALUR<
    OPCODE_SEXT, size,
    (outs GPIReg64:$rd),
    (ins intype:$rs1), 
    [(set GPIReg64:$rd, (sext intype:$rs1))], 
    !strconcat("sext.", sizename), "$rd, $rs1"> {
}

def SExt8  : SExt<GPIReg8,  "b", 0>;
def SExt16 : SExt<GPIReg16, "w", 1>;
def SExt32 : SExt<GPIReg32, "d", 2>;

// sext + "trunc" (sext always sext to i64, so we need to extract subreg for smaller regs)
def : Pat<(i16(sext GPIReg8:$dest)), (EXTRACT_SUBREG (SExt8 GPIReg8:$dest), ISubReg16)>;
def : Pat<(i32(sext GPIReg8:$dest)), (EXTRACT_SUBREG (SExt8 GPIReg8:$dest), ISubReg32)>;
def : Pat<(i32(sext GPIReg16:$dest)), (EXTRACT_SUBREG (SExt16 GPIReg16:$dest), ISubReg32)>;
// sext (inreg)
def : Pat<(sext_inreg GPIReg64:$dest, i8),  (SExt8  (EXTRACT_SUBREG GPIReg64:$dest, ISubReg8))>;
def : Pat<(sext_inreg GPIReg64:$dest, i16), (SExt16 (EXTRACT_SUBREG GPIReg64:$dest, ISubReg16))>;
def : Pat<(sext_inreg GPIReg64:$dest, i32), (SExt32 (EXTRACT_SUBREG GPIReg64:$dest, ISubReg32))>;
// sext (inreg) + "trunc" (sext always sext to i64, so we need to extract subreg for smaller regs)
def : Pat<(sext_inreg GPIReg16:$dest, i8), (EXTRACT_SUBREG (SExt8 (EXTRACT_SUBREG GPIReg16:$dest, ISubReg8)), ISubReg16)>;
def : Pat<(sext_inreg GPIReg32:$dest, i8), (EXTRACT_SUBREG (SExt8 (EXTRACT_SUBREG GPIReg32:$dest, ISubReg8)), ISubReg32)>;
def : Pat<(sext_inreg GPIReg32:$dest, i16), (EXTRACT_SUBREG (SExt16 (EXTRACT_SUBREG GPIReg32:$dest, ISubReg16)), ISubReg16)>;

// trunc (simply extract subreg)
def : Pat<(i8 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, ISubReg8)>;
def : Pat<(i8 (trunc GPIReg16:$src)), (EXTRACT_SUBREG GPIReg16:$src, ISubReg8)>;
def : Pat<(i8 (trunc GPIReg32:$src)), (EXTRACT_SUBREG GPIReg32:$src, ISubReg8)>;
def : Pat<(i16 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, ISubReg16)>;
def : Pat<(i16 (trunc GPIReg32:$src)), (EXTRACT_SUBREG GPIReg32:$src, ISubReg16)>;
def : Pat<(i32 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, ISubReg32)>;

// zext
// Generic zext, simply "addi.<targetsize> value, 0" to zext.
def : Pat<(i64 (zext GPIReg8:$src)), (SUBREG_TO_REG GPIReg64, (AddRIb GPIReg8:$src, 0), ISubReg8)>;
def : Pat<(i64 (zext GPIReg16:$src)), (SUBREG_TO_REG GPIReg64, (AddRIw GPIReg16:$src, 0), ISubReg16)>;
def : Pat<(i64 (zext GPIReg32:$src)), (SUBREG_TO_REG GPIReg64, (AddRId GPIReg32:$src, 0), ISubReg32)>;
def : Pat<(i32 (zext GPIReg8:$src)), (SUBREG_TO_REG GPIReg32, (AddRIb GPIReg8:$src, 0), ISubReg32)>;
def : Pat<(i32 (zext GPIReg16:$src)), (SUBREG_TO_REG GPIReg32, (AddRIw GPIReg16:$src, 0), ISubReg32)>;
def : Pat<(i16 (zext GPIReg8:$src)), (SUBREG_TO_REG GPIReg16, (AddRIb GPIReg8:$src, 0), ISubReg16)>;
// Any instruction that defines a x-bit result leaves the high half of the register to 0
// except the ones that may become no-op such as Truncate which may become EXTRACT_SUBREG.
def implicitTrunc32 : PatLeaf<(i32 GPIReg32:$src), [{return doesImplicitTruncate(N->getOpcode());}]>;
def implicitTrunc16 : PatLeaf<(i16 GPIReg16:$src), [{return doesImplicitTruncate(N->getOpcode());}]>;
def implicitTrunc8  : PatLeaf<(i8  GPIReg8:$src),  [{return doesImplicitTruncate(N->getOpcode());}]>;
// Use a SUBREG_TO_REG to utilize implicit zext, this is possible when the x-bit value is defined by
// an operation that implicitly zext.
def : Pat<(i64 (zext implicitTrunc8:$src)), (SUBREG_TO_REG GPIReg64, GPIReg8:$src, ISubReg8)>;
def : Pat<(i64 (zext implicitTrunc16:$src)), (SUBREG_TO_REG GPIReg64, GPIReg16:$src, ISubReg16)>;
def : Pat<(i64 (zext implicitTrunc32:$src)), (SUBREG_TO_REG GPIReg64, GPIReg32:$src, ISubReg32)>;
def : Pat<(i32 (zext implicitTrunc8:$src)), (SUBREG_TO_REG GPIReg32, GPIReg8:$src, ISubReg8)>;
def : Pat<(i32 (zext implicitTrunc16:$src)), (SUBREG_TO_REG GPIReg32, GPIReg16:$src, ISubReg16)>;
def : Pat<(i16 (zext implicitTrunc8:$src)), (SUBREG_TO_REG GPIReg16, GPIReg8:$src, ISubReg8)>;
// Optimize "and-based" zext. Note: this may be generated by an extend of trunc.
def : Pat<(i64 (and (anyext implicitTrunc8:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG GPIReg64, GPIReg8:$src, ISubReg8)>;
def : Pat<(i64 (and (anyext implicitTrunc16:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG GPIReg64, GPIReg16:$src, ISubReg16)>;
def : Pat<(i64 (and (anyext implicitTrunc32:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG GPIReg64, GPIReg32:$src, ISubReg32)>;
def : Pat<(i32 (and (anyext implicitTrunc8:$src), (i32 0x0000FFFF))), (SUBREG_TO_REG GPIReg32, GPIReg8:$src, ISubReg8)>;
def : Pat<(i32 (and (anyext implicitTrunc16:$src), (i32 0x0000FFFF))), (SUBREG_TO_REG GPIReg32, GPIReg16:$src, ISubReg16)>;
def : Pat<(i16 (and (anyext implicitTrunc8:$src), (i16 0x00FF))), (SUBREG_TO_REG GPIReg16, GPIReg8:$src, ISubReg8)>;

//===----------------------------------------------------------------------===//
// Constant move
//===----------------------------------------------------------------------===//

// MoveI: Move 18-bits unsigned immediate, upper bits are set to 0
def pimm18 : ImmLeaf<i64, [{ return isUInt<18>(static_cast<uint64_t>(Imm)); }]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def MoveI : AltairXInstMove<OPCODE_MOVEI,
    (outs GPIReg64:$rd),
    (ins i64imm:$imm),
    [(set GPIReg64:$rd, pimm18:$imm)],
    "movei", "$rd, $imm">;

// moven: Move 18-bits **negative** immediate, upper bits are set to 1
def nimm18 : ImmLeaf<i64, [{ return -262144 < Imm && Imm < 0; }]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def MoveN : AltairXInstMove<OPCODE_MOVEN,
    (outs GPIReg64:$rd),
    (ins i64imm:$imm),
    [(set GPIReg64:$rd, nimm18:$imm)],
    "moven", "$rd, $imm">;

// umove: Move 16-bits shifted immeditates, other bits are set to 0
def imm16shifted16 : ImmLeaf<i64, [{ return isShiftedUInt<16, 16>(Imm); }]>;
def imm16shifted32 : ImmLeaf<i64, [{ return isShiftedUInt<16, 32>(Imm); }]>;
def imm16shifted48 : ImmLeaf<i64, [{ return isShiftedUInt<16, 48>(Imm); }]>;

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class UMove<bits<2> shift, ImmLeaf immPat> 
  : AltairXInstMoveShift<OPCODE_UMOVE, shift,
    (outs GPIReg64:$rd),
    (ins i64imm:$imm),
    [(set GPIReg64:$rd, immPat:$imm)],
    "umove", "$rd, $imm"> {
}

// UMove0 unneeded, because MoveI can handle 16-bits values already!
def UMove16 : UMove<1, imm16shifted16>;
def UMove32 : UMove<2, imm16shifted32>;
def UMove48 : UMove<3, imm16shifted48>;

// TODO: 
// MOVEIX | moveix | imm             | imm |= imm2(24)<<8 
// EXT    | ext    | rA,rB,imm1,imm2 | rA = (rB>>imm1)&((1<<imm2)-1) 
// INS    | ins    | rA,rB,imm1,imm2 | rA |= (rB<<imm1)&((1<<imm2)-1) 

//===----------------------------------------------------------------------===//
// MDU (mul, div, rem)
//===----------------------------------------------------------------------===//



//===----------------------------------------------------------------------===//
// Load
//===----------------------------------------------------------------------===//

// They use with different virtual registers, but generate the same opcode
def zextloadi8_i16 : PatFrag<(ops node:$ptr), (i16 (zextloadi8 node:$ptr))>;
def zextloadi8_i32 : PatFrag<(ops node:$ptr), (i32 (zextloadi8 node:$ptr))>;
def zextloadi8_i64 : PatFrag<(ops node:$ptr), (i64 (zextloadi8 node:$ptr))>;
def zextloadi16_i32 : PatFrag<(ops node:$ptr), (i32 (zextloadi16 node:$ptr))>;
def zextloadi16_i64 : PatFrag<(ops node:$ptr), (i64 (zextloadi16 node:$ptr))>;
def zextloadi32_i64 : PatFrag<(ops node:$ptr), (i64 (zextloadi32 node:$ptr))>;

// "Typed" sextload
def sextloadi8_i16 : PatFrag<(ops node:$ptr), (i16 (sextloadi8 node:$ptr))>;
def sextloadi8_i32 : PatFrag<(ops node:$ptr), (i32 (sextloadi8 node:$ptr))>;
def sextloadi8_i64 : PatFrag<(ops node:$ptr), (i64 (sextloadi8 node:$ptr))>;
def sextloadi16_i32 : PatFrag<(ops node:$ptr), (i32 (sextloadi16 node:$ptr))>;
def sextloadi16_i64 : PatFrag<(ops node:$ptr), (i64 (sextloadi16 node:$ptr))>;
def sextloadi32_i64 : PatFrag<(ops node:$ptr), (i64 (sextloadi32 node:$ptr))>;

// Load Reg + Reg << Shift
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadRRInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSURR<OPCODE_LD, size,
  (outs regclass:$reg),
  (ins GPIReg64:$base, GPIReg64:$offset, i64imm:$shift),
  [(set regclass:$reg, (opnode (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)))],
  !strconcat("ld.", sizename), "$reg, $base[$offset << $shift]"> {
}

// basic loads for each size
def LoadRRb : LoadRRInst<GPIReg8,  load, 0, "b">;
def LoadRRw : LoadRRInst<GPIReg16, load, 1, "w">;
def LoadRRd : LoadRRInst<GPIReg32, load, 2, "d">;
def LoadRRq : LoadRRInst<GPIReg64, load, 3, "q">;

// zextload: use instruction to ensure no additional operations are generated 
def LoadRRbZX16 : LoadRRInst<GPIReg16, zextloadi8_i16, 0, "b">;
def LoadRRbZX32 : LoadRRInst<GPIReg32, zextloadi8_i32, 0, "b">;
def LoadRRbZX64 : LoadRRInst<GPIReg64, zextloadi8_i64, 0, "b">;
def LoadRRwZX32 : LoadRRInst<GPIReg32, zextloadi16_i32, 1, "w">;
def LoadRRwZX64 : LoadRRInst<GPIReg64, zextloadi16_i64, 1, "w">;
def LoadRRdZX64 : LoadRRInst<GPIReg64, zextloadi32_i64, 2, "d">;

// sextload: actually perform a sext and an extract subreg for non 64-bits int
def : Pat<(sextloadi8_i16 (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), 
  (EXTRACT_SUBREG (SExt8 (LoadRRb GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), ISubReg16)>;
def : Pat<(sextloadi8_i32 (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), 
  (EXTRACT_SUBREG (SExt8 (LoadRRb GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), ISubReg32)>;
def : Pat<(sextloadi16_i32 (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), 
  (EXTRACT_SUBREG (SExt16 (LoadRRw GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), ISubReg32)>;

def : Pat<(sextloadi8_i64 (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), 
  (SExt8 (LoadRRb GPIReg64:$base, GPIReg64:$offset, i64imm:$shift))>;
def : Pat<(sextloadi16_i64 (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), 
  (SExt16 (LoadRRw GPIReg64:$base, GPIReg64:$offset, i64imm:$shift))>;
def : Pat<(sextloadi32_i64 (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), 
  (SExt32 (LoadRRd GPIReg64:$base, GPIReg64:$offset, i64imm:$shift))>;

// Load Reg + Imm
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadRIInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSURI<OPCODE_LDI, size,
  (outs regclass:$reg),
  (ins GPIReg64:$base, i64imm:$offset),
  [(set regclass:$reg, (opnode (addrimm GPIReg64:$base, i64imm:$offset)))],
  !strconcat("ldi.", sizename), "$reg, $offset[$base]"> {
}

// basic loads for each size (imm version)
def LoadRIb : LoadRIInst<GPIReg8,  load, 0, "b">;
def LoadRIw : LoadRIInst<GPIReg16, load, 1, "w">;
def LoadRId : LoadRIInst<GPIReg32, load, 2, "d">;
def LoadRIq : LoadRIInst<GPIReg64, load, 3, "q">;

// zextload: use instruction to ensure no additional operations are generated  (imm version)
def LoadRIbZX16 : LoadRIInst<GPIReg16, zextloadi8_i16, 0, "b">;
def LoadRIbZX32 : LoadRIInst<GPIReg32, zextloadi8_i32, 0, "b">;
def LoadRIbZX64 : LoadRIInst<GPIReg64, zextloadi8_i64, 0, "b">;
def LoadRIwZX32 : LoadRIInst<GPIReg32, zextloadi16_i32, 1, "w">;
def LoadRIwZX64 : LoadRIInst<GPIReg64, zextloadi16_i64, 1, "w">;
def LoadRIdZX64 : LoadRIInst<GPIReg64, zextloadi32_i64, 2, "d">;

// sextload: actually perform a sext and an extract subreg for non 64-bits int
def : Pat<(sextloadi8_i16 (addrimm GPIReg64:$base, i64imm:$offset)), 
  (EXTRACT_SUBREG (SExt8 (LoadRIb GPIReg64:$base, i64imm:$offset)), ISubReg16)>;
def : Pat<(sextloadi8_i32 (addrimm GPIReg64:$base, i64imm:$offset)), 
  (EXTRACT_SUBREG (SExt8 (LoadRIb GPIReg64:$base, i64imm:$offset)), ISubReg32)>;
def : Pat<(sextloadi16_i32 (addrimm GPIReg64:$base, i64imm:$offset)), 
  (EXTRACT_SUBREG (SExt16 (LoadRIw GPIReg64:$base, i64imm:$offset)), ISubReg32)>;

def : Pat<(sextloadi8_i64 (addrimm GPIReg64:$base, i64imm:$offset)), 
  (SExt8 (LoadRIb GPIReg64:$base, i64imm:$offset))>;
def : Pat<(sextloadi16_i64 (addrimm GPIReg64:$base, i64imm:$offset)), 
  (SExt16 (LoadRIw GPIReg64:$base, i64imm:$offset))>;
def : Pat<(sextloadi32_i64 (addrimm GPIReg64:$base, i64imm:$offset)), 
  (SExt32 (LoadRId GPIReg64:$base, i64imm:$offset))>;

// Load SP + Imm
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadSPInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSUSP<OPCODE_LDS, size,
  (outs regclass:$reg),
  (ins SPReg64:$base, i64imm:$offset),
  [(set regclass:$reg, (opnode (addrimmsp SPReg64:$base, i64imm:$offset)))],
  !strconcat("ldi.", sizename), "$reg, $offset[$base]"> {
}

// basic loads for each size (SP version)
def LoadSPb : LoadSPInst<GPIReg8,  load, 0, "b">;
def LoadSPw : LoadSPInst<GPIReg16, load, 1, "w">;
def LoadSPd : LoadSPInst<GPIReg32, load, 2, "d">;
def LoadSPq : LoadSPInst<GPIReg64, load, 3, "q">;

// zextload: use instruction to ensure no additional operations are generated  (SP version)
def LoadSPbZX16 : LoadSPInst<GPIReg16, zextloadi8_i16, 0, "b">;
def LoadSPbZX32 : LoadSPInst<GPIReg32, zextloadi8_i32, 0, "b">;
def LoadSPbZX64 : LoadSPInst<GPIReg64, zextloadi8_i64, 0, "b">;
def LoadSPwZX32 : LoadSPInst<GPIReg32, zextloadi16_i32, 1, "w">;
def LoadSPwZX64 : LoadSPInst<GPIReg64, zextloadi16_i64, 1, "w">;
def LoadSPdZX64 : LoadSPInst<GPIReg64, zextloadi32_i64, 2, "d">;

// sextload: actually perform a sext and an extract subreg for non 64-bits int
def : Pat<(sextloadi8_i16 (addrimmsp SPReg64:$base, i64imm:$offset)), 
  (EXTRACT_SUBREG (SExt8 (LoadSPb SPReg64:$base, i64imm:$offset)), ISubReg16)>;
def : Pat<(sextloadi8_i32 (addrimmsp SPReg64:$base, i64imm:$offset)), 
  (EXTRACT_SUBREG (SExt8 (LoadSPb SPReg64:$base, i64imm:$offset)), ISubReg32)>;
def : Pat<(sextloadi16_i32 (addrimmsp SPReg64:$base, i64imm:$offset)), 
  (EXTRACT_SUBREG (SExt16 (LoadSPw SPReg64:$base, i64imm:$offset)), ISubReg32)>;

def : Pat<(sextloadi8_i64 (addrimmsp SPReg64:$base, i64imm:$offset)), 
  (SExt8 (LoadSPb SPReg64:$base, i64imm:$offset))>;
def : Pat<(sextloadi16_i64 (addrimmsp SPReg64:$base, i64imm:$offset)), 
  (SExt16 (LoadSPw SPReg64:$base, i64imm:$offset))>;
def : Pat<(sextloadi32_i64 (addrimmsp SPReg64:$base, i64imm:$offset)), 
  (SExt32 (LoadSPd SPReg64:$base, i64imm:$offset))>;

//===----------------------------------------------------------------------===//
// Store
//===----------------------------------------------------------------------===//

// Store Reg + Reg << Shift
let isReMaterializable = 1, hasSideEffects = 0, mayStore = 1 in
class StoreRRInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSURR<OPCODE_ST, size, 
  (outs),
  (ins regclass:$reg, GPIReg64:$base, GPIReg64:$offset, i64imm:$shift),
  [(opnode regclass:$reg, (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift))],
  !strconcat("st.", sizename), "$reg, $base[$offset << $shift]"> {
}

// basic store for each size (imm version)
def StoreRRb : StoreRRInst<GPIReg8,  store, 0, "b">;
def StoreRRw : StoreRRInst<GPIReg16, store, 1, "w">;
def StoreRRd : StoreRRInst<GPIReg32, store, 2, "d">;
def StoreRRq : StoreRRInst<GPIReg64, store, 3, "q">;

// truncstore (imm version)
def StoreRRbT16 : StoreRRInst<GPIReg16, truncstorei8, 0, "b">;
def StoreRRbT32 : StoreRRInst<GPIReg32, truncstorei8, 0, "b">;
def StoreRRbT64 : StoreRRInst<GPIReg64, truncstorei8, 0, "b">;
def StoreRRwT32 : StoreRRInst<GPIReg32, truncstorei16, 1, "w">;
def StoreRRwT64 : StoreRRInst<GPIReg64, truncstorei16, 1, "w">;
def StoreRRdT64 : StoreRRInst<GPIReg64, truncstorei32, 2, "d">;

// Store Reg + Imm
let isReMaterializable = 1, hasSideEffects = 0, mayStore = 1 in
class StoreRIInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSURI<OPCODE_STI, size, 
  (outs),
  (ins regclass:$reg, GPIReg64:$base, i64imm:$offset),
  [(opnode regclass:$reg, (addrimm GPIReg64:$base, i64imm:$offset))],
  !strconcat("sti.", sizename), "$reg, $offset[$base]"> {
}

// basic store for each size
def StoreRIb : StoreRIInst<GPIReg8,  store, 0, "b">;
def StoreRIw : StoreRIInst<GPIReg16, store, 1, "w">;
def StoreRId : StoreRIInst<GPIReg32, store, 2, "d">;
def StoreRIq : StoreRIInst<GPIReg64, store, 3, "q">;

// truncstore
def StoreRIbT16 : StoreRIInst<GPIReg16, truncstorei8, 0, "b">;
def StoreRIbT32 : StoreRIInst<GPIReg32, truncstorei8, 0, "b">;
def StoreRIbT64 : StoreRIInst<GPIReg64, truncstorei8, 0, "b">;
def StoreRIwT32 : StoreRIInst<GPIReg32, truncstorei16, 1, "w">;
def StoreRIwT64 : StoreRIInst<GPIReg64, truncstorei16, 1, "w">;
def StoreRIdT64 : StoreRIInst<GPIReg64, truncstorei32, 2, "d">;

// Store SP + Imm
let isReMaterializable = 1, hasSideEffects = 0, mayStore = 1 in
class StoreSPInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSUSP<OPCODE_STS, size, 
  (outs),
  (ins regclass:$reg, SPReg64:$base, i64imm:$offset),
  [(opnode regclass:$reg, (addrimmsp SPReg64:$base, i64imm:$offset))],
  !strconcat("sti.", sizename), "$reg, $offset[$base]"> {
}

// basic store for each size
def StoreSPb : StoreRIInst<GPIReg8,  store, 0, "b">;
def StoreSPw : StoreRIInst<GPIReg16, store, 1, "w">;
def StoreSPd : StoreRIInst<GPIReg32, store, 2, "d">;
def StoreSPq : StoreRIInst<GPIReg64, store, 3, "q">;

// truncstore
def StoreSPbT16 : StoreRIInst<GPIReg16, truncstorei8, 0, "b">;
def StoreSPbT32 : StoreRIInst<GPIReg32, truncstorei8, 0, "b">;
def StoreSPbT64 : StoreRIInst<GPIReg64, truncstorei8, 0, "b">;
def StoreSPwT32 : StoreRIInst<GPIReg32, truncstorei16, 1, "w">;
def StoreSPwT64 : StoreRIInst<GPIReg64, truncstorei16, 1, "w">;
def StoreSPdT64 : StoreRIInst<GPIReg64, truncstorei32, 2, "d">;

//===----------------------------------------------------------------------===//
// Comparisons
//===----------------------------------------------------------------------===//

class CMPInst<SDPatternOperator setccop, RegisterClass regclass, bits<2> size, string sizename> 
  : AltairXInstCMP<OPCODE_CMP, size,
  (outs),
  (ins regclass:$rs1, regclass:$rs2),
  [(set CmpFlag, (i64 (setccop regclass:$rs1, regclass:$rs2)))],
  !strconcat("cmp.", sizename), "$rs1, $rs2"> {
}

multiclass CMPInstImpl<SDPatternOperator setccop> {
  def b : CMPInst<setccop, GPIReg8, 0, "b">;
  def w : CMPInst<setccop, GPIReg16, 1, "w">;
  def d : CMPInst<setccop, GPIReg32, 2, "d">;
  def q : CMPInst<setccop, GPIReg64, 3, "q">;
}

defm CMPEQ  : CMPInstImpl<seteq>;
defm CMPNE  : CMPInstImpl<setne>;
defm CMPGT  : CMPInstImpl<setgt>;
defm CMPGE  : CMPInstImpl<setge>;
defm CMPLT  : CMPInstImpl<setlt>;
defm CMPLE  : CMPInstImpl<setle>;
defm CMPUGT : CMPInstImpl<setugt>;
defm CMPUGE : CMPInstImpl<setuge>;
defm CMPULT : CMPInstImpl<setult>;
defm CMPULE : CMPInstImpl<setule>;

//===----------------------------------------------------------------------===//
// Branch
//===----------------------------------------------------------------------===//

def brtarget : Operand<OtherVT>;
def condcode : Operand<i32> {
  let PrintMethod = "printCondCode";
}

// AltairXISD::BRCOND $bb, $cond, $cmpflag
def SDT_AltairXbrcond : SDTypeProfile<0, 3, [SDTCisVT<0, OtherVT>, SDTCisVT<1, i32>, SDTCisVT<2, i64>]>;
def AltairXbrcond : SDNode<"AltairXISD::BRCOND", SDT_AltairXbrcond, [SDNPHasChain]>;

let Uses = [CmpFlag] in
def B 
  : AltairXInstBR<OPCODE_B, 
    (outs), 
    (ins brtarget:$target, condcode:$cond),
    [(AltairXbrcond bb:$target, imm:$cond, CmpFlag)],
    "b${cond}", "$target"> {

  bits<4> cond;
  let Inst{4-1} = cond;
}

let isBarrier = 1 in
def BRA : AltairXInstBR<OPCODE_BRA, 
  (outs), 
  (ins brtarget:$target),
  [(br bb:$target)],
  "bra", "$target">;

// TODO: Implement analyzeBranch, reverseBranchCondition, removeBranch, insertBranch...
// in InstrInfo to enable branch optimization

//===----------------------------------------------------------------------===//
// Conditional move and booleans
//===----------------------------------------------------------------------===//

def scmpcondcode : Operand<i32> {
  let PrintMethod = "printSCMPCondCode";
}

// iX = AltairXISD::SCMP $rA, $rB, comp
def SDT_AltairXscmp : SDTypeProfile<1, 3, [SDTCisInt<0>, SDTCisInt<1>, SDTCisSameAs<1, 2>, SDTCisVT<3, i32>]>;
def AltairXscmp : SDNode<"AltairXISD::SCMP", SDT_AltairXscmp, []>;

class SCMPInst<bits<2> size, string sizename, RegisterClass outregclass, RegisterClass inregclass> 
  : AltairXInstALURR<OPCODE_SCMP, size,
    (outs outregclass:$rd), 
    (ins inregclass:$rs1, inregclass:$rs2, scmpcondcode:$cond),
    [(set outregclass:$rd, (AltairXscmp inregclass:$rs1, inregclass:$rs2, imm:$cond))],
    !strconcat("s${cond}.", sizename), "$rd, $rs1, $rs2"> {

  bits<4> cond;
  let Inst{4-1} = cond;
}

multiclass SCMPInstImpl<RegisterClass outregclass> {
  def b : SCMPInst<0, "b", outregclass, GPIReg8>;
  def w : SCMPInst<1, "w", outregclass, GPIReg16>;
  def d : SCMPInst<2, "d", outregclass, GPIReg32>;
  def q : SCMPInst<3, "q", outregclass, GPIReg64>;
}

defm SCMPi8 : SCMPInstImpl<GPIReg8>;
defm SCMPi16 : SCMPInstImpl<GPIReg16>;
defm SCMPi32 : SCMPInstImpl<GPIReg32>;
defm SCMPi64 : SCMPInstImpl<GPIReg64>;

// i = AltairXISD::CMOVE tval, fval, bool
def SDT_AltairXcmove : SDTypeProfile<1, 3, [SDTCisInt<0>, SDTCisInt<1>, SDTCisSameAs<1, 2>, SDTCisVT<3, i64>]>;
def AltairXcmove : SDNode<"AltairXISD::CMOVE", SDT_AltairXcmove, []>;

// CMOVE, first parameter is also the output
let Constraints = "$out = $rd" in
class CMOVEInst<bits<2> size, string sizename, RegisterClass regclass> 
  : AltairXInstALURR<OPCODE_CMOVE, size,
    (outs regclass:$out),
    (ins regclass:$rd, regclass:$rs1, GPIReg64:$rs2),
    [(set regclass:$out, (AltairXcmove regclass:$rd, regclass:$rs1, GPIReg64:$rs2))],
    !strconcat("cmove.", sizename), "$rd, $rs1, $rs2"> {
}

def CMOVEb : CMOVEInst<0, "b", GPIReg8>;
def CMOVEw : CMOVEInst<1, "w", GPIReg16>;
def CMOVEd : CMOVEInst<2, "d", GPIReg32>;
def CMOVEq : CMOVEInst<3, "q", GPIReg64>;

//===----------------------------------------------------------------------===//
// Return
//===----------------------------------------------------------------------===//

def AltairXret : SDNode<"AltairXISD::RET", SDTNone, [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

let isReturn = 1, isBarrier = 1, hasDelaySlot = 1, isTerminator = 1, hasCtrlDep = 1 in
def AltairXPseudoRET : AltairXPseudo<(outs), (ins), [(AltairXret)], "ret">;
