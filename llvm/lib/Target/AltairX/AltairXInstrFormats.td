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
// Defined in AltairXInstrInfo.h
class InstFormat<bits<64> val> {
  bits<64> Value = val;
}
def InstFormatPseudo : InstFormat<0>;
def InstFormatMoveImm18 : InstFormat<1>;
def InstFormatRegReg : InstFormat<2>; // two inputs, single output (ALU, LSU, MDU, FPU, EFU)
def InstFormatRegRegReg : InstFormat<3>; // three inputs, single output (cmove)
def InstFormatUnary : InstFormat<4>; // single input, single output (FPU)
def InstFormatALURegImm9 : InstFormat<5>;
def InstFormatALURegRegImm9 : InstFormat<6>; // used by conditional moves
def InstFormatMDURegImm9 : InstFormat<7>;
def InstFormatLSURegImm10 : InstFormat<8>;
def InstFormatFPURegImm16 : InstFormat<9>;
def InstFormatBRURelImm23 : InstFormat<10>;
def InstFormatBRURelImm24 : InstFormat<11>;
def InstFormatBRUAbsImm24 : InstFormat<12>;
def InstFormatBRUIndirect : InstFormat<13>;
def InstFormatCMPRegReg : InstFormat<14>; // cmp has a specific format
def InstFormatCMPRegImm9 : InstFormat<15>;
def InstFormatCUImplicit : InstFormat<16>;

//===----------------------------------------------------------------------===//
// Opcodes
//===----------------------------------------------------------------------===//
// AltairX ISA
// https://docs.google.com/spreadsheets/d/1AmdMslRcXIX9pKGBSRJJcx2IvRyzBLjA61SzxmlEYf8/edit#gid=0
class AltairXOpcode {
  bits<8> Value;
  let Value{0} = 0;
}

let Value = 0 in
def OPCODE_TO_DETERMINE : AltairXOpcode;

class AltairXOpcodeALU0<bits<4> val>
  : AltairXOpcode {
  let Value{1-4} = val;
  let Value{5-7} = 0b000;
}

def OPCODE_NOP : AltairXOpcodeALU0<0b0000>;
def OPCODE_MOVEIX : AltairXOpcodeALU0<0b0000>;
def OPCODE_MOVEI : AltairXOpcodeALU0<0b1000>;
def OPCODE_EXT : AltairXOpcodeALU0<0b0110>;
def OPCODE_INS : AltairXOpcodeALU0<0b1110>;

def OPCODE_ADDS : AltairXOpcodeALU0<0b0001>;
def OPCODE_SUBS : AltairXOpcodeALU0<0b1001>;
def OPCODE_RTL  : AltairXOpcodeALU0<0b0101>;
def OPCODE_RTR  : AltairXOpcodeALU0<0b1101>;

def OPCODE_CMP  : AltairXOpcodeALU0<0b0011>;
def OPCODE_TEST : AltairXOpcodeALU0<0b1011>;

class AltairXOpcodeALU1Generic // Used for se/sen/slt implementation
  : AltairXOpcode {
  let Value{5-7} = 0b100;
}

class AltairXOpcodeALU1<bits<4> val>
  : AltairXOpcodeALU1Generic {
  let Value{1-4} = val;
}

def OPCODE_ADD : AltairXOpcodeALU1<0b0000>;
def OPCODE_SUB : AltairXOpcodeALU1<0b1000>;
def OPCODE_XOR : AltairXOpcodeALU1<0b0100>;
def OPCODE_OR  : AltairXOpcodeALU1<0b1100>;
def OPCODE_AND : AltairXOpcodeALU1<0b0010>;
def OPCODE_LSL : AltairXOpcodeALU1<0b1010>;
def OPCODE_ASR : AltairXOpcodeALU1<0b0110>;
def OPCODE_LSR : AltairXOpcodeALU1<0b1110>;

def OPCODE_SE   : AltairXOpcodeALU1<0b0001>;
def OPCODE_SEN  : AltairXOpcodeALU1<0b1001>;
def OPCODE_SLTS : AltairXOpcodeALU1<0b0101>;
def OPCODE_SLTU : AltairXOpcodeALU1<0b1101>;

def OPCODE_SAND   : AltairXOpcodeALU1<0b0011>;
def OPCODE_SBIT   : AltairXOpcodeALU1<0b1011>;
def OPCODE_CMOVEN : AltairXOpcodeALU1<0b0111>;
def OPCODE_CMOVE  : AltairXOpcodeALU1<0b1111>;

class AltairXOpcodeLSU<bits<4> val>
  : AltairXOpcode {
  let Value{1-4} = val;
  let Value{5-7} = 0b010;
}

