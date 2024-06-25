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

#include "AltairXCommon.h"
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

namespace {

MCFixupKind getImmFixupFor(std::uint32_t type) noexcept
{
  switch(type) {
  case AltairX::InstFormatALURegImm9:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix9lo);
  case AltairX::InstFormatLSURegImm10:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix10lo);
  default:
    llvm_unreachable("Missing MoveIX impl");
  }

  return FK_NONE;
}

MCFixupKind getMoveIXFixupFor(const MCInst& MI) noexcept {
  switch (MI.getOpcode()) {
  case AltairX::MOVEIX9:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix9hi24);
  case AltairX::MOVEIX10:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix10hi24);
  default:
    return FK_NONE;
  }
}

} // namespace

std::uint64_t
AltairXMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  if (MO.isReg()) {
    return MCC.getRegisterInfo()->getEncodingValue(MO.getReg());
  }

  if (MO.isImm()) {
    return static_cast<std::uint32_t>(MO.getImm());
  }

  assert(MO.isExpr() && "Expected an expression");

  if(auto fixup = getMoveIXFixupFor(MI); fixup != 0) {
    Fixups.emplace_back(MCFixup::create(
      0, MO.getExpr(), fixup, MI.getLoc()));
  } else {
    Fixups.emplace_back(MCFixup::create(
      0, MO.getExpr(), getImmFixupFor(MCII.get(MI.getOpcode()).TSFlags), MI.getLoc()));
  }

  return 0;
}

std::uint64_t AltairXMCCodeEmitter::getLSUImmGlobalValue(
    const MCInst &MI, std::uint32_t OpIdx, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand& MO = MI.getOperand(OpIdx);

  if(MO.isImm()) {
    return static_cast<std::uint32_t>(MO.getImm());
  }

  assert(MO.isExpr() && "Expected an expression");

  return 0;
}

std::uint64_t
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

std::uint64_t
AltairXMCCodeEmitter::getCallTargetOpValue(const MCInst& MI, std::uint32_t OpIdx,
  SmallVectorImpl<MCFixup>& Fixups,
  const MCSubtargetInfo& STI) const
{
  const MCOperand& MO = MI.getOperand(OpIdx);

  // If the destination is an immediate, we have nothing to do.
  if(MO.isImm()) {
    return MO.getImm();
  }
  assert(MO.isExpr() && "Unexpected ADR target type!");

  Fixups.emplace_back(MCFixup::create(
    0, MO.getExpr(),
    static_cast<MCFixupKind>(AltairX::fixup_altairx_call_imm24),
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