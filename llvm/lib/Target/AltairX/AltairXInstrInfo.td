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

// Integer x-bits signed immediate for each type
multiclass SizedSImm<int size> {
  def s64imm#size : ImmLeaf<i64, "return isInt<" # size # ">(Imm);">;
  def s32imm#size : ImmLeaf<i32, "return isInt<" # size # ">(Imm);">;
  def s16imm#size : ImmLeaf<i16, "return isInt<" # size # ">(Imm);">;
  def s8imm#size  : ImmLeaf<i8,  "return isInt<" # size # ">(Imm);">;
}

defm "" : SizedSImm<32>;
defm "" : SizedSImm<64>;

multiclass SizedSTImm<int size> {
  def s64timm#size : TImmLeaf<i64, "return isInt<" # size # ">(Imm);">;
  def s32timm#size : TImmLeaf<i32, "return isInt<" # size # ">(Imm);">;
  def s16timm#size : TImmLeaf<i16, "return isInt<" # size # ">(Imm);">;
  def s8timm#size  : TImmLeaf<i8,  "return isInt<" # size # ">(Imm);">;
}

defm "" : SizedSTImm<64>;

// Integer x-bits unsigned immediate for each type
multiclass SizedUImm<int size> {
  def u64imm#size : ImmLeaf<i64, "return isUInt<" # size # ">(Imm);">;
  def u32imm#size : ImmLeaf<i32, "return isUInt<" # size # ">(Imm);">;
  def u16imm#size : ImmLeaf<i16, "return isUInt<" # size # ">(Imm);">;
  def u8imm#size  : ImmLeaf<i8,  "return isUInt<" # size # ">(Imm);">;
}

defm "" : SizedUImm<18>;

//multiclass ShiftedUImm<int size, int shift> {
//  def u64imm#size#sh#shift : ImmLeaf<i64, "return isShiftedUInt<" # size # "," # shift # ">(Imm);">;

// Addressing mode
def addr : ComplexPattern<iPTR, 3, "selectAddr", [], []>;
def addrimm : ComplexPattern<iPTR, 2, "selectAddrImm", [add, frameindex], []>;
def addrimmsp : ComplexPattern<iPTR, 2, "selectAddrImmSP", [add], []>;

def SDT_AltairXGAWRAPPER : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>]>;
def AltairXGAWRAPPER : SDNode<"AltairXISD::GAWRAPPER", SDT_AltairXGAWRAPPER, []>;

def AltairXGlobalAddrValue : AltairXPseudo<
  (outs GPIReg64:$ptr), 
  (ins i64imm:$addr),
  [(set GPIReg64:$ptr, (AltairXGAWRAPPER tglobaladdr:$addr))],
  "move $ptr, $addr"
>;

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

// Call, does set LR (link register)
def STD_AltairXcall : SDTypeProfile<0, -1, [SDTCisPtrTy<0>]>;
def AltairXcall : SDNode<"AltairXISD::CALL", STD_AltairXcall, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

def calltarget : Operand<i64> {
  let EncoderMethod = "getBRTargetOpValue";
}

let Defs = [LR], Uses = [R0], isCall = 1, isBarrier = 1, hasDelaySlot = 1, hasCtrlDep = 1 in
def Call : AltairXCallInst<OPCODE_CALL,
  (outs),
  (ins calltarget:$addr),
  [(AltairXcall tglobaladdr:$addr)],
  "call", "$addr">;
def : Pat<(AltairXcall texternalsym:$func), (Call texternalsym:$func)>;

// Jump, do not set LR (link register)
def STD_AltairXjump : SDTypeProfile<0, -1, [SDTCisPtrTy<0>]>;
def AltairXjump : SDNode<"AltairXISD::JUMP", STD_AltairXcall, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

let Uses = [R0], isCall = 1, isBarrier = 1, hasDelaySlot = 1, hasCtrlDep = 1 in
def Jump : AltairXCallInst<OPCODE_JUMP,
  (outs),
  (ins calltarget:$addr),
  [(AltairXjump tglobaladdr:$addr)],
  "jump", "$addr">;
def : Pat<(AltairXjump texternalsym:$func), (Jump texternalsym:$func)>;

def JUMPBR  : AltairXSpecialJumpInst<OPCODE_SPECIAL_JUMP, [], "jumpbr", 0b1000>;
def CALLBR  : AltairXSpecialJumpInst<OPCODE_SPECIAL_JUMP, [], "callbr", 0b1001>;
def SYSCALL : AltairXSpecialJumpInst<OPCODE_SPECIAL_JUMP, [], "syscall", 0b0010>;
//def INT     : AltairXSpecialJumpInst<OPCODE_SPECIAL_JUMP, [], "int", 0b1010>;

//===----------------------------------------------------------------------===//
// Branches
//===----------------------------------------------------------------------===//

