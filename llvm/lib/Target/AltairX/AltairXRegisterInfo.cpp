//===-- AltairXRegisterInfo.cpp - AltairX Register Information
//----------------===//
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

#include "AltairXRegisterInfo.h"
#include "AltairXSubtarget.h"
#include "llvm/Support/Debug.h"

#define GET_REGINFO_TARGET_DESC
#include "AltairXGenRegisterInfo.inc"

#define DEBUG_TYPE "altairx-reginfo"

namespace llvm {

AltairXRegisterInfo::AltairXRegisterInfo(const AltairXSubtarget &ST)
    : AltairXGenRegisterInfo(AltairX::R1, 0, 0, AltairX::R0), Subtarget(ST) {}

const MCPhysReg *
AltairXRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return AltairX_CalleeSavedRegs_SaveList;
}

const uint32_t*
AltairXRegisterInfo::getCallPreservedMask(const MachineFunction& MF,
  CallingConv::ID) const
{
  return AltairX_CalleeSavedRegs_RegMask;
}

const TargetRegisterClass *
AltairXRegisterInfo::intRegClass(unsigned int Size) {
  switch (Size) {
  case 8:
    return &AltairX::GPIReg8RegClass;
  case 16:
    return &AltairX::GPIReg16RegClass;
  case 32:
    return &AltairX::GPIReg32RegClass;
  case 64:
    return &AltairX::GPIReg64RegClass;
  default:
    return nullptr;
  }
}

const TargetRegisterClass* AltairXRegisterInfo::MVTRegClass(MVT Type)
{
  return intRegClass(Type.getSizeInBits());
}

BitVector
AltairXRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved{getNumRegs()};
  markSuperRegs(Reserved, AltairX::R0); // sp

  return Reserved;
}

bool AltairXRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  llvm_unreachable("Unsupported eliminateFrameIndex");
}

bool AltairXRegisterInfo::requiresRegisterScavenging(
    const MachineFunction &MF) const {
  return true;
}

bool AltairXRegisterInfo::requiresFrameIndexScavenging(
    const MachineFunction &MF) const {
  return true;
}

bool AltairXRegisterInfo::requiresFrameIndexReplacementScavenging(
    const MachineFunction &MF) const {
  return true;
}

bool AltairXRegisterInfo::trackLivenessAfterRegAlloc(
    const MachineFunction &MF) const {
  return true;
}

Register
AltairXRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return AltairX::R63;
}

} // namespace llvm
