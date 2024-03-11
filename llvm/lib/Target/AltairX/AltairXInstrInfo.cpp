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

#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

#include "AltairXMachineFunction.h"
#include "AltairXRegisterInfo.h"
#include "AltairXTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "AltairXGenInstrInfo.inc"

namespace {

bool IsGPIReg(MCRegister Reg) {
  return AltairX::GPIReg64RegClass.contains(Reg) ||
         AltairX::GPIReg32RegClass.contains(Reg) ||
         AltairX::GPIReg16RegClass.contains(Reg) ||
         AltairX::GPIReg8RegClass.contains(Reg);
}

std::uint32_t GetGPIRegCopy(MCRegister Reg) {
  if (AltairX::GPIReg64RegClass.contains(Reg)) {
    return AltairX::AddRIq;
  } else if (AltairX::GPIReg32RegClass.contains(Reg)) {
    return AltairX::AddRId;
  } else if (AltairX::GPIReg16RegClass.contains(Reg)) {
    return AltairX::AddRIw;
  } else if (AltairX::GPIReg8RegClass.contains(Reg)) {
    return AltairX::AddRIb;
  }

  llvm_unreachable("Wrong register class");
}

bool IsMDUReg(MCRegister Reg) {
  return AltairX::MDUReg64RegClass.contains(Reg) ||
         AltairX::MDUReg32RegClass.contains(Reg) ||
         AltairX::MDUReg16RegClass.contains(Reg) ||
         AltairX::MDUReg8RegClass.contains(Reg);
}

std::uint32_t GetGPIRegToMDURegCopy(MCRegister Reg) {
  if (AltairX::MDUReg64RegClass.contains(Reg)) {
    return AltairX::MOVEQRq;
  } else if (AltairX::MDUReg32RegClass.contains(Reg)) {
    return AltairX::MOVEQRd;
  } else if (AltairX::MDUReg16RegClass.contains(Reg)) {
    return AltairX::MOVEQRw;
  } else if (AltairX::MDUReg8RegClass.contains(Reg)) {
    return AltairX::MOVEQRb;
  }

  llvm_unreachable("Wrong register class");
}

std::uint32_t GetMDURegToGPIRegCopy(MCRegister Reg) {
  if (AltairX::MDUReg64RegClass.contains(Reg)) {
    return AltairX::MOVERQq;
  } else if (AltairX::MDUReg32RegClass.contains(Reg)) {
    return AltairX::MOVERQd;
  } else if (AltairX::MDUReg16RegClass.contains(Reg)) {
    return AltairX::MOVERQw;
  } else if (AltairX::MDUReg8RegClass.contains(Reg)) {
    return AltairX::MOVERQb;
  }

  llvm_unreachable("Wrong register class");
}

bool IsRIReg(MCRegister Reg)
{
  return AltairX::RIReg32RegClass.contains(Reg);
}

std::uint32_t GetGPIRegToRIRegCopy(MCRegister Reg)
{
  if(AltairX::RIReg32RegClass.contains(Reg)) {
    return AltairX::MOVEIR;
  }

  llvm_unreachable("Wrong register class");
}

std::uint32_t GetRIRegToGPIRegCopy(MCRegister Reg)
{
  if(AltairX::RIReg32RegClass.contains(Reg)) {
    return AltairX::MOVERI;
  }

  llvm_unreachable("Wrong register class");
}

} // namespace

AltairXInstrInfo::AltairXInstrInfo(const AltairXSubtarget &STI)
    : AltairXGenInstrInfo(AltairX::ADJCALLSTACKDOWN, AltairX::ADJCALLSTACKUP),
      Subtarget(STI) {}

void AltairXInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, MCRegister DestReg,
                                   MCRegister SrcReg, bool KillSrc) const {
  if (IsMDUReg(DestReg) && IsGPIReg(SrcReg)) { // MOVEQR
    BuildMI(MBB, MI, DL, get(GetGPIRegToMDURegCopy(DestReg)), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (IsGPIReg(DestReg) && IsMDUReg(SrcReg)) { // MOVERQ
    BuildMI(MBB, MI, DL, get(GetMDURegToGPIRegCopy(SrcReg)), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (IsRIReg(DestReg) && IsGPIReg(SrcReg)) { // MOVEIR
    BuildMI(MBB, MI, DL, get(GetGPIRegToRIRegCopy(DestReg)), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (IsGPIReg(DestReg) && IsRIReg(SrcReg)) { // MOVERI
    BuildMI(MBB, MI, DL, get(GetRIRegToGPIRegCopy(SrcReg)), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (IsGPIReg(DestReg) && IsGPIReg(SrcReg)) { // ADDI r, 0
    BuildMI(MBB, MI, DL, get(GetGPIRegCopy(DestReg)), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addImm(0);
  } else {
    llvm_unreachable("Unsuported physical reg copy");
  }
}
/*
MachineInstr *AltairXInstrInfo::foldMemoryOperandImpl(
    MachineFunction &MF, MachineInstr &MI, ArrayRef<unsigned> Ops,
    MachineBasicBlock::iterator InsertPt, int FrameIndex, LiveIntervals *LIS,
    VirtRegMap *VRM) const {
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();

  if(IsRIReg(SrcReg) && DstReg.isVirtual()) {
    MF.getRegInfo().constrainRegClass(DstReg, &AltairX::GPIReg32RegClass);
    return nullptr;
  }

  if(IsRIReg(DstReg) && SrcReg.isVirtual()) {
    MF.getRegInfo().constrainRegClass(SrcReg, &AltairX::GPIReg32RegClass);
    return nullptr;
  }

  return nullptr;
}
*/
void AltairXInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool KillSrc, int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI, Register VReg [[maybe_unused]]) const {

  MachineFunction& MF = *MBB.getParent();
  MachineFrameInfo& MFI = MF.getFrameInfo();

  MachinePointerInfo PtrInfo = MachinePointerInfo::getFixedStack(MF, FrameIndex);
  auto *MMO = MF.getMachineMemOperand(PtrInfo, MachineMemOperand::MOStore,
                                      MFI.getObjectSize(FrameIndex),
                                      MFI.getObjectAlign(FrameIndex));

  const std::uint32_t spillSize = TRI->getSpillSize(*RC);
  std::uint32_t opcode{};
  if (spillSize == 1) {
    opcode = AltairX::SPILLb;
  } else if (spillSize == 2) {
    opcode = AltairX::SPILLw;
  } else if (spillSize == 4) {
    opcode = AltairX::SPILLd;
  } else if (spillSize == 8) {
    opcode = AltairX::SPILLq;
  } else {
    llvm_unreachable("Wrong spill size");
  }

  BuildMI(MBB, MI, DebugLoc(), get(opcode))
    .addReg(SrcReg, getKillRegState(KillSrc))
    .addFrameIndex(FrameIndex)
    .addImm(0)
    .addMemOperand(MMO);
}

void AltairXInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
    int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI, Register VReg [[maybe_unused]]) const {

  MachineFunction& MF = *MBB.getParent();
  MachineFrameInfo& MFI = MF.getFrameInfo();
  MachinePointerInfo PtrInfo = MachinePointerInfo::getFixedStack(MF, FrameIndex);
  auto *MMO = MF.getMachineMemOperand(PtrInfo, MachineMemOperand::MOLoad,
                                      MFI.getObjectSize(FrameIndex),
                                      MFI.getObjectAlign(FrameIndex));

  std::uint32_t opcode{};
  const std::uint32_t spillSize = TRI->getSpillSize(*RC);
  if (spillSize == 1) {
    opcode = AltairX::RELOADb;
  } else if (spillSize == 2) {
    opcode = AltairX::RELOADw;
  } else if (spillSize == 4) {
    opcode = AltairX::RELOADd;
  } else if (spillSize == 8) {
    opcode = AltairX::RELOADq;
  } else {
    llvm_unreachable("Wrong spill size");
  }

  BuildMI(MBB, MI, DebugLoc(), get(opcode))
    .addReg(DestReg, getDefRegState(true))
    .addFrameIndex(FrameIndex)
    .addImm(0)
    .addMemOperand(MMO);
}

bool AltairXInstrInfo::expandPostRAPseudo(MachineInstr & MI) const {
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