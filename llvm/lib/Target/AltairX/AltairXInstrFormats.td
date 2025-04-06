//=== AltairXInstrFormats.td - AltairX Instruction Formats -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Format specifies the encoding used by the instruction
//===----------------------------------------------------------------------===//
class InstFormat<bits<64> val> {
  bits<64> Value = val;
}
def InstFormatPseudo : InstFormat<0>;
def InstFormatRegReg : InstFormat<1>;
def InstFormatRegImm : InstFormat<2>;

//===----------------------------------------------------------------------===//
// Opcodes
//===----------------------------------------------------------------------===//
// AltairX ISA
// https://docs.google.com/spreadsheets/d/1AmdMslRcXIX9pKGBSRJJcx2IvRyzBLjA61SzxmlEYf8/edit#gid=0
class AltairXOpcode {
  bits<8> Value;
}

class AltairXOpcodeALU0<bits<4> val>
  : AltairXOpcode {
  let Value{7-5} = 0b000;
  let Value{4-1} = val;
  let Value{0} = 0;
}

def OPCODE_MOVEIX : AltairXOpcodeALU0<0b0000>;
def OPCODE_EXT : AltairXOpcodeALU0<0b0110>;
def OPCODE_INS : AltairXOpcodeALU0<0b1110>;
def OPCODE_MOVEI : AltairXOpcodeALU0<0b0010>;
def OPCODE_MOVEN : AltairXOpcodeALU0<0b1010>;
def OPCODE_UMOVE : AltairXOpcodeALU0<0b1100>;

def OPCODE_CMP  : AltairXOpcodeALU0<0b0001>;
def OPCODE_TEST : AltairXOpcodeALU0<0b1001>;

def OPCODE_CMOVE : AltairXOpcodeALU0<0b0011>;
def OPCODE_SEXT : AltairXOpcodeALU0<0b1011>;

class AltairXOpcodeALU1Generic // Used for se/stl implementation
  : AltairXOpcode {
  let Value{7-5} = 0b100;
  let Value{0} = 0;
}

// SLT, SLTU, SE
def OPCODE_SCMP : AltairXOpcodeALU1Generic;

class AltairXOpcodeALU1<bits<4> val>
  : AltairXOpcodeALU1Generic {
  let Value{4-1} = val;
}

def OPCODE_ADD : AltairXOpcodeALU1<0b0000>;
def OPCODE_SUB : AltairXOpcodeALU1<0b1000>;
def OPCODE_XOR : AltairXOpcodeALU1<0b0100>;
def OPCODE_OR : AltairXOpcodeALU1<0b1100>;
def OPCODE_AND : AltairXOpcodeALU1<0b0010>;
def OPCODE_LSL : AltairXOpcodeALU1<0b1010>;
def OPCODE_ASR : AltairXOpcodeALU1<0b0110>;
def OPCODE_LSR : AltairXOpcodeALU1<0b1110>;

class AltairXOpcodeLSU<bits<4> val>
  : AltairXOpcode {
  let Value{7-5} = 0b010;
  let Value{4-1} = val;
  let Value{0} = 0;
}

def OPCODE_LD   : AltairXOpcodeLSU<0b0000>;
def OPCODE_ST   : AltairXOpcodeLSU<0b1000>;
def OPCODE_LDV  : AltairXOpcodeLSU<0b0100>;
def OPCODE_STV  : AltairXOpcodeLSU<0b1100>;
def OPCODE_LDI  : AltairXOpcodeLSU<0b0010>;
def OPCODE_STI  : AltairXOpcodeLSU<0b1010>;
def OPCODE_LDVI : AltairXOpcodeLSU<0b0110>;
def OPCODE_STVI : AltairXOpcodeLSU<0b1110>;
def OPCODE_LDS  : AltairXOpcodeLSU<0b0001>;
def OPCODE_STS  : AltairXOpcodeLSU<0b1001>;
def OPCODE_LDVS : AltairXOpcodeLSU<0b0101>;
def OPCODE_STVS : AltairXOpcodeLSU<0b1101>;

class AltairXOpcodeMDU<bits<4> val>
  : AltairXOpcode {
  let Value{7-5} = 0b011;
  let Value{4-1} = val;
  let Value{0} = 0;
}

def OPCODE_DIV    : AltairXOpcodeMDU<0b0000>;
def OPCODE_DIVU   : AltairXOpcodeMDU<0b1000>;
def OPCODE_MUL    : AltairXOpcodeMDU<0b0100>;
def OPCODE_MULU   : AltairXOpcodeMDU<0b1100>;
def OPCODE_MOVERQ : AltairXOpcodeMDU<0b0010>;
def OPCODE_MOVEQR : AltairXOpcodeMDU<0b1010>;

