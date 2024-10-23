//===-- AltairXInstPrinter.cpp - Convert AltairX MCInst to assembly syntax ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class prints an AltairX MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "AltairXInstPrinter.h"

#include "AltairXCommon.h"
#include "AltairXInstrInfo.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-isel"

#define PRINT_ALIAS_INSTR
#include "AltairXGenAsmWriter.inc"

AltairXInstPrinter::AltairXInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                                   const MCRegisterInfo &MRI)
    : MCInstPrinter(MAI, MII, MRI) {}

void AltairXInstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) const {
  if(Reg >= AltairX::R0) // TODO: do this correctly :)
  {
    OS << getRegisterName(Reg, AltairX::AltairXRegAltNameIndex);
  }
  else
  {
    OS << getRegisterName(Reg, AltairX::NoRegAltName);
  }
}

void AltairXInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                 StringRef Annot, const MCSubtargetInfo &STI,
                                 raw_ostream &O) {

  // Replace "add a, b, 0" with "move a, b"
  if ((MI->getOpcode() == AltairX::AddRIb ||
       MI->getOpcode() == AltairX::AddRIw ||
       MI->getOpcode() == AltairX::AddRId ||
       MI->getOpcode() == AltairX::AddRIq) &&
      MI->getOperand(2).isImm() && MI->getOperand(2).getImm() == 0) {
    O << '\t' << "move ";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
  }
  // Try to print any aliases first.
  else if (!printAliasInstr(MI, Address, O)) {
    printInstruction(MI, Address, O);
  }

  printAnnotation(O, Annot);
}

void AltairXInstPrinter::printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);

  if (Op.isReg()) {
    printRegName(O, Op.getReg());
    return;
  }

  if (Op.isImm()) {
    O << Op.getImm();
    return;
  }

  assert(Op.isExpr() && "unknown operand kind in printOperand");
  Op.getExpr()->print(O, &MAI, true);
}

namespace {

std::string_view condCodeToString(AltairX::CondCode condCode) {
  switch(condCode) {
  case llvm::AltairX::CondCode::NE:
    return "ne";
  case llvm::AltairX::CondCode::EQ:
    return "eq";
  case llvm::AltairX::CondCode::L:
    return "l";
  case llvm::AltairX::CondCode::LE:
    return "le";
  case llvm::AltairX::CondCode::G:
    return "g";
  case llvm::AltairX::CondCode::GE:
    return "ge";
  case llvm::AltairX::CondCode::LS:
    return "ls";
  case llvm::AltairX::CondCode::LES:
    return "les";
  case llvm::AltairX::CondCode::GS:
    return "gs";
  case llvm::AltairX::CondCode::GES:
    return "ges";
  default:
    llvm_unreachable("Invalid AltairX::CondCode");
    break;
  }
}

std::string_view SCMPCondCodeToString(AltairX::SCMPCondCode condCode)
{
  switch(condCode) {
  case AltairX::SCMPCondCode::EQ:
    return "e";
  case AltairX::SCMPCondCode::NE:
    return "en";
  case AltairX::SCMPCondCode::LT:
    return "lt";
  case AltairX::SCMPCondCode::LTU:
    return "ltu";
  default:
    llvm_unreachable("Invalid AltairX::SCMPCondCode");
    break;
  }
}

} // namespace

void AltairXInstPrinter::printCondCode(const MCInst *MI, uint32_t OpIdx,
                                       raw_ostream &OS) {
  const auto cc = MI->getOperand(OpIdx).getImm();
  OS << condCodeToString(static_cast<AltairX::CondCode>(cc));
}

void AltairXInstPrinter::printSCMPCondCode(const MCInst *MI, uint32_t OpIdx,
                                           raw_ostream &OS) {
  const auto cc = MI->getOperand(OpIdx).getImm();
  OS << SCMPCondCodeToString(static_cast<AltairX::SCMPCondCode>(cc));
}