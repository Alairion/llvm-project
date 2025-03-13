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

defm "" : SizedUImm<3>;
defm "" : SizedUImm<18>;

// Addressing mode
def addr : ComplexPattern<iPTR, 3, "selectAddr", [], []>;
def addrimm : ComplexPattern<iPTR, 2, "selectAddrImm", [add, frameindex], []>;

def SDT_AltairXGAWRAPPER : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>]>;
def AltairXGAWRAPPER : SDNode<"AltairXISD::GAWRAPPER", SDT_AltairXGAWRAPPER, []>;

def AltairXGlobalAddrValue : AltairXPseudo<
  (outs GPIReg64:$ptr),
  (ins i64imm:$addr),
  [(set GPIReg64:$ptr, (AltairXGAWRAPPER tglobaladdr:$addr))],
  "move $ptr, $addr"
>;

def : Pat<(AltairXGAWRAPPER tjumptable:$addr),
  (AltairXGlobalAddrValue tjumptable:$addr)>;

//===----------------------------------------------------------------------===//
// Procedure calling
//===----------------------------------------------------------------------===//

// Procedure calling
// These are target-independent nodes, but have target-specific formats.
def SDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_CallSeqStart, [SDNPHasChain, SDNPOutGlue]>;
def SDT_CallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END", SDT_CallSeqEnd, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

