//===- AltairXBranchPatcher.cxx - AltairX Register Information Impl - C++ -===//
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

#include "AltairXBranchPatcher.h"
#include "AltairXCommon.h"
#include "AltairXSubtarget.h"

#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"

namespace llvm {

char AltairXBranchPatcher::ID = 0;

INITIALIZE_PASS_BEGIN(AltairXBranchPatcher, "altairx-branch-patcher",
                      "Replace PseudoBRC with BRC and fix related operations",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(MachineBranchProbabilityInfoWrapperPass)
INITIALIZE_PASS_END(AltairXBranchPatcher, "altairx-branch-patcher",
                    "Replace PseudoBRC with BRC and fix related operations",
                    false, false)

FunctionPass *llvm::createAltairXBranchPatcherPass() {
  return new AltairXBranchPatcher();
}

AltairXBranchPatcher::AltairXBranchPatcher() : MachineFunctionPass(ID) {
  initializeAltairXBranchPatcherPass(*PassRegistry::getPassRegistry());
}

void AltairXBranchPatcher::getAnalysisUsage(AnalysisUsage &analysis) const {
  analysis.addRequired<MachineBranchProbabilityInfoWrapperPass>();
  MachineFunctionPass::getAnalysisUsage(analysis);
}

bool AltairXBranchPatcher::runOnMachineFunction(MachineFunction &func) {
  target = &func.getTarget();
  instInfo = func.getSubtarget<AltairXSubtarget>().getInstrInfo();
  branchInfo =
      &getAnalysis<MachineBranchProbabilityInfoWrapperPass>().getMBPI();

  for (auto &block : func) {
    runOnMachineBasicBlock(block);
  }

  return false;
}

void AltairXBranchPatcher::runOnMachineBasicBlock(MachineBasicBlock &block) {
  auto last = block.getLastNonDebugInstr();
  if (last == block.end()) {
    return; // empty block (always fallthrough)
  }

  if (!AltairXInstrInfo::isUncondBranchOpcode(*last) &&
      !AltairXInstrInfo::isCondBranchOpcode(*last)) {
    return; // no terminator (always fallthrough)
  }

  if (AltairXInstrInfo::isUncondBranchOpcode(*last) &&
      last == block.getFirstNonDebugInstr()) {
    return; // single unconditional branch
  }

  const auto secondLast = std::prev(last);
  if (AltairXInstrInfo::isCondBranchOpcode(*last)) {
    runOnPseudoBRC(block, *last);
  } else if (AltairXInstrInfo::isCondBranchOpcode(*secondLast)) {
    runOnPseudoBRC(block, *secondLast);
  }
}

namespace {

template <typename It> // It::value_type compatible with const machineInstr&
auto findNearestCmp(It begin, It end) {
  return std::find_if(begin, end, AltairXInstrInfo::isAnyCmp);
}

struct BRCOperands {
  // for brc
  AltairX::BRCondCode cc{};
  // for cmp
  bool swapCMPOperands{};
};

BRCOperands analysePseudoBRC(ISD::CondCode value) {
  switch (value) {
  case ISD::SETOEQ:
    return {AltairX::BRCondCode::EQ, false};
  case ISD::SETOGT:
    return {AltairX::BRCondCode::LT, true};
  case ISD::SETOGE:
    return {AltairX::BRCondCode::GE, false};
  case ISD::SETOLT:
    return {AltairX::BRCondCode::LT, false};
  case ISD::SETOLE:
    return {AltairX::BRCondCode::GE, true};
  case ISD::SETONE:
    return {AltairX::BRCondCode::NE, false};
  case ISD::SETO: // AXIMPR: support NaN properly
    return {AltairX::BRCondCode::EQ, false};
  case ISD::SETUO: // AXIMPR: support NaN properly
    return {AltairX::BRCondCode::NE, false};
  case ISD::SETUEQ:
    return {AltairX::BRCondCode::EQ, false};
  case ISD::SETUGT:
    return {AltairX::BRCondCode::LTU, true};
  case ISD::SETUGE:
    return {AltairX::BRCondCode::GEU, false};
  case ISD::SETULT:
    return {AltairX::BRCondCode::LTU, false};
  case ISD::SETULE:
    return {AltairX::BRCondCode::GEU, true};
  case ISD::SETUNE:
    return {AltairX::BRCondCode::NE, false};
  case ISD::SETEQ:
    return {AltairX::BRCondCode::EQ, false};
  case ISD::SETGT:
    return {AltairX::BRCondCode::LT, true};
  case ISD::SETGE:
    return {AltairX::BRCondCode::GE, false};
  case ISD::SETLT:
    return {AltairX::BRCondCode::LT, false};
  case ISD::SETLE:
    return {AltairX::BRCondCode::GE, true};
  case ISD::SETNE:
    return {AltairX::BRCondCode::NE, false};
  default:
    llvm_unreachable("Unsupported ISD::CondCode");
    break;
  }
}

} // namespace

void AltairXBranchPatcher::runOnPseudoBRC(MachineBasicBlock &block,
                                          MachineInstr &inst) {
  const auto cc = static_cast<ISD::CondCode>(inst.getOperand(1).getImm());
  const auto [nativeCC, swapCMPOps] = analysePseudoBRC(cc);

  auto rend = block.rend().getInstrIterator();
  auto cmpIt = findNearestCmp(inst.getIterator().getReverse(), rend);
  assert(cmpIt != rend && "BRC without CMP");
  MachineInstr *cmpInst = to_address(cmpIt);
  if (swapCMPOps) {
    auto *newCmp = BuildMI(block, *cmpInst, cmpInst->getDebugLoc(),
                           instInfo->get(cmpInst->getOpcode()))
                       .addReg(cmpInst->getOperand(1).getReg())
                       .addReg(cmpInst->getOperand(0).getReg())
                       .getInstr();
    cmpInst->eraseFromParent();
    cmpInst = newCmp;
  }

  if (AltairXInstrInfo::isFCmp(*cmpInst)) {
    runOnFCmp(block, *cmpInst);
  } else {
    runOnCmp(block, *cmpInst);
  }

  auto *target = inst.getOperand(0).getMBB();
  const auto probability = branchInfo->getEdgeProbability(&block, target);
  // Set prediction bit if probability >= 50%
  const bool likely =
      probability.getNumerator() >= probability.getDenominator() / 2u;

  BuildMI(block, inst, inst.getDebugLoc(), instInfo->get(AltairX::BRC))
      .addMBB(target)
      .addImm(static_cast<int64_t>(nativeCC))
      .addImm(static_cast<int64_t>(likely));

  inst.eraseFromParent();
}

namespace {

uint32_t getCmpImmVersion(uint32_t opcode) {
  switch (opcode) {
  case AltairX::CmpRRb:
    return AltairX::CmpRIb;
  case AltairX::CmpRRw:
    return AltairX::CmpRIw;
  case AltairX::CmpRRd:
    return AltairX::CmpRId;
  case AltairX::CmpRRq:
    return AltairX::CmpRIq;
  default:
    llvm_unreachable("Expected CmpRR");
  }
}

} // namespace

void AltairXBranchPatcher::runOnCmp(MachineBasicBlock &block,
                                    MachineInstr &inst) {
  const auto reg = inst.getOperand(1).getReg();
  const auto killing = inst.getOperand(1).isKill();

  MachineOperand *operand = AltairXInstrInfo::getLatestRegDef(inst, reg);
  if (!operand) {
    return;
  }
  MachineInstr *definition = operand->getParent();

  // Check if definition is a MoveI
  switch (definition->getOpcode()) {
  case AltairX::MoveIb:
    [[fallthrough]];
  case AltairX::MoveIw:
    [[fallthrough]];
  case AltairX::MoveId:
    [[fallthrough]];
  case AltairX::MoveIq:
    break;
  default:
    return; // nothing to do
  }

  const auto imm = definition->getOperand(1).getImm();
  if (!isInt<32>(imm)) {
    return; // if imm does not fit imm size we will use a reg anyway...
  }

  const auto cmpRIOpcode = getCmpImmVersion(inst.getOpcode());
  BuildMI(block, inst, inst.getDebugLoc(), instInfo->get(cmpRIOpcode))
      .addReg(inst.getOperand(0).getReg())
      .addImm(imm);

  if (killing) {
    definition->eraseFromParent();
  }

  inst.eraseFromParent();
}


void AltairXBranchPatcher::runOnFCmp(MachineBasicBlock &block,
                                     MachineInstr &inst) {
  // TODO: magic with fmovei and fcmpi, currently unsupported!
}

} // namespace llvm
