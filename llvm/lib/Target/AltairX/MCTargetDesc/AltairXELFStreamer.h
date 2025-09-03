//===-- AltairXELFStreamer.h - ELF Streamer for AltairX ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements ELF streamer information for the AltairX backend.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXELFSTREAMER_H
#define LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXELFSTREAMER_H

#include "llvm/MC/MCELFStreamer.h"
#include "AltairXOptionRecord.h"

namespace llvm {

class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCSubtargetInfo;
struct MCDwarfFrameInfo;

class AltairXELFStreamer : public MCELFStreamer {
  SmallVector<std::unique_ptr<AltairXOptionRecord>, 8> AltairXOptionRecords;
  AltairXRegInfoRecord *RegInfoRecord;
  SmallVector<MCSymbol *, 4> Labels;

public:
  AltairXELFStreamer(MCContext &Context, std::unique_ptr<MCAsmBackend> MAB,
                     std::unique_ptr<MCObjectWriter> OW,
                     std::unique_ptr<MCCodeEmitter> Emitter);

  /// Reimplement emitInstruction to support bundles properly
  void emitInstruction(const MCInst& MI, const MCSubtargetInfo& STI) override;

  /// Emits all the option records stored up until the point it's called.
  void EmitAltairXOptionRecords();
};

MCELFStreamer *createAltairXELFStreamer(MCContext &Context,
                                        std::unique_ptr<MCAsmBackend> TAB,
                                        std::unique_ptr<MCObjectWriter> OW,
                                        std::unique_ptr<MCCodeEmitter> Emitter);
}

#endif
