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

#include "llvm/Support/Debug.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"

#include "AltairXSubtarget.h"
#include "AltairXInstrInfo.h"

#define GET_REGINFO_TARGET_DESC
#include "AltairXGenRegisterInfo.inc"

#define DEBUG_TYPE "altairx-reginfo"

namespace llvm {

AltairXRegisterInfo::AltairXRegisterInfo(const AltairXSubtarget &ST)
    : AltairXGenRegisterInfo(AltairX::LR, 0, 0, AltairX::R0), Subtarget(ST) {}

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

const TargetRegisterClass *AltairXRegisterInfo::MVTRegClass(MVT Type) {
  return intRegClass(Type.getSizeInBits());
}

BitVector
AltairXRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector reserved{getNumRegs()};
  markSuperRegs(reserved, getStackRegister());
  markSuperRegs(reserved, getZeroRegister());

  if (getFrameLowering(MF)->hasFP(MF)) {
    markSuperRegs(reserved, AltairX::R31b);
  }

  return reserved;
}

namespace {

std::uint32_t getSpillStoreRI(std::uint32_t opcode) {
  switch (opcode) {
  case AltairX::SPILLb:
    return AltairX::StoreRIb;
  case AltairX::SPILLw:
    return AltairX::StoreRIw;
  case AltairX::SPILLd:
    return AltairX::StoreRId;
  case AltairX::SPILLq:
    return AltairX::StoreRIq;
  case AltairX::StoreRIb: [[fallthrough]];
  case AltairX::StoreRIw: [[fallthrough]];
  case AltairX::StoreRId: [[fallthrough]];
  case AltairX::StoreRIq:
    return opcode; // return identity for StoreRIs
  default:
    llvm_unreachable("Invalid spill instruction");
  }
}

std::uint32_t getSpillStoreSP(std::uint32_t opcode) {
  switch (opcode) {
  case AltairX::SPILLb:
    return AltairX::StoreSPb;
  case AltairX::SPILLw:
    return AltairX::StoreSPw;
  case AltairX::SPILLd:
    return AltairX::StoreSPd;
  case AltairX::SPILLq:
    return AltairX::StoreSPq;
  case AltairX::StoreRIb: // StoreRI may be morphed into StoreSP
    return AltairX::StoreSPb;
  case AltairX::StoreRIw:
    return AltairX::StoreSPw;
  case AltairX::StoreRId:
    return AltairX::StoreSPd;
  case AltairX::StoreRIq:
    return AltairX::StoreSPq;
  default:
    llvm_unreachable("Invalid spill instruction");
  }
}

std::uint32_t getReloadLoadRI(std::uint32_t opcode) {
  switch (opcode) {
  case AltairX::RELOADb:
    return AltairX::LoadRIb;
  case AltairX::RELOADw:
    return AltairX::LoadRIw;
  case AltairX::RELOADd:
    return AltairX::LoadRId;
  case AltairX::RELOADq:
    return AltairX::LoadRIq;
  case AltairX::LoadRIb: [[fallthrough]];
  case AltairX::LoadRIw: [[fallthrough]];
  case AltairX::LoadRId: [[fallthrough]];
  case AltairX::LoadRIq:
    return opcode; // return identity for LoadRIs
  default:
    llvm_unreachable("Invalid reload instruction");
  }
}

std::uint32_t getReloadLoadSP(std::uint32_t opcode) {
  switch (opcode) {
  case AltairX::RELOADb:
    return AltairX::LoadSPb;
  case AltairX::RELOADw:
    return AltairX::LoadSPw;
  case AltairX::RELOADd:
    return AltairX::LoadSPd;
  case AltairX::RELOADq:
    return AltairX::LoadSPq;
  case AltairX::LoadRIb: // LoadRI may be morphed into LoadSP
    return AltairX::LoadSPb;
  case AltairX::LoadRIw:
    return AltairX::LoadSPw;
  case AltairX::LoadRId:
    return AltairX::LoadSPd;
  case AltairX::LoadRIq:
    return AltairX::LoadSPq;
  default:
    llvm_unreachable("Invalid reload instruction");
  }
}

bool isSpill(std::uint32_t opcode) {
  return opcode == AltairX::SPILLb || opcode == AltairX::SPILLw ||
         opcode == AltairX::SPILLd || opcode == AltairX::SPILLq ||
         opcode == AltairX::StoreRIb || opcode == AltairX::StoreRIw ||
         opcode == AltairX::StoreRId || opcode == AltairX::StoreRIq;
}

bool isReload(std::uint32_t opcode) {
  return opcode == AltairX::RELOADb || opcode == AltairX::RELOADw ||
         opcode == AltairX::RELOADd || opcode == AltairX::RELOADq ||
         opcode == AltairX::LoadRIb || opcode == AltairX::LoadRIw ||
         opcode == AltairX::LoadRId || opcode == AltairX::LoadRIq;
}

void replaceFrameIndex(MachineBasicBlock::iterator II,
                       const AltairXInstrInfo &TII, Register Reg,
                       Register FrameReg, std::int64_t Offset,
                       std::uint64_t StackSize, RegScavenger *RS, int SPAdj) {
  assert(RS && "Need register scavenger.");

  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc DL = MI.getDebugLoc();
  const std::uint32_t opcode = MI.getOpcode();

  // Handle StoreRIs too. They may be generated by load on stack args (since
  // they aren't spills)
  if (isSpill(opcode)) {

    if (AltairX::RIReg32RegClass.contains(Reg)) {
      auto tmpReg = RS->FindUnusedReg(&AltairX::GPIReg32RegClass);
      if (!tmpReg) {
        tmpReg = RS->scavengeRegisterBackwards(AltairX::GPIReg32RegClass, II,
                                               false, SPAdj);
        assert(tmpReg && "Register scavenging failed.");

        LLVM_DEBUG(
            dbgs()
            << "Scavenged register "
            << printReg(tmpReg,
                        MBB.getParent()->getSubtarget().getRegisterInfo())
            << " for FrameReg="
            << printReg(FrameReg,
                        MBB.getParent()->getSubtarget().getRegisterInfo())
            << "+Offset=" << Offset << "\n");

        RS->setRegUsed(tmpReg);
      }

      TII.copyPhysReg(MBB, II, DL, tmpReg, Reg, getKillRegState(Reg));
      Reg = tmpReg;
    }

    std::uint32_t realOpcode{};
    if (AltairX::SPReg64RegClass.contains(FrameReg) && isUInt<16>(Offset)) {
      realOpcode = getSpillStoreSP(opcode);
    } else if (isInt<32>(Offset)) {
      realOpcode = getSpillStoreRI(opcode);
    } else {
      llvm_unreachable("Unsupported offset for spill");
    }

    BuildMI(MBB, II, DL, TII.get(realOpcode))
        .addReg(Reg, getKillRegState(MI.getOperand(0).isKill()))
        .addReg(FrameReg, 0)
        .addImm(Offset)
        .addMemOperand(*MI.memoperands_begin());
  } else if (isReload(opcode)) {

    Register tmpReg{};
    if (AltairX::RIReg32RegClass.contains(Reg)) {
      tmpReg = RS->FindUnusedReg(&AltairX::GPIReg32RegClass);
      if (!tmpReg) {
        tmpReg = RS->scavengeRegisterBackwards(AltairX::GPIReg32RegClass, II,
                                               false, SPAdj);
        assert(tmpReg && "Register scavenging failed.");

        LLVM_DEBUG(
            dbgs()
            << "Scavenged register "
            << printReg(tmpReg,
                        MBB.getParent()->getSubtarget().getRegisterInfo())
            << " for FrameReg="
            << printReg(FrameReg,
                        MBB.getParent()->getSubtarget().getRegisterInfo())
            << "+Offset=" << Offset << "\n");
      }

      RS->setRegUsed(tmpReg);
      std::swap(Reg, tmpReg);
    }

    std::uint32_t realOpcode{};
    if (AltairX::SPReg64RegClass.contains(FrameReg) && isUInt<16>(Offset)) {
      realOpcode = getReloadLoadSP(opcode);
    } else if (isInt<32>(Offset)) {
      realOpcode = getReloadLoadRI(opcode);
    } else {
      llvm_unreachable("Unsupported offset for reload");
    }

    BuildMI(MBB, II, DL, TII.get(realOpcode), Reg)
        .addReg(FrameReg, 0)
        .addImm(Offset)
        .addMemOperand(*MI.memoperands_begin());

    if (tmpReg) {
      TII.copyPhysReg(MBB, II, DL, tmpReg, Reg, true);
    }
  }

  // Erase old instruction.
  MBB.erase(II);
}

} // namespace

