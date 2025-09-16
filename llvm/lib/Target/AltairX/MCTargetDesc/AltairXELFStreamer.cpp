//===-------- AltairXELFStreamer.cpp - ELF Object Output ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AltairXELFStreamer.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

AltairXELFStreamer::AltairXELFStreamer(MCContext &Context,
                                       std::unique_ptr<MCAsmBackend> MAB,
                                       std::unique_ptr<MCObjectWriter> OW,
                                       std::unique_ptr<MCCodeEmitter> Emitter)
    : MCELFStreamer(Context, std::move(MAB), std::move(OW),
                    std::move(Emitter)) {
  RegInfoRecord = new AltairXRegInfoRecord(this, Context);
  AltairXOptionRecords.emplace_back(
      std::unique_ptr<AltairXRegInfoRecord>(RegInfoRecord));
}

void AltairXELFStreamer::emitInstruction(const MCInst& MI,
  const MCSubtargetInfo& STI) {
  if (MI.getOpcode() == AltairX::BUNDLE) {
    for (uint32_t i = 0; i < MI.getNumOperands(); ++i) {
      // MCStream is responsible for registering symbols that need relocation.
      // It does not understand bundles so we give bundle instructions one
      // by one.
      MCStreamer::emitInstruction(*MI.getOperand(i).getInst(), STI);
    }
  }

  // Let the whole bundle be emited for real
  MCObjectStreamer::emitInstruction(MI, STI);
}

void AltairXELFStreamer::EmitAltairXOptionRecords() {
  for (const auto &I : AltairXOptionRecords) {
    I->EmitAltairXOptionRecord();
  }
}

MCELFStreamer *
llvm::createAltairXELFStreamer(MCContext &Context,
                               std::unique_ptr<MCAsmBackend> MAB,
                               std::unique_ptr<MCObjectWriter> OW,
                               std::unique_ptr<MCCodeEmitter> Emitter) {
  return new AltairXELFStreamer(Context, std::move(MAB), std::move(OW),
                                std::move(Emitter));
}
