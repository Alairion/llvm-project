//===-- AltairXInstPrinter.cpp - Convert AltairX MCInst to assembly syntax -==//
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

AltairXInstPrinter::AltairXInstPrinter(const MCAsmInfo &MAI,
                                       const MCInstrInfo &MII,
                                       const MCRegisterInfo &MRI)
    : MCInstPrinter(MAI, MII, MRI) {}

void AltairXInstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) {
  OS << getRegisterName(Reg, AltairX::AltairXRegPrettyNameIndex);
}

void AltairXInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                   StringRef Annot, const MCSubtargetInfo &STI,
                                   raw_ostream &O) {
  if (MI->getOpcode() == AltairX::BUNDLE) {
    const MCInst *first = MI->getOperand(0).getInst();
    assert(first);
    printSingleInst(first, Address, Annot, STI, O);
    O << "\n\t";
    const MCInst *second = MI->getOperand(1).getInst();
    assert(second);
    printSingleInst(second, Address, Annot, STI, O);
  } else {
    printSingleInst(MI, Address, Annot, STI, O);
  }
}

void AltairXInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &O) {
  const MCOperand &op = MI->getOperand(OpNo);

  if (op.isReg()) {
    printRegName(O, op.getReg());
    return;
  }

  if (op.isImm()) {
    O << op.getImm();
    return;
  }

  assert(op.isExpr() && "unknown operand kind in printOperand");
  op.getExpr()->print(O, &MAI, true);
}

void AltairXInstPrinter::printSingleInst(const MCInst *MI, uint64_t Address,
                                         StringRef Annot,
                                         const MCSubtargetInfo &STI,
                                         raw_ostream &O) {
  // Try to print any aliases first.
  if (!printAliasInstr(MI, Address, O)) {
    printInstruction(MI, Address, O);
  }

  printAnnotation(O, Annot);
}

namespace {

std::string_view condCodeToString(AltairX::BRCondCode condCode) {
  switch (condCode) {
  case llvm::AltairX::BRCondCode::EQ:
    return "eq";
  case llvm::AltairX::BRCondCode::NE:
    return "ne";
  case llvm::AltairX::BRCondCode::LTU:
    return "ltu";
  case llvm::AltairX::BRCondCode::GEU:
    return "geu";
  case llvm::AltairX::BRCondCode::LT:
    return "lt";
  case llvm::AltairX::BRCondCode::GE:
    return "ge";
  default:
    llvm_unreachable("Invalid AltairX::CondCode");
    break;
  }
}

std::string_view SCMPCondCodeToString(AltairX::SCMPCondCode condCode) {
  switch (condCode) {
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

void AltairXInstPrinter::printRelBranchTarget(const MCInst *MI, unsigned OpNo,
                                              unsigned Value, raw_ostream &O) {
  const MCOperand & op = MI->getOperand(OpNo);
  op.getExpr()->print(O, &MAI, true);
}

void AltairXInstPrinter::printCondCode(const MCInst *MI, uint32_t OpIdx,
                                       raw_ostream &OS) {
  const auto cc = MI->getOperand(OpIdx).getImm();
  OS << condCodeToString(static_cast<AltairX::BRCondCode>(cc));
}

void AltairXInstPrinter::printBRCPrediction(const MCInst *MI, uint32_t OpIdx,
                                            raw_ostream &OS) {
  const auto value = MI->getOperand(OpIdx).getImm();
  OS << (value ? 't' : 'f');
}

void AltairXInstPrinter::printSCMPCondCode(const MCInst *MI, uint32_t OpIdx,
                                           raw_ostream &OS) {
  const auto cc = MI->getOperand(OpIdx).getImm();
  OS << SCMPCondCodeToString(static_cast<AltairX::SCMPCondCode>(cc));
}