def OPCODE_LD  : AltairXOpcodeLSU<0b0000>;
def OPCODE_LDS : AltairXOpcodeLSU<0b1000>;
def OPCODE_FLD : AltairXOpcodeLSU<0b0100>;

def OPCODE_ST  : AltairXOpcodeLSU<0b0010>;
def OPCODE_FST : AltairXOpcodeLSU<0b0110>;

def OPCODE_LDI  : AltairXOpcodeLSU<0b0001>;
def OPCODE_LDIS : AltairXOpcodeLSU<0b1001>;
def OPCODE_FLDI : AltairXOpcodeLSU<0b0101>;

def OPCODE_STI  : AltairXOpcodeLSU<0b0011>;
def OPCODE_FSTI : AltairXOpcodeLSU<0b0111>;

class AltairXOpcodeMDU<bits<4> val>
  : AltairXOpcode {
  let Value{1-4} = val;
  let Value{5-7} = 0b011;
}

def OPCODE_DIV   : AltairXOpcodeMDU<0b0000>;
def OPCODE_DIVU  : AltairXOpcodeMDU<0b1000>;
def OPCODE_MUL   : AltairXOpcodeMDU<0b0100>;
def OPCODE_MULU  : AltairXOpcodeMDU<0b1100>;
def OPCODE_GETMD : AltairXOpcodeMDU<0b0010>;
def OPCODE_SETMD : AltairXOpcodeMDU<0b1010>;

class AltairXOpcodeGenericBranch // Used for brcond implementation
  : AltairXOpcode {
  let Value{5-7} = 0b111;
}

// BNE, BEQ, BLT, BGE, BLTU, BGEU
def OPCODE_BRC : AltairXOpcodeGenericBranch;

class AltairXOpcodeBRU<bits<4> val>
  : AltairXOpcodeGenericBranch {
  let Value{1-4} = val;
}

def OPCODE_BRA   : AltairXOpcodeBRU<0b0001>;
def OPCODE_CALLR : AltairXOpcodeBRU<0b1001>;
def OPCODE_JUMP  : AltairXOpcodeBRU<0b0101>;
def OPCODE_CALL  : AltairXOpcodeBRU<0b1101>;

def OPCODE_INDIRECT_CALLR : AltairXOpcodeBRU<0b1011>;
def OPCODE_INDIRECT_CALL  : AltairXOpcodeBRU<0b1111>;

class AltairXOpcodeFPU<bits<4> val>
  : AltairXOpcode {
  let Value{1-4} = val;
  let Value{5-7} = 0b110;
}

def OPCODE_FADD  : AltairXOpcodeFPU<0b0000>;
def OPCODE_FSUB  : AltairXOpcodeFPU<0b1000>;
def OPCODE_FMUL  : AltairXOpcodeFPU<0b0100>;
def OPCODE_FNMUL : AltairXOpcodeFPU<0b1100>;
def OPCODE_FMIN  : AltairXOpcodeFPU<0b0010>;
def OPCODE_FMAX  : AltairXOpcodeFPU<0b1010>;
def OPCODE_FNEG  : AltairXOpcodeFPU<0b0110>;
def OPCODE_FABS  : AltairXOpcodeFPU<0b1110>;

def OPCODE_FCMOVE : AltairXOpcodeFPU<0b0001>;
def OPCODE_FSE    : AltairXOpcodeFPU<0b1001>;
def OPCODE_FSEN   : AltairXOpcodeFPU<0b0101>;
def OPCODE_FSLT   : AltairXOpcodeFPU<0b1101>;

def OPCODE_FMOVE  : AltairXOpcodeFPU<0b0011>;
def OPCODE_FCMP   : AltairXOpcodeFPU<0b1011>;
def OPCODE_FMOVEI : AltairXOpcodeFPU<0b0111>;
def OPCODE_FCMPI  : AltairXOpcodeFPU<0b1111>;

// with "11" as "size"
def OPCODE_ITOF : AltairXOpcodeFPU<0b0100>; 
def OPCODE_FTOI : AltairXOpcodeFPU<0b1100>;
def OPCODE_FTOD : AltairXOpcodeFPU<0b0010>; 
def OPCODE_DTOF : AltairXOpcodeFPU<0b1010>; 
def OPCODE_ITOD : AltairXOpcodeFPU<0b0110>; 
def OPCODE_DTOI : AltairXOpcodeFPU<0b1110>; 

class AltairXOpcodeEFU<bits<4> val>
  : AltairXOpcode {
  let Value{1-4} = val;
  let Value{5-7} = 0b101;
}

def OPCODE_FDIV  : AltairXOpcodeEFU<0b0000>;
def OPCODE_FSQRT : AltairXOpcodeEFU<0b0100>;
def OPCODE_FSIN  : AltairXOpcodeEFU<0b1100>;

