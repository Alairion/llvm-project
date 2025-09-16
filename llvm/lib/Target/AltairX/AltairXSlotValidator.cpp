//===-- AltairXSlotValidator.cxx - C++ ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AltairXSlotValidator.h"
#include "AltairXCommon.h"
#include "AltairXSubtarget.h"

#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

#define DEBUG_TYPE "slot-validator"

namespace llvm {

char AltairXSlotValidator::ID = 0;

INITIALIZE_PASS(
    AltairXSlotValidator, "altairx-slot-validator",
    "Ensure that instruction only available on a specific slot are well bundled",
    false, false)

FunctionPass *createAltairXSlotValidatorPass() {
  return new AltairXSlotValidator();
}

AltairXSlotValidator::AltairXSlotValidator() : MachineFunctionPass(ID) {
  initializeAltairXSlotValidatorPass(*PassRegistry::getPassRegistry());
}

bool AltairXSlotValidator::runOnMachineFunction(MachineFunction &func) {
  target = &func.getTarget();
  instInfo = func.getSubtarget<AltairXSubtarget>().getInstrInfo();
  for (auto &block : func) {
    runOnMachineBasicBlock(block);
  }

  return false;
}

namespace {
bool isSecondSlotOnly(const MachineInstr &inst) {
  switch (inst.getOpcode()) {
  case AltairX::BRK:
    return true;
  case AltairX::SYSCALL:
    return true;
  case AltairX::GetIR:
    return true;
  case AltairX::SetFR:
    return true;
  default:
    return false;
  }
}

// Return the instruction before it that is suitable for bundles
MachineInstr *ensureBundable(MachineBasicBlock &block,
                             MachineBasicBlock::iterator it,
                             const TargetInstrInfo *instInfo) {
  if (it != block.begin()) {
    auto prevIt = std::prev(it);
    if (prevIt->getOpcode() != AltairX::BUNDLE) {
      return to_address(prevIt);
    }
  }

  // Add a nop to bundle it with it
  return BuildMI(block, it, it->getDebugLoc(), instInfo->get(AltairX::NOP));
}

} // namespace

void AltairXSlotValidator::runOnMachineBasicBlock(MachineBasicBlock &block) {
  for (auto it = block.begin(); it != block.end(); ++it) {
    // For instruction of CU and VU:
    // - bundle them with previous instruction if possible
    // - otherwise, add a noop, then bundle with it
    if (isSecondSlotOnly(*it)) {
      auto* prev = ensureBundable(block, it, instInfo);
      it->bundleWithPred();
      finalizeBundle(block, prev->getIterator(), std::next(it->getIterator()));
    }

  }
}

} // namespace llvm