let Defs = [R0], Uses = [R0], hasSideEffects = 1, isCodeGenOnly = 1 in {
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
// Branches (unconditional)
//===----------------------------------------------------------------------===//

// Absolute
def calltarget : Operand<i64> {
  let EncoderMethod = "getAbsBranchTargetOpValue";
}

// Call sets R31 (link register)
def STD_AltairXcall : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;
def AltairXcall : SDNode<"AltairXISD::CALL", STD_AltairXcall, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

let Defs = [R31], isCall = 1 in
def Call : AltairXInstAbsBRA<OPCODE_CALL,
  (outs),
  (ins calltarget:$target),
  [(AltairXcall tglobaladdr:$target)],
  "call", "$target">;
def : Pat<(AltairXcall texternalsym:$func), (Call texternalsym:$func)>;

// Jump does not set R31 (link register)
def STD_AltairXjump : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;
def AltairXjump : SDNode<"AltairXISD::JUMP", STD_AltairXjump, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

let isCall = 1 in // not sure about that
def Jump : AltairXInstAbsBRA<OPCODE_JUMP,
  (outs),
  (ins calltarget:$target),
  [(AltairXjump tglobaladdr:$target)],
  "jump", "$target">;
def : Pat<(AltairXjump texternalsym:$func), (Jump texternalsym:$func)>;

// Relative
def brtarget : Operand<OtherVT> {
  let EncoderMethod = "getRelBranchTargetOpValue";
  let PrintMethod = "printRelBranchTarget";
  let OperandType = "OPERAND_PCREL";
}

let isTerminator = 1 in
def BRA : AltairXInstRelBRA<OPCODE_BRA,
  (outs),
  (ins brtarget:$target),
  [(br bb:$target)],
  "bra", "$target">;

//===----------------------------------------------------------------------===//
// Branches (indirect)
//===----------------------------------------------------------------------===//

// Real instructions:
// Indirect call, store link address to first operand, and jump to second operand
let isCall = 1 in
def IndirectCallLink : AltairXInstIndirectBRA<OPCODE_INDIRECT_CALL,
  (outs GPIReg64:$link),
  (ins GPIReg64:$target),
  [], // no default pattern
  "call", "$link, $target">;

// Indirect callr, store link address to first operand, and add second operand to PC
let isCall = 1 in
def IndirectCallRelLink : AltairXInstIndirectBRA<OPCODE_INDIRECT_CALL,
  (outs GPIReg64:$link),
  (ins GPIReg64:$target),
  [], // no default pattern
  "call", "$link, $target">;

// Pseudo instruction for instruction selection!
def STD_AltairXindirectcall : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def AltairXindirectcall : SDNode<"AltairXISD::INDIRECT_CALL", STD_AltairXindirectcall, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

// IndirectCall rB is IndirectCallLink r31, rB
// Simplifies instruction selection
let Defs = [R31], isCall = 1, isBarrier = 1, isIndirectBranch = 1 in
def IndirectCall : AltairXPseudo<
  (outs),
  (ins GPIReg64:$target),
  [(AltairXindirectcall GPIReg64:$target)],
  "call", "$target">;

// Indirect jump to first operand
def STD_AltairXindirectjump : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def AltairXindirectjump : SDNode<"AltairXISD::INDIRECT_JUMP", STD_AltairXindirectjump, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

// Indirect jump is call $zero, rB
let isBarrier = 1, isIndirectBranch = 1, isTerminator = 1 in
def IndirectJump : AltairXPseudo<
  (outs),
  (ins GPIReg64:$target),
  [(AltairXindirectjump GPIReg64:$target)],
  "jump", "$target">;

// Indirect jump to first operand
def STD_AltairXindirectbra : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def AltairXindirectbra : SDNode<"AltairXISD::INDIRECT_BRA", STD_AltairXindirectbra, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;

// Indirect jump is call $zero, rB
let isBarrier = 1, isIndirectBranch = 1, isTerminator = 1 in
def IndirectBRA : AltairXPseudo<
  (outs),
  (ins GPIReg64:$target),
  [(AltairXindirectbra GPIReg64:$target)],
  "bra", "$target">;

//===----------------------------------------------------------------------===//
// Branches (conditional)
//===----------------------------------------------------------------------===//

// Comparisons

// FR = AltairXISD::CMP iY, iY
def SDT_AltairXcmp : SDTypeProfile<0, 2, [SDTCisInt<0>, SDTCisSameAs<0, 1>]>;
def AltairXcmp : SDNode<"AltairXISD::CMP", SDT_AltairXcmp, []>;

let isCompare = 1, Defs = [FR] in
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

let isCompare = 1, Defs = [FR] in
class CMPInstRI<bits<2> size, string sizename, RegisterClass regclass, Operand immop>
  : AltairXInstCMPRI<OPCODE_CMP, size,
  (outs),
  (ins regclass:$rs1, immop:$imm),
  [],
  !strconcat("cmp.", sizename), "$rs1, $imm"> {
}

def CmpRIb : CMPInstRI<0, "b", GPIReg8,  i8imm>;
def CmpRIw : CMPInstRI<1, "w", GPIReg16, i16imm>;
def CmpRId : CMPInstRI<2, "d", GPIReg32, i32imm>;
def CmpRIq : CMPInstRI<3, "q", GPIReg64, i64imm>;

def condcode : Operand<i32> {
  let EncoderMethod = "getImmOpValue";
  let PrintMethod = "printCondCode";
}

def brcpred : Operand<i32> {
  let EncoderMethod = "getImmOpValue";
  let PrintMethod = "printBRCPrediction";
}

// AltairXISD::BRCOND $bb, $cond, $fr
def SDT_AltairXbrcond : SDTypeProfile<0, 3, [SDTCisVT<0, OtherVT>, SDTCisVT<1, i32>, SDTCisVT<2, i32>]>;
def AltairXbrcond : SDNode<"AltairXISD::BRCOND", SDT_AltairXbrcond, [SDNPHasChain]>;

// PseudoBRC is a BRC that supports all predicates for simplicity (e.g. reverseBranch implementation)
// It is extended in AltairXBranchPatcher pass, which does more than this actually.
let isBranch = 1, isTerminator = 1, Uses = [FR] in
def PseudoBRC : AltairXPseudo<
  (outs),
  (ins brtarget:$target, i32imm:$cond),
  [(AltairXbrcond bb:$target, imm:$cond, FR)]>;

let Uses = [FR] in
def BRC : AltairXInstBRC<OPCODE_BRC,
    (outs),
    (ins brtarget:$target, condcode:$cond, brcpred:$pred),
    [], // no pattern, generated by PseudoBRC expansion
    "b${cond}.${pred}", "$target"> {
}

//===----------------------------------------------------------------------===//
// Procedure return
//===----------------------------------------------------------------------===//

def STD_AltairXret : SDTypeProfile<0, 0, []>;
def AltairXret : SDNode<"AltairXISD::RET", STD_AltairXret, [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

// Ret is IndirectCall $zero, $lr
// use a pseudo instruction to mark it as return
let Uses = [R31], isReturn = 1, isBarrier = 1, isTerminator = 1, hasCtrlDep = 1 in
def Ret : AltairXPseudo<(outs), (ins), [(AltairXret)], "ret">;
//def RETI : AltairXSpecialJumpInst<OPCODE_SPECIAL_JUMP, [], "reti", 0b1100>;

//===----------------------------------------------------------------------===//
// MoveIX support
//===----------------------------------------------------------------------===//

// moveix encoding of imm is a bit tricky, but basically consist of:
// (imm >> N) ^ -1 for most signed imm and (imm >> N) for unsigned immediates
def moveiximm : Operand<i64> {
  let EncoderMethod = "getMoveIXOpValue";
}

// Parent format is used to know dynamically the size of the imm
def MOVEIX23PCREL : AltairXInstMoveIX<InstFormatBRURelImm23, (outs), (ins moveiximm:$imm), []>;
def MOVEIX24PCREL : AltairXInstMoveIX<InstFormatBRURelImm24, (outs), (ins moveiximm:$imm), []>;
def MOVEIX24ABS : AltairXInstMoveIX<InstFormatBRUAbsImm24, (outs), (ins moveiximm:$imm), []>;
def MOVEIX9 : AltairXInstMoveIX<InstFormatALURegImm9, (outs), (ins moveiximm:$imm), []>;
def MOVEIX10 : AltairXInstMoveIX<InstFormatLSURegImm10, (outs), (ins moveiximm:$imm), []>;
def MOVEIX18 : AltairXInstMoveIX<InstFormatMoveImm18, (outs), (ins moveiximm:$imm), []>;

//===----------------------------------------------------------------------===//
// ALU basics operations
//===----------------------------------------------------------------------===//

class ALUInstRR<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode, RegisterClass regclass, RegisterClass rightclass = regclass>
  : AltairXInstALURR<opcode, size,
    (outs regclass:$rd),
    (ins regclass:$rs1, rightclass:$rs2),
    [(set regclass:$rd, (opnode regclass:$rs1, rightclass:$rs2))],
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
  def RRb : ALUInstRR<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8>;
  def RRw : ALUInstRR<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16>;
  def RRd : ALUInstRR<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32>;
  def RRq : ALUInstRR<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64>;
  def RIb : ALUInstRI<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8, i8imm, s8imm32>;
  def RIw : ALUInstRI<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16, i16imm, s16imm32>;
  def RId : ALUInstRI<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32, i32imm, s32imm32>;
  def RIq : ALUInstRI<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64, i64imm, s64imm32>;
}

let isAdd = 1, isReMaterializable = 1 in
defm Add : ALUInst<"add", OPCODE_ADD, add>;
defm Sub : ALUInst<"sub", OPCODE_SUB, sub>;
defm Xor : ALUInst<"xor", OPCODE_XOR, xor>;
defm Or  : ALUInst<"or",  OPCODE_OR,  or>;
defm And : ALUInst<"and", OPCODE_AND, and>;

def shiftAmount : Operand<i64> {
  let EncoderMethod = "getImmOpValue";
}

// Shifts always use 64-bits right operands
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
multiclass ALUInstShift<string name, AltairXOpcode opcode, SDNode opnode> {
  def RRb : ALUInstRR<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8,  GPIReg64>;
  def RRw : ALUInstRR<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16, GPIReg64>;
  def RRd : ALUInstRR<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32, GPIReg64>;
  def RRq : ALUInstRR<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64, GPIReg64>;
  def RIb : ALUInstRI<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8,  shiftAmount, s64imm32>;
  def RIw : ALUInstRI<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16, shiftAmount, s64imm32>;
  def RId : ALUInstRI<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32, shiftAmount, s64imm32>;
  def RIq : ALUInstRI<!strconcat(name, ".q"), 3, opcode, opnode, GPIReg64, shiftAmount, s64imm32>;
}

defm Lsl : ALUInstShift<"lsl", OPCODE_LSL, shl>;
defm Asr : ALUInstShift<"asr", OPCODE_ASR, sra>;
defm Lsr : ALUInstShift<"lsr", OPCODE_LSR, srl>;
defm RtL : ALUInstShift<"rtl", OPCODE_RTL, rotl>;
defm RtR : ALUInstShift<"rtr", OPCODE_RTR, rotr>;

//===----------------------------------------------------------------------===//
// MDU (mul, div, rem)
//===----------------------------------------------------------------------===//

// These instructions are specified as "no outs" because register allocation seems to behave
// incorrectly with forced registers that are part of a register class.
class MDUInstRR<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode,
  Register outreg, Register byproductreg, RegisterClass inclass>
  : AltairXInstMDURR<opcode, size,
    (outs),
    (ins inclass:$rs1, inclass:$rs2),
    [(set outreg, (opnode inclass:$rs1, inclass:$rs2))],
    name, "$rs1, $rs2"> {
  let Defs = [outreg, byproductreg];
}

class MDUInstRI<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode,
  Register outreg, Register byproductreg, RegisterClass inclass, Operand immop, ImmLeaf immpat>
  : AltairXInstMDURI<opcode, size,
    (outs),
    (ins inclass:$rs1, immop:$imm),
    [(set outreg, (opnode inclass:$rs1, immpat:$imm))],
    name, "$rs1, $imm"> {
  let Defs = [outreg, byproductreg];
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
multiclass MDUInst<string name, AltairXOpcode opcode, SDNode opnode, list<Register> outregs, list<Register> byproductregs> {
  // All of ops defined by this class set a "main" register
  def RRb : MDUInstRR<!strconcat(name, ".b"), 0, opcode, opnode, outregs[0], byproductregs[0], GPIReg8>;
  def RIb : MDUInstRI<!strconcat(name, ".b"), 0, opcode, opnode, outregs[0], byproductregs[0], GPIReg8, i8imm, s8imm32>;
  def RRw : MDUInstRR<!strconcat(name, ".w"), 1, opcode, opnode, outregs[1], byproductregs[1], GPIReg16>;
  def RIw : MDUInstRI<!strconcat(name, ".w"), 1, opcode, opnode, outregs[1], byproductregs[1], GPIReg16, i16imm, s16imm32>;
  def RRd : MDUInstRR<!strconcat(name, ".d"), 2, opcode, opnode, outregs[2], byproductregs[2], GPIReg32>;
  def RId : MDUInstRI<!strconcat(name, ".d"), 2, opcode, opnode, outregs[2], byproductregs[2], GPIReg32, i32imm, s32imm32>;
  def RRq : MDUInstRR<!strconcat(name, ".q"), 3, opcode, opnode, outregs[3], byproductregs[3], GPIReg64>;
  def RIq : MDUInstRI<!strconcat(name, ".q"), 3, opcode, opnode, outregs[3], byproductregs[3], GPIReg64, i64imm, s64imm32>;
}

defm Div  : MDUInst<"div",  OPCODE_DIV,  sdiv, [RQb, RQw, RQd, RQ], [RQRb, RQRw, RQRd, RQR]>;
defm DivU : MDUInst<"divu", OPCODE_DIVU, udiv, [RQb, RQw, RQd, RQ], [RQRb, RQRw, RQRd, RQR]>;
defm Rem  : MDUInst<"div",  OPCODE_DIV,  srem, [RQRb, RQRw, RQRd, RQR], [RQb, RQw, RQd, RQ]>;
defm RemU : MDUInst<"divu", OPCODE_DIVU, urem, [RQRb, RQRw, RQRd, RQR], [RQb, RQw, RQd, RQ]>;

defm Mul   : MDUInst<"mul",  OPCODE_MUL,  mul, [RPLb, RPLw, RPLd, RPL], [RPHb, RPHw, RPHd, RPH]>;
defm MulH  : MDUInst<"mul",  OPCODE_MUL,  mulhs, [RPHb, RPHw, RPHd, RPH], [RPLb, RPLw, RPLd, RPL]>;
defm MulHU : MDUInst<"mulu", OPCODE_MULU, mulhu, [RPHb, RPHw, RPHd, RPH], [RPLb, RPLw, RPLd, RPL]>;

class GetMDInst<RegisterClass outclass, RegisterClass inclass>
  : AltairXInstMDUGetMD<OPCODE_GETMD,
    (outs outclass:$rd),
    (ins inclass:$pq),
    [], // no pattern here, this instructions will be generated by copyPhysReg when needed
    "getmd", "$rd, $pq"> {
}

// R <- Q/QR/PH/PL
def GetMDb : GetMDInst<GPIReg8, MDUReg8>;
def GetMDw : GetMDInst<GPIReg16, MDUReg16>;
def GetMDd : GetMDInst<GPIReg32, MDUReg32>;
def GetMDq : GetMDInst<GPIReg64, MDUReg64>;

class SetMDInst<RegisterClass outclass, RegisterClass inclass>
  : AltairXInstMDUSetMD<OPCODE_SETMD,
    (outs outclass:$pq),
    (ins inclass:$rs1),
    [], // no pattern here, this instructions will be generated by copyPhysReg when needed
    "setmd", "$pq, $rs1"> {
}

// Q/QR/PH/PL <- R
def SetMDb : SetMDInst<MDUReg8, GPIReg8>;
def SetMDw : SetMDInst<MDUReg16, GPIReg16>;
def SetMDd : SetMDInst<MDUReg32, GPIReg32>;
def SetMDq : SetMDInst<MDUReg64, GPIReg64>;

//===----------------------------------------------------------------------===//
// sext, zext and trunc
// ADDS, SUBS
//===----------------------------------------------------------------------===//

// add/sub i[8,16,32,64] integers, then sext the result to i64
// Default pattern will match (sext (add/sub x, y) to iX)
class ALUSExtInstRR<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode, RegisterClass regclass>
  : AltairXInstALURR<opcode, size,
    (outs GPIReg64:$rd),
    (ins regclass:$rs1, regclass:$rs2),
    [(set GPIReg64:$rd, (i64 (sext (opnode regclass:$rs1, regclass:$rs2))))],
    name, "$rd, $rs1, $rs2"> {
}

// Imm version
class ALUSExtInstRI<string name, bits<2> size, AltairXOpcode opcode, SDNode opnode, RegisterClass regclass, Operand immop, ImmLeaf immpat>
  : AltairXInstALURI<opcode, size,
    (outs GPIReg64:$rd),
    (ins regclass:$rs1, immop:$imm),
    [(set GPIReg64:$rd, (i64 (sext (opnode regclass:$rs1, immpat:$imm))))],
    name, "$rd, $rs1, $imm"> {
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
multiclass ALUSExtInst<string name, AltairXOpcode opcode, SDNode opnode> {
  def RRb : ALUSExtInstRR<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8>;
  def RRw : ALUSExtInstRR<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16>;
  def RRd : ALUSExtInstRR<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32>;
  def RIb : ALUSExtInstRI<!strconcat(name, ".b"), 0, opcode, opnode, GPIReg8, i8imm, s8imm32>;
  def RIw : ALUSExtInstRI<!strconcat(name, ".w"), 1, opcode, opnode, GPIReg16, i16imm, s16imm32>;
  def RId : ALUSExtInstRI<!strconcat(name, ".d"), 2, opcode, opnode, GPIReg32, i32imm, s32imm32>;
  // No 64-bits, because default pattern can not be matched!
}

defm AddS : ALUSExtInst<"adds", OPCODE_ADDS, add>;
defm SubS : ALUSExtInst<"subs", OPCODE_SUBS, sub>;

// sext
def : Pat<(i16 (sext GPIReg8:$src)),  (EXTRACT_SUBREG (AddSRIb GPIReg8:$src,  0), SubReg16)>;
def : Pat<(i32 (sext GPIReg8:$src)),  (EXTRACT_SUBREG (AddSRIb GPIReg8:$src,  0), SubReg32)>;
def : Pat<(i32 (sext GPIReg16:$src)), (EXTRACT_SUBREG (AddSRIw GPIReg16:$src, 0), SubReg32)>;
def : Pat<(i64 (sext GPIReg8:$src)),  (AddSRIb GPIReg8:$src, 0)>;
def : Pat<(i64 (sext GPIReg16:$src)), (AddSRIw GPIReg16:$src, 0)>;
def : Pat<(i64 (sext GPIReg32:$src)), (AddSRId GPIReg32:$src, 0)>;

// sext inreg
def : Pat<(sext_inreg GPIReg16:$src, i8),  (EXTRACT_SUBREG (AddSRIb (SUBREG_TO_REG GPIReg16, $src, SubReg8),  0), SubReg16)>;
def : Pat<(sext_inreg GPIReg32:$src, i8),  (EXTRACT_SUBREG (AddSRIb (SUBREG_TO_REG GPIReg32, $src, SubReg8),  0), SubReg32)>;
def : Pat<(sext_inreg GPIReg32:$src, i16), (EXTRACT_SUBREG (AddSRIw (SUBREG_TO_REG GPIReg32, $src, SubReg16), 0), SubReg32)>;
def : Pat<(sext_inreg GPIReg64:$src, i8),  (AddSRIb (EXTRACT_SUBREG GPIReg64:$src, SubReg8), 0)>;
def : Pat<(sext_inreg GPIReg64:$src, i16), (AddSRIw (EXTRACT_SUBREG GPIReg64:$src, SubReg16), 0)>;
def : Pat<(sext_inreg GPIReg64:$src, i32), (AddSRId (EXTRACT_SUBREG GPIReg64:$src, SubReg32), 0)>;

// trunc (simply extract subreg)
def : Pat<(i8  (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, SubReg8)>;
def : Pat<(i8  (trunc GPIReg16:$src)), (EXTRACT_SUBREG GPIReg16:$src, SubReg8)>;
def : Pat<(i8  (trunc GPIReg32:$src)), (EXTRACT_SUBREG GPIReg32:$src, SubReg8)>;
def : Pat<(i16 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, SubReg16)>;
def : Pat<(i16 (trunc GPIReg32:$src)), (EXTRACT_SUBREG GPIReg32:$src, SubReg16)>;
def : Pat<(i32 (trunc GPIReg64:$src)), (EXTRACT_SUBREG GPIReg64:$src, SubReg32)>;

// anyext/zext
// Generic anyext/zext, simply "add.<targetsize> value, 0" to zext.
// This will be a fallback
def : Pat<(i64 (zext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg64, (AddRIb GPIReg8:$src, 0),  SubReg8)>;
def : Pat<(i64 (zext GPIReg16:$src)), (SUBREG_TO_REG GPIReg64, (AddRIw GPIReg16:$src, 0), SubReg16)>;
def : Pat<(i64 (zext GPIReg32:$src)), (SUBREG_TO_REG GPIReg64, (AddRId GPIReg32:$src, 0), SubReg32)>;
def : Pat<(i32 (zext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg32, (AddRIb GPIReg8:$src,  0), SubReg8)>;
def : Pat<(i32 (zext GPIReg16:$src)), (SUBREG_TO_REG GPIReg32, (AddRIw GPIReg16:$src, 0), SubReg16)>;
def : Pat<(i16 (zext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg16, (AddRIb GPIReg8:$src,  0), SubReg8)>;
// same as zext
def : Pat<(i16 (anyext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg16, (AddRIb GPIReg8:$src,  0), SubReg8)>;
def : Pat<(i32 (anyext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg32, (AddRIb GPIReg8:$src,  0), SubReg8)>;
def : Pat<(i32 (anyext GPIReg16:$src)), (SUBREG_TO_REG GPIReg32, (AddRIw GPIReg16:$src, 0), SubReg16)>;
def : Pat<(i64 (anyext GPIReg8:$src)),  (SUBREG_TO_REG GPIReg64, (AddRIb GPIReg8:$src,  0), SubReg8)>;
def : Pat<(i64 (anyext GPIReg16:$src)), (SUBREG_TO_REG GPIReg64, (AddRIw GPIReg16:$src, 0), SubReg16)>;
def : Pat<(i64 (anyext GPIReg32:$src)), (SUBREG_TO_REG GPIReg64, (AddRId GPIReg32:$src, 0), SubReg32)>;

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
  def : Pat<(i64 (zext pat8:$src)),  (SUBREG_TO_REG reg64, reg8:$src,  SubReg8)>;
  def : Pat<(i64 (zext pat16:$src)), (SUBREG_TO_REG reg64, reg16:$src, SubReg16)>;
  def : Pat<(i64 (zext pat32:$src)), (SUBREG_TO_REG reg64, reg32:$src, SubReg32)>;
  def : Pat<(i32 (zext pat8:$src)),  (SUBREG_TO_REG reg32, reg8:$src,  SubReg8)>;
  def : Pat<(i32 (zext pat16:$src)), (SUBREG_TO_REG reg32, reg16:$src, SubReg16)>;
  def : Pat<(i16 (zext pat8:$src)),  (SUBREG_TO_REG reg16, reg8:$src,  SubReg8)>;
  // same as zext
  def : Pat<(i64 (anyext pat8:$src)),  (SUBREG_TO_REG reg64, reg8:$src,  SubReg8)>;
  def : Pat<(i64 (anyext pat16:$src)), (SUBREG_TO_REG reg64, reg16:$src, SubReg16)>;
  def : Pat<(i64 (anyext pat32:$src)), (SUBREG_TO_REG reg64, reg32:$src, SubReg32)>;
  def : Pat<(i32 (anyext pat8:$src)),  (SUBREG_TO_REG reg32, reg8:$src,  SubReg8)>;
  def : Pat<(i32 (anyext pat16:$src)), (SUBREG_TO_REG reg32, reg16:$src, SubReg16)>;
  def : Pat<(i16 (anyext pat8:$src)),  (SUBREG_TO_REG reg16, reg8:$src,  SubReg8)>;
  // Optimize "and-based" zext. This may be generated by an extend of trunc.
  def : Pat<(i64 (and (anyext pat8:$src),  (i64 0xFFFFFFFF))), (SUBREG_TO_REG reg64, reg8:$src,  SubReg8)>;
  def : Pat<(i64 (and (anyext pat16:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG reg64, reg16:$src, SubReg16)>;
  def : Pat<(i64 (and (anyext pat32:$src), (i64 0xFFFFFFFF))), (SUBREG_TO_REG reg64, reg32:$src, SubReg32)>;
  def : Pat<(i32 (and (anyext pat8:$src),  (i32 0x0000FFFF))), (SUBREG_TO_REG reg32, reg8:$src,  SubReg8)>;
  def : Pat<(i32 (and (anyext pat16:$src), (i32 0x0000FFFF))), (SUBREG_TO_REG reg32, reg16:$src, SubReg16)>;
  def : Pat<(i16 (and (anyext pat8:$src),  (i16 0x000000FF))), (SUBREG_TO_REG reg16, reg8:$src,  SubReg8)>;
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
// Loads
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
class LoadRRInst<AltairXOpcode opcode, RegisterClass regclass, SDPatternOperator opnode, string name, bits<2> size, string sizename>
  : AltairXInstLSURR<opcode, size,
  (outs regclass:$reg),
  (ins GPIReg64:$base, GPIReg64:$offset, shiftAmount:$shift),
  [(set regclass:$reg, (opnode (addr GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift)))],
  !strconcat(name, ".", sizename), "$reg, $base[$offset << $shift]"> {
}

// basic loads for each int size
def LoadRRb : LoadRRInst<OPCODE_LD, GPIReg8,  load, "ld", 0, "b">;
def LoadRRw : LoadRRInst<OPCODE_LD, GPIReg16, load, "ld", 1, "w">;
def LoadRRd : LoadRRInst<OPCODE_LD, GPIReg32, load, "ld", 2, "d">;
def LoadRRq : LoadRRInst<OPCODE_LD, GPIReg64, load, "ld", 3, "q">;

// zextload: use instruction to ensure no additional operations are generated
def LoadRRbZX16 : LoadRRInst<OPCODE_LD, GPIReg16, zextloadi8_i16,  "ld", 0, "b">;
def LoadRRbZX32 : LoadRRInst<OPCODE_LD, GPIReg32, zextloadi8_i32,  "ld", 0, "b">;
def LoadRRbZX64 : LoadRRInst<OPCODE_LD, GPIReg64, zextloadi8_i64,  "ld", 0, "b">;
def LoadRRwZX32 : LoadRRInst<OPCODE_LD, GPIReg32, zextloadi16_i32, "ld", 1, "w">;
def LoadRRwZX64 : LoadRRInst<OPCODE_LD, GPIReg64, zextloadi16_i64, "ld", 1, "w">;
def LoadRRdZX64 : LoadRRInst<OPCODE_LD, GPIReg64, zextloadi32_i64, "ld", 2, "d">;

// extload: use instruction to ensure no additional operations are generated
def LoadRRbAX16 : LoadRRInst<OPCODE_LD, GPIReg16, extloadi8_i16,  "ld", 0, "b">;
def LoadRRbAX32 : LoadRRInst<OPCODE_LD, GPIReg32, extloadi8_i32,  "ld", 0, "b">;
def LoadRRbAX64 : LoadRRInst<OPCODE_LD, GPIReg64, extloadi8_i64,  "ld", 0, "b">;
def LoadRRwAX32 : LoadRRInst<OPCODE_LD, GPIReg32, extloadi16_i32, "ld", 1, "w">;
def LoadRRwAX64 : LoadRRInst<OPCODE_LD, GPIReg64, extloadi16_i64, "ld", 1, "w">;
def LoadRRdAX64 : LoadRRInst<OPCODE_LD, GPIReg64, extloadi32_i64, "ld", 2, "d">;

// basic loads for each float size
def FLoadRRs : LoadRRInst<OPCODE_FLD, FReg32, load, "fld", 1, "s">;
def FLoadRRd : LoadRRInst<OPCODE_FLD, FReg64, load, "fld", 2, "d">;

// sext (Load Reg + Reg << Shift) to i64
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadSExtRRInst<SDPatternOperator opnode, bits<2> size, string sizename>
  : AltairXInstLSURR<OPCODE_LDS, size,
  (outs GPIReg64:$reg),
  (ins GPIReg64:$base, GPIReg64:$offset, shiftAmount:$shift),
  [(set GPIReg64:$reg, (opnode (addr GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift)))],
  !strconcat("lds.", sizename), "$reg, $base[$offset << $shift]"> {
}

def LoadSExtRRb : LoadSExtRRInst<sextloadi8_i64,  0, "b">;
def LoadSExtRRw : LoadSExtRRInst<sextloadi16_i64, 1, "w">;
def LoadSExtRRd : LoadSExtRRInst<sextloadi32_i64, 2, "d">;

def : Pat<(sextloadi8_i16 (addr GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift)),
  (EXTRACT_SUBREG (LoadSExtRRb GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift), SubReg16)>;
def : Pat<(sextloadi8_i32 (addr GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift)),
  (EXTRACT_SUBREG (LoadSExtRRb GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift), SubReg32)>;
def : Pat<(sextloadi16_i32 (addr GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift)),
  (EXTRACT_SUBREG (LoadSExtRRw GPIReg64:$base, GPIReg64:$offset, u64imm3:$shift), SubReg32)>;

// Load Reg + Imm
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadRIInst<AltairXOpcode opcode, RegisterClass regclass, SDPatternOperator opnode, string name, bits<2> size, string sizename>
  : AltairXInstLSURI<opcode, size,
  (outs regclass:$reg),
  (ins GPIReg64:$base, i64imm:$offset),
  [(set regclass:$reg, (opnode (addrimm GPIReg64:$base, i64imm:$offset)))],
  !strconcat(name, ".", sizename), "$reg, $offset[$base]"> {
}

// basic loads for each size (imm version)
def LoadRIb : LoadRIInst<OPCODE_LDI, GPIReg8,  load, "ld", 0, "b">;
def LoadRIw : LoadRIInst<OPCODE_LDI, GPIReg16, load, "ld", 1, "w">;
def LoadRId : LoadRIInst<OPCODE_LDI, GPIReg32, load, "ld", 2, "d">;
def LoadRIq : LoadRIInst<OPCODE_LDI, GPIReg64, load, "ld", 3, "q">;

// zextload: use instruction to ensure no additional operations are generated  (imm version)
def LoadRIbZX16 : LoadRIInst<OPCODE_LDI, GPIReg16, zextloadi8_i16,  "ld", 0, "b">;
def LoadRIbZX32 : LoadRIInst<OPCODE_LDI, GPIReg32, zextloadi8_i32,  "ld", 0, "b">;
def LoadRIbZX64 : LoadRIInst<OPCODE_LDI, GPIReg64, zextloadi8_i64,  "ld", 0, "b">;
def LoadRIwZX32 : LoadRIInst<OPCODE_LDI, GPIReg32, zextloadi16_i32, "ld", 1, "w">;
def LoadRIwZX64 : LoadRIInst<OPCODE_LDI, GPIReg64, zextloadi16_i64, "ld", 1, "w">;
def LoadRIdZX64 : LoadRIInst<OPCODE_LDI, GPIReg64, zextloadi32_i64, "ld", 2, "d">;

// zextload: use instruction to ensure no additional operations are generated  (imm version)
def LoadRIbAX16 : LoadRIInst<OPCODE_LDI, GPIReg16, extloadi8_i16,  "ld", 0, "b">;
def LoadRIbAX32 : LoadRIInst<OPCODE_LDI, GPIReg32, extloadi8_i32,  "ld", 0, "b">;
def LoadRIbAX64 : LoadRIInst<OPCODE_LDI, GPIReg64, extloadi8_i64,  "ld", 0, "b">;
def LoadRIwAX32 : LoadRIInst<OPCODE_LDI, GPIReg32, extloadi16_i32, "ld", 1, "w">;
def LoadRIwAX64 : LoadRIInst<OPCODE_LDI, GPIReg64, extloadi16_i64, "ld", 1, "w">;
def LoadRIdAX64 : LoadRIInst<OPCODE_LDI, GPIReg64, extloadi32_i64, "ld", 2, "d">;

// basic loads for each float size (imm version)
def FLoadRIs : LoadRIInst<OPCODE_FLDI, FReg32, load, "fld", 1, "s">;
def FLoadRId : LoadRIInst<OPCODE_FLDI, FReg64, load, "fld", 2, "d">;

// sext (Load Reg + Imm) to i64
let canFoldAsLoad = 1, isReMaterializable = 1, hasSideEffects = 0, mayLoad = 1 in
class LoadSExtRIInst<SDPatternOperator opnode, bits<2> size, string sizename>
  : AltairXInstLSURI<OPCODE_LDIS, size,
  (outs GPIReg64:$reg),
  (ins GPIReg64:$base, i64imm:$offset),
  [(set GPIReg64:$reg, (opnode (addrimm GPIReg64:$base, i64imm:$offset)))],
  !strconcat("lds.", sizename), "$reg, $offset[$base]"> {
}

def LoadSExtRIb : LoadSExtRIInst<sextloadi8_i64,  0, "b">;
def LoadSExtRIw : LoadSExtRIInst<sextloadi16_i64, 1, "w">;
def LoadSExtRId : LoadSExtRIInst<sextloadi32_i64, 2, "d">;

def : Pat<(sextloadi8_i16 (addrimm GPIReg64:$base, i64imm:$offset)),
  (EXTRACT_SUBREG (LoadSExtRIb GPIReg64:$base, i64imm:$offset), SubReg16)>;
def : Pat<(sextloadi8_i32 (addrimm GPIReg64:$base, i64imm:$offset)),
  (EXTRACT_SUBREG (LoadSExtRIb GPIReg64:$base, i64imm:$offset), SubReg32)>;
def : Pat<(sextloadi16_i32 (addrimm GPIReg64:$base, i64imm:$offset)),
  (EXTRACT_SUBREG (LoadSExtRIw GPIReg64:$base, i64imm:$offset), SubReg32)>;

//===----------------------------------------------------------------------===//
// Store
//===----------------------------------------------------------------------===//

// Store Reg + Reg << Shift
let isReMaterializable = 1, hasSideEffects = 0, mayStore = 1 in
class StoreRRInst<AltairXOpcode opcode, RegisterClass regclass, SDPatternOperator opnode, string name, bits<2> size, string sizename>
  : AltairXInstLSURR<opcode, size,
  (outs),
  (ins regclass:$reg, GPIReg64:$base, GPIReg64:$offset, i64imm:$shift),
  [(opnode regclass:$reg, (addr GPIReg64:$base, GPIReg64:$offset, i64imm:$shift))],
  !strconcat(name, ".", sizename), "$reg, $base[$offset << $shift]"> {
}

// basic store for each float size
def StoreRRb : StoreRRInst<OPCODE_ST, GPIReg8,  store, "st", 0, "b">;
def StoreRRw : StoreRRInst<OPCODE_ST, GPIReg16, store, "st", 1, "w">;
def StoreRRd : StoreRRInst<OPCODE_ST, GPIReg32, store, "st", 2, "d">;
def StoreRRq : StoreRRInst<OPCODE_ST, GPIReg64, store, "st", 3, "q">;

// truncstore
def StoreRRbT16 : StoreRRInst<OPCODE_ST, GPIReg16, truncstorei8,  "st", 0, "b">;
def StoreRRbT32 : StoreRRInst<OPCODE_ST, GPIReg32, truncstorei8,  "st", 0, "b">;
def StoreRRbT64 : StoreRRInst<OPCODE_ST, GPIReg64, truncstorei8,  "st", 0, "b">;
def StoreRRwT32 : StoreRRInst<OPCODE_ST, GPIReg32, truncstorei16, "st", 1, "w">;
def StoreRRwT64 : StoreRRInst<OPCODE_ST, GPIReg64, truncstorei16, "st", 1, "w">;
def StoreRRdT64 : StoreRRInst<OPCODE_ST, GPIReg64, truncstorei32, "st", 2, "d">;

// basic store for each float size
def FStoreRRs : StoreRRInst<OPCODE_FST, FReg32, store, "fst", 1, "s">;
def FStoreRRd : StoreRRInst<OPCODE_FST, FReg64, store, "fst", 2, "d">;

// Store Reg + Imm
let isReMaterializable = 1, hasSideEffects = 0, mayStore = 1 in
class StoreRIInst<AltairXOpcode opcode, RegisterClass regclass, SDPatternOperator opnode, string name, bits<2> size, string sizename>
  : AltairXInstLSURI<opcode, size,
  (outs),
  (ins regclass:$reg, GPIReg64:$base, i64imm:$offset),
  [(opnode regclass:$reg, (addrimm GPIReg64:$base, i64imm:$offset))],
  !strconcat(name, ".", sizename), "$reg, $offset[$base]"> {
}

// basic store for each size (imm version)
def StoreRIb : StoreRIInst<OPCODE_STI, GPIReg8,  store, "st", 0, "b">;
def StoreRIw : StoreRIInst<OPCODE_STI, GPIReg16, store, "st", 1, "w">;
def StoreRId : StoreRIInst<OPCODE_STI, GPIReg32, store, "st", 2, "d">;
def StoreRIq : StoreRIInst<OPCODE_STI, GPIReg64, store, "st", 3, "q">;

// truncstore (imm version)
def StoreRIbT16 : StoreRIInst<OPCODE_STI, GPIReg16, truncstorei8,  "st", 0, "b">;
def StoreRIbT32 : StoreRIInst<OPCODE_STI, GPIReg32, truncstorei8,  "st", 0, "b">;
def StoreRIbT64 : StoreRIInst<OPCODE_STI, GPIReg64, truncstorei8,  "st", 0, "b">;
def StoreRIwT32 : StoreRIInst<OPCODE_STI, GPIReg32, truncstorei16, "st", 1, "w">;
def StoreRIwT64 : StoreRIInst<OPCODE_STI, GPIReg64, truncstorei16, "st", 1, "w">;
def StoreRIdT64 : StoreRIInst<OPCODE_STI, GPIReg64, truncstorei32, "st", 2, "d">;

// basic store for each float size (imm version)
def FStoreRIs : StoreRIInst<OPCODE_FSTI, FReg32, store, "fst", 1, "s">;
def FStoreRId : StoreRIInst<OPCODE_FSTI, FReg64, store, "fst", 2, "d">;

//===----------------------------------------------------------------------===//
// Booleans and conditional move
// These instructions return i8 because we support i8 natively in every
// instruction and booleans are i8
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
  : AltairXInstALURRR<OPCODE_CMOVE, size,
    (outs regclass:$out),
    (ins regclass:$rd, GPIReg8:$rs1, regclass:$rs2),
    [(set regclass:$out, (AltairXcmove regclass:$rd, GPIReg8:$rs1, regclass:$rs2))],
    !strconcat("cmove.", sizename), "$rd, $rs1, $rs2"> {
}

let Constraints = "$out = $rd" in
class CMoveRIInst<bits<2> size, string sizename, RegisterClass regclass, Operand immop, ImmLeaf immpat>
  : AltairXInstALURRI<OPCODE_CMOVE, size,
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

//===----------------------------------------------------------------------===//
// Floats (FPU)
//===----------------------------------------------------------------------===//

// Basic operations
class FPUInstRR<string name, bits<2> size, string sizename, AltairXOpcode opcode, PatFrags opnode, RegisterClass regclass>
  : AltairXInstFPURR<opcode, size,
    (outs regclass:$rd),
    (ins regclass:$rs1, regclass:$rs2),
    [(set regclass:$rd, (opnode regclass:$rs1, regclass:$rs2))],
    !strconcat(name, ".", sizename), "$rd, $rs1, $rs2"> {
}

def FAddRRs : FPUInstRR<"fadd", 1, "s", OPCODE_FADD, any_fadd, FReg32>;
def FAddRRd : FPUInstRR<"fadd", 2, "d", OPCODE_FADD, any_fadd, FReg64>;

def FSubRRs : FPUInstRR<"fsub", 1, "s", OPCODE_FSUB, any_fsub, FReg32>;
def FSubRRd : FPUInstRR<"fsub", 2, "d", OPCODE_FSUB, any_fsub, FReg64>;

def FMulRRs : FPUInstRR<"fmul", 1, "s", OPCODE_FMUL, any_fmul, FReg32>;
def FMulRRd : FPUInstRR<"fmul", 2, "d", OPCODE_FMUL, any_fmul, FReg64>;

// Float copy
class FPUInstMove<bits<2> size, string sizename, RegisterClass regclass>
  : AltairXInstFPUUnary<OPCODE_FMOVE, size,
    (outs regclass:$rd),
    (ins regclass:$rs1),
    [], // no pattern here, this instructions will be generated by copyPhysReg when needed
    !strconcat("fmove.", sizename), "$rd, $rs1"> {
}

def FMoveRs : FPUInstMove<1, "s", FReg32>;
def FMoveRd : FPUInstMove<2, "d", FReg64>;

def VIMoveRb : FPUInstMove<1, "d", VIReg8>; // int version for compatibility

// iX = AltairXISD::ITOF/FTOI outval, inval
def SDT_AltairXitof : SDTypeProfile<1, 1, [SDTCisFP<0>, SDTCisVT<1, f64>]>;
def AltairXitof : SDNode<"AltairXISD::ITOF", SDT_AltairXitof, []>;
def SDT_AltairXftoi : SDTypeProfile<1, 1, [SDTCisVT<0, f64>, SDTCisFP<1>]>;
def AltairXftoi : SDNode<"AltairXISD::FTOI", SDT_AltairXftoi, []>;

// Float conversions (extend, trunc, int <-> float)
class FPUCastInst<string name, bits<2> size, AltairXOpcode opcode, SDPatternOperator opnode, RegisterClass outclass, RegisterClass inclass>
  : AltairXInstFPUUnary<opcode, size,
    (outs outclass:$rd),
    (ins inclass:$rs1),
    [(set outclass:$rd, (opnode inclass:$rs1))],
    name, "$rd, $rs1"> {
}

/*
any_sint_to_fp
any_uint_to_fp // unsupported for u64
any_fp_to_sint
any_fp_to_uint // unsupported for u64
*/

// From int
def IqToFs : FPUCastInst<"itof", 1, OPCODE_ITOF, AltairXitof, FReg32, FReg64>;
def IqToFd : FPUCastInst<"itod", 2, OPCODE_ITOF, AltairXitof, FReg64, FReg64>;

// To int
def FsToIq : FPUCastInst<"ftoi", 1, OPCODE_FTOI, AltairXftoi, FReg64, FReg32>;
def FdToIq : FPUCastInst<"dtoi", 2, OPCODE_FTOI, AltairXftoi, FReg64, FReg64>;

// Extend
def FsToFd : FPUCastInst<"ftod", 2, OPCODE_FTOD, any_fpextend, FReg64, FReg32>;
// Trunc
def FdToFs : FPUCastInst<"dtof", 1, OPCODE_DTOF, any_fpround, FReg32, FReg64>;

// Float comparisons
// FR = AltairXISD::FCMP iY, iY
def SDT_AltairXfcmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def AltairXfcmp : SDNode<"AltairXISD::FCMP", SDT_AltairXfcmp, []>;

let isCompare = 1, Defs = [FR] in
class FCMPInstRR<bits<2> size, string sizename, RegisterClass regclass>
  : AltairXInstCMPRR<OPCODE_CMP, size,
  (outs),
  (ins regclass:$rs1, regclass:$rs2),
  [(AltairXfcmp regclass:$rs1, regclass:$rs2)],
  !strconcat("fcmp.", sizename), "$rs1, $rs2"> {
}

def FCmpRRs : FCMPInstRR<1, "s", FReg32>;
def FCmpRRd : FCMPInstRR<2, "d", FReg64>;

// FSCMP supports natively 3 predicates
// - Equal (se)
// - Not equal (sen)
// - Less than (lt)
// - Greater than predicate can be matched by swapping the compared values: (a > b) -> (b < a)
// Greater or equal, less than or equal, need an additional xor or setne, this is done during ISelLowering
class FSCMPInstRR<AltairXOpcode opcode, string name, bits<2> size, string sizename,
  SDPatternOperator setccop, RegisterClass regclass>
  : AltairXInstALURR<opcode, size,
    (outs VIReg8:$rd),
    (ins regclass:$rs1, regclass:$rs2),
    [(set VIReg8:$rd, (setccop regclass:$rs1, regclass:$rs2))],
    !strconcat(name, ".", sizename), "$rd, $rs1, $rs2"> {
}

multiclass FSCMPInstImpl<AltairXOpcode opcode, string name, SDPatternOperator setccop> {
  def RRs : FSCMPInstRR<opcode, name, 1, "s", setccop, FReg32>;
  def RRd : FSCMPInstRR<opcode, name, 2, "d", setccop, FReg64>;
}

defm FSe  : FSCMPInstImpl<OPCODE_FSE,  "fse",  seteq>;
defm FSen : FSCMPInstImpl<OPCODE_FSEN, "fsen", seteq>;
defm FSlt : FSCMPInstImpl<OPCODE_FSLT, "fslt", setlt>;

// iX = AltairXISD::FCMOVE tval, fval, bool
def SDT_AltairXfcmove : SDTypeProfile<1, 3, [SDTCisFP<0>, SDTCisFP<1>, SDTCisSameAs<1, 3>, SDTCisVT<2, i8>]>;
def AltairXfcmove : SDNode<"AltairXISD::FCMOVE", SDT_AltairXfcmove, []>;

// FCMOVE, first parameter is also the output
let Constraints = "$out = $rd" in
class FCMoveRRInst<bits<2> size, string sizename, RegisterClass regclass>
  : AltairXInstALURR<OPCODE_FCMOVE, size,
    (outs regclass:$out),
    (ins regclass:$rd, VIReg8:$rs1, regclass:$rs2),
    [(set regclass:$out, (AltairXfcmove regclass:$rd, VIReg8:$rs1, regclass:$rs2))],
    !strconcat("fcmove.", sizename), "$rd, $rs1, $rs2"> {
}

def FCMoveRRs : FCMoveRRInst<1, "s", FReg32>;
def FCMoveRRd : FCMoveRRInst<2, "d", FReg64>;

//===----------------------------------------------------------------------===//
// Floats (EFU)
//===----------------------------------------------------------------------===//

// This instruction is specified as "no outs" because register allocation seems to behave
// incorrectly with forced registers that are part of a register class.
class EFUInstRR<string name, bits<2> size, string sizename, AltairXOpcode opcode,
  PatFrags opnode, Register outreg, RegisterClass inclass>
  : AltairXInstEFURR<opcode, size,
    (outs),
    (ins inclass:$rs1, inclass:$rs2),
    [(set outreg, (opnode inclass:$rs1, inclass:$rs2))],
    !strconcat(name, ".", sizename), "$rs1, $rs2"> {
  let Defs = [outreg];
}

def FDivRRs : EFUInstRR<"fdiv", 1, "s", OPCODE_FDIV, any_fdiv, VQs, FReg32>;
def FDivRRd : EFUInstRR<"fdiv", 2, "d", OPCODE_FDIV, any_fdiv, VQd, FReg64>;

class GetEFInst<RegisterClass outclass, RegisterClass inclass>
  : AltairXInstEFUGetEF<OPCODE_SETEF,
    (outs outclass:$rd),
    (ins inclass:$rq),
    [], // no pattern here, this instructions will be generated by copyPhysReg when needed
    "getef", "$rd, $rq"> {
}

def GetEFs : GetEFInst<FReg32, EFReg32>;
def GetEFd : GetEFInst<FReg64, EFReg64>;

class SetEFInst<RegisterClass outclass, RegisterClass inclass>
  : AltairXInstEFUSetEF<OPCODE_SETEF,
    (outs outclass:$rq),
    (ins inclass:$rs1),
    [], // no pattern here, this instructions will be generated by copyPhysReg when needed
    "setef", "$rq, $rs1"> {
}

def SetEFs : SetEFInst<EFReg32, FReg32>;
def SetEFd : SetEFInst<EFReg64, FReg64>;

// Simple binary floating point operators
// FADD: supported
// FSUB: supported
// FMUL: supported
// FDIV: supported
// FREM: supported?
// FMA: expand - Perform a * b + c with no intermediate rounding step.
// FMAD: expand - Perform a * b + c, while getting the same result as the separately rounded operations.

// Simple unary floating point operators
// FNEG: supported
// FABS: supported
// FSQRT: supported
// FCBRT: expand (libcall)
// FSIN: supported
// FCOS: expand (no libcall?)
// FTAN: supported
// FASIN: expand (libcall)
// FACOS: expand (libcall)
// FATAN: supported
// FSINH: expand (libcall)
// FCOSH: expand (libcall)
// FTANH: expand (libcall)
// FPOW: expand (libcall) float exponent
// FPOWI: expand (libcall) integer exponent
// FLDEXP: expand (libcall) ldexp, inspired by libm (op0 * 2**op1).
// FATAN2: supported atan2, inspired by libm.
// FFREXP: expand (libcall) frexp, extract fractional and exponent component of a floating-point value. Returns the two components as separate return values.
// FLOG: expand (libcall)
// FLOG2: expand (libcall)
// FLOG10: expand (libcall)
// FEXP: supported
// FEXP2: expand (libcall)
// FEXP10: expand (libcall)

// Int/float conversions
// FP_TO_[US]INT: supported Convert a floating point value to a signed or unsigned integer. - These have the same semantics as fptosi and fptoui in IR. If the FP value cannot fit in the integer type, the results are undefined.
// FP_TO_[US]INT_SAT: expand (libcall) NaN -> zero; out of range -> min/max; rounding towards zero

// Rounding functions
// FCEIL: expand (libcall) closest int bigger than input
// FFLOOR: expand (libcall) closest int smaller than input
// FTRUNC: expand (libcall) closest toward 0
// FRINT: expand (libcall) Ties to even (default mode)
// FNEARBYINT: expand (libcall) (float to int) Ties to even (default mode)
// FROUND: expand (libcall) Away from 0
// FROUNDEVEN: expand (libcall) Ties to even
// SET/GET_ROUNDING: expand (libcall) -> NOOP (unsupported)
// L(L)ROUND: expand (libcall) Casting (float to int) Ties to even (default mode)
// L(L)RINT: expand (libcall) Casting (float to int) rounding operation toward 0
// FP_ROUND: expand (libcall) Casting (float to float) rounding operation toward 0
// FP_EXTEND: expand (libcall) Extend a smaller FP type into a larger FP type.

// FCOPYSIGN: expand (libcall) Return the value of X with the sign of Y.
// FGETSIGN: expand (libcall)	Return the sign bit of the specified floating point value as an integer 0/1 value.
// FCANONICALIZE: ???? Returns platform specific canonical encoding of a floating point number.

// These operators are used to perform promotions and truncation for half-precision (16 bit) floating numbers.
// FP16_TO_FP, FP_TO_FP16: supported

// FMINNUM/FMAXNUM: supported NaN, notNaN -> notNaN, 0.0, -0.0 -> 0.0 or -0.0 IEEE 754-2008 semantics
// FMINIMUM/FMAXIMUM: expand (libcall) follow IEEE 754-2019 semantics.
// FMINIMUMNUM/FMAXIMUMNUM: expand (libcall) same with FMINNUM_IEEE and FMAXNUM_IEEE besides if either operand is sNaN

// FSINCOS: expand

//===----------------------------------------------------------------------===//
// Miscellaneous
//===----------------------------------------------------------------------===//

// intern regs moves
// Does not put pattern here, this instructions will be generated by copyPhysReg when needed
// R <- FR/PC
def GetIR : AltairXInstCUGetIR<OPCODE_GETIR,
    (outs GPIReg32:$rd),
    (ins RIReg32:$ir),
    [],
    "getir", "$rd, $ir"> {
}

// FR <- R
def SetFR : AltairXInstCUSetFR<OPCODE_SETFR,
    (outs RIReg32:$ri),
    (ins GPIReg32:$rs1),
    [],
    "setfr", "$ri, $rs1"> {
}

// BITCAST
// Generic bitcast can be done using bypass
// To bitcast from an int to a float:
//   add acc, rX, 0 ; can be omitted if previous instruction is the producer of rX
//   fmove vX, ba1 ; can be omitted if next instruction is the only consumer of vX
// To bitcast from an float to a int:
//   fmove acc, vX ; can be omitted if previous instruction is the producer of vX
//   add rX, bf1, 0 ; can be omitted if next instruction is the only consumer of rX
class BitcastInst<RegisterClass inclass, RegisterClass outclass>
 : AltairXPseudo<
  (outs outclass:$rd),
  (ins inclass:$rs1),
  [(set outclass:$rd, (bitconvert inclass:$rs1))],
  "bitcast $rd, $rs1"
>;

def BitcastIqToFd : BitcastInst<GPIReg64, FReg64>;
def BitcastIdToFs : BitcastInst<GPIReg32, FReg32>;
def BitcastFdToIq : BitcastInst<FReg64, GPIReg64>;
def BitcastFsToId : BitcastInst<FReg32, GPIReg32>;


// Assembler aliases
include "AltairXInstrAliases.td"
