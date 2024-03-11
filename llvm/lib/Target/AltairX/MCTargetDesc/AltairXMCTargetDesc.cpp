//===-- AltairXMCTargetDesc.cpp - AltairX Target Descriptions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides AltairX specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/AltairXTargetInfo.h"

#include "AltairXInstPrinter.h"
#include "AltairXMCAsmInfo.h"
#include "AltairXMCAsmBackend.h"
#include "AltairXMCCodeEmitter.h"
#include "AltairXMCTargetDesc.h"

#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "AltairXGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "AltairXGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "AltairXGenRegisterInfo.inc"

static MCInstrInfo *createAltairXMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitAltairXMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createAltairXMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitAltairXMCRegisterInfo(X, AltairX::LR);
  return X;
}

static MCSubtargetInfo *
createAltairXMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  if (CPU.empty()) {
    CPU = "generic";
  }

  return createAltairXMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

static MCInstPrinter *createAltairXMCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new AltairXInstPrinter(MAI, MII, MRI);
}

static MCAsmBackend *createAltairXMCAsmBackend(const Target &T,
                                               const MCSubtargetInfo &STI,
                                               const MCRegisterInfo &MRI,
                                               const MCTargetOptions &Options) {
  return new AltairXMCAsmBackend{T, STI, MRI, Options};
}

static MCAsmInfo *createAltairXMCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new AltairXMCAsmInfo(TT);

  unsigned WP = MRI.getDwarfRegNum(AltairX::R1, true);
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfaRegister(nullptr, WP);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCCodeEmitter *createAltairXMCCodeEmitter(const MCInstrInfo &II,
                                                 MCContext &Ctx) {
  return new AltairXMCCodeEmitter{II, Ctx};
}

extern "C" void LLVMInitializeAltairXTargetMC() {
  Target* T = &getTheAltairXTarget();

  // Register the MCCodeEmitter
  TargetRegistry::RegisterMCAsmBackend(*T, createAltairXMCAsmBackend);
  // Register the MC asm info.
  TargetRegistry::RegisterMCAsmInfo(*T, createAltairXMCAsmInfo);
  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(*T, createAltairXMCInstrInfo);
  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(*T, createAltairXMCRegisterInfo);
  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(*T, createAltairXMCSubtargetInfo);
  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(*T, createAltairXMCInstPrinter);
  // Register the MCCodeEmitter
  TargetRegistry::RegisterMCCodeEmitter(*T, createAltairXMCCodeEmitter);
}
