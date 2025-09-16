//===----- SemaAltairX.h ------ AltairX target-specific routines ----*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file declares semantic analysis functions specific to AltairX.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_SEMA_SEMAALTAIRX_H
#define LLVM_CLANG_SEMA_SEMAALTAIRX_H

#include "clang/AST/ASTFwd.h"
#include "clang/Sema/SemaBase.h"

namespace clang {

class ParsedAttr;

class SemaAltairX : public SemaBase {
public:
  SemaAltairX(Sema &S);
  bool CheckAltairXBuiltinFunctionCall(const TargetInfo &TI, unsigned BuiltinID,
                                       CallExpr *TheCall);
};

} // namespace clang

#endif // LLVM_CLANG_SEMA_SEMAALTAIRX_H