def brtarget : Operand<OtherVT> {
  let EncoderMethod = "getCallTargetOpValue";
  //let OperandType = "OPERAND_PCREL";
}
def condcode : Operand<i32> {
  let EncoderMethod = "getImmOpValue";
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

//===----------------------------------------------------------------------===//
// Procedure return
//===----------------------------------------------------------------------===//

def STD_AltairXret : SDTypeProfile<0, 0, []>;
def AltairXret : SDNode<"AltairXISD::RET", STD_AltairXret, [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

let Uses = [LR], isReturn = 1, isBarrier = 1, hasDelaySlot = 1, isTerminator = 1, hasCtrlDep = 1 in
def RET  : AltairXSpecialJumpInst<OPCODE_SPECIAL_JUMP, [(AltairXret)], "ret", 0b0100>;
def RETI : AltairXSpecialJumpInst<OPCODE_SPECIAL_JUMP, [], "reti", 0b1100>;

//===----------------------------------------------------------------------===//
// MoveIX support
//===----------------------------------------------------------------------===//

// moveix encoding of imm is a bit tricky, but basically consist of:
// (imm >> N) ^ -1 for most signed imm and (imm >> N) for unsigned immediates
def moveiximm : Operand<i64> {
  let EncoderMethod = "getMoveIXOpValue";
}

// Parent format is used to know dynamically the size of the imm
def MOVEIX24PCREL : AltairXInstMoveIX<InstFormatBRURelImm24, (outs), (ins moveiximm:$imm), []>;
def MOVEIX24ABS : AltairXInstMoveIX<InstFormatBRUAbsImm24, (outs), (ins moveiximm:$imm), []>;
def MOVEIX9 : AltairXInstMoveIX<InstFormatALURegImm9, (outs), (ins moveiximm:$imm), []>;
def MOVEIX10 : AltairXInstMoveIX<InstFormatLSURegImm10, (outs), (ins moveiximm:$imm), []>;
def MOVEIX18 : AltairXInstMoveIX<InstFormatMoveImm18, (outs), (ins moveiximm:$imm), []>;

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
  def RRq : ALUInstRR<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64>;
  def RIq : ALUInstRI<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64, i64imm, s64imm32>;
  def RRd : ALUInstRR<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32>;
  def RId : ALUInstRI<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32, i32imm, s32imm32>;
  def RRw : ALUInstRR<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16>;
  def RIw : ALUInstRI<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16, i16imm, s16imm32>;
  def RRb : ALUInstRR<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8>;
  def RIb : ALUInstRI<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8, i8imm, s8imm32>;
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
  def RRq : ALUInstRR<!strconcat(name, ".q"),  3, opcode, opnode, GPIReg64>;
  def RIq : ALUInstRI<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64, i64imm, s64imm32>;
  def RRd : ALUInstRR<!strconcat(name, ".d"),  2, opcode, opnode, GPIReg32>;
  def RId : ALUInstRI<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32, i64imm, s64imm32>;
  def RRw : ALUInstRR<!strconcat(name, ".w"),  1, opcode, opnode, GPIReg16>;
  def RIw : ALUInstRI<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16, i64imm, s64imm32>;
  def RRb : ALUInstRR<!strconcat(name, ".b"),  0, opcode, opnode, GPIReg8>;
  def RIb : ALUInstRI<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8, i64imm, s64imm32>;
}

defm Lsl : ALUInstShift<"lsl", OPCODE_LSL, shl>;
defm Asr : ALUInstShift<"asr", OPCODE_ASR, sra>;
defm Lsr : ALUInstShift<"lsr", OPCODE_LSR, srl>;

// intern regs moves
// Does not put pattern here, this instructions will be generated by copyPhysReg when needed
// R <- LR/BR
def MOVERI : AltairXInstCUMoveRI<
    (outs GPIReg32:$rd),
    (ins RIReg32:$ri),
    [],
    "move", "$rd, $ri"> {
}

// LR/BR <- R
def MOVEIR : AltairXInstCUMoveIR<
    (outs RIReg32:$ri),
    (ins GPIReg32:$rs1),
    [],
    "move", "$ri, $rs1"> {
}

//===----------------------------------------------------------------------===//
// MDU (mul, div, rem)
//===----------------------------------------------------------------------===//

class MDUInstRR<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode, RegisterClass inclass, RegisterClass outclass>
  : AltairXInstMDURR<opcode, size,
    (outs outclass:$rd),
    (ins inclass:$rs1, inclass:$rs2),
    [(set outclass:$rd, (opnode inclass:$rs1, inclass:$rs2))],
    name, "$rs1, $rs2"> {
}

class MDUInstRI<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode, RegisterClass inclass, RegisterClass outclass, Operand immop, ImmLeaf immpat>
  : AltairXInstMDURI<opcode, size,
    (outs outclass:$rd),
    (ins inclass:$rs1, immop:$imm),
    [(set outclass:$rd, (opnode inclass:$rs1, immpat:$imm))],
    name, "$rs1, $imm"> {
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
multiclass MDUInst<string name, AltairXOpcode opcode, SDNode opnode, 
  list<RegisterClass> outclasses, list<Register> impldefs> {
  // All of ops defined by this class set a "main" register and another one
  let Defs = [impldefs[3]] in {
    def RRq : MDUInstRR<!strconcat(name, ".q"),  3, opcode, opnode, GPIReg64, outclasses[3]>;
    def RIq : MDUInstRI<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64, outclasses[3], i64imm, s64imm32>;
  }
  let Defs = [impldefs[2]] in {
    def RRd : MDUInstRR<!strconcat(name, ".d"),  2, opcode, opnode, GPIReg32, outclasses[2]>;
    def RId : MDUInstRI<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32, outclasses[2], i32imm, s32imm32>;
  }
  let Defs = [impldefs[1]] in {
    def RRw : MDUInstRR<!strconcat(name, ".w"),  1, opcode, opnode, GPIReg16, outclasses[1]>;
    def RIw : MDUInstRI<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16, outclasses[1], i16imm, s16imm32>;
  }
  let Defs = [impldefs[0]] in {
    def RRb : MDUInstRR<!strconcat(name, ".b"),  0, opcode, opnode, GPIReg8,  outclasses[0]>;
    def RIb : MDUInstRI<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8,  outclasses[0], i8imm, s8imm32>;
  }
}

defm Div : MDUInst<"div", OPCODE_DIV, sdiv, [QReg8, QReg16, QReg32, QReg64], [RQRb, RQRw, RQRd, RQR]>;
defm DivU : MDUInst<"divu", OPCODE_DIVU, udiv, [QReg8, QReg16, QReg32, QReg64], [RQRb, RQRw, RQRd, RQR]>;
defm Rem : MDUInst<"div", OPCODE_DIV, srem, [QRReg8, QRReg16, QRReg32, QRReg64], [RQb, RQw, RQd, RQ]>;
defm RemU : MDUInst<"divu", OPCODE_DIVU, urem, [QRReg8, QRReg16, QRReg32, QRReg64], [RQb, RQw, RQd, RQ]>;

defm Mul : MDUInst<"mul", OPCODE_MUL, mul, [PLReg8, PLReg16, PLReg32, PLReg64], [RPHb, RPHw, RPHd, RPH]>;
defm MulH : MDUInst<"mul", OPCODE_MUL, mulhs, [PHReg8, PHReg16, PHReg32, PHReg64], [RPLb, RPLw, RPLd, RPL]>;
defm MulHU : MDUInst<"mulu", OPCODE_MULU, mulhu, [PHReg8, PHReg16, PHReg32, PHReg64], [RPLb, RPLw, RPLd, RPL]>;

class MDUMoveInstRI<string name, AltairXOpcode opcode, RegisterClass outclass, RegisterClass inclass>
  : AltairXInstMDUMoveRI<opcode,
    (outs outclass:$rd),
    (ins inclass:$pq),
    [], // no pattern here, this instructions will be generated by copyPhysReg when needed
    name, "$rd, $pq"> {
}

// R <- Q/QR/PH/PL
def MOVERQq : MDUMoveInstRI<"move", OPCODE_MOVERQ, GPIReg64, MDUReg64>;
def MOVERQd : MDUMoveInstRI<"move", OPCODE_MOVERQ, GPIReg32, MDUReg32>;
def MOVERQw : MDUMoveInstRI<"move", OPCODE_MOVERQ, GPIReg16, MDUReg16>;
def MOVERQb : MDUMoveInstRI<"move", OPCODE_MOVERQ, GPIReg8, MDUReg8>;

class MDUMoveInstIR<string name, AltairXOpcode opcode, RegisterClass outclass, RegisterClass inclass>
  : AltairXInstMDUMoveIR<opcode,
    (outs outclass:$pq),
    (ins inclass:$rs1),
    [], // no pattern here, this instructions will be generated by copyPhysReg when needed
    name, "$pq, $rs1"> {
}

// Q/QR/PH/PL <- R
def MOVEQRq : MDUMoveInstIR<"move", OPCODE_MOVEQR, MDUReg64, GPIReg64>;
def MOVEQRd : MDUMoveInstIR<"move", OPCODE_MOVEQR, MDUReg32, GPIReg32>;
def MOVEQRw : MDUMoveInstIR<"move", OPCODE_MOVEQR, MDUReg16, GPIReg16>;
def MOVEQRb : MDUMoveInstIR<"move", OPCODE_MOVEQR, MDUReg8, GPIReg8>;

//===----------------------------------------------------------------------===//
// sext, zext and trunc
//===----------------------------------------------------------------------===//

// sext (to i64)
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
class SExt<RegisterClass intype, bits<2> insize, RegisterClass outtype, bits<2> outsize, string sizename> 
  : AltairXInstSExt<
    OPCODE_SEXT, outsize, insize,
    (outs outtype:$rd),
    (ins intype:$rs1), 
    [(set outtype:$rd, (sext intype:$rs1))], 
    !strconcat("sext.", sizename), "$rd, $rs1, " # insize> {
}

def SExt8to16  : SExt<GPIReg8,  0, GPIReg16, 1, "w">;
def SExt8to32  : SExt<GPIReg8,  0, GPIReg32, 2, "d">;
def SExt8to64  : SExt<GPIReg8,  0, GPIReg64, 3, "q">;
def SExt16to32 : SExt<GPIReg16, 1, GPIReg32, 2, "d">;
def SExt16to64 : SExt<GPIReg16, 1, GPIReg64, 3, "q">;
def SExt32to64 : SExt<GPIReg32, 2, GPIReg64, 3, "q">;

// sext (inreg)
def : Pat<(sext_inreg GPIReg16:$dest, i8),  (SExt8to16  (EXTRACT_SUBREG GPIReg16:$dest, ISubReg8))>;
def : Pat<(sext_inreg GPIReg32:$dest, i8),  (SExt8to32  (EXTRACT_SUBREG GPIReg32:$dest, ISubReg8))>;
def : Pat<(sext_inreg GPIReg64:$dest, i8),  (SExt8to64  (EXTRACT_SUBREG GPIReg64:$dest, ISubReg8))>;
def : Pat<(sext_inreg GPIReg32:$dest, i16), (SExt16to32 (EXTRACT_SUBREG GPIReg32:$dest, ISubReg16))>;
def : Pat<(sext_inreg GPIReg64:$dest, i16), (SExt16to64 (EXTRACT_SUBREG GPIReg64:$dest, ISubReg16))>;
def : Pat<(sext_inreg GPIReg64:$dest, i32), (SExt32to64 (EXTRACT_SUBREG GPIReg64:$dest, ISubReg32))>;

// trunc (simply extract subreg)
def : Pat<(i8 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, ISubReg8)>;
def : Pat<(i8 (trunc GPIReg16:$src)), (EXTRACT_SUBREG GPIReg16:$src, ISubReg8)>;
def : Pat<(i8 (trunc GPIReg32:$src)), (EXTRACT_SUBREG GPIReg32:$src, ISubReg8)>;
def : Pat<(i16 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, ISubReg16)>;
def : Pat<(i16 (trunc GPIReg32:$src)), (EXTRACT_SUBREG GPIReg32:$src, ISubReg16)>;
def : Pat<(i32 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, ISubReg32)>;

// anyext/zext
// Generic anyext/zext, simply "add.<targetsize> value, 0" to zext.
// This will be a fallback
def : Pat<(i64 (zext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg64, (AddRIb GPIReg8:$src, 0), ISubReg8)>;
def : Pat<(i64 (zext GPIReg16:$src)), (SUBREG_TO_REG GPIReg64, (AddRIw GPIReg16:$src, 0), ISubReg16)>;
def : Pat<(i64 (zext GPIReg32:$src)), (SUBREG_TO_REG GPIReg64, (AddRId GPIReg32:$src, 0), ISubReg32)>;
def : Pat<(i32 (zext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg32, (AddRIb GPIReg8:$src,  0), ISubReg8)>;
def : Pat<(i32 (zext GPIReg16:$src)), (SUBREG_TO_REG GPIReg32, (AddRIw GPIReg16:$src, 0), ISubReg16)>;
def : Pat<(i16 (zext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg16, (AddRIb GPIReg8:$src,  0), ISubReg8)>;
def : Pat<(i16 (anyext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg16, (AddRIb GPIReg8:$src,  0), ISubReg8)>;
def : Pat<(i32 (anyext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg32, (AddRIb GPIReg8:$src,  0), ISubReg8)>;
def : Pat<(i32 (anyext GPIReg16:$src)), (SUBREG_TO_REG GPIReg32, (AddRIw GPIReg16:$src, 0), ISubReg16)>;
def : Pat<(i64 (anyext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg64, (AddRIb GPIReg8:$src,  0), ISubReg8)>;
def : Pat<(i64 (anyext GPIReg16:$src)), (SUBREG_TO_REG GPIReg64, (AddRIw GPIReg16:$src, 0), ISubReg16)>;
def : Pat<(i64 (anyext GPIReg32:$src)), (SUBREG_TO_REG GPIReg64, (AddRId GPIReg32:$src, 0), ISubReg32)>;

// Any instruction that defines a x-bit result leaves the high half of the register to 0
// except the ones that may become no-op such as Truncate which may become EXTRACT_SUBREG.
multiclass ZExtPats<list<PatLeaf> pats, list<RegisterClass> regclasses> {
  defvar pat8 = pats[0];
  defvar pat16 = pats[1];
  defvar pat32 = pats[2];
  defvar reg8 = regclasses[0];
  defvar reg16 = regclasses[1];
  defvar reg32 = regclasses[2];
  defvar reg64 = regclasses[3];

  // Use a SUBREG_TO_REG to utilize implicit anyext/zext, this is possible when the x-bit value is defined by
  // an operation that implicitly zext.
  def : Pat<(i64 (zext pat8:$src)), (SUBREG_TO_REG reg64, reg8:$src, ISubReg8)>;
  def : Pat<(i64 (zext pat16:$src)), (SUBREG_TO_REG reg64, reg16:$src, ISubReg16)>;
  def : Pat<(i64 (zext pat32:$src)), (SUBREG_TO_REG reg64, reg32:$src, ISubReg32)>;
  def : Pat<(i32 (zext pat8:$src)), (SUBREG_TO_REG reg32, reg8:$src, ISubReg8)>;
  def : Pat<(i32 (zext pat16:$src)), (SUBREG_TO_REG reg32, reg16:$src, ISubReg16)>;
  def : Pat<(i16 (zext pat8:$src)), (SUBREG_TO_REG reg16, reg8:$src, ISubReg8)>;
  def : Pat<(i64 (anyext pat8:$src)), (SUBREG_TO_REG reg64, reg8:$src, ISubReg8)>;
  def : Pat<(i64 (anyext pat16:$src)), (SUBREG_TO_REG reg64, reg16:$src, ISubReg16)>;
  def : Pat<(i64 (anyext pat32:$src)), (SUBREG_TO_REG reg64, reg32:$src, ISubReg32)>;
  def : Pat<(i32 (anyext pat8:$src)), (SUBREG_TO_REG reg32, reg8:$src, ISubReg8)>;
  def : Pat<(i32 (anyext pat16:$src)), (SUBREG_TO_REG reg32, reg16:$src, ISubReg16)>;
  def : Pat<(i16 (anyext pat8:$src)), (SUBREG_TO_REG reg16, reg8:$src, ISubReg8)>;
  // Optimize "and-based" zext. This may be generated by an extend of trunc.
  def : Pat<(i64 (and (anyext pat8:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG reg64, reg8:$src, ISubReg8)>;
  def : Pat<(i64 (and (anyext pat16:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG reg64, reg16:$src, ISubReg16)>;
  def : Pat<(i64 (and (anyext pat32:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG reg64, reg32:$src, ISubReg32)>;
  def : Pat<(i32 (and (anyext pat8:$src), (i32 0x0000FFFF))), (SUBREG_TO_REG reg32, reg8:$src, ISubReg8)>;
  def : Pat<(i32 (and (anyext pat16:$src), (i32 0x0000FFFF))), (SUBREG_TO_REG reg32, reg16:$src, ISubReg16)>;
  def : Pat<(i16 (and (anyext pat8:$src), (i16 0x00FF))), (SUBREG_TO_REG reg16, reg8:$src, ISubReg8)>;
}

def implicitTruncGPI32 : PatLeaf<(i32 GPIReg32:$src), [{return doesImplicitTruncate(N->getOpcode()) && !outputsInMDUReg(N->getOpcode());}]>;
def implicitTruncGPI16 : PatLeaf<(i16 GPIReg16:$src), [{return doesImplicitTruncate(N->getOpcode()) && !outputsInMDUReg(N->getOpcode());}]>;
def implicitTruncGPI8  : PatLeaf<(i8  GPIReg8:$src),  [{return doesImplicitTruncate(N->getOpcode()) && !outputsInMDUReg(N->getOpcode());}]>;

defm : ZExtPats<
  [implicitTruncGPI8, implicitTruncGPI16, implicitTruncGPI32],
  [GPIReg8, GPIReg16, GPIReg32, GPIReg64]
>;

def implicitTruncMDU32 : PatLeaf<(i32 MDUReg32:$src), [{return doesImplicitTruncate(N->getOpcode()) && outputsInMDUReg(N->getOpcode());}]>;
def implicitTruncMDU16 : PatLeaf<(i16 MDUReg16:$src), [{return doesImplicitTruncate(N->getOpcode()) && outputsInMDUReg(N->getOpcode());}]>;
def implicitTruncMDU8  : PatLeaf<(i8  MDUReg8:$src),  [{return doesImplicitTruncate(N->getOpcode()) && outputsInMDUReg(N->getOpcode());}]>;

defm : ZExtPats<
  [implicitTruncMDU8, implicitTruncMDU16, implicitTruncMDU32],
  [MDUReg8, MDUReg16, MDUReg32, MDUReg64]
>;

//===----------------------------------------------------------------------===//
// Constant move
//===----------------------------------------------------------------------===//

// x = AltairXISD::MOVECONSTANT $val
// This ISD instruction can be used to force a constant out of a instruction by creating an wrapper node
def SDT_AltairXConstantToReg : SDTypeProfile<1, 1, [SDTCisInt<0>, SDTCisSameAs<0, 1>]>;
def AltairXConstantToReg : SDNode<"AltairXISD::CONSTANTTOREG", SDT_AltairXConstantToReg, []>;

// mayLoad because for bigger imms we may put value in executable data and load from that location
let hasSideEffects = 0, mayLoad = 1 in
class ConstantToReg<RegisterClass regclass, Operand immop, ImmLeaf immpat>
  : AltairXPseudo<
    (outs regclass:$rd),
    (ins immop:$imm),
    [(set regclass:$rd, immpat:$imm)],
    "move $rd, $imm"> {
}

def ConstantToRegb : ConstantToReg<GPIReg8,  i8imm,  s8imm64>;
def ConstantToRegw : ConstantToReg<GPIReg16, i16imm, s16imm64>;
def ConstantToRegd : ConstantToReg<GPIReg32, i32imm, s32imm64>;
def ConstantToRegq : ConstantToReg<GPIReg64, i64imm, s64imm64>;

def : Pat<(AltairXConstantToReg s8timm64:$imm), (ConstantToRegb $imm)>;
def : Pat<(AltairXConstantToReg s16timm64:$imm), (ConstantToRegw $imm)>;
def : Pat<(AltairXConstantToReg s32timm64:$imm), (ConstantToRegd $imm)>;
def : Pat<(AltairXConstantToReg s64timm64:$imm), (ConstantToRegq $imm)>;

// MoveI: Move 18-bits signed imm
// Can be moveix-ed for a total of 42 (18 + 24) bits signed imm
// Typed version are for code generation, this instruction has no size parameter!
class MoveInst<string name, AltairXOpcode opcode, RegisterClass regclass, Operand immop>
  : AltairXInstMoveI<opcode,
    (outs regclass:$rd),
    (ins immop:$imm),
    [], // no pattern, will be generated by extension of ConstantToReg on postRA expand
    name, "$rd, $imm"> {
}

def MoveIb : MoveInst<"movei", OPCODE_MOVEI, GPIReg8 , i8imm>;
def MoveIw : MoveInst<"movei", OPCODE_MOVEI, GPIReg16, i16imm>;
def MoveId : MoveInst<"movei", OPCODE_MOVEI, GPIReg32, i32imm>;
def MoveIq : MoveInst<"movei", OPCODE_MOVEI, GPIReg64, i64imm>;

// TODO:
// EXT    | ext    | rA,rB,imm1,imm2 | rA = (rB>>imm1)&((1<<imm2)-1) 
// INS    | ins    | rA,rB,imm1,imm2 | rA |= (rB<<imm1)&((1<<imm2)-1) 

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

// "Typed" extload
def extloadi8_i16 : PatFrag<(ops node:$ptr), (i16 (extloadi8 node:$ptr))>;
def extloadi8_i32 : PatFrag<(ops node:$ptr), (i32 (extloadi8 node:$ptr))>;
def extloadi8_i64 : PatFrag<(ops node:$ptr), (i64 (extloadi8 node:$ptr))>;
def extloadi16_i32 : PatFrag<(ops node:$ptr), (i32 (extloadi16 node:$ptr))>;
def extloadi16_i64 : PatFrag<(ops node:$ptr), (i64 (extloadi16 node:$ptr))>;
def extloadi32_i64 : PatFrag<(ops node:$ptr), (i64 (extloadi32 node:$ptr))>;

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

// extload: use instruction to ensure no additional operations are generated 
def LoadRRbAX16 : LoadRRInst<GPIReg16, extloadi8_i16, 0, "b">;
def LoadRRbAX32 : LoadRRInst<GPIReg32, extloadi8_i32, 0, "b">;
def LoadRRbAX64 : LoadRRInst<GPIReg64, extloadi8_i64, 0, "b">;
def LoadRRwAX32 : LoadRRInst<GPIReg32, extloadi16_i32, 1, "w">;
def LoadRRwAX64 : LoadRRInst<GPIReg64, extloadi16_i64, 1, "w">;
def LoadRRdAX64 : LoadRRInst<GPIReg64, extloadi32_i64, 2, "d">;

// Load Reg + Imm
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadRIInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSURI<OPCODE_LDI, size,
  (outs regclass:$reg),
  (ins GPIReg64:$base, i64imm:$offset),
  [(set regclass:$reg, (opnode (addrimm GPIReg64:$base, i64imm:$offset)))],
  !strconcat("ld.", sizename), "$reg, $offset[$base]"> {
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

// zextload: use instruction to ensure no additional operations are generated  (imm version)
def LoadRIbAX16 : LoadRIInst<GPIReg16, extloadi8_i16, 0, "b">;
def LoadRIbAX32 : LoadRIInst<GPIReg32, extloadi8_i32, 0, "b">;
def LoadRIbAX64 : LoadRIInst<GPIReg64, extloadi8_i64, 0, "b">;
def LoadRIwAX32 : LoadRIInst<GPIReg32, extloadi16_i32, 1, "w">;
def LoadRIwAX64 : LoadRIInst<GPIReg64, extloadi16_i64, 1, "w">;
def LoadRIdAX64 : LoadRIInst<GPIReg64, extloadi32_i64, 2, "d">;

// Load SP + Imm
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadSPInst<RegisterClass regclass, SDPatternOperator opnode, bits<2> size, string sizename> 
  : AltairXInstLSUSP<OPCODE_LDS, size,
  (outs regclass:$reg),
  (ins SPReg64:$base, i64imm:$offset),
  [(set regclass:$reg, (opnode (addrimmsp SPReg64:$base, i64imm:$offset)))],
  !strconcat("lds.", sizename), "$reg, $offset[$base]"> {
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

// zextload: use instruction to ensure no additional operations are generated  (SP version)
def LoadSPbAX16 : LoadSPInst<GPIReg16, extloadi8_i16, 0, "b">;
def LoadSPbAX32 : LoadSPInst<GPIReg32, extloadi8_i32, 0, "b">;
def LoadSPbAX64 : LoadSPInst<GPIReg64, extloadi8_i64, 0, "b">;
def LoadSPwAX32 : LoadSPInst<GPIReg32, extloadi16_i32, 1, "w">;
def LoadSPwAX64 : LoadSPInst<GPIReg64, extloadi16_i64, 1, "w">;
def LoadSPdAX64 : LoadSPInst<GPIReg64, extloadi32_i64, 2, "d">;

// sextload: actually perform a sext
multiclass SExtLoadPat<PatFrag loadpat, SExt sextinst, LoadRRInst loadrrinst, LoadRIInst loadriinst, LoadSPInst loadspinst> {
  // reg-reg version
  def : Pat<(loadpat (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift)), 
    (sextinst (loadrrinst GPIReg64:$base, GPIReg64:$offset, i64imm:$shift))>;
  // reg-imm version
  def : Pat<(loadpat (addrimm GPIReg64:$base, i64imm:$offset)), 
    (sextinst (loadriinst GPIReg64:$base, i64imm:$offset))>;
  // reg-imm stack pointer version
  def : Pat<(loadpat (addrimmsp GPIReg64:$base, i64imm:$offset)), 
    (sextinst (loadspinst GPIReg64:$base, i64imm:$offset))>;
}

defm : SExtLoadPat<sextloadi8_i16, SExt8to16, LoadRRb, LoadRIb, LoadSPb>;
defm : SExtLoadPat<sextloadi8_i32, SExt8to32, LoadRRb, LoadRIb, LoadSPb>;
defm : SExtLoadPat<sextloadi8_i64, SExt8to64, LoadRRb, LoadRIb, LoadSPb>;
defm : SExtLoadPat<sextloadi16_i32, SExt16to32, LoadRRw, LoadRIw, LoadSPw>;
defm : SExtLoadPat<sextloadi16_i64, SExt16to64, LoadRRw, LoadRIw, LoadSPw>;
defm : SExtLoadPat<sextloadi32_i64, SExt32to64, LoadRRd, LoadRId, LoadSPd>;

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
  !strconcat("st.", sizename), "$reg, $offset[$base]"> {
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
  !strconcat("sts.", sizename), "$reg, $offset[$base]"> {
}

// basic store for each size
def StoreSPb : StoreSPInst<GPIReg8,  store, 0, "b">;
def StoreSPw : StoreSPInst<GPIReg16, store, 1, "w">;
def StoreSPd : StoreSPInst<GPIReg32, store, 2, "d">;
def StoreSPq : StoreSPInst<GPIReg64, store, 3, "q">;

// truncstore
def StoreSPbT16 : StoreSPInst<GPIReg16, truncstorei8, 0, "b">;
def StoreSPbT32 : StoreSPInst<GPIReg32, truncstorei8, 0, "b">;
def StoreSPbT64 : StoreSPInst<GPIReg64, truncstorei8, 0, "b">;
def StoreSPwT32 : StoreSPInst<GPIReg32, truncstorei16, 1, "w">;
def StoreSPwT64 : StoreSPInst<GPIReg64, truncstorei16, 1, "w">;
def StoreSPdT64 : StoreSPInst<GPIReg64, truncstorei32, 2, "d">;

//===----------------------------------------------------------------------===//
// Spill/reload helpers
//===----------------------------------------------------------------------===//

// Some registers such as the accumulators of the DIV and MUL instructions (MDURegs regclasses)
// require to be copied into a GPIReg to be loaded or stored
// Also, depending on the frame register used in practice (either R0 or R31), and the offset,
// we have different instructions that may be used.
// These pseudo instruction are generated and expanded in Prologue/Epilogue Insertion & Frame Finalization
// pass, and thus are never extended in Post-RA pseudo instruction expansion pass.

let hasSideEffects = 0, mayStore = 1 in
class SpillInst<RegisterClass regclass> 
  : AltairXPseudo<(outs), (ins regclass:$reg, GPIReg64:$base, i64imm:$offset), []> {
}

def SPILLb : SpillInst<SpillableReg8>;
def SPILLw : SpillInst<SpillableReg16>;
def SPILLd : SpillInst<SpillableReg32>;
def SPILLq : SpillInst<SpillableReg64>;

let hasSideEffects = 0, mayLoad = 1 in
class ReloadInst<RegisterClass regclass> 
  : AltairXPseudo<(outs regclass:$reg), (ins GPIReg64:$base, i64imm:$offset), []> {
}

def RELOADb : SpillInst<SpillableReg8>;
def RELOADw : SpillInst<SpillableReg16>;
def RELOADd : SpillInst<SpillableReg32>;
def RELOADq : SpillInst<SpillableReg64>;

//===----------------------------------------------------------------------===//
// Comparisons
//===----------------------------------------------------------------------===//

// iX = AltairXISD::CMP iY, iY
def SDT_AltairXcmp : SDTypeProfile<0, 2, [SDTCisInt<0>, SDTCisSameAs<0, 1>]>;
def AltairXcmp : SDNode<"AltairXISD::CMP", SDT_AltairXcmp, []>;

let isCompare = 1, Defs = [CmpFlag] in
class CMPInstRR<bits<2> size, string sizename, RegisterClass regclass> 
  : AltairXInstCMPRR<OPCODE_CMP, size,
  (outs),
  (ins regclass:$rs1, regclass:$rs2),
  [(AltairXcmp regclass:$rs1, regclass:$rs2)],
  !strconcat("cmp.", sizename), "$rs1, $rs2"> {
}

def CmpRRb : CMPInstRR<0, "b", GPIReg8>;
def CmpRRw : CMPInstRR<1, "w", GPIReg16>;
def CmpRRd : CMPInstRR<2, "d", GPIReg32>;
def CmpRRq : CMPInstRR<3, "q", GPIReg64>;

let isCompare = 1, Defs = [CmpFlag] in
class CMPInstRI<bits<2> size, string sizename, RegisterClass regclass, Operand immop, ImmLeaf immpat> 
  : AltairXInstCMPRI<OPCODE_CMP, size,
  (outs),
  (ins regclass:$rs1, immop:$imm),
  [(AltairXcmp regclass:$rs1, immpat:$imm)],
  !strconcat("cmp.", sizename), "$rs1, $imm"> {
}
def CmpRIb : CMPInstRI<0, "b", GPIReg8 , i8imm,  s8imm32>;
def CmpRIw : CMPInstRI<1, "w", GPIReg16, i16imm, s16imm32>;
def CmpRId : CMPInstRI<2, "d", GPIReg32, i32imm, s32imm32>;
def CmpRIq : CMPInstRI<3, "q", GPIReg64, i64imm, s64imm32>;

//===----------------------------------------------------------------------===//
// Booleans and conditional move
// These instructions return i8 because we support i8 natively in every instruction and booleans are i8
//===----------------------------------------------------------------------===//

// Pattern that should match this instruction
//  and(setne(i, 0), setne(j, 0)) -> sand(i, j)
// Imm version are useless in practice
// compiler will generate a simplier code that can be matched using se or sen
class SANDInstRR<bits<2> size, string sizename, RegisterClass regclass> 
  : AltairXInstALURR<OPCODE_SAND, size, // this technically returns a 1-bit int
    (outs GPIReg8:$rd), 
    (ins regclass:$rs1, regclass:$rs2),
    [(set GPIReg8:$rd, (and (setne regclass:$rs1, 0), (setne regclass:$rs2, 0)))],
    !strconcat("sand.", sizename), "$rd, $rs1, $rs2"> {
}

def SAndRRb : SANDInstRR<0, "b", GPIReg8>;
def SAndRRw : SANDInstRR<1, "w", GPIReg16>;
def SAndRRd : SANDInstRR<2, "d", GPIReg32>;
def SAndRRq : SANDInstRR<3, "q", GPIReg64>;

// SCMP supports natively 4 predicates
// - Equal (se)
// - Not equal (sen)
// - Less than (lt, ltu)
// - Greater than predicate can be matched by swapping the compared values: (a > b) -> (b < a)
// Greater or equal, less than or equal, need an additional xor or setne, this is done during ISelLowering
class SCMPInstRR<AltairXOpcode opcode, string name, bits<2> size, string sizename,
  SDPatternOperator setccop, RegisterClass regclass>
  : AltairXInstALURR<opcode, size,
    (outs GPIReg8:$rd),
    (ins regclass:$rs1, regclass:$rs2),
    [(set GPIReg8:$rd, (setccop regclass:$rs1, regclass:$rs2))],
    !strconcat(name, ".", sizename), "$rd, $rs1, $rs2"> {
}

class SCMPInstRI<AltairXOpcode opcode, string name, bits<2> size, string sizename, 
  SDPatternOperator setccop, RegisterClass regclass, Operand immop, ImmLeaf immpat>
  : AltairXInstALURI<opcode, size,
    (outs GPIReg8:$rd),
    (ins regclass:$rs1, immop:$imm),
    [(set GPIReg8:$rd, (setccop regclass:$rs1, immpat:$imm))],
    !strconcat(name, ".", sizename), "$rd, $rs1, $imm"> {
}

multiclass SCMPInstImpl<AltairXOpcode opcode, string name, SDPatternOperator setccop> {
  def RRb : SCMPInstRR<opcode, name, 0, "b", setccop, GPIReg8>;
  def RRw : SCMPInstRR<opcode, name, 1, "w", setccop, GPIReg16>;
  def RRd : SCMPInstRR<opcode, name, 2, "d", setccop, GPIReg32>;
  def RRq : SCMPInstRR<opcode, name, 3, "q", setccop, GPIReg64>;
  def RIb : SCMPInstRI<opcode, name, 0, "b", setccop, GPIReg8,  i8imm,  s8imm32>;
  def RIw : SCMPInstRI<opcode, name, 1, "w", setccop, GPIReg16, i16imm, s16imm32>;
  def RId : SCMPInstRI<opcode, name, 2, "d", setccop, GPIReg32, i32imm, s32imm32>;
  def RIq : SCMPInstRI<opcode, name, 3, "q", setccop, GPIReg64, i64imm, s64imm32>;
}

defm Se : SCMPInstImpl<OPCODE_SE, "se", seteq>;
defm Sen : SCMPInstImpl<OPCODE_SEN, "sen", setne>;
defm Slts : SCMPInstImpl<OPCODE_SLTS, "slts", setlt>;
defm Sltu : SCMPInstImpl<OPCODE_SLTU, "sltu", setult>;

// iX = AltairXISD::SBIT $rA, $rB
def SDT_AltairXsbit : SDTypeProfile<1, 2, [SDTCisInt<0>, SDTCisInt<1>, SDTCisSameAs<1, 2>]>;
def AltairXsbit : SDNode<"AltairXISD::SBIT", SDT_AltairXsbit, []>;

class SBITInstRR<bits<2> size, RegisterClass regclass> 
  : AltairXInstALURR<OPCODE_SBIT, size,
    (outs GPIReg8:$rd),
    (ins regclass:$rs1, regclass:$rs2),
    [(set GPIReg8:$rd, (AltairXsbit regclass:$rs1, regclass:$rs2))],
    "sbit", "$rd, $rs1, $rs2"> {
}

class SBITInstRI<bits<2> size, RegisterClass regclass, Operand immop, ImmLeaf immpat> 
  : AltairXInstALURI<OPCODE_SBIT, size,
    (outs GPIReg8:$rd),
    (ins regclass:$rs1, immop:$imm),
    [(set GPIReg8:$rd, (AltairXsbit regclass:$rs1, immpat:$imm))],
    "sbit", "$rd, $rs1, $imm"> {
}

def SBitRRb : SBITInstRR<0, GPIReg8>;
def SBitRRw : SBITInstRR<1, GPIReg16>;
def SBitRRd : SBITInstRR<2, GPIReg32>;
def SBitRRq : SBITInstRR<3, GPIReg64>;
def SBitRIb : SBITInstRI<0, GPIReg8,  i8imm,  s8imm32>;
def SBitRIw : SBITInstRI<1, GPIReg16, i16imm, s16imm32>;
def SBitRId : SBITInstRI<2, GPIReg32, i32imm, s32imm32>;
def SBitRIq : SBITInstRI<3, GPIReg64, i64imm, s64imm32>;

// iX = AltairXISD::CMOVE tval, fval, bool
def SDT_AltairXcmove : SDTypeProfile<1, 3, [SDTCisInt<0>, SDTCisInt<1>, SDTCisSameAs<1, 3>, SDTCisVT<2, i8>]>;
def AltairXcmove : SDNode<"AltairXISD::CMOVE", SDT_AltairXcmove, []>;

// CMOVE, first parameter is also the output
let Constraints = "$out = $rd" in
class CMoveRRInst<bits<2> size, string sizename, RegisterClass regclass> 
  : AltairXInstALURR<OPCODE_CMOVE, size,
    (outs regclass:$out),
    (ins regclass:$rd, GPIReg8:$rs1, regclass:$rs2),
    [(set regclass:$out, (AltairXcmove regclass:$rd, GPIReg8:$rs1, regclass:$rs2))],
    !strconcat("cmove.", sizename), "$rd, $rs1, $rs2"> {
}

let Constraints = "$out = $rd" in
class CMoveRIInst<bits<2> size, string sizename, RegisterClass regclass, Operand immop, ImmLeaf immpat> 
  : AltairXInstALURI<OPCODE_CMOVE, size,
    (outs regclass:$out),
    (ins regclass:$rd, GPIReg8:$rs1, immop:$imm),
    [(set regclass:$out, (AltairXcmove regclass:$rd, GPIReg8:$rs1, immpat:$imm))],
    !strconcat("cmove.", sizename), "$rd, $rs1, $imm"> {
}

def CMoveRRb : CMoveRRInst<0, "b", GPIReg8>;
def CMoveRRw : CMoveRRInst<1, "w", GPIReg16>;
def CMoveRRd : CMoveRRInst<2, "d", GPIReg32>;
def CMoveRRq : CMoveRRInst<3, "q", GPIReg64>;

def CMoveRIb : CMoveRIInst<0, "b", GPIReg8,  i8imm,  s8imm32>;
def CMoveRIw : CMoveRIInst<1, "w", GPIReg16, i16imm, s16imm32>;
def CMoveRId : CMoveRIInst<2, "d", GPIReg32, i32imm, s32imm32>;
def CMoveRIq : CMoveRIInst<3, "q", GPIReg64, i64imm, s64imm32>;
