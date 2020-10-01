//=== AltairX.h - Top-level interface for AltairX representation ----*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM AltairX backend.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_ALTAIRX_H
#define LLVM_LIB_TARGET_ALTAIRX_ALTAIRX_H

#include "MCTargetDesc/AltairXMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class FunctionPass;

// Declare functions to create passes here!

} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_ALTAIRX_H
