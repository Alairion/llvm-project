//===-- AltairXInstrInfo.h - AltairX Instruction Information ----*- C++ -*-===//
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
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;

  bool expandPostRAPseudo(MachineInstr &MI) const override;

private:
  void expandPostRAGlobalAddrValue(MachineInstr &inst) const;
  void expandPostRARet(MachineInstr &inst) const;
  void expandPostRAIndirectCall(MachineInstr &inst) const;
  void expandPostRAIndirectJump(MachineInstr &inst) const;
  void expandPostRAConstantToReg(MachineInstr &inst) const;

  // All bitcasts can be handled with add + fmove
  // Caller must provide the right opcodes for the register class in use
  void makeBitcastToFloat(MachineInstr &inst, Register dest, Register src,
                          uint32_t add, uint32_t fmove) const;
  void makeBitcastToInt(MachineInstr &inst, Register dest, Register src,
                        uint32_t add, uint32_t fmove) const;
  void expandPostRABitcast(MachineInstr &inst) const;

public:
  // Ensure that inst can read `reg` value from its accumulator (r56)
  // Returns an instruction located right before `inst`
  // that put `reg` into the accumulator of `inst` unit
  // `opcode` is the "move" instruction, it should be AddRI for ints, and FMove for floats.
  MachineInstr *ensureInAccumulator(MachineInstr &inst, Register reg, uint32_t opcode,
                                    bool addZero) const;

  // Ensure that inst can read `reg` value from `bypass`
  MachineInstr* ensureFromAccumulator(MachineInstr& inst, Register reg, Register bypass, uint32_t opcode,
    bool addZero);

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, Register SrcReg,
                           bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI, Register VReg,
                           MachineInstr::MIFlag Flags) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI, Register DestReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI, Register VReg,
                            MachineInstr::MIFlag Flags) const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const override;

  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;

  MachineBasicBlock *getBranchDestBlock(const MachineInstr &MI) const override;

  // Find the last definition of reg before MI
  // Return the *operand* if found, nullptr otherwise.
  // A register may not be defined by an operand.
  static MachineOperand *getLatestRegDef(MachineInstr & inst, Register reg);

  static bool isUncondBranchOpcode(const MachineInstr &inst) {
    return inst.getOpcode() == AltairX::BRA;
  }

  static bool isCondBranchOpcode(const MachineInstr &inst) {
    return inst.getOpcode() == AltairX::BRC ||
           inst.getOpcode() == AltairX::BRCb ||
           inst.getOpcode() == AltairX::BRCw ||
           inst.getOpcode() == AltairX::BRCd ||
           inst.getOpcode() == AltairX::BRCq ||
           inst.getOpcode() == AltairX::FBRCs ||
           inst.getOpcode() == AltairX::FBRCd;
  }

  static bool isCmp(const MachineInstr &inst) {
    return inst.getOpcode() == AltairX::CmpRIb ||
           inst.getOpcode() == AltairX::CmpRIw ||
           inst.getOpcode() == AltairX::CmpRId ||
           inst.getOpcode() == AltairX::CmpRIq ||
           inst.getOpcode() == AltairX::CmpRRb ||
           inst.getOpcode() == AltairX::CmpRRw ||
           inst.getOpcode() == AltairX::CmpRRd ||
           inst.getOpcode() == AltairX::CmpRRq;
  }

  static bool isFCmp(const MachineInstr& inst) {
    return inst.getOpcode() == AltairX::FCmpRRs ||
           inst.getOpcode() == AltairX::FCmpRRd;
  }

  static bool isAnyCmp(const MachineInstr &inst) {
    return isCmp(inst) || isFCmp(inst);
  }

protected:
  const AltairXSubtarget &Subtarget;
};
} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_INSTRINFO_H
