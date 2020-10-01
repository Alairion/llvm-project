//===-- AltairXRegisterInfo.h - AltairX Register Information Impl -----*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the AltairX implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_REGISTERINFO_H
#define LLVM_LIB_TARGET_ALTAIRX_REGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "AltairXGenRegisterInfo.inc"

namespace llvm {

class AltairXSubtarget;

class AltairXRegisterInfo : public AltairXGenRegisterInfo {
public:
  AltairXRegisterInfo(const AltairXSubtarget &Subtarget);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override;
  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override;
  bool requiresFrameIndexReplacementScavenging(
      const MachineFunction &MF) const override;

  bool trackLivenessAfterRegAlloc(const MachineFunction &MF) const override;

  bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  static const TargetRegisterClass *intRegClass(unsigned Size);
  static const TargetRegisterClass *MVTRegClass(MVT Type);


protected:
  const AltairXSubtarget &Subtarget;
};

} // end namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_REGISTERINFO_H
