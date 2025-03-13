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

AltairXInstrInfo::AltairXInstrInfo(const AltairXSubtarget &STI)
    : AltairXGenInstrInfo(AltairX::ADJCALLSTACKDOWN, AltairX::ADJCALLSTACKUP),
      Subtarget(STI) {}

namespace {

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

uint32_t getFRegCopy(MCRegister reg) {
  if (AltairX::FReg64RegClass.contains(reg)) {
    return AltairX::FMoveRd;
  } else if (AltairX::FReg32RegClass.contains(reg)) {
    return AltairX::FMoveRs;
  }

  llvm_unreachable("Wrong register class");
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

uint32_t getFRegToEFRegCopy(MCRegister reg) {
  if (AltairX::FReg32RegClass.contains(reg)) {
    return AltairX::SetEFs;
  } else if (AltairX::FReg64RegClass.contains(reg)) {
    return AltairX::SetEFd;
  }

  llvm_unreachable("Wrong register class");
}

uint32_t getEFRegToFRegCopy(MCRegister reg) {
  if (AltairX::FReg32RegClass.contains(reg)) {
    return AltairX::GetEFs;
  } else if (AltairX::FReg64RegClass.contains(reg)) {
    return AltairX::GetEFd;
  }

  llvm_unreachable("Wrong register class");
}

uint32_t getSpecialRegCopyOpcode(MCRegister dest, MCRegister src) {
  using Info = AltairXRegisterInfo;
  if (Info::isMDUReg(dest) && Info::isGPIReg(src)) { // SetMD
    return getGPIRegToMDURegCopy(dest);
  } else if (Info::isGPIReg(dest) && Info::isMDUReg(src)) { // GetMD
    return getMDURegToGPIRegCopy(src);
  } else if (Info::isRIReg(dest) && Info::isGPIReg(src)) { // SetFR
    return getGPIRegToRIRegCopy(dest);
  } else if (Info::isGPIReg(dest) && Info::isRIReg(src)) { // GetRI
    return getRIRegToGPIRegCopy(src);
  } else if (Info::isEFReg(dest) && Info::isFReg(src)) { // SetEF
    return getFRegToEFRegCopy(src);
  } else if (Info::isFReg(dest) && Info::isEFReg(src)) { // GetEF
    return getEFRegToFRegCopy(dest);
  } else {
    llvm_unreachable("Unsuported physical reg copy");
  }
}

uint32_t getBitcastAdd(MCRegister reg) {
  if (AltairX::GPIReg64RegClass.contains(reg)) {
    return AltairX::AddRIq;
  } else if (AltairX::GPIReg32RegClass.contains(reg)) {
    return AltairX::AddRId;
  } else if (AltairX::GPIReg8RegClass.contains(reg)) {
    return AltairX::AddRIb;
  }

  llvm_unreachable("Wrong register class");
}
uint32_t getBitcastFMove(MCRegister reg) {

  if (AltairX::FReg64RegClass.contains(reg)) {
    return AltairX::FMoveRd;
  } else if (AltairX::FReg32RegClass.contains(reg)) {
    return AltairX::FMoveRs;
  } else if (AltairX::VIReg8RegClass.contains(reg)) {
    return AltairX::VIMoveRb;
  }

  llvm_unreachable("Wrong register class");
}

} // namespace

void AltairXInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const DebugLoc &DL, MCRegister DestReg,
                                   MCRegister SrcReg, bool KillSrc) const {
  using Info = AltairXRegisterInfo;
  if (Info::isGPIReg(DestReg) && Info::isGPIReg(SrcReg)) { // Add r, 0
    BuildMI(MBB, MI, DL, get(getGPIRegCopy(DestReg)), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc))
        .addImm(0);
  } else if (Info::isFReg(DestReg) && Info::isFReg(SrcReg)) { // FMove r
    BuildMI(MBB, MI, DL, get(getFRegCopy(DestReg)), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  } else if (Info::isGPIReg(DestReg) && Info::isVIReg(SrcReg)) { // bitcast
    makeBitcastToInt(*MI, getBitcastAdd(DestReg), getBitcastFMove(SrcReg));
  } else if (Info::isVIReg(DestReg) && Info::isGPIReg(SrcReg)) { // bitcast
    makeBitcastToFloat(*MI, getBitcastAdd(SrcReg), getBitcastFMove(DestReg));
  } else {
    // Special registers moves (EF, RI, ...)
    BuildMI(MBB, MI, DL, get(getSpecialRegCopyOpcode(DestReg, SrcReg)), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  }
}

namespace {

uint32_t getSpillOpcode(uint32_t spillSize, Register reg) {
  if (AltairXRegisterInfo::isFReg(reg)) {
    switch (spillSize) {
    case 4:
      return AltairX::FStoreRIs;
    case 8:
      return AltairX::FStoreRId;
    default:
      llvm_unreachable("Unspillable reg class");
    }
  } else {
    switch (spillSize) {
    case 1:
      return AltairX::StoreRIb;
    case 2:
      return AltairX::StoreRIw;
    case 4:
      return AltairX::StoreRId;
    case 8:
      return AltairX::StoreRIq;
    default:
      llvm_unreachable("Unspillable reg class");
    }
  }
}

uint32_t getReloadOpcode(uint32_t spillSize, Register reg) {
  if (AltairXRegisterInfo::isFReg(reg)) {
    switch (spillSize) {
    case 4:
      return AltairX::FLoadRIs;
    case 8:
      return AltairX::FLoadRId;
    default:
      llvm_unreachable("Unspillable reg class");
    }
  } else {
    switch (spillSize) {
    case 1:
      return AltairX::LoadRIb;
    case 2:
      return AltairX::LoadRIw;
    case 4:
      return AltairX::LoadRId;
    case 8:
      return AltairX::LoadRIq;
    default:
      llvm_unreachable("Unspillable reg class");
    }
  }

  llvm_unreachable("Unspillable reg class");
}

} // namespace

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

  const auto spillSize = TRI->getSpillSize(*RC);
  BuildMI(MBB, MI, DebugLoc(), get(getSpillOpcode(spillSize, SrcReg)))
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

  const auto spillSize = TRI->getSpillSize(*RC);
  BuildMI(MBB, MI, DebugLoc(), get(getReloadOpcode(spillSize, DestReg)),
          DestReg)
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
  case AltairX::BitcastIqToFd:
    [[fallthrough]];
  case AltairX::BitcastIdToFs:
    [[fallthrough]];
  case AltairX::BitcastFdToIq:
    [[fallthrough]];
  case AltairX::BitcastFsToId:
    expandPostRABitcast(inst);
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

namespace {

// Return true if instruction is known to do nothing
// Most no-op instructions are dropper before machine instruction
// creation. This function only lists the one that won't be.
static bool isNoop(const MachineInstr& inst)
{
  return inst.getOpcode() == AltairX::KILL ||
    inst.getOpcode() == AltairX::EXTRACT_SUBREG ||
    inst.getOpcode() == AltairX::SUBREG_TO_REG;
}

}

void AltairXInstrInfo::makeBitcastToFloat(MachineInstr &inst, uint32_t add,
                                          uint32_t fmove) const {
  MachineBasicBlock &block = *inst.getParent();
  DebugLoc dl{inst.getDebugLoc()};
  const llvm::TargetRegisterInfo *regInfo = this->Subtarget.getRegisterInfo();

  const Register destReg = inst.getOperand(0).getReg();
  const Register srcReg = inst.getOperand(1).getReg();

  // If previous instruction defines the source register, a copy of the value
  // will be in the accumulator, so no additional operation is required.
  auto it = inst.getIterator();
  if (it != block.getFirstNonDebugInstr()) {
    const auto previous = std::prev(it);
    if (!previous->definesRegister(srcReg, regInfo)) {
      BuildMI(block, inst, dl, get(add), AltairX::R56)
        .addReg(srcReg).addImm(0);
    }
  } else {
    BuildMI(block, inst, dl, get(add), AltairX::R56).addReg(srcReg).addImm(0);
  }

  // If next instruction kills the destination register
  // use the bypass directly
  if (it != block.getLastNonDebugInstr()) {
    const auto next = std::next(it);
    if (!isNoop(*next) && next->killsRegister(destReg, regInfo)) {
      for (auto &op :
           make_range(next->operands_begin() + 1, next->operands_end())) {
        if (op.isReg() && op.getReg() == destReg) {
          op.setReg(AltairX::R57);
        }
      }

      return;
    }
  }

  // Else use an add to move the acc to a register
  BuildMI(block, inst, dl, get(fmove), destReg)
      .addReg(AltairX::R57, getKillRegState(true));
}

void AltairXInstrInfo::makeBitcastToInt(MachineInstr &inst, uint32_t add,
                                        uint32_t fmove) const {
  MachineBasicBlock &block = *inst.getParent();
  DebugLoc dl{inst.getDebugLoc()};
  const llvm::TargetRegisterInfo *regInfo = this->Subtarget.getRegisterInfo();

  const Register destReg = inst.getOperand(0).getReg();
  const Register srcReg = inst.getOperand(1).getReg();

  // If previous instruction defines the source register, a copy of the value
  // will be in the accumulator, so no additional operation is required.
  const auto it = inst.getIterator();
  if (it->getOpcode() != AltairX::KILL && it != block.getFirstNonDebugInstr()) {
    const auto previous = std::prev(it);
    if (!previous->definesRegister(srcReg, regInfo)) {
      BuildMI(block, inst, dl, get(fmove), AltairX::R56).addReg(srcReg);
    }
  } else {
    BuildMI(block, inst, dl, get(fmove), AltairX::R56).addReg(srcReg);
  }

  // If next instruction kills the destination register
  // use the bypass directly
  if (it != block.getLastNonDebugInstr()) {
    const auto next = std::next(it);
    if (!isNoop(*next) && next->killsRegister(destReg, regInfo)) {
      for (auto &op :
           make_range(next->operands_begin() + 1, next->operands_end())) {
        if (op.isReg() && op.getReg() == destReg) {
          op.setReg(AltairX::R59);
        }
      }

      return;
    }
  }

  // Else use an add to move the acc to a register
  BuildMI(block, inst, dl, get(add), destReg)
      .addReg(AltairX::R59, getKillRegState(true))
      .addImm(0);
}

void AltairXInstrInfo::expandPostRABitcast(MachineInstr &inst) const {
  switch(inst.getOpcode())
  {
  case AltairX::BitcastIdToFs:
    makeBitcastToFloat(inst, AltairX::AddRId, AltairX::FMoveRs);
    break;
  case AltairX::BitcastIqToFd:
    makeBitcastToFloat(inst, AltairX::AddRIq, AltairX::FMoveRd);
    break;
  case AltairX::BitcastFsToId:
    makeBitcastToInt(inst, AltairX::AddRId, AltairX::FMoveRs);
    break;
  case AltairX::BitcastFdToIq:
    makeBitcastToInt(inst, AltairX::AddRIq, AltairX::FMoveRd);
    break;
  default:
    llvm_unreachable("Unknown bitcast");
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
