//=== AltairXISelLowering.h - AltairX DAG Lowering Interface ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that AltairX uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_ISELLOWERING_H
#define LLVM_LIB_TARGET_ALTAIRX_ISELLOWERING_H

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/IR/Function.h"

namespace llvm {
namespace AltairXISD {
enum NodeType {
  // Start the numbering from where ISD NodeType finishes.
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  CONSTANTTOREG,
  RET,
  CALL,
  JUMP,
  INDIRECT_CALL,
  INDIRECT_JUMP,
  INDIRECT_BRA,
  CMP,
  FCMP,
  BRCOND,
  SCMP,
  SBIT,
  CMOVE,
  FSCMP,
  FCMOVE,
  GAWRAPPER,
  ITOF,
  FTOI,
};
}

class AltairXSubtarget;

class AltairXTargetLowering : public TargetLowering {
public:
  explicit AltairXTargetLowering(const TargetMachine &TM,
                                 const AltairXSubtarget &STI);

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  const char *getTargetNodeName(unsigned Opcode) const override;

protected:
  // Subtarget Info
  const AltairXSubtarget &Subtarget;

private:
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
                      SelectionDAG &DAG) const override;

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context, const Type* RetTy) const override;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  void HandleByVal(CCState *State, unsigned int &Size,
                   Align Align) const override;

  EVT getSetCCResultType(const DataLayout &, LLVMContext &,
                         EVT VT) const override;

  // Float handling:
  SDValue LowerFP_TO_SINT(SDValue Op, SelectionDAG& DAG) const;
  SDValue LowerFP_TO_UINT(SDValue Op, SelectionDAG& DAG) const;
  SDValue LowerSINT_TO_FP(SDValue Op, SelectionDAG& DAG) const;
  SDValue LowerUINT_TO_FP(SDValue Op, SelectionDAG& DAG) const;

  // Address mode related:
  template <typename NodeT>
  SDValue getGlobalAddressWrapper(SelectionDAG &DAG, const NodeT *node) const;

  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerJumpTable(SDValue Op, SelectionDAG& DAG) const;

  // Conditions and branches
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT(SDValue Op, SelectionDAG& DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBRIND(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;

  // Others
  SDValue LowerVASTART(SDValue Op, SelectionDAG& DAG) const;
};
} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_ISELLOWERING_H
