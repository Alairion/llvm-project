//===-- AltairXInstrInfo.cpp - AltairX Instruction Information
//----------------===//
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

#include "AltairXInstrInfo.h"

#include "AltairXMachineFunction.h"
#include "AltairXTargetMachine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "AltairXGenInstrInfo.inc"

AltairXInstrInfo::AltairXInstrInfo(const AltairXSubtarget &STI)
    : AltairXGenInstrInfo(AltairX::ADJCALLSTACKDOWN, AltairX::ADJCALLSTACKUP),
      Subtarget(STI) {}

void AltairXInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, MCRegister DestReg,
                                   MCRegister SrcReg, bool KillSrc) const {
  // Use addi to copy reg to reg
  BuildMI(MBB, MI, DL, get(AltairX::AddRI), DestReg)
    .addReg(SrcReg, getKillRegState(KillSrc))
    .addImm(0);
}

bool AltairXInstrInfo::expandPostRAPseudo(MachineInstr &) const {
  return false;
}

MachineBasicBlock *
AltairXInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case AltairX::BRA:
    return MI.getOperand(0).getMBB();
  case AltairX::B:
    return MI.getOperand(1).getMBB();
  default:
    llvm_unreachable("unexpected opcode!");
  }
}