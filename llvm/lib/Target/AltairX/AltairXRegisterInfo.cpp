//===-- AltairXRegisterInfo.cpp - AltairX Register Information ------------===//
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

#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/Debug.h"

#include "AltairXInstrInfo.h"
#include "AltairXSubtarget.h"

#define GET_REGINFO_TARGET_DESC
#include "AltairXGenRegisterInfo.inc"

#define DEBUG_TYPE "altairx-reginfo"

namespace llvm {

AltairXRegisterInfo::AltairXRegisterInfo(const AltairXSubtarget &ST)
    : AltairXGenRegisterInfo(AltairX::R31, 0, 0, AltairX::R0), Subtarget(ST) {}

const MCPhysReg *
AltairXRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return AltairX_CalleeSavedRegs_SaveList;
}

const uint32_t *
AltairXRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                          CallingConv::ID) const {
  return AltairX_CalleeSavedRegs_RegMask;
}

const TargetRegisterClass *AltairXRegisterInfo::intRegClass(unsigned int size) {
  switch (size) {
  case 8:
    return &AltairX::GPIReg8RegClass;
  case 16:
    return &AltairX::GPIReg16RegClass;
  case 32:
    return &AltairX::GPIReg32RegClass;
  case 64:
    return &AltairX::GPIReg64RegClass;
  default:
    llvm_unreachable("Unsuported int size!");
  }
}

const TargetRegisterClass* AltairXRegisterInfo::floatRegClass(unsigned int size)
{
  switch(size) {
  case 32:
    return &AltairX::FReg32RegClass;
  case 64:
    return &AltairX::FReg64RegClass;
  default:
    llvm_unreachable("Unsuported float size!");
  }
}

const TargetRegisterClass *AltairXRegisterInfo::MVTRegClass(MVT type) {
  if (type.isFloatingPoint()) {
    return floatRegClass(type.getSizeInBits());
  }

  return intRegClass(type.getSizeInBits());
}

bool AltairXRegisterInfo::isGPIReg(Register reg) {
  return AltairX::GPIReg64RegClass.contains(reg) ||
         AltairX::GPIReg32RegClass.contains(reg) ||
         AltairX::GPIReg16RegClass.contains(reg) ||
         AltairX::GPIReg8RegClass.contains(reg);
}

bool AltairXRegisterInfo::isFReg(Register reg) {
  return AltairX::FReg32RegClass.contains(reg) ||
         AltairX::FReg64RegClass.contains(reg);
}

bool AltairXRegisterInfo::isVIReg(Register reg) {
  return AltairX::VIReg8RegClass.contains(reg);
}

bool AltairXRegisterInfo::isMDUReg(Register reg) {
  return AltairX::MDUReg64RegClass.contains(reg) ||
         AltairX::MDUReg32RegClass.contains(reg) ||
         AltairX::MDUReg16RegClass.contains(reg) ||
         AltairX::MDUReg8RegClass.contains(reg);
}

bool AltairXRegisterInfo::isRIReg(Register reg) {
  return AltairX::RIReg32RegClass.contains(reg);
}

bool AltairXRegisterInfo::isEFReg(Register reg) {
  return AltairX::EFReg32RegClass.contains(reg) ||
         AltairX::EFReg64RegClass.contains(reg);
}

BitVector
AltairXRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector reserved{getNumRegs()};

  reserved.set(getStackRegister());
  reserved.set(getLinkRegister());
  // Frame register may be used as a general purpose register if absent
  // so it needs to be entirely marked
  for (auto reg : subregs_inclusive(getFrameRegister(MF))) {
    reserved.set(reg);
  }
  reserved.set(getZeroRegister());

  return reserved;
}

