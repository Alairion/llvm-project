//===---- AltairXISelDAGToDAG.h - A Dag to Dag Inst Selector for AltairX
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

#ifndef LLVM_LIB_TARGET_ALTAIRX_ISELDAGTODAG_H
#define LLVM_LIB_TARGET_ALTAIRX_ISELDAGTODAG_H

#include "AltairXSubtarget.h"
#include "AltairXTargetMachine.h"
#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {
class AltairXDAGToDAGISel : public SelectionDAGISel {
public:
  explicit AltairXDAGToDAGISel(char &ID, AltairXTargetMachine &TM,
                               CodeGenOpt::Level OL)
      : SelectionDAGISel(ID, TM, OL), Subtarget(nullptr) {}

  // Pass Name
  StringRef getPassName() const override {
    return "AltairX DAG->DAG Pattern Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  bool doesImplicitTruncate(unsigned Opcode) const noexcept;
  bool selectAddr(SDValue N, SDValue &Base, SDValue &Offset, SDValue &Shift) const;
  bool selectAddrImm(SDValue N, SDValue& Base, SDValue& Offset) const;
  bool selectAddrImmSP(SDValue N, SDValue& Base, SDValue& Offset) const;

  void Select(SDNode *Node) override;

#include "AltairXGenDAGISel.inc"

private:
  const AltairXSubtarget *Subtarget;
};
} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_ISELDAGTODAG_H
