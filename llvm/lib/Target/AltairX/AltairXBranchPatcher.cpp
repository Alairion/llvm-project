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

struct BRCOperands {
  AltairX::BRCondCode cc{};
  const MachineOperand& left;
  const MachineOperand& right;
};

BRCOperands analysePseudoBRC(ISD::CondCode value, const MachineOperand& left, const MachineOperand& right) {
  switch (value) {
  case ISD::SETOEQ:
    return {AltairX::BRCondCode::EQ, left, right};
  case ISD::SETOGT:
    return {AltairX::BRCondCode::LT, right, left};
  case ISD::SETOGE:
    return {AltairX::BRCondCode::GE, left, right};
  case ISD::SETOLT:
    return {AltairX::BRCondCode::LT, left, right};
  case ISD::SETOLE:
    return {AltairX::BRCondCode::GE, right, left};
  case ISD::SETONE:
    return {AltairX::BRCondCode::NE, left, right};
  case ISD::SETO:
    llvm_unreachable("ISD::SETO must be lowered before!");
  case ISD::SETUO:
    llvm_unreachable("ISD::SETUO must be lowered before!");
  case ISD::SETUEQ:
    return {AltairX::BRCondCode::EQU, left, right};
  case ISD::SETUGT:
    return {AltairX::BRCondCode::LTU, right, left};
  case ISD::SETUGE:
    return {AltairX::BRCondCode::GEU, left, right};
  case ISD::SETULT:
    return {AltairX::BRCondCode::LTU, left, right};
  case ISD::SETULE:
    return {AltairX::BRCondCode::GEU, right, left};
  case ISD::SETUNE:
    return {AltairX::BRCondCode::NEU, left, right};
  case ISD::SETEQ:
    return {AltairX::BRCondCode::EQ, left, right};
  case ISD::SETGT:
    return {AltairX::BRCondCode::LT, right, left};
  case ISD::SETGE:
    return {AltairX::BRCondCode::GE, left, right};
  case ISD::SETLT:
    return {AltairX::BRCondCode::LT, left, right};
  case ISD::SETLE:
    return {AltairX::BRCondCode::GE, right, left};
  case ISD::SETNE:
    return {AltairX::BRCondCode::NE, left, right};
  default:
    llvm_unreachable("Unsupported ISD::CondCode");
    break;
  }
}

uint32_t getCmpOpcodeForBRC(const MachineInstr& brc)
{
  switch(brc.getOpcode())
  {
  case AltairX::BRCb:
    return AltairX::CmpRRb;
  case AltairX::BRCw:
    return AltairX::CmpRRw;
  case AltairX::BRCd:
    return AltairX::CmpRRd;
  case AltairX::BRCq:
    return AltairX::CmpRRq;
  case AltairX::FBRCs:
    return AltairX::FCmpRRs;
  case AltairX::FBRCd:
    return AltairX::FCmpRRd;
  default:
    llvm_unreachable("Must be a BRC!");
  }
}

} // namespace

void AltairXBranchPatcher::runOnPseudoBRC(MachineBasicBlock &block,
                                          MachineInstr &inst) {
  // if right operand is already an imm, throw error
  if(inst.getOperand(3).isImm()) {
    llvm_unreachable("Please use ConstantToReg + BRC in codegen!");
    return;
  }

  const DebugLoc dl{inst.getDebugLoc()};
  const auto cc = static_cast<ISD::CondCode>(inst.getOperand(1).getImm());
  const auto&& [nativeCC, left, right] =
      analysePseudoBRC(cc, inst.getOperand(2), inst.getOperand(3));
  const uint32_t cmpOpcode = getCmpOpcodeForBRC(inst);

  // Creates a reg-reg cmp by default, runOn[F]Cmp will optimize it if possible
  MachineInstr *cmp = BuildMI(block, inst, dl, instInfo->get(cmpOpcode))
                          .add(left)
                          .add(right)
                          .getInstr();

  if (AltairXInstrInfo::isFCmp(*cmp)) {
    runOnFCmp(block, *cmp);
  } else {
    runOnCmp(block, *cmp);
  }

  auto *target = inst.getOperand(0).getMBB();
  const auto probability = branchInfo->getEdgeProbability(&block, target);
  // Set prediction bit if probability >= 50%
  const bool likely =
      probability.getNumerator() >= probability.getDenominator() / 2u;

  BuildMI(block, inst, dl, instInfo->get(AltairX::BRC))
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
  auto& right = inst.getOperand(1);
  const auto reg = right.getReg();
  const auto killing = right.isKill();

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
    return; // if imm does not fit imm size we will use a reg
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
  // AXIMPR: use fmovei and fcmpi
}

} // namespace llvm
