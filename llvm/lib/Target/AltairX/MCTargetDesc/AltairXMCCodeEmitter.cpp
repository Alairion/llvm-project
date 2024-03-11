//===-- AltairXMCCodeEmitter.cpp - AltairX Asm Info ------------------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the AltairXMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#include "AltairXMCCodeEmitter.h"
#include "AltairXMCAsmBackend.h"

#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"

#define GET_INSTRINFO_ENUM
#define GET_INSTRMAP_INFO
#include "AltairXGenInstrInfo.inc"

//STATISTIC(MCNumFixups, "Number of MC fixups created.");

namespace llvm {

#include "AltairXGenCodeEmitter.inc"

 std::uint32_t
AltairXMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  if (MO.isReg()) {
    return MCC.getRegisterInfo()->getEncodingValue(MO.getReg());
  }

  assert(MO.isImm() && "did not expect relocated expression");
  return static_cast<std::uint32_t>(MO.getImm());
}

std::uint32_t
AltairXMCCodeEmitter::getBRTargetOpValue(const MCInst &MI, std::uint32_t OpIdx,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  const MCOperand& MO = MI.getOperand(OpIdx);

  // If the destination is an immediate, we have nothing to do.
  if(MO.isImm()) {
    return MO.getImm();
  }
  assert(MO.isExpr() && "Unexpected ADR target type!");

  Fixups.emplace_back(MCFixup::create(
      0, MO.getExpr(),
      static_cast<MCFixupKind>(AltairX::fixup_altairx_pcrel_br_imm23),
      MI.getLoc()));

  //++MCNumFixups;

  // All of the information is in the fixup.
  return 0;
}

void AltairXMCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                             raw_ostream &OS,
                                             SmallVectorImpl<MCFixup> &Fixups,
                                             const MCSubtargetInfo &STI) const {
  
  auto opcode = getBinaryCodeForInstr(Inst, Fixups, STI);
  OS.write(reinterpret_cast<const char *>(&opcode), 4);
}

}