//===-- AltairXTargetInfo.cpp - AltairX Target Implementation -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/AltairXTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheAltairXTarget() {
  static Target TheAltairXTarget;
  return TheAltairXTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAltairXTargetInfo() {
  RegisterTarget<Triple::altairx> X(getTheAltairXTarget(), "altairx",
                                  "AltairX K1", "AltairX");
}
