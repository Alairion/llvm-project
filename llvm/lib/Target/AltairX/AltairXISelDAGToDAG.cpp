//===-- AltairXISelDAGToDAG.cpp - A Dag to Dag Inst Selector for AltairX
//------===//
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

bool AltairXDAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const AltairXSubtarget &>(MF.getSubtarget());
  return SelectionDAGISel::runOnMachineFunction(MF);
}

bool AltairXDAGToDAGISel::doesImplicitTruncate(unsigned Opcode) const noexcept {
  return Opcode != ISD::TRUNCATE && Opcode != TargetOpcode::EXTRACT_SUBREG &&
         Opcode != ISD::CopyFromReg && Opcode != ISD::AssertSext &&
         Opcode != ISD::AssertZext && Opcode != ISD::AssertAlign &&
         Opcode != ISD::FREEZE;
}

bool AltairXDAGToDAGISel::outputsInMDUReg(unsigned Opcode) const noexcept {
  return Opcode == ISD::MUL || Opcode == ISD::SDIV || Opcode == ISD::UDIV ||
         Opcode == ISD::SREM || Opcode == ISD::UREM ||
         Opcode == ISD::SMUL_LOHI || Opcode == ISD::UMUL_LOHI ||
         Opcode == ISD::SDIVREM || Opcode == ISD::UDIVREM;
}

namespace
{

std::optional<std::uint64_t> selectAddrRRShift(SDValue N) {
  if(N.getOpcode() != ISD::SHL) {
    return std::nullopt; // Not a shift
  }

  // can only shift by a constant <= 7
  auto* constant = dyn_cast<ConstantSDNode>(N.getOperand(1));
  if(!constant || !isUInt<3>(constant->getZExtValue())) {
    return std::nullopt;
  }

  return std::make_optional(constant->getZExtValue());
}

}

bool AltairXDAGToDAGISel::selectAddr(SDValue N, SDValue &Base,
                                     SDValue &Offset, SDValue &Shift) const {
  //if (N.getOpcode() == ISD::FrameIndex ||
  //  N.getOpcode() == ISD::TargetExternalSymbol ||
  //  N.getOpcode() == ISD::TargetGlobalAddress ||
  //  N.getOpcode() == ISD::TargetGlobalTLSAddress) {
  //  return false; // direct calls.
  //}
  
  // Check if this particular node is reused in any non-memory related
  // operation.  If yes, do not try to fold this node into the address
  // computation, since the computation will be kept.
  //const SDNode* Node = N.getNode();
  //for(SDNode* UI : Node->uses()) {
  //  if(!isa<MemSDNode>(*UI))
  //    return false;
  //}

  SDLoc DL{N};
  SDValue left = N.getOperand(0);
  SDValue right = N.getOperand(1);

  // Try to match a shift on left and right
  if(auto matchedShift = selectAddrRRShift(right); matchedShift) {
    Base = left;
    Offset = right.getOperand(0);
    Shift = CurDAG->getTargetConstant(*matchedShift, DL, MVT::i64);
    return true;
  }

  if(auto matchedShift = selectAddrRRShift(left); matchedShift) {
    Base = right;
    Offset = left.getOperand(0);
    Shift = CurDAG->getTargetConstant(*matchedShift, DL, MVT::i64);
    return true;
  }

  // Try to match Reg + Reg
  if(N.getOpcode() == ISD::ADD && !isa<ConstantSDNode>(right)) {
    Base = left;
    Offset = right;
    Shift = CurDAG->getTargetConstant(0, DL, MVT::i64);
    return true;
  }

  return false; // may be matched by selectAddrImm or selectAddrImmSP
}

bool AltairXDAGToDAGISel::selectAddrImm(SDValue N, SDValue &Base,
                                        SDValue &Offset) const {
  // Load at given address directly
  if (N.getOpcode() != ISD::ADD) {
    Base = N;
    Offset = CurDAG->getTargetConstant(0, SDLoc{N}, MVT::i64);
    return true;
  }

  auto left = N.getOperand(0);
  auto right = N.getOperand(1);

  if(auto* value = dyn_cast<ConstantSDNode>(right); value) {
    if(isInt<32>(value->getSExtValue())) {
      Base = left;
      Offset = right;
      return true;
    }
  }

  return false; // may be matched by selectAddrImmSP
}

bool AltairXDAGToDAGISel::selectAddrImmSP(SDValue N, SDValue &Base,
                                          SDValue &Offset) const {
  if (Base.getResNo() != AltairX::SPReg64) {
    return false;
  }

  // Load at given address directly
  if(N.getOpcode() != ISD::ADD) {
    Base = N;
    Offset = CurDAG->getTargetConstant(0, SDLoc{N}, MVT::i64);
    return true;
  }

  auto left = N.getOperand(0);
  auto right = N.getOperand(1);
  if(auto* value = dyn_cast<ConstantSDNode>(right); value) {
    if(isUInt<16>(value->getZExtValue())) {
      Base = left;
      Offset = right;
      return true;
    }
  }

  return false; // may be matched selectAddrImm
}

void AltairXDAGToDAGISel::Select(SDNode *Node) {

  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    LLVM_DEBUG(errs() << "== "; Node->dump(CurDAG); errs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  // Instruction Selection not handled by the auto-generated tablegen selection
  // should be handled here.
  //switch (Node->getOpcode()) {
  //default:
  //  break;
  //}

  // Select the default instruction
  SelectCode(Node);
}
