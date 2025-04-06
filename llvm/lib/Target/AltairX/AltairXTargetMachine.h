//===-- AltairXTargetMachine.h - Define TargetMachine for AltairX ---*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the AltairX specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_TARGETMACHINE_H
#define LLVM_LIB_TARGET_ALTAIRX_TARGETMACHINE_H

#include "AltairXSubtarget.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class AltairXTargetMachine : public LLVMTargetMachine {

  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  mutable StringMap<std::unique_ptr<AltairXSubtarget>> SubtargetMap;

public:
  AltairXTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       std::optional<Reloc::Model> RM,
                       std::optional<CodeModel::Model> CM, CodeGenOpt::Level OL,
                       bool JIT);

  const AltairXSubtarget *getSubtargetImpl(const Function &F) const override;
  const AltairXSubtarget *getSubtargetImpl() const = delete;

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_TARGETMACHINE_H
