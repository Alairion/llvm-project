//===-- AltairXMCAsmBackend.cpp - AltairX MC Asm Backend -------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the AltairXMCAsmBackend class.
//
//===----------------------------------------------------------------------===//

#include "AltairXMCAsmBackend.h"

#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {

std::unique_ptr<MCObjectTargetWriter>
AltairXMCAsmBackend::createObjectTargetWriter() const {
  return std::make_unique<MCObjectTargetWriter>();
}
bool AltairXMCAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count,
                                       const MCSubtargetInfo *STI) const {}

// Target Fixup Interfaces
unsigned AltairXMCAsmBackend::getNumFixupKinds() const {}

void AltairXMCAsmBackend::applyFixup(const MCAssembler &Asm,
                                     const MCFixup &Fixup,
                                     const MCValue &Target,
                                     MutableArrayRef<char> Data, uint64_t Value,
                                     bool IsResolved,
                                     const MCSubtargetInfo *STI) const {}

// Target Relaxation Interfaces
bool AltairXMCAsmBackend::fixupNeedsRelaxation(
    const MCFixup &Fixup, uint64_t Value, const MCRelaxableFragment *DF,
    const MCAsmLayout &Layout) const {}

} // namespace llvm