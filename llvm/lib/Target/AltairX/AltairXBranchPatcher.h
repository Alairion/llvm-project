//===- AltairXBranchPatcher.h - AltairX Register Information Impl --- C++ -===//
// 
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// 
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_BRANCHPATCHER_H
#define LLVM_LIB_TARGET_ALTAIRX_BRANCHPATCHER_H

#include "AltairX.h"
#include "AltairXInstrInfo.h"

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

namespace llvm {

class MachineBranchProbabilityInfo;

/// This class has the following responsabilities:
/// - Expand PseudoBRC to BRC
///   - This may invert CMP operands
///   - The reason with PseudoBRC is not expanded by postrapseudos pass is
///     because other passes will use AltairXInstrInfo::analyzeBranch
///     and it only work on PseudoBRC for simplicity
/// - Insert prediction information in BRC
///   - This information is fetched from MachineBranchProbabilityInfo
/// - Then (CMPRR, reg, (ConstantToReg x)) with x fitting in i32
///   - Replace with (CMPRI, reg, x)
///   - That's why it must be performed before AltairXMoveIX filler!
class AltairXBranchPatcher : public MachineFunctionPass {
public:
  AltairXBranchPatcher();
  StringRef getPassName() const override { return "AltairX Branch Patcher"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnMachineFunction(MachineFunction &F) override;

  static char ID;

private:
  void runOnMachineBasicBlock(MachineBasicBlock &MBB);
  void runOnPseudoBRC(MachineBasicBlock &MBB, MachineInstr &MI);
  void runOnCMP(MachineBasicBlock &MBB, MachineInstr &MI);

  const TargetMachine *TM{};
  const AltairXInstrInfo *TII{};
  const MachineBranchProbabilityInfo *MBPI{};
};

} // namespace llvm

#endif
