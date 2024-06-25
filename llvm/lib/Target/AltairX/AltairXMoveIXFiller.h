//===-- AltairXMoveIXFiller.h - AltairX Register Information Impl --- C++ -===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
// This file contains the AltairX implementation of the TargetRegisterInfo
// class.
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_MOVEIXFILLER_H
#define LLVM_LIB_TARGET_ALTAIRX_MOVEIXFILLER_H

#include "AltairX.h"
#include "AltairXInstrInfo.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

namespace llvm
{

class AltairXMoveIXFiller : public MachineFunctionPass {
public:
  AltairXMoveIXFiller();
  StringRef getPassName() const override { return "AltairX MoveIX Filler"; }

  bool runOnMachineFunction(MachineFunction &F) override;

  static char ID;

private:
  void runOnMachineBasicBlock(MachineBasicBlock& bb);

  const TargetMachine* TM{};
  const AltairXInstrInfo* TII{};
};

}

#endif
