//===-- AltairXISelDAGToDAG.cpp - A Dag to Dag Inst Selector for AltairX --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the AltairX target.
//
//===----------------------------------------------------------------------===//

#include "AltairXISelDAGToDAG.h"
#include "AltairXSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-isel"
#define PASS_NAME "AltairX Instruction Selection"

bool AltairXDAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const AltairXSubtarget &>(MF.getSubtarget());
  return SelectionDAGISel::runOnMachineFunction(MF);
}

bool AltairXDAGToDAGISel::doesImplicitTruncate(unsigned opcode) const noexcept {
  return opcode != ISD::TRUNCATE && opcode != TargetOpcode::EXTRACT_SUBREG &&
         opcode != ISD::CopyFromReg && opcode != ISD::AssertSext &&
         opcode != ISD::AssertZext && opcode != ISD::AssertAlign &&
         opcode != ISD::FREEZE;
}

bool AltairXDAGToDAGISel::outputsInMDUReg(unsigned opcode) const noexcept {
  return opcode == ISD::MUL || opcode == ISD::SDIV || opcode == ISD::UDIV ||
         opcode == ISD::SREM || opcode == ISD::UREM ||
         opcode == ISD::SMUL_LOHI || opcode == ISD::UMUL_LOHI ||
         opcode == ISD::SDIVREM || opcode == ISD::UDIVREM;
}

namespace {

std::optional<std::uint64_t> selectAddrRRShift(SDValue N) {
  if (N.getOpcode() != ISD::SHL) {
    return std::nullopt; // Not a shift
  }

  // can only shift by a constant <= 7
  auto *constant = dyn_cast<ConstantSDNode>(N.getOperand(1));
  if (!constant || !isUInt<3>(constant->getZExtValue())) {
    return std::nullopt;
  }

  return std::make_optional(constant->getZExtValue());
}

} // namespace

bool AltairXDAGToDAGISel::selectAddr(SDValue N, SDValue &Base, SDValue &Offset,
                                     SDValue &Shift) const {

  // Check if this particular node is reused in any non-memory related
  // operation.  If yes, do not try to fold this node into the address
  // computation, since the computation will be kept.
  // const SDNode* Node = N.getNode();
  // for(SDNode* UI : Node->uses()) {
  //  if(!isa<MemSDNode>(*UI))
  //    return false;
  //}

  SDLoc dl{N};

  if (N.getOpcode() != ISD::ADD) {
    return false;
  }

  SDValue left = N.getOperand(0);
  SDValue right = N.getOperand(1);

  // Try to match a shift on left and right
  if (auto matchedShift = selectAddrRRShift(right); matchedShift) {
    Base = left;
    Offset = right.getOperand(0);
    Shift = CurDAG->getTargetConstant(*matchedShift, dl, MVT::i64);
    return true;
  }

  if (auto matchedShift = selectAddrRRShift(left); matchedShift) {
    Base = right;
    Offset = left.getOperand(0);
    Shift = CurDAG->getTargetConstant(*matchedShift, dl, MVT::i64);
    return true;
  }

  // Try to match Reg + Reg
  if (!isa<ConstantSDNode>(right)) {
    Base = left;
    Offset = right;
    Shift = CurDAG->getTargetConstant(0, dl, MVT::i64);
    return true;
  }

  return false; // may be matched by selectAddrImm or selectAddrImmSP
}

bool AltairXDAGToDAGISel::selectAddrImm(SDValue N, SDValue &Base,
                                        SDValue &Offset) const {
  // Load at given address directly
  SDLoc dl{N};

  if (N.getOpcode() == ISD::FrameIndex) {
    auto *node = cast<FrameIndexSDNode>(N);
    Base = CurDAG->getTargetFrameIndex(node->getIndex(), MVT::i64);
    Offset = CurDAG->getTargetConstant(0, dl, MVT::i64);
    return true;
  }

  if (N.getOpcode() == AltairXISD::GAWRAPPER) {
    Base = CurDAG->getRegister(AltairX::ZERO, MVT::i64);
    Offset = N.getOperand(0);
    return true;
  }

  if (N.getOpcode() != ISD::ADD) {
    Base = N;
    Offset = CurDAG->getTargetConstant(0, dl, MVT::i64);
    return true;
  }

  auto left = N.getOperand(0);
  auto right = N.getOperand(1);

  if (auto *value = dyn_cast<ConstantSDNode>(right); value) {
    const auto constval = value->getSExtValue();
    if (isInt<32>(constval)) {
      Base = left;
      Offset = CurDAG->getTargetConstant(constval, dl, MVT::i64);
      return true;
    }
  }

  if (auto *value = dyn_cast<ConstantSDNode>(left); value) {
    const auto constval = value->getSExtValue();
    if (isInt<32>(constval)) {
      Base = right;
      Offset = CurDAG->getTargetConstant(constval, dl, MVT::i64);
      return true;
    }
  }

  return false;
}

void AltairXDAGToDAGISel::Select(SDNode *Node) {

  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    LLVM_DEBUG(errs() << "== "; Node->dump(CurDAG); errs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  SDLoc dl{Node};

  // Instruction Selection not handled by the auto-generated tablegen selection
  // should be handled here.
  switch (Node->getOpcode()) {
  case ISD::FrameIndex: {
    // Will later become add rX, r0, imm
    SDValue imm = CurDAG->getTargetConstant(0, dl, MVT::i64);
    SDValue fi = CurDAG->getTargetFrameIndex(
        cast<FrameIndexSDNode>(Node)->getIndex(), MVT::i64);
    ReplaceNode(Node,
                CurDAG->getMachineNode(AltairX::AddRIq, dl, MVT::i64, fi, imm));
    return;
  }
  default:
    break;
  }

  // Select the default instruction
  SelectCode(Node);
}

namespace
{

class AltairXDAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;
  explicit AltairXDAGToDAGISelLegacy(AltairXTargetMachine& tm,
    CodeGenOptLevel OptLevel)
    : SelectionDAGISelLegacy(
      ID, std::make_unique<AltairXDAGToDAGISel>(tm, OptLevel))
  {
  }
};

} // end anonymous namespace

char AltairXDAGToDAGISelLegacy::ID = 0;

INITIALIZE_PASS(AltairXDAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAltairXISelDag(AltairXTargetMachine &TM,
                                         CodeGenOptLevel OptLevel) {
  return new AltairXDAGToDAGISelLegacy(TM, OptLevel);
}

