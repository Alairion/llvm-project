//===- AltairXOptionRecord.h - Abstraction for storing information -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AltairXOptionRecord - Abstraction for storing arbitrary information in
// ELF files. Arbitrary information (e.g. register usage) can be stored in AltairX
// specific ELF sections like .AltairX.options. Specific records should subclass
// AltairXOptionRecord and provide an implementation to EmitAltairXOptionRecord which
// basically just dumps the information into an ELF section.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_ALTAIRXOPTIONRECORD_H
#define LLVM_LIB_TARGET_ALTAIRX_ALTAIRXOPTIONRECORD_H

#include "AltairXMCTargetDesc.h"
#include "llvm/MC/MCContext.h"

namespace llvm {

class AltairXELFStreamer;

class AltairXOptionRecord {
public:
  virtual ~AltairXOptionRecord() = default;

  virtual void EmitAltairXOptionRecord() = 0;
};

class AltairXRegInfoRecord : public AltairXOptionRecord {
public:
  AltairXRegInfoRecord(AltairXELFStreamer *S, MCContext &Context)
      : Streamer(S), Context(Context) {
  }

  ~AltairXRegInfoRecord() override = default;

  void EmitAltairXOptionRecord() override;

private:
  AltairXELFStreamer *Streamer;
  MCContext &Context;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ALTAIRX_ALTAIRXOPTIONRECORD_H
