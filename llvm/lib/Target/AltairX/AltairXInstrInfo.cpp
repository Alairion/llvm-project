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
#include "llvm/MC/MCContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

#include "AltairXMachineFunction.h"
#include "AltairXRegisterInfo.h"
#include "AltairXTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "AltairXGenInstrInfo.inc"
#include "AltairXGenDFAPacketizer.inc"

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

namespace {

bool isConstantToReg(const MachineInstr &MI) {
  return MI.getOpcode() == AltairX::ConstantToRegb ||
         MI.getOpcode() == AltairX::ConstantToRegw ||
         MI.getOpcode() == AltairX::ConstantToRegd ||
         MI.getOpcode() == AltairX::ConstantToRegq;
}

std::uint32_t getMoveIForReg(MCRegister Reg) {
  if (AltairX::GPIReg64RegClass.contains(Reg)) {
    return AltairX::MoveIq;
  } else if (AltairX::GPIReg32RegClass.contains(Reg)) {
    return AltairX::MoveId;
  } else if (AltairX::GPIReg16RegClass.contains(Reg)) {
    return AltairX::MoveIw;
  } else if (AltairX::GPIReg8RegClass.contains(Reg)) {
    return AltairX::MoveIb;
  }

  llvm_unreachable("Wrong register class");
}

} // namespace

bool AltairXInstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  MachineBasicBlock &MBB = *MI.getParent();
  auto TRI = Subtarget.getRegisterInfo();
  DebugLoc DL = MI.getDebugLoc();

  if (MI.getOpcode() == AltairX::AltairXGlobalAddrValue) {
    const auto dest = MI.getOperand(0).getReg();
    const GlobalValue *global = MI.getOperand(1).getGlobal();

    BuildMI(MBB, MI, MI.getDebugLoc(), get(AltairX::AddRIq))
        .addReg(dest, getDefRegState(true))
        .addReg(TRI->getZeroRegister(), 0)
        .addGlobalAddress(global);
  } else if (isConstantToReg(MI)) {
    const auto dest = MI.getOperand(0).getReg();
    const std::int64_t imm = MI.getOperand(1).getImm();

    if (isInt<42>(imm)) {
      // Fits movei + moveix
      BuildMI(MBB, MI, MI.getDebugLoc(), get(getMoveIForReg(dest)))
          .addReg(dest, getDefRegState(true))
          .addImm(imm);
    } else {
      assert(AltairX::GPIReg64RegClass.contains(dest));
      // movei + moveix supports the following ranges:
      // 0000 0000 0000 0000 to 0000 01FF FFFF FFFF (2^41-1)
      // FFFF FE00 0000 0000 (-2^41) to FFFF FFFF FFFF FFFF
      // Range that we have to cover somehow
      // 0000 0200 0000 0000 (2^41) to FFFF FDFF FFFF FFFF (-2^41 - 1)
      const std::uint64_t uimm = static_cast<std::uint64_t>(imm);
      const auto lowvalue = uimm & 0xFFFFFFFFull;
      const auto highvalue = (uimm >> 32) & 0xFFFFFFFFull;

      BuildMI(MBB, MI, MI.getDebugLoc(), get(AltairX::MoveIq))
        .addReg(dest, getDefRegState(true))
        .addImm(highvalue);
      
      BuildMI(MBB, MI, MI.getDebugLoc(), get(AltairX::LslRIq))
        .addReg(dest)
        .addReg(dest)
        .addImm(32);

      BuildMI(MBB, MI, MI.getDebugLoc(), get(AltairX::AddRIq))
        .addReg(dest)
        .addReg(dest)
        .addImm(lowvalue);
    }
  } else {
    return false;
  }

  MBB.erase(MI);
  return true;
}