namespace {

bool isSpill(std::uint32_t opcode) {
  static constexpr std::array validOpcodes = {
      AltairX::StoreRIb,    AltairX::StoreRIbT16, AltairX::StoreRIbT32,
      AltairX::StoreRIbT64, AltairX::StoreRId,    AltairX::StoreRIdT64,
      AltairX::StoreRIq,    AltairX::StoreRIw,    AltairX::StoreRIwT32,
      AltairX::StoreRIwT64, AltairX::FStoreRIs,   AltairX::FStoreRId};
  return std::find(validOpcodes.begin(), validOpcodes.end(), opcode) !=
         validOpcodes.end();
}

bool isReload(std::uint32_t opcode) {
  static constexpr std::array validOpcodes = {
      AltairX::LoadRIb,     AltairX::LoadRIbAX16, AltairX::LoadRIbAX32,
      AltairX::LoadRIbAX64, AltairX::LoadRIbZX16, AltairX::LoadRIbZX32,
      AltairX::LoadRIbZX64, AltairX::LoadRId,     AltairX::LoadRIdAX64,
      AltairX::LoadRIdZX64, AltairX::LoadRIq,     AltairX::LoadRIw,
      AltairX::LoadRIwAX32, AltairX::LoadRIwAX64, AltairX::LoadRIwZX32,
      AltairX::LoadRIwZX64, AltairX::FLoadRIs,    AltairX::FLoadRId};

  return std::find(validOpcodes.begin(), validOpcodes.end(), opcode) !=
         validOpcodes.end();
}

void replaceFrameIndex(MachineBasicBlock::iterator II,
                       const AltairXInstrInfo &TII, Register Reg,
                       Register FrameReg, std::int64_t Offset,
                       std::uint64_t StackSize, RegScavenger *RS, int SPAdj) {
  if(!isInt<32>(Offset)) {
    // The real limit for load/store is 33 bits, but it will never be valid
    // in real application
    llvm_unreachable("Unsupported offset for spill, reload or frame addr!");
  }

  MachineInstr &inst = *II;
  MachineBasicBlock &block = *inst.getParent();
  const auto opcode = inst.getOpcode();
  DebugLoc dl = inst.getDebugLoc();

  if (isSpill(opcode)) {
    BuildMI(block, II, dl, TII.get(opcode))
        .addReg(Reg, getKillRegState(inst.getOperand(0).isKill()))
        .addReg(FrameReg, 0)
        .addImm(Offset)
        .addMemOperand(*inst.memoperands_begin());
  } else if (isReload(opcode)) {
    BuildMI(block, II, dl, TII.get(opcode), Reg)
        .addReg(FrameReg, 0)
        .addImm(Offset)
        .addMemOperand(*inst.memoperands_begin());
  } else if (opcode == AltairX::AddRIq) {
    // This handles frameindex addr computation
    BuildMI(block, II, dl, TII.get(AltairX::AddRIq), Reg)
        .addReg(FrameReg, 0)
        .addImm(Offset);
  } else {
    llvm_unreachable("Unsupported Instruction for frame index elemination");
  }

  // Erase old instruction.
  block.erase(II);
}

std::pair<int, int> getFrameIndexRange(const std::vector<CalleeSavedInfo>& info)
{
  if(!info.empty()) {
    return std::make_pair(info.front().getFrameIdx(), info.back().getFrameIdx());
  }

  return std::make_pair(0, -1);
}

} // namespace

bool AltairXRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected SP adjustment value");

  MachineInstr &inst = *II;
  MachineFunction &func = *inst.getParent()->getParent();
  MachineFrameInfo& frameInfo = func.getFrameInfo();

  const int frameIndex = inst.getOperand(FIOperandNum).getIndex();
  const auto [minFrameIndex, maxFrameIndex] =
      getFrameIndexRange(frameInfo.getCalleeSavedInfo());

  Register frameReg{};
  if (frameIndex >= minFrameIndex && frameIndex <= maxFrameIndex) {
    frameReg = getStackRegister();
  } else {
    frameReg = getFrameRegister(func);
  }

  const std::uint64_t stackSize = frameInfo.getStackSize();
  std::int64_t offset = frameInfo.getObjectOffset(frameIndex);
  offset += stackSize;
  offset += inst.getOperand(FIOperandNum + 1).getImm();

  const auto& instInfo = *func.getSubtarget<AltairXSubtarget>().getInstrInfo();
  const Register reg = inst.getOperand(0).getReg();
  replaceFrameIndex(II, instInfo, reg, frameReg, offset, stackSize, RS, SPAdj);

  return true;
}

bool AltairXRegisterInfo::requiresRegisterScavenging(
    const MachineFunction &MF) const {
  return true;
}

bool AltairXRegisterInfo::requiresFrameIndexScavenging(
    const MachineFunction &MF) const {
  return false;
}

bool AltairXRegisterInfo::requiresFrameIndexReplacementScavenging(
    const MachineFunction &MF) const {
  return false;
}

bool AltairXRegisterInfo::trackLivenessAfterRegAlloc(
    const MachineFunction &MF) const {
  return true;
}

Register AltairXRegisterInfo::getStackRegister() const { return AltairX::R0; }

Register AltairXRegisterInfo::getLinkRegister() const { return AltairX::R31; }

Register AltairXRegisterInfo::getZeroRegister() const { return AltairX::ZERO; }

Register
AltairXRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  if (getFrameLowering(MF)->hasFP(MF)) {
    return AltairX::R30;
  }

  return getStackRegister();
}

} // namespace llvm
