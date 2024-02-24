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

#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {

void AltairXMCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                             raw_ostream &OS,
                                             SmallVectorImpl<MCFixup> &Fixups,
                                             const MCSubtargetInfo &STI) const {
  auto opcode = Inst.getOpcode();
  OS.write(reinterpret_cast<const char *>(&opcode), sizeof(opcode));
}

} // namespace llvm