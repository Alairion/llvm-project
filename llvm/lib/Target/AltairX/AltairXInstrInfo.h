//===-- AltairXInstrInfo.h - AltairX Instruction Information ----------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the AltairX implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_INSTRINFO_H
#define LLVM_LIB_TARGET_ALTAIRX_INSTRINFO_H

#include "AltairX.h"
#include "AltairXRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "AltairXGenInstrInfo.inc"

namespace llvm {

class AltairXInstrInfo : public AltairXGenInstrInfo {
public:
  explicit AltairXInstrInfo(const AltairXSubtarget &STI);
  
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc) const override;

  bool expandPostRAPseudo(MachineInstr& MI) const override;

  MachineBasicBlock* getBranchDestBlock(const MachineInstr& MI) const override;

protected:
  const AltairXSubtarget &Subtarget;
};
} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_INSTRINFO_H
