//===-- AltairXMCCodeEmitter.h - AltairX Asm Info ------------------------*- C++ -*--===//
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

#ifndef LLVM_LIB_TARGET_ALTAIRX_MCCODEEMITTER_ALTAIRXMCCODEEMITTER_H
#define LLVM_LIB_TARGET_ALTAIRX_MCCODEEMITTER_ALTAIRXMCCODEEMITTER_H

#include "llvm/MC/MCCodeEmitter.h"

namespace llvm {

class MCInstrInfo;
class MCContext;

class AltairXMCCodeEmitter : public MCCodeEmitter {
public:
  explicit AltairXMCCodeEmitter(const MCInstrInfo &instrInfo,
                                MCContext &context)
      : MCII{instrInfo}, MCC{context} {}

protected:
  void encodeInstruction(const MCInst &Inst, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

private:
  const MCInstrInfo &MCII;
  MCContext &MCC;
};

} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXMCASMINFO_H