class AltairXOpcodeGenericBranch // Used for brcond implementation
  : AltairXOpcode {
  let Value{7-5} = 0b111;
  let Value{0} = 0;
}

// BNE, BEQ ,BL, BLE, BG, BGE, BLS, BLES, BGS, BGES
def OPCODE_B : AltairXOpcodeGenericBranch;

class AltairXOpcodeBRU<bits<4> val>
  : AltairXOpcodeGenericBranch {
  let Value{4-1} = val;
}

def OPCODE_BRA  : AltairXOpcodeBRU<0b0101>;

def OPCODE_JUMP : AltairXOpcodeBRU<0b0011>;
def OPCODE_CALL : AltairXOpcodeBRU<0b1011>;
def OPCODE_RET  : AltairXOpcodeBRU<0b1111>;

class AltairXOpcodeFPU<bits<4> val>
  : AltairXOpcode {
  let Value{7-5} = 0b110;
  let Value{4-1} = val;
  let Value{0} = 0;
}

class AltairXOpcodeFPUD<bits<4> val>
  : AltairXOpcode {
  let Value{7-5} = 0b001;
  let Value{4-1} = val;
  let Value{0} = 0;
}

class AltairXOpcodeEFU<bits<4> val>
  : AltairXOpcode {
  let Value{7-5} = 0b101;
  let Value{4-1} = val;
  let Value{0} = 0;
}

//===----------------------------------------------------------------------===//
// Instruction formats
//===----------------------------------------------------------------------===//
class AltairXInst<dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr, InstFormat format>
  : Instruction {
  field bits<32> Inst;
  // SoftFail is a field the disassembler can use to provide a way for
  // instructions to not match without killing the whole decode process. It is
  // mainly used for ARM, but Tablegen expects this field to exist or it fails
  // to build the decode table.
  field bits<32> SoftFail = 0;
  let Size = 4;

  let Namespace = "AltairX";

  bits<8> Opcode = 0;
  let Inst{7-0} = Opcode;

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let Pattern = pattern;
  let AsmString = opcodestr # " " # argstr;

  let TSFlags = format.Value;
}

// Pseudo instructions
class AltairXPseudo<dag outs, dag ins, list<dag> pattern, string opcodestr = "", string argstr = "">
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatPseudo>,
    Sched<[]> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

class AltairXInstALURR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> rs1;
  bits<6> rs2;

  let Inst{31-26} = rd;
  let Inst{25-20} = rs1;
  let Inst{19-14} = rs2;
  let Inst{10} = 0;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstALUR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> rs1;

  let Inst{31-26} = rd;
  let Inst{25-20} = rs1;
  let Inst{10} = 0;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstALURI<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegImm> {
  bits<6> rd;
  bits<6> rs1;
  bits<9> imm;

  let Inst{31-26} = rd;
  let Inst{25-20} = rs1;
  let Inst{19-11} = imm;
  let Inst{10} = 1;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstMove<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegImm> {
  bits<6> rd;
  bits<18> imm;

  let Inst{31-26} = rd;
  let Inst{25-8} = imm;
  let Opcode = opcode.Value;

  let isMoveImm = true;
}

class AltairXInstMoveShift<AltairXOpcode opcode, bits<2> shift, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegImm> {
  bits<6> rd;
  bits<16> imm;

  let Inst{31-26} = rd;
  let Inst{25-10} = imm;
  let Inst{8-9} = shift;
  let Opcode = opcode.Value;
  
  let isMoveImm = true;
}

class AltairXInstLSURR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> reg;
  bits<6> base;
  bits<6> offset;
  bits<3> shift;

  let Inst{31-26} = reg;
  let Inst{25-20} = offset;
  let Inst{19-14} = base;
  let Inst{11-13} = shift;
  let Inst{10} = 0;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstLSURI<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> reg;
  bits<6> base;
  bits<10> offset;

  let Inst{31-26} = reg;
  let Inst{25-20} = base;
  let Inst{19-10} = offset;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstLSUSP<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> reg;
  bits<16> offset;

  let Inst{31-26} = reg;
  let Inst{25-10} = offset;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstCMP<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rs1;
  bits<6> rs2;

  let Inst{25-20} = rs1;
  let Inst{19-14} = rs2;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
  
  let isCompare = 1;
  let Defs = [CmpFlag];
}

class AltairXInstBR<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegImm> {
  bits<24> target;

  let Inst{31-8} = target;
  let Opcode = opcode.Value;
  
  let isBranch = 1;
  let isTerminator = 1;
  let hasDelaySlot = 1;
}

class AltairXCallInst<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegImm> {
  bits<24> imm;

  let Inst{31-8} = imm;
  let Opcode = opcode.Value;
}
