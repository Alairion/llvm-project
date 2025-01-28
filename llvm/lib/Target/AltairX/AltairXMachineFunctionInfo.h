//=== AltairXMachineFunctionInfo.h - Private data used for AltairX *- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the AltairX specific subclass of MachineFunctionInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_MACHINE_FUNCTION_INFO_H
#define LLVM_LIB_TARGET_ALTAIRX_MACHINE_FUNCTION_INFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class AltairXMachineFunctionInfo : public MachineFunctionInfo {
public:
  AltairXMachineFunctionInfo(const Function &, const TargetSubtargetInfo *){};

  // Used for vastart lowering
  int VarArgsFrameIndex = 0;
};

} // end of namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_MACHINEFUNCTION_H
