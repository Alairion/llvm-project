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

std::uint64_t
AltairXMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  if (MO.isReg()) {
    return MCC.getRegisterInfo()->getEncodingValue(MO.getReg());
  }

  const auto fixup = getImmFixupFor(MI);
  if (MO.isImm()) {
    return AltairXMCAsmBackend::adjustImmValue(fixup, MO.getImm());
  }

  assert(MO.isExpr() && "Expected an expression");
  Fixups.emplace_back(MCFixup::create(0, MO.getExpr(), fixup, MI.getLoc()));

  return 0;
}

std::uint64_t
AltairXMCCodeEmitter::getImmOpValue(const MCInst &MI, std::uint32_t OpIdx,
                                    SmallVectorImpl<MCFixup> &Fixups,
                                    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  assert(MO.isImm() && "Operand must be an immediate!");
  return MO.getImm();
}

std::uint64_t
AltairXMCCodeEmitter::getMoveIXOpValue(const MCInst &MI, std::uint32_t OpIdx,
                                       SmallVectorImpl<MCFixup> &Fixups,
                                       const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  const MCFixupKind fixup = getMoveIXFixupFor(MI);

  if (MO.isImm()) {
    return AltairXMCAsmBackend::adjustImmValue(fixup, MO.getImm());
  }

  assert(MO.isExpr() && "Expected an expression");
  Fixups.emplace_back(MCFixup::create(0, MO.getExpr(), fixup, MI.getLoc()));

  return 0; // let fixup handle the value when it's known!
}

std::uint64_t
AltairXMCCodeEmitter::getBRTargetOpValue(const MCInst &MI, std::uint32_t OpIdx,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  return getMachineOpValue(MI, MO, Fixups, STI);
}

std::uint64_t AltairXMCCodeEmitter::getCallTargetOpValue(
    const MCInst &MI, std::uint32_t OpIdx, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  return getMachineOpValue(MI, MO, Fixups, STI);
}

void AltairXMCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                             raw_ostream &OS,
                                             SmallVectorImpl<MCFixup> &Fixups,
                                             const MCSubtargetInfo &STI) const {
  if(Inst.getOpcode() == AltairX::BUNDLE) {
    const MCInst *first = Inst.getOperand(0).getInst();
    assert(first);
    auto firstOpcode = getBinaryCodeForInstr(*first, Fixups, STI);
    firstOpcode |= 1; // set first bit to one to indicate bundle!
    OS.write(reinterpret_cast<const char *>(&firstOpcode), 4);

    const MCInst *second = Inst.getOperand(1).getInst();
    assert(second);
    const auto secondOpcode = getBinaryCodeForInstr(*second, Fixups, STI);
    OS.write(reinterpret_cast<const char *>(&secondOpcode), 4);
  } else {
    const auto opcode = getBinaryCodeForInstr(Inst, Fixups, STI);
    OS.write(reinterpret_cast<const char *>(&opcode), 4);
  }

}

MCFixupKind AltairXMCCodeEmitter::getImmFixupFor(const MCInst& MI) const
{
  switch(MCII.get(MI.getOpcode()).TSFlags) {
  case AltairX::InstFormatBRURelImm24:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_pcrel24lo);
  case AltairX::InstFormatBRUAbsImm24:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_abs24lo);
  case AltairX::InstFormatALURegImm9:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix9lo);
  case AltairX::InstFormatLSURegImm10:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix10lo);
  case AltairX::InstFormatMoveImm18:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix18lo);
  default:
    llvm_unreachable("Missing MoveIX impl");
    return FK_NONE;
  }
}

MCFixupKind AltairXMCCodeEmitter::getMoveIXFixupFor(const MCInst& MI) const
{
  switch(MI.getOpcode()) {
  case AltairX::MOVEIX24PCREL:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_pcrel24hi);
  case AltairX::MOVEIX24ABS:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_abs24hi);
  case AltairX::MOVEIX9:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix9hi);
  case AltairX::MOVEIX10:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix10hi);
  case AltairX::MOVEIX18:
    return static_cast<MCFixupKind>(AltairX::fixup_altairx_moveix18hi);
  default:
    llvm_unreachable("Missing MoveIX impl");
    return FK_NONE;
  }
}

}