def OPCODE_SETEF : AltairXOpcodeEFU<0b0111>;
def OPCODE_GETEF : AltairXOpcodeEFU<0b1111>;

class AltairXOpcodeCU<bits<4> val>
  : AltairXOpcode {
  let Value{1-4} = val;
  let Value{5-7} = 0b101;
}

def OPCODE_BRK     : AltairXOpcodeCU<0b1011>;
def OPCODE_SYSCALL : AltairXOpcodeCU<0b0111>;
def OPCODE_RETI    : AltairXOpcodeCU<0b1111>;

def OPCODE_GETIR : AltairXOpcodeCU<0b0000>; // R <- IR
def OPCODE_SETFR : AltairXOpcodeCU<0b1000>; // FR <- R

//===----------------------------------------------------------------------===//
// Instruction formats
//===----------------------------------------------------------------------===//
class AltairXInst<dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr, InstFormat format>
  : Instruction {
  field bits<32> Inst = 0;
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
  let AsmString = opcodestr # "\t" # argstr;

  let TSFlags = format.Value;
}

// Pseudo instructions
class AltairXPseudo<dag outs, dag ins, list<dag> pattern, string opcodestr = "", string argstr = "">
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatPseudo>,
    Sched<[]> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

// ALU
class AltairXInstMoveIX<InstFormat parentFormat, dag outs, dag ins, list<dag> pattern>
  : AltairXInst<outs, ins, pattern, "moveix", "$imm", parentFormat>{
  bits<24> imm;

  let Inst{31-8} = imm;
  let Opcode = 0; // always a second slot instruction
}

class AltairXInstALURR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> rs1;
  bits<6> rs2;

  let Inst{31-26} = rd;
  let Inst{25-20} = rs1;
  let Inst{19-14} = rs2;
  let Inst{11-13} = 0; // left-shift (imm3)
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstALURI<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatALURegImm9> {
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

class AltairXInstALURRR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInstALURR<opcode, size, outs, ins, pattern, opcodestr, argstr> {
    let TSFlags = InstFormatRegRegReg.Value;
}

class AltairXInstALURRI<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInstALURI<opcode, size, outs, ins, pattern, opcodestr, argstr> {
    let TSFlags = InstFormatALURegRegImm9.Value;
}

class AltairXInstSExt<AltairXOpcode opcode, bits<2> size, bits<2> insize, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> rs1;

  let Inst{31-26} = rd;
  let Inst{25-20} = rs1;
  let Inst{19-14} = 0b111111; // zero
  let Inst{13-12} = insize;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstCMPRR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatCMPRegReg> {
  bits<6> rs1;
  bits<6> rs2;

  let Inst{31-26} = 0b111111; // zero
  let Inst{25-20} = rs1;
  let Inst{19-14} = rs2;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
  
  let isCompare = 1;
}

class AltairXInstCMPRI<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatCMPRegImm9> {
  bits<6> rs1;
  bits<9> imm;

  let Inst{31-26} = 0b111111; // zero
  let Inst{25-20} = rs1;
  let Inst{19-11} = imm;
  let Inst{10} = 1;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
  
  let isCompare = 1;
}

class AltairXInstMoveI<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatMoveImm18> {
  bits<6> rd;
  bits<18> imm;

  let Inst{31-26} = rd;
  let Inst{25-8} = imm;
  let Opcode = opcode.Value;

  let isMoveImm = true;
}

// MDU
class AltairXInstMDURR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rs1;
  bits<6> rs2;

  let Inst{31-26} = 0b111111; // zero, MDU outputs in special registers
  let Inst{25-20} = rs1;
  let Inst{19-14} = rs2;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstMDURI<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatMDURegImm9> {
  bits<6> rs1;
  bits<9> imm;

  let Inst{31-26} = 0b111111; // zero, MDU outputs in special registers
  let Inst{25-20} = rs1;
  let Inst{19-11} = imm;
  let Inst{10} = 1;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstMDUGetMD<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> pq;

  let Inst{31-26} = rd;
  let Inst{25-20} = 0b111111; // zero
  let Inst{19-14} = 0b111111; // zero
  let Inst{12-11} = pq{0-1};
  let Opcode = opcode.Value;
}

class AltairXInstMDUSetMD<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rs1;
  bits<6> pq;

  let Inst{31-26} = 0b111111; // zero
  let Inst{25-20} = rs1;
  let Inst{19-14} = 0b111111; // zero
  let Inst{12-11} = pq{0-1};
  let Opcode = opcode.Value;
}

