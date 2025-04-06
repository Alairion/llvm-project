//===--- AltairX.h - Declare AltairX target feature support ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares AltairXTargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_ALTAIRX_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_ALTAIRX_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY AltairXTargetInfo : public TargetInfo {
  static const char *const GCCRegNames[];

public:
  AltairXTargetInfo(const llvm::Triple &Triple, const TargetOptions &)
    : TargetInfo(Triple) {
    // Description string has to be kept in sync with backend string at
    // llvm/lib/Target/AltairX/AltairXTargetMachine.cpp
    resetDataLayout(
      "e" // Little endian
      "-m:e" // ELF name mangling
      "-p:64:64:64:64" // 64-bit pointers, 64-bit aligned
      "-i64:64" // 64-bit integers, 64 bit aligned
      "-n8:16:32:64" // 8, 16, 32 and 64 bits native integers
      "-S64" // 64-bit natural stack alignment
    );

    SuitableAlign = 128; // Minimum valid align for any type

    PointerWidth = 64;
    PointerAlign = 64;
    BoolWidth = 8; 
    BoolAlign = 8;
    IntWidth = 32;
    IntAlign = 32;
    LongWidth = 64;
    LongAlign = 64;
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  ArrayRef<const char *> getGCCRegNames() const override;

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  ArrayRef<Builtin::Info> getTargetBuiltins() const override { return {}; }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &info) const override {
    return false;
  }

  std::string_view getClobbers() const override {
    return "";
  }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_ALTAIRX_H
