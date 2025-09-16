//===------ SemaAltairX.cpp -------- AltairX target-specific routines -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis functions specific to AltairX.
//
//===----------------------------------------------------------------------===//

#include "clang/Sema/SemaAltairX.h"
#include "clang/Basic/TargetBuiltins.h"
#include "clang/Sema/Sema.h"

namespace clang {

SemaAltairX::SemaAltairX(Sema &S) : SemaBase(S) {}

bool SemaAltairX::CheckAltairXBuiltinFunctionCall(const TargetInfo &TI,
                                                  unsigned BuiltinID,
                                                  CallExpr *TheCall) {
  switch (BuiltinID) {
  default:
    return false;
  // Basic intrinsics.
  case AltairX::BI__builtin_altairx_syscall0:
    [[fallthrough]];
  case AltairX::BI__builtin_altairx_syscall1:
    [[fallthrough]];
  case AltairX::BI__builtin_altairx_syscall2:
    [[fallthrough]];
  case AltairX::BI__builtin_altairx_syscall3:
    [[fallthrough]];
  case AltairX::BI__builtin_altairx_syscall4:
    [[fallthrough]];
  case AltairX::BI__builtin_altairx_syscall5:
    [[fallthrough]];
  case AltairX::BI__builtin_altairx_syscall6:
    [[fallthrough]];
  case AltairX::BI__builtin_altairx_syscall7:
    return SemaRef.BuiltinConstantArgRange(TheCall, 0, 0, 32767);
  }
}

} // namespace clang
