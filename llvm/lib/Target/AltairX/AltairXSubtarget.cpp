//===-- AltairXSubtarget.cpp - AltairX Subtarget Information
//--------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the AltairX specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "AltairXSubtarget.h"
#include "AltairX.h"
#include "AltairXMachineFunction.h"
#include "AltairXRegisterInfo.h"
#include "AltairXTargetMachine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AltairXGenSubtargetInfo.inc"

AltairXSubtarget::AltairXSubtarget(const Triple &TT, StringRef CPU,
                                   StringRef FS, StringRef TuneCPU,
                                   const TargetMachine &TM)
    : AltairXGenSubtargetInfo(TT, CPU, TuneCPU, FS), TSInfo(),
      InstrInfo(initializeSubtargetDependencies(TT, CPU, FS, TuneCPU, TM)),
      FrameLowering(*this), TLInfo(TM, *this), RegInfo(*this) {}

AltairXSubtarget &AltairXSubtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef FS, StringRef TuneCPU,
    const TargetMachine &TM) {
  if (CPU.empty())
    CPU = "generic";

  // Parse features string.
  ParseSubtargetFeatures(CPU, FS, TuneCPU);

  return *this;
}
