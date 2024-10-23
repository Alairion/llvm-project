//===-- AltairXMoveIXFiller.cxx - AltairX Register Information Impl - C++--===//
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

#include "AltairXMoveIXFiller.h"
#include "AltairXSubtarget.h"
#include "AltairXCommon.h"

#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

namespace llvm
{

char AltairXMoveIXFiller::ID = 0;

INITIALIZE_PASS(
    AltairXMoveIXFiller, "altairx-moveix-filler",
    "Add 'moveix' instruction next to instructions requiring bigger immediates",
    false, false)

FunctionPass *llvm::createAltairXMoveIXFillerPass() {
  return new AltairXMoveIXFiller();
}

AltairXMoveIXFiller::AltairXMoveIXFiller() : MachineFunctionPass(ID) {
  initializeAltairXMoveIXFillerPass(*PassRegistry::getPassRegistry());
}

bool AltairXMoveIXFiller::runOnMachineFunction(MachineFunction &F) {
  TM = &F.getTarget(); 
  TII = F.getSubtarget<AltairXSubtarget>().getInstrInfo();
  for (auto &MBB : F) {
    runOnMachineBasicBlock(MBB);
  }

  return false;
}

namespace
{

bool fitsImm(const MachineInstr &inst, std::int64_t imm) {
  switch (inst.getDesc().TSFlags) {
  case AltairX::InstFormatMoveImm18:
    return llvm::isInt<18>(imm);
  case AltairX::InstFormatALURegImm9:
    return llvm::isInt<9>(imm);
  case AltairX::InstFormatLSURegImm10:
    return llvm::isInt<10>(imm);
  case AltairX::InstFormatLSURegImm16:
    return llvm::isUInt<16>(static_cast<std::uint64_t>(imm));
  case AltairX::InstFormatFPURegImm16:
    llvm_unreachable("todo: impl-fpu");
  case AltairX::InstFormatBRURelImm24:
    return llvm::isInt<26>(imm); // always aligned on 4 bytes
  case AltairX::InstFormatBRUAbsImm24:
    return llvm::isUInt<26>(
        static_cast<std::uint64_t>(imm)); // always aligned on 4 bytes
  default:
    return false;
  }
}

inline constexpr std::uint32_t noImm = std::numeric_limits<std::uint32_t>::max();
std::uint32_t immOperandIndex(const MachineInstr& inst)
{
  switch(inst.getDesc().TSFlags) {
  case AltairX::InstFormatMoveImm18:
    return 1;
  case AltairX::InstFormatALURegImm9:
    return 2;
  case AltairX::InstFormatLSURegImm10:
    return 2;
  case AltairX::InstFormatLSURegImm16:
    llvm_unreachable("LDP/STP must not be matched if imm exceed 16-bits!");
  case AltairX::InstFormatFPURegImm16:
    llvm_unreachable("todo: impl-fpu");
  case AltairX::InstFormatBRURelImm24:
    return 0;
  case AltairX::InstFormatBRUAbsImm24:
    return 0;
  default:
    return noImm;
  }
}

std::uint32_t getMoveIX(const MachineInstr& inst)
{
  switch(inst.getDesc().TSFlags) {
  case AltairX::InstFormatMoveImm18:
    return AltairX::MOVEIX18;
  case AltairX::InstFormatALURegImm9:
    return AltairX::MOVEIX9;
  case AltairX::InstFormatLSURegImm10:
    return AltairX::MOVEIX10;
  case AltairX::InstFormatLSURegImm16:
    llvm_unreachable("LDP/STP must not be matched if imm exceed 16-bits!");
  case AltairX::InstFormatFPURegImm16:
    llvm_unreachable("todo: impl-fpu");
  case AltairX::InstFormatBRURelImm24:
    llvm_unreachable("todo: moveix");
  case AltairX::InstFormatBRUAbsImm24:
    llvm_unreachable("todo: moveix");
  default:
    return false;
  }
}

}

// add operand ranges to fix unexpected moveix (ex shift on loadrr)

void AltairXMoveIXFiller::runOnMachineBasicBlock(MachineBasicBlock &MBB) {
  for (auto it = MBB.begin(); it != MBB.end(); ++it) {
    const auto immIndex = immOperandIndex(*it);
    if(immIndex == noImm) {
      continue;
    }

    auto& op = it->getOperand(immIndex);
    if (op.isGlobal()) {
      BuildMI(MBB, std::next(it), it->getDebugLoc(), TII->get(getMoveIX(*it)))
        .addGlobalAddress(op.getGlobal());
      ++it;
    } else if (op.isImm() && !fitsImm(*it, op.getImm())) {
      MachineBasicBlock::iterator last =
          BuildMI(MBB, std::next(it), it->getDebugLoc(),
                  TII->get(getMoveIX(*it)))
              .addImm(op.getImm());
      last->bundleWithPred();
      
      finalizeBundle(MBB, it.getInstrIterator(), std::next(last).getInstrIterator());
      ++it;
    }
  }
}

} // namespace llvm
