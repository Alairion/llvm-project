//===-- AltairXFrameLowering.h - Define frame lowering for AltairX ----*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the AltairXTargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_FRAMELOWERING_H
#define LLVM_LIB_TARGET_ALTAIRX_FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class AltairXSubtarget;

class AltairXFrameLowering : public TargetFrameLowering {
protected:
  const AltairXSubtarget &STI;

public:
  explicit AltairXFrameLowering(const AltairXSubtarget &STI)
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align{8}, 0,
                            Align{8}),
        STI(STI) {}

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool hasReservedCallFrame(const MachineFunction &MF) const override;
  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;

  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                            RegScavenger *RS) const override;

  bool hasFP(const MachineFunction &MF) const override;
};
} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_FRAMELOWERING_H
