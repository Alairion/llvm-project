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
#include "AltairXCommon.h"
#include "AltairXSubtarget.h"

#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

#define DEBUG_TYPE "moveix-filler"

namespace llvm {

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

bool AltairXMoveIXFiller::runOnMachineFunction(MachineFunction &func) {
  target = &func.getTarget();
  instInfo = func.getSubtarget<AltairXSubtarget>().getInstrInfo();
  for (auto &block : func) {
    runOnMachineBasicBlock(block);
  }

  return false;
}

namespace {

bool fitsImm(const MachineInstr &inst, std::int64_t imm) {
  switch (inst.getDesc().TSFlags) {
  case AltairX::InstFormatMoveImm18:
    return llvm::isInt<18>(imm);
  case AltairX::InstFormatALURegImm9:
    return llvm::isInt<9>(imm);
  case AltairX::InstFormatALURegRegImm9:
    return llvm::isInt<9>(imm);
  case AltairX::InstFormatLSURegImm10:
    return llvm::isInt<10>(imm);
  case AltairX::InstFormatFPURegImm16:
    llvm_unreachable("todo: impl-fpu");
  case AltairX::InstFormatBRURelImm23:
    return llvm::isInt<25>(imm); // always aligned on 4 bytes
  case AltairX::InstFormatBRURelImm24:
    return llvm::isInt<26>(imm); // always aligned on 4 bytes
  case AltairX::InstFormatBRUAbsImm24:
    return llvm::isUInt<26>(
        static_cast<std::uint64_t>(imm)); // always aligned on 4 bytes
  case AltairX::InstFormatCMPRegImm9:
    return llvm::isInt<9>(imm);
  default:
    llvm_unreachable("Unknown instruction type!");
  }
}

inline constexpr std::uint32_t noImm =
    std::numeric_limits<std::uint32_t>::max();

std::uint32_t immOperandIndex(const MachineInstr &inst) {
  switch (inst.getDesc().TSFlags) {
  case AltairX::InstFormatMoveImm18:
    return 1;
  case AltairX::InstFormatALURegImm9:
    return 2;
  case AltairX::InstFormatALURegRegImm9:
    return 3;
  case AltairX::InstFormatLSURegImm10:
    return 2;
  case AltairX::InstFormatFPURegImm16:
    llvm_unreachable("todo: impl-fpu");
  case AltairX::InstFormatBRURelImm23:
    return noImm; // assume always in range (add option?)
  case AltairX::InstFormatBRURelImm24:
    return noImm; // assume always in range (add option?)
  case AltairX::InstFormatBRUAbsImm24:
    return 0;
  case AltairX::InstFormatCMPRegImm9:
    return 1;
  default:
    return noImm;
  }
}

std::uint32_t getMoveIX(const MachineInstr &inst) {
  switch (inst.getDesc().TSFlags) {
  case AltairX::InstFormatMoveImm18:
    return AltairX::MOVEIX18;
  case AltairX::InstFormatALURegImm9:
    return AltairX::MOVEIX9;
  case AltairX::InstFormatALURegRegImm9:
    return AltairX::MOVEIX9;
  case AltairX::InstFormatLSURegImm10:
    return AltairX::MOVEIX10;
  case AltairX::InstFormatFPURegImm16:
    llvm_unreachable("todo: impl-fpu");
  case AltairX::InstFormatBRURelImm23:
    return AltairX::MOVEIX23PCREL;
  case AltairX::InstFormatBRURelImm24:
    return AltairX::MOVEIX24PCREL;
  case AltairX::InstFormatBRUAbsImm24:
    return AltairX::MOVEIX24ABS;
  case AltairX::InstFormatCMPRegImm9:
    return AltairX::MOVEIX9;
  default:
    llvm_unreachable("Unknown instruction type!");
  }
}



} // namespace

// add operand ranges to fix unexpected moveix (ex shift on loadrr)

void AltairXMoveIXFiller::runOnMachineBasicBlock(MachineBasicBlock &block) {
  for (auto it = block.begin(); it != block.end(); ++it) {
    const auto immIndex = immOperandIndex(*it);
    if (immIndex == noImm) {
      continue;
    }

    const auto makeBuilder = [&]() {
      return BuildMI(block, std::next(it), it->getDebugLoc(),
                     instInfo->get(getMoveIX(*it)));
    };

    llvm::MachineInstr *moveix{nullptr};
    auto &op = it->getOperand(immIndex);
    if (op.isGlobal()) {
      moveix = makeBuilder().addGlobalAddress(op.getGlobal()).getInstr();
    } else if (op.isJTI()) {
      moveix = makeBuilder().addJumpTableIndex(op.getIndex()).getInstr();
    } else if (op.isCPI()) {
      moveix = makeBuilder().addConstantPoolIndex(op.getIndex()).getInstr();
    } else if (op.isImm()) {
      if (!fitsImm(*it, op.getImm())) {
        moveix = makeBuilder().addImm(op.getImm()).getInstr();
      }
    } else if (op.isMBB()) {
      moveix = makeBuilder().addMBB(op.getMBB()).getInstr();
    } else if (op.isSymbol()) {
      moveix = makeBuilder().addExternalSymbol(op.getSymbolName()).getInstr();
    } else {
      LLVM_DEBUG(it->dump());
      llvm_unreachable("Unsupported immediate type!");
    }

    if (moveix) {
      moveix->bundleWithPred();
      finalizeBundle(block, it.getInstrIterator(),
                     std::next(moveix->getIterator()));
    }
  }
}

} // namespace llvm
