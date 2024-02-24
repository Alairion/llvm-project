//=== AltairXISelLowering.h - AltairX DAG Lowering Interface --------*- C++
//-*-===//
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

  RET, // AltairXRET
  CALL, // AltairXCALL
  JUMP, // AltairXJUMP
  BRCOND, // AltairXBRCOND
  SCMP, // AltairXSCMP
  CMOVE, // AltairXCMOVE
  CMOVETMP, // AltairXCMOVE
  GAWRAPPER, // AltairXGAWRAPPER
};
}

class AltairXSubtarget;

class AltairXTargetLowering : public TargetLowering {
public:
  explicit AltairXTargetLowering(const TargetMachine &TM,
                                 const AltairXSubtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;

protected:
  // Subtarget Info
  const AltairXSubtarget &Subtarget;

private:
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerReturnAddr(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG& DAG) const;
  SDValue LowerBRCOND(SDValue Op, SelectionDAG& DAG) const;

  using RegsToPassVector = SmallVector<std::pair<unsigned int, SDValue>, 8>;

  SDValue getGlobalAddressWrapper(SDValue GA, const GlobalValue *GV,
                                  SelectionDAG &DAG) const;

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
                      LLVMContext &Context) const override;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  void HandleByVal(CCState *State, unsigned int &Size, Align Align) const override;

  SDValue LowerMemOpCallTo(SDValue Chain, SDValue Arg, const SDLoc &dl,
                           SelectionDAG &DAG, const CCValAssign &VA,
                           ISD::ArgFlagsTy Flags) const;
};
} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_ISELLOWERING_H