// LSU
class AltairXInstLSURR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> reg;
  bits<6> base;
  bits<6> offset;
  bits<3> shift;

  let Inst{31-26} = reg;
  let Inst{25-20} = base;
  let Inst{19-14} = offset;
  let Inst{13-11} = shift;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstLSURI<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatLSURegImm10> {
  bits<6> reg;
  bits<6> base;
  bits<10> offset;

  let Inst{31-26} = reg;
  let Inst{25-20} = base;
  let Inst{19-10} = offset;
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

// BRU conditional branches (beq, blt, ...), always relative
class AltairXInstBRC<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatBRURelImm23> {
  bits<23> target;
  bits<3> cond;
  bits<1> pred;

  let Inst{31-9} = target;
  let Inst{8} = pred;
  let Opcode{7-5} = opcode.Value{7-5};
  let Opcode{3-1} = cond; // real opcode is dynamically determined based on predicate!
  
  let isBranch = 1;
  let isTerminator = 1;
}

// BRU generic unconditional branches, InstFormat will be used to discriminate relative jumps from absolute jumps
class AltairXInstBRA<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr, InstFormat format>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, format> {
  bits<24> target;

  let Inst{31-8} = target;
  let Opcode = opcode.Value;
  
  let isBranch = 1;
  let isBarrier = 1;
}

// BRU unconditional relative branches (callr, bra)
class AltairXInstRelBRA<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInstBRA<opcode, outs, ins, pattern, opcodestr, argstr, InstFormatBRURelImm24>;

// BRU unconditional absolute branches (call, jump)
class AltairXInstAbsBRA<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInstBRA<opcode, outs, ins, pattern, opcodestr, argstr, InstFormatBRUAbsImm24>;

// BRU unconditional indirect branches (callr, call)
class AltairXInstIndirectBRA<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatBRUIndirect> {
  bits<6> link;
  bits<6> target;
  
  let Inst{31-26} = link;
  let Inst{25-20} = target;
  let Inst{19-14} = 0b111111; // zero
  let Opcode = opcode.Value;
  
  let isBranch = 1;
  let isBarrier = 1;
  let isIndirectBranch = 1;
}

// FPU
class AltairXInstFPURR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> rs1;
  bits<6> rs2;

  let Inst{31-26} = rd;
  let Inst{25-20} = rs1;
  let Inst{19-14} = rs2;
  let Inst{9-8} = size; // 0 = f16, 1 = f32, 2 = f64, 3 = 128
  let Opcode = opcode.Value;
}

class AltairXInstFPUUnary<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> rs1;

  let Inst{31-26} = rd;
  let Inst{25-20} = rs1;
  let Inst{19-14} = 0b111111; // zero
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstFCMPRR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInstCMPRR<opcode, size, outs, ins, pattern, opcodestr, argstr> {
}

// EFU
class AltairXInstEFURR<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInstMDURR<opcode, size, outs, ins, pattern, opcodestr, argstr> {
}

class AltairXInstEFUUnary<AltairXOpcode opcode, bits<2> size, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatUnary> {
  bits<6> rs1;

  let Inst{31-26} = 0b111111; // zero
  let Inst{25-20} = rs1;
  let Inst{19-14} = 0b111111; // zero
  let Inst{9-8} = size;
  let Opcode = opcode.Value;
}

class AltairXInstEFUGetEF<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;

  let Inst{31-26} = rd;
  let Inst{25-20} = 0b111111; // zero
  let Inst{19-14} = 0b111111; // zero
  let Opcode = opcode.Value;
}

class AltairXInstEFUSetEF<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr> 
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rs1;

  let Inst{31-26} = 0b111111; // zero
  let Inst{25-20} = rs1;
  let Inst{19-14} = 0b111111; // zero
  let Opcode = opcode.Value;
}

// CU
class AltairXInstCUImplicit<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr>
  : AltairXInst<outs, ins, pattern, opcodestr, "", InstFormatCUImplicit> {
  let Opcode = opcode.Value;
}

class AltairXInstCUSetFR<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rs1;
  bits<6> ri;

  let Inst{31-26} = 0b111111; // zero
  let Inst{25-20} = rs1;
  let Inst{19-14} = 0b111111; // zero
  let Opcode = opcode.Value;
}

class AltairXInstCUGetIR<AltairXOpcode opcode, dag outs, dag ins, list<dag> pattern, string opcodestr, string argstr>
  : AltairXInst<outs, ins, pattern, opcodestr, argstr, InstFormatRegReg> {
  bits<6> rd;
  bits<6> ir;

  let Inst{31-26} = rd;
  let Inst{25-20} = 0b111111; // zero
  let Inst{19-14} = 0b111111; // zero
  let Inst{13-11} = ir{0-2};
  let Opcode = opcode.Value;
}
