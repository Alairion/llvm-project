//===-- AltairXFrameLowering.cpp - AltairX Frame Information
//------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the AltairXTargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "AltairXFrameLowering.h"

#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/Debug.h"

#include "AltairXSubtarget.h"
#include "AltairXInstrInfo.h"
#include "AltairXRegisterInfo.h"

using namespace llvm;

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register.  This is true if the function has variable sized allocas,
// if it needs dynamic stack realignment, if frame pointer elimination is
// disabled, or if the frame address is taken.
bool AltairXFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo& MFI = MF.getFrameInfo();
  const TargetRegisterInfo* TRI = STI.getRegisterInfo();

  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
    MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken() ||
    TRI->hasStackRealignment(MF);
}

MachineBasicBlock::iterator AltairXFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  return MBB.erase(I);
}

void AltairXFrameLowering::emitPrologue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  MachineFrameInfo& MFI = MF.getFrameInfo();

  const auto stackSize = static_cast<std::int64_t>(MFI.getStackSize());
  if(stackSize == 0 && !MFI.adjustsStack()) {
    return;
  }

  const AltairXInstrInfo& TII = *STI.getInstrInfo();
  const AltairXRegisterInfo& TRI = *STI.getRegisterInfo();
  //MachineModuleInfo& MMI = MF.getMMI();
  //const MCRegisterInfo* MRI = MMI.getContext().getRegisterInfo();
  auto MBBI = MBB.begin();
  DebugLoc DL{};

  const Register stackReg = TRI.getStackRegister();

  // "Allocate" stack space: add sp, sp, -stackSize
  BuildMI(MBB, MBBI, DebugLoc{}, TII.get(AltairX::AddRIq), stackReg)
    .addReg(stackReg)
    .addImm(-stackSize);

  // emit ".cfi_def_cfa_offset StackSize"
  //auto CFIIndex =
  //  MF.addFrameInst(MCCFIInstruction::cfiDefCfaOffset(nullptr, stackSize));
  //BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
  //  .addCFIIndex(CFIIndex);

  const auto& CSI = MFI.getCalleeSavedInfo();
  if(!CSI.empty()) {
    // Find the instruction past the last instruction that saves a callee-saved
    // register to the stack.
    std::advance(MBBI, CSI.size());

    // Iterate over list of callee-saved registers and emit .cfi_offset
    // directives.
    //for(const CalleeSavedInfo& I : CSI) {
    //  const std::int64_t offset = MFI.getObjectOffset(I.getFrameIdx());
    //  auto reg = I.getReg();
    //
    //}
  }

  // if framepointer enabled, set it to point to the stack pointer.
  if(hasFP(MF)) {
    const Register frameReg = TRI.getFrameRegister(MF);

    // Insert instruction "move $fp, $sp" at this location.
    BuildMI(MBB, MBBI, DL, TII.get(AltairX::AddRIq), frameReg)
      .addReg(stackReg)
      .addImm(0)
      .setMIFlag(MachineInstr::FrameSetup);

    // emit ".cfi_def_cfa_register $fp"
    //unsigned CFIIndex = MF.addFrameInst(MCCFIInstruction::createDefCfaRegister(
    //  nullptr, MRI->getDwarfRegNum(FP, true)));
    //BuildMI(MBB, MBBI, dl, TII.get(TargetOpcode::CFI_INSTRUCTION))
    //  .addCFIIndex(CFIIndex);
    //if(RegInfo.hasStackRealignment(MF)) {
    //  // addiu $Reg, $zero, -MaxAlignment
    //  // andi $sp, $sp, $Reg
    //  Register VR = MF.getRegInfo().createVirtualRegister(RC);
    //  assert((Log2(MFI.getMaxAlign()) < 16) &&
    //    "Function's alignment size requirement is not supported.");
    //  int64_t MaxAlign = -(int64_t)MFI.getMaxAlign().value();
    //
    //  BuildMI(MBB, MBBI, dl, TII.get(ADDiu), VR).addReg(ZERO).addImm(MaxAlign);
    //  BuildMI(MBB, MBBI, dl, TII.get(AND), SP).addReg(SP).addReg(VR);
    //
    //  if(hasBP(MF)) {
    //    // move $s7, $sp
    //    unsigned BP = STI.isABI_N64() ? Mips::S7_64 : Mips::S7;
    //    BuildMI(MBB, MBBI, dl, TII.get(MOVE), BP)
    //      .addReg(SP)
    //      .addReg(ZERO);
    //  }
    //}
  }
}

void AltairXFrameLowering::emitEpilogue(MachineFunction &MF,
                                        MachineBasicBlock &MBB) const {
  MachineFrameInfo& MFI = MF.getFrameInfo();
  const AltairXInstrInfo& TII = *STI.getInstrInfo();
  const AltairXRegisterInfo& TRI = *STI.getRegisterInfo();
  auto MBBI = MBB.getFirstTerminator();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc{};

  const Register stackReg = TRI.getStackRegister();

  // if framepointer enabled, restore the stack pointer.
  if(hasFP(MF)) {
    const Register frameReg = TRI.getFrameRegister(MF);

    // Find the first instruction that restores a callee-saved register.
    auto it = MBBI;
    const auto& CSI = MFI.getCalleeSavedInfo();
    std::advance(it, -static_cast<std::ptrdiff_t>(CSI.size()));

    // Insert instruction "move sp, fp" at this location.    
    BuildMI(MBB, it, DL, TII.get(AltairX::AddRIq), stackReg)
      .addReg(frameReg)
      .addImm(0);
  }

  const auto stackSize = static_cast<std::int64_t>(MFI.getStackSize());
  if (stackSize == 0) {
    return;
  }

  // Adjust stack.
  BuildMI(MBB, MBBI, DebugLoc{}, TII.get(AltairX::AddRIq), stackReg)
    .addReg(stackReg)
    .addImm(stackSize);
}

bool AltairXFrameLowering::hasReservedCallFrame(
    const MachineFunction &MF) const {
  return true;
}

// This method is called immediately before PrologEpilogInserter scans the
// physical registers used to determine what callee saved registers should be
// spilled. This method is optional.
void AltairXFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                                BitVector &SavedRegs,
                                                RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  // Mark FP as used
  if(hasFP(MF)) {
    SavedRegs.set(AltairX::R31);
  }
}

bool AltairXFrameLowering::assignCalleeSavedSpillSlots(
    MachineFunction &MF, const TargetRegisterInfo *TRI,
    std::vector<CalleeSavedInfo> &CSI) const {
  return TargetFrameLowering::assignCalleeSavedSpillSlots(MF, TRI, CSI);
}