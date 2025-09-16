//===-- AltairXSlotValidator.h - AltairX Register Information Impl --- C++ -===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_SLOTVALIDATOR_H
#define LLVM_LIB_TARGET_ALTAIRX_SLOTVALIDATOR_H

#include "AltairX.h"
#include "AltairXInstrInfo.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

namespace llvm {

class AltairXSlotValidator : public MachineFunctionPass {
public:
  AltairXSlotValidator();
  StringRef getPassName() const override { return "AltairX Slot Validator"; }

  bool runOnMachineFunction(MachineFunction &F) override;

  static char ID;

private:
  void runOnMachineBasicBlock(MachineBasicBlock &block);

  const TargetMachine *target{};
  const AltairXInstrInfo *instInfo{};
};

} // namespace llvm

#endif
