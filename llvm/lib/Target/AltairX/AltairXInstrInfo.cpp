//===-- AltairXInstrInfo.cpp - AltairX Instruction Information ------------===//
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
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

#include "AltairXCommon.h"
#include "AltairXMachineFunctionInfo.h"
#include "AltairXRegisterInfo.h"
#include "AltairXTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "AltairXGenDFAPacketizer.inc"
#include "AltairXGenInstrInfo.inc"

namespace {

bool isGPIReg(MCRegister reg) {
  return AltairX::GPIReg64RegClass.contains(reg) ||
         AltairX::GPIReg32RegClass.contains(reg) ||
         AltairX::GPIReg16RegClass.contains(reg) ||
         AltairX::GPIReg8RegClass.contains(reg);
}

uint32_t getGPIRegCopy(MCRegister reg) {
  if (AltairX::GPIReg64RegClass.contains(reg)) {
    return AltairX::AddRIq;
  } else if (AltairX::GPIReg32RegClass.contains(reg)) {
    return AltairX::AddRId;
  } else if (AltairX::GPIReg16RegClass.contains(reg)) {
    return AltairX::AddRIw;
  } else if (AltairX::GPIReg8RegClass.contains(reg)) {
    return AltairX::AddRIb;
  }

  llvm_unreachable("Wrong register class");
}

bool isMDUReg(MCRegister reg) {
  return AltairX::MDUReg64RegClass.contains(reg) ||
         AltairX::MDUReg32RegClass.contains(reg) ||
         AltairX::MDUReg16RegClass.contains(reg) ||
         AltairX::MDUReg8RegClass.contains(reg);
}

uint32_t getGPIRegToMDURegCopy(MCRegister reg) {
  if (AltairX::MDUReg64RegClass.contains(reg)) {
    return AltairX::SetMDq;
  } else if (AltairX::MDUReg32RegClass.contains(reg)) {
    return AltairX::SetMDd;
  } else if (AltairX::MDUReg16RegClass.contains(reg)) {
    return AltairX::SetMDw;
  } else if (AltairX::MDUReg8RegClass.contains(reg)) {
    return AltairX::SetMDb;
  }

  llvm_unreachable("Wrong register class");
}

uint32_t getMDURegToGPIRegCopy(MCRegister reg) {
  if (AltairX::MDUReg64RegClass.contains(reg)) {
    return AltairX::GetMDq;
  } else if (AltairX::MDUReg32RegClass.contains(reg)) {
    return AltairX::GetMDd;
  } else if (AltairX::MDUReg16RegClass.contains(reg)) {
    return AltairX::GetMDw;
  } else if (AltairX::MDUReg8RegClass.contains(reg)) {
    return AltairX::GetMDb;
  }

  llvm_unreachable("Wrong register class");
}

bool isRIReg(MCRegister reg) { return AltairX::RIReg32RegClass.contains(reg); }

uint32_t getGPIRegToRIRegCopy(MCRegister reg) {
  if (AltairX::RIReg32RegClass.contains(reg)) {
    return AltairX::SetFR;
  }

  llvm_unreachable("Wrong register class");
}

uint32_t getRIRegToGPIRegCopy(MCRegister reg) {
  if (AltairX::RIReg32RegClass.contains(reg)) {
    return AltairX::GetIR;
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
  if (isMDUReg(DestReg) && isGPIReg(SrcReg)) { // MOVEQR
    BuildMI(MBB, MI, DL, get(getGPIRegToMDURegCopy(DestReg)), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (isGPIReg(DestReg) && isMDUReg(SrcReg)) { // MOVERQ
    BuildMI(MBB, MI, DL, get(getMDURegToGPIRegCopy(SrcReg)), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (isRIReg(DestReg) && isGPIReg(SrcReg)) { // MOVEIR
    BuildMI(MBB, MI, DL, get(getGPIRegToRIRegCopy(DestReg)), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (isGPIReg(DestReg) && isRIReg(SrcReg)) { // MOVERI
    BuildMI(MBB, MI, DL, get(getRIRegToGPIRegCopy(SrcReg)), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (isGPIReg(DestReg) && isGPIReg(SrcReg)) { // ADDI r, 0
    BuildMI(MBB, MI, DL, get(getGPIRegCopy(DestReg)), DestReg)
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

  if(isRIReg(SrcReg) && DstReg.isVirtual()) {
    MF.getRegInfo().constrainRegClass(DstReg, &AltairX::GPIReg32RegClass);
    return nullptr;
  }

  if(isRIReg(DstReg) && SrcReg.isVirtual()) {
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

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  MachinePointerInfo PtrInfo =
      MachinePointerInfo::getFixedStack(MF, FrameIndex);
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

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachinePointerInfo PtrInfo =
      MachinePointerInfo::getFixedStack(MF, FrameIndex);
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

uint32_t getMoveIForReg(MCRegister reg) {
  if (AltairX::GPIReg64RegClass.contains(reg)) {
    return AltairX::MoveIq;
  } else if (AltairX::GPIReg32RegClass.contains(reg)) {
    return AltairX::MoveId;
  } else if (AltairX::GPIReg16RegClass.contains(reg)) {
    return AltairX::MoveIw;
  } else if (AltairX::GPIReg8RegClass.contains(reg)) {
    return AltairX::MoveIb;
  }

  llvm_unreachable("Wrong register class");
}

} // namespace

bool AltairXInstrInfo::expandPostRAPseudo(MachineInstr &inst) const {

  switch (inst.getOpcode()) {
  case AltairX::AltairXGlobalAddrValue:
    expandPostRAGlobalAddrValue(inst);
    break;
  case AltairX::Ret:
    expandPostRARet(inst);
    break;
  case AltairX::IndirectCall:
    expandPostRAIndirectCall(inst);
    break;
  case AltairX::IndirectJump:
    expandPostRAIndirectJump(inst);
    break;
  case AltairX::ConstantToRegb:
    [[fallthrough]];
  case AltairX::ConstantToRegw:
    [[fallthrough]];
  case AltairX::ConstantToRegd:
    [[fallthrough]];
  case AltairX::ConstantToRegq:
    expandPostRAConstantToReg(inst);
    break;
  default:
    return false;
  }

  MachineBasicBlock &block = *inst.getParent();
  block.erase(inst);

  return true;
}

void AltairXInstrInfo::expandPostRAGlobalAddrValue(MachineInstr &inst) const {
  MachineBasicBlock &block = *inst.getParent();

  const auto dest = inst.getOperand(0).getReg();
  const auto addr = inst.getOperand(1);

  if (addr.isGlobal()) {
    const GlobalValue *global = addr.getGlobal();
    BuildMI(block, inst, inst.getDebugLoc(), get(AltairX::MoveIq))
        .addReg(dest, getDefRegState(true))
        .addGlobalAddress(global);
  } else if (addr.isJTI()) {
    BuildMI(block, inst, inst.getDebugLoc(), get(AltairX::MoveIq))
        .addReg(dest, getDefRegState(true))
        .addJumpTableIndex(addr.getIndex());
  } else {
    llvm_unreachable("Unsupported operand type!");
  }
}

void AltairXInstrInfo::expandPostRARet(MachineInstr &inst) const {
  MachineBasicBlock &block = *inst.getParent();
  auto *TRI = Subtarget.getRegisterInfo();

  BuildMI(block, inst, inst.getDebugLoc(), get(AltairX::IndirectCallLink))
      .addReg(TRI->getZeroRegister())
      .addReg(TRI->getLinkRegister());
}

void AltairXInstrInfo::expandPostRAIndirectCall(MachineInstr &inst) const {
  MachineBasicBlock &block = *inst.getParent();
  auto *TRI = Subtarget.getRegisterInfo();

  BuildMI(block, inst, inst.getDebugLoc(), get(AltairX::IndirectCallLink))
      .addReg(TRI->getLinkRegister())
      .addReg(inst.getOperand(0).getReg());
}

void AltairXInstrInfo::expandPostRAIndirectJump(MachineInstr &inst) const {
  MachineBasicBlock &block = *inst.getParent();
  auto *TRI = Subtarget.getRegisterInfo();

  BuildMI(block, inst, inst.getDebugLoc(), get(AltairX::IndirectCallLink))
      .addReg(TRI->getZeroRegister())
      .addReg(inst.getOperand(0).getReg());
}

void AltairXInstrInfo::expandPostRAConstantToReg(MachineInstr &inst) const {
  MachineBasicBlock &block = *inst.getParent();
  DebugLoc dl{inst.getDebugLoc()};

  const auto dest = inst.getOperand(0).getReg();
  const std::int64_t imm = inst.getOperand(1).getImm();

  if (isInt<42>(imm)) {
    // Fits movei + moveix
    BuildMI(block, inst, dl, get(getMoveIForReg(dest)))
        .addReg(dest, getDefRegState(true))
        .addImm(imm);
  } else {
    assert(AltairX::GPIReg64RegClass.contains(dest) && "Out of range imm");

    // movei + moveix supports the following ranges:
    // 0000 0000 0000 0000 to 0000 01FF FFFF FFFF (2^41-1)
    // FFFF FE00 0000 0000 (-2^41) to FFFF FFFF FFFF FFFF
    // Range that we have to cover somehow
    // 0000 0200 0000 0000 (2^41) to FFFF FDFF FFFF FFFF (-2^41 - 1)
    // AXIMPR: Some pattern could be matched with less instructions
    const std::uint64_t uimm = static_cast<std::uint64_t>(imm);
    const auto lowvalue = uimm & 0xFFFFFFFFull;
    const auto highvalue = (uimm >> 32) & 0xFFFFFFFFull;

    BuildMI(block, inst, dl, get(AltairX::MoveIq))
        .addReg(dest, getDefRegState(true))
        .addImm(highvalue);

    BuildMI(block, inst, dl, get(AltairX::LslRIq))
        .addReg(dest)
        .addReg(dest)
        .addImm(32);

    if (lowvalue != 0) { // this is an obvious optimization
      BuildMI(block, inst, dl, get(AltairX::AddRIq))
          .addReg(dest)
          .addReg(dest)
          .addImm(lowvalue);
    }
  }
}

bool AltairXInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *&TBB,
                                     MachineBasicBlock *&FBB,
                                     SmallVectorImpl<MachineOperand> &Cond,
                                     bool AllowModify) const {
  auto last = MBB.getLastNonDebugInstr();
  if (last == MBB.end()) {
    return false;
  }

  if (!isUnpredicatedTerminator(*last)) {
    return false;
  }

  // If there is only one terminator instruction, process it.
  auto secondLast = std::prev(last);
  if (last == MBB.begin() || !isUnpredicatedTerminator(*secondLast)) {
    if (isUncondBranchOpcode(*last)) {
      TBB = getBranchDestBlock(*last);
      return false;
    }

    if (isCondBranchOpcode(*last)) {
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
  if (AllowModify && isUncondBranchOpcode(*last) &&
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
  if (isCondBranchOpcode(*secondLast) && isUncondBranchOpcode(*last)) {
    TBB = getBranchDestBlock(*secondLast);
    Cond.emplace_back(secondLast->getOperand(1));
    FBB = getBranchDestBlock(*last);
    return false;
  }

  // If the block ends with two unconditional branches, handle it.  The second
  // one is not executed, so remove it.
  if (isUncondBranchOpcode(*secondLast) && isUncondBranchOpcode(*last)) {
    TBB = getBranchDestBlock(*secondLast);
    if (AllowModify) {
      last->eraseFromParent();
    }

    return false;
  }

  return true;
}

bool AltairXInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "Expected from analyseBranch");
  assert(Cond[0].getParent()->getOpcode() == AltairX::PseudoBRC &&
         "AltairXInstrInfo::reverseBranchCondition only works with PseudoBRC!");

  const auto cc = static_cast<ISD::CondCode>(Cond[0].getImm());
  const auto inversed = ISD::getSetCCInverse(cc, MVT::i64); // any int type
  Cond[0].setImm(static_cast<int64_t>(inversed));
  return false;
}

unsigned AltairXInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                        int *BytesRemoved) const {
  auto last = MBB.getLastNonDebugInstr();
  if (last == MBB.end()) {
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

unsigned AltairXInstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {
  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");

  if (!FBB) {
    if (Cond.empty()) { // Unconditional branch
      BuildMI(&MBB, DL, get(AltairX::BRA)).addMBB(TBB);
    } else {
      BuildMI(&MBB, DL, get(AltairX::PseudoBRC))
          .addMBB(TBB)
          .addImm(Cond[0].getImm());
    }

    if (BytesAdded) {
      *BytesAdded = 4;
    }

    return 1;
  }

  // Two-way conditional branch.
  BuildMI(&MBB, DL, get(AltairX::PseudoBRC))
      .addMBB(TBB)
      .addImm(Cond[0].getImm());
  BuildMI(&MBB, DL, get(AltairX::BRA)).addMBB(FBB);
  if (BytesAdded) {
    *BytesAdded = 8;
  }

  return 2;
}

MachineBasicBlock *
AltairXInstrInfo::getBranchDestBlock(const MachineInstr &inst) const {
  switch (inst.getOpcode()) {
  case AltairX::BRA:
    [[fallthrough]];
  case AltairX::PseudoBRC:
    [[fallthrough]];
  case AltairX::BRC:
    return inst.getOperand(0).getMBB();
  default:
    llvm_unreachable("unexpected opcode!");
  }
}

MachineOperand *AltairXInstrInfo::getLatestRegDef(MachineInstr &inst,
                                                  Register reg) {
  const MachineBasicBlock &block = *inst.getParent();
  const auto end = block.rend().getInstrIterator();
  for (auto begin = inst.getIterator().getReverse(); begin != end; ++begin) {
    MachineInstr &inst = *begin;
    if (inst.getNumOperands() == 0) {
      continue;
    }

    MachineOperand *operand = inst.findRegisterDefOperand(reg, false, false);
    if (operand) {
      return operand;
    }
  }

  return nullptr;
}
