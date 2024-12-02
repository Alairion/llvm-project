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
#include "AltairXSubtarget.h"
#include "AltairXCommon.h"

#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"

namespace llvm
{

char AltairXBranchPatcher::ID = 0;

INITIALIZE_PASS_BEGIN(AltairXBranchPatcher, "altairx-branch-patcher",
                      "Replace PseudoBRC with BRC and fix related operations",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(MachineBranchProbabilityInfo)
INITIALIZE_PASS_END(AltairXBranchPatcher, "altairx-branch-patcher",
                    "Replace PseudoBRC with BRC and fix related operations",
                    false, false)

FunctionPass *llvm::createAltairXBranchPatcherPass() {
  return new AltairXBranchPatcher();
}

AltairXBranchPatcher::AltairXBranchPatcher() : MachineFunctionPass(ID) {
  initializeAltairXBranchPatcherPass(*PassRegistry::getPassRegistry());
}

void AltairXBranchPatcher::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MachineBranchProbabilityInfo>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

bool AltairXBranchPatcher::runOnMachineFunction(MachineFunction &F) {
  TM = &F.getTarget(); 
  TII = F.getSubtarget<AltairXSubtarget>().getInstrInfo();
  MBPI = &getAnalysis<MachineBranchProbabilityInfo>();

  for (auto &MBB : F) {
    runOnMachineBasicBlock(MBB);
  }

  return false;
}

void AltairXBranchPatcher::runOnMachineBasicBlock(MachineBasicBlock &MBB) {
  auto last = MBB.getLastNonDebugInstr();
  if(last == MBB.end()) {
    return; // empty block (always fallthrough)
  }

  if (!AltairXInstrInfo::isUncondBranchOpcode(*last) &&
      !AltairXInstrInfo::isCondBranchOpcode(*last)) {
    return; // no terminator (always fallthrough)
  }

  if (AltairXInstrInfo::isUncondBranchOpcode(*last) &&
      last == MBB.getFirstNonDebugInstr()) {
    return; // single unconditional branch
  }

  const auto secondLast = std::prev(last);
  if (AltairXInstrInfo::isCondBranchOpcode(*last)) {
    runOnPseudoBRC(MBB, *last);
  } else if (AltairXInstrInfo::isCondBranchOpcode(*secondLast)) {
    runOnPseudoBRC(MBB, *secondLast);
  }
}

namespace {

template <typename It> // It::value_type compatible with const machineInstr&
auto findNearestCmp(It begin, It end) {
  return std::find_if(begin, end, [](auto &instr) {
    return AltairXInstrInfo::isCompare(instr);
  });
}

struct BRCOperands {
  // for brc
  AltairX::BRCondCode cc{};
  // for cmp
  bool swapCMPOperands{};
};

BRCOperands analysePseudoBRC(ISD::CondCode value) {
  switch (value) {
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

void AltairXBranchPatcher::runOnPseudoBRC(MachineBasicBlock &MBB,
                                          MachineInstr &MI) {
  const auto cc = static_cast<ISD::CondCode>(MI.getOperand(1).getImm());
  const auto [nativeCC, swapCMPOps] = analysePseudoBRC(cc);

  auto rend = MBB.rend().getInstrIterator();
  auto cmpIt = findNearestCmp(MI.getIterator().getReverse(), rend);
  assert(cmpIt != rend && "BRC without CMP");
  MachineInstr *cmpInst = to_address(cmpIt);
  if (swapCMPOps) {
    const auto opcode = TII->get(cmpInst->getOpcode());
    auto *newCmp = BuildMI(MBB, *cmpInst, cmpInst->getDebugLoc(), opcode)
                       .addReg(cmpInst->getOperand(1).getReg())
                       .addReg(cmpInst->getOperand(0).getReg())
                       .getInstr();
    cmpInst->removeFromParent();
    cmpInst = newCmp;
  }

  runOnCMP(MBB, *cmpInst);

  auto* target = MI.getOperand(0).getMBB();
  const auto probability = MBPI->getEdgeProbability(&MBB, target);
  // Set prediction bit if probability >= 50%
  const bool likely = probability.getNumerator() >= probability.getDenominator() / 2u;

  BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(AltairX::BRC))
    .addMBB(target)
    .addImm(static_cast<int64_t>(nativeCC))
    .addImm(static_cast<int64_t>(likely));

  MI.removeFromParent();
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

}

void AltairXBranchPatcher::runOnCMP(MachineBasicBlock &MBB, MachineInstr &MI) {
  const auto reg = MI.getOperand(1).getReg();
  const auto killing = MI.getOperand(1).isKill();

  MachineOperand* operand = AltairXInstrInfo::getLatestRegDef(MI, reg);
  if(!operand) {
    return;
  }
  MachineInstr* definition = operand->getParent();

  // Check if definition is a MoveI
  switch(definition->getOpcode()) {
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

  const auto cmpRIOpcode = getCmpImmVersion(MI.getOpcode());
  BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(cmpRIOpcode))
      .addReg(MI.getOperand(0).getReg())
      .addImm(imm);

  if(killing) {
    definition->removeFromParent();
  }
  MI.removeFromParent();
}

} // namespace llvm
