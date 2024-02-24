//===-- AltairXTargetMachine.cpp - Define TargetMachine for AltairX
//-------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about AltairX target spec.
//
//===----------------------------------------------------------------------===//

#include "AltairXTargetMachine.h"
#include "AltairXISelDAGToDAG.h"
#include "AltairXSubtarget.h"
#include "AltairXTargetObjectFile.h"
#include "TargetInfo/AltairXTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAltairXTarget() {
  // Register the target.
  //- Little endian Target Machine
  RegisterTargetMachine<AltairXTargetMachine> X(getTheAltairXTarget());
}

static std::string computeDataLayout() {
  return "e" // Little endian
      "-m:e" // ELF name mangling
      "-p:64:64:64:64" // 64-bit pointers, 64-bit aligned
      "-i64:64" // 64-bit integers, 64 bit aligned
      "-n8:16:32:64" // 8, 16, 32 and 64 bits native integers
      "-S64"; // 64-bit natural stack alignment
}

static Reloc::Model getEffectiveRelocModel(std::optional<CodeModel::Model> CM,
                                           std::optional<Reloc::Model> RM) {

  if (!RM) {
    return Reloc::Static;
  }

  return *RM;
}

AltairXTargetMachine::AltairXTargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           std::optional<Reloc::Model> RM,
                                           std::optional<CodeModel::Model> CM,
                                           CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(), TT, CPU, FS, Options,
                        getEffectiveRelocModel(CM, RM),
                        getEffectiveCodeModel(CM, CodeModel::Medium), OL),
      TLOF(std::make_unique<AltairXTargetObjectFile>()) {
  initAsmInfo();
}

const AltairXSubtarget *
AltairXTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU = !CPUAttr.hasAttribute(Attribute::None)
                        ? CPUAttr.getValueAsString().str()
                        : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None)
                       ? FSAttr.getValueAsString().str()
                       : TargetFS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = std::make_unique<AltairXSubtarget>(TargetTriple, CPU, CPU, FS, *this);
  }
  return I.get();
}

namespace {
class AltairXPassConfig : public TargetPassConfig {
public:
  AltairXPassConfig(AltairXTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  AltairXTargetMachine &getAltairXTargetMachine() const {
    return getTM<AltairXTargetMachine>();
  }

  bool addInstSelector() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *AltairXTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AltairXPassConfig(*this, PM);
}

// Install an instruction selector pass using
// the ISelDag to gen AltairX code.
bool AltairXPassConfig::addInstSelector() {
  char ID;
  addPass(
      new AltairXDAGToDAGISel(ID, getAltairXTargetMachine(), getOptLevel()));
  return false;
}

// Implemented by targets that want to run passes immediately before
// machine code is emitted. return true if -print-machineinstrs should
// print out the code after the passes.
void AltairXPassConfig::addPreEmitPass() {}