bool AltairXRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                              int SPAdj, unsigned FIOperandNum,
                                              RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected");
  MachineInstr& MI = *II;
  MachineOperand& frameOp = MI.getOperand(FIOperandNum);
  const int frameIndex = frameOp.getIndex();

  MachineFunction& MF = *MI.getParent()->getParent();
  MachineFrameInfo& MFI = MF.getFrameInfo();
  const auto& TII = *MF.getSubtarget<AltairXSubtarget>().getInstrInfo();
  //const auto* TFI = getFrameLowering(MF);
  const std::uint64_t stackSize = MF.getFrameInfo().getStackSize();
  std::int64_t offset = MF.getFrameInfo().getObjectOffset(frameIndex);
  offset += stackSize;
  offset += MI.getOperand(FIOperandNum + 1).getImm();

  LLVM_DEBUG(dbgs() << "\nFunction  : " << MF.getName() << "\n"
                    << MI << "\n"
                    << "FrameIndex  : " << frameIndex << "\n"
                    << "FrameOffset : " << offset << "\n"
                    << "StackSize   : " << stackSize << "\n"
                    << "Offset      : " << offset << "\n"
                    << "<--------->\n");

  Register reg = MI.getOperand(0).getReg();

  const std::vector<CalleeSavedInfo>& CSI = MFI.getCalleeSavedInfo();
  int minCSFI = 0;
  int maxCSFI = -1;
  if(!CSI.empty()) {
    minCSFI = CSI.front().getFrameIdx();
    maxCSFI = CSI.back().getFrameIdx();
  }

  Register frameReg{};
  // callee saved regs are always stored using SP
  if (frameIndex >= minCSFI && frameIndex <= maxCSFI) {
    frameReg = getStackRegister();
  } else {
    frameReg = getFrameRegister(MF);
  }

  replaceFrameIndex(II, TII, reg, frameReg, offset, stackSize,
                    RS, SPAdj);

  return true;
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
AltairXRegisterInfo::getStackRegister() const {
  return AltairX::R0;
}

Register
AltairXRegisterInfo::getZeroRegister() const
{
  return AltairX::ZERO;
}

Register
AltairXRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  if (getFrameLowering(MF)->hasFP(MF)) {
    return AltairX::R31;
  }

  return getStackRegister();
}

} // namespace llvm