bool AltairXInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *&TBB,
                                     MachineBasicBlock *&FBB,
                                     SmallVectorImpl<MachineOperand> &Cond,
                                     bool AllowModify) const {
  return true; // Can't handle indirect branch.
  auto last = MBB.getLastNonDebugInstr();
  if (last == MBB.end()) {
    return false;
  }

  if (!isUnpredicatedTerminator(*last)) {
    return false;
  }

  // If there is only one terminator instruction, process it.
  auto secondLast = std::prev(last);
  if(last == MBB.begin() || !isUnpredicatedTerminator(*secondLast)) {
    if(isUncondBranchOpcode(*last)) {
      TBB = getBranchDestBlock(*last);
      return false;
    }
    
    if(isCondBranchOpcode(*last)) {
      // Block ends with fall-through condbranch.
      TBB = getBranchDestBlock(*last);
      Cond.emplace_back(last->getOperand(1));
      return false;
    }

    return true; // Can't handle indirect branch.
  }

  // If AllowModify is true and the block ends with two or more unconditional
  // branches, delete all but the first unconditional branch.
  if (AllowModify && isUncondBranchOpcode(*secondLast)) {
    auto it = secondLast;
    while (isUncondBranchOpcode(*secondLast)) {
      last->eraseFromParent();
      last = secondLast;
      if (it == MBB.begin() || !isUnpredicatedTerminator(*--it)) {
        // Return now the only terminator is an unconditional branch.
        TBB = last->getOperand(0).getMBB();
        return false;
      } else {
        secondLast = to_address(it);
      }
    }
  }

  // If we're allowed to modify and the block ends in a unconditional branch
  // which could simply fallthrough, remove the branch.
  if(AllowModify && isUncondBranchOpcode(*last) &&
      MBB.isLayoutSuccessor(getBranchDestBlock(*last))) {
    last->eraseFromParent();
    last = secondLast;
    secondLast = std::prev(secondLast);
    if (last == MBB.begin() || !isUnpredicatedTerminator(*secondLast)) {
      assert(!isUncondBranchOpcode(*last) &&
             "unreachable unconditional branches removed above");

      if (isCondBranchOpcode(*last)) {
        // Block ends with fall-through condbranch.
        TBB = getBranchDestBlock(*last);
        Cond.emplace_back(last->getOperand(1));
        return false;
      }

      return true; // Can't handle indirect branch.
    }
  }

  // If the block ends with a B and a Bcc, handle it.
  if(isCondBranchOpcode(*secondLast) && isUncondBranchOpcode(*last)) {
    TBB = getBranchDestBlock(*secondLast);
    Cond.emplace_back(secondLast->getOperand(1));
    FBB = getBranchDestBlock(*last);
    return false;
  }

  // If the block ends with two unconditional branches, handle it.  The second
  // one is not executed, so remove it.
  if(isUncondBranchOpcode(*secondLast) && isUncondBranchOpcode(*last)) {
    TBB = TBB = getBranchDestBlock(*secondLast);
    if (AllowModify) {
      last->eraseFromParent();
    }

    return false;
  }

  return true;
}

unsigned AltairXInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                        int *BytesRemoved) const {
  auto last = MBB.getLastNonDebugInstr();
  if(last == MBB.end()) {
    return 0;
  }

  if (!isUncondBranchOpcode(*last) && !isCondBranchOpcode(*last)) {
    return 0;
  }

  // Remove the branch.
  last->eraseFromParent();
  if (MBB.empty()) {
    if (BytesRemoved) {
      *BytesRemoved = 4;
    }

    return 1;
  }

  // We may have two branches with the first one being a conditional and the
  // last a non conditional
  last = MBB.getLastNonDebugInstr();
  if (!isCondBranchOpcode(*last)) {
    if (BytesRemoved) {
      *BytesRemoved = 4;
    }

    return 1;
  }

  // Remove the branch.
  last->eraseFromParent();
  if (BytesRemoved) {
    *BytesRemoved = 8;
  }

  return 2;
}

unsigned AltairXInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                        MachineBasicBlock *TBB,
                                        MachineBasicBlock *FBB,
                                        ArrayRef<MachineOperand> Cond,
                                        const DebugLoc &DL,
                                        int *BytesAdded) const {
  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");

  if(!FBB) {
    if (Cond.empty()) { // Unconditional branch
      BuildMI(&MBB, DL, get(AltairX::BRA)).addMBB(TBB);
    } else {
      BuildMI(&MBB, DL, get(AltairX::B)).addMBB(TBB).addImm(Cond[0].getImm());
    }

    if (BytesAdded) {
      *BytesAdded = 4;
    }

    return 1;
  }

  // Two-way conditional branch.
  BuildMI(&MBB, DL, get(AltairX::B)).addMBB(TBB).addImm(Cond[0].getImm());
  BuildMI(&MBB, DL, get(AltairX::BRA)).addMBB(FBB);
  if (BytesAdded) {
    *BytesAdded = 8;
  }

  return 2;
}

MachineBasicBlock *
AltairXInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case AltairX::BRA:
    [[fallthrough]];
  case AltairX::B:
    return MI.getOperand(0).getMBB();
  default:
    llvm_unreachable("unexpected opcode!");
  }
}