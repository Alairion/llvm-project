//===--- AltairX.cpp - AltairX ToolChain Implementations ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AltairX.h"
#include "CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Options.h"
#include "llvm/Option/ArgList.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

AltairXToolChain::AltairXToolChain(const Driver &D, const llvm::Triple &Triple,
                               const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  // ProgramPaths are found via 'PATH' environment variable.
}

bool AltairXToolChain::isPICDefault() const { return true; }

bool AltairXToolChain::isPIEDefault(const llvm::opt::ArgList &Args) const {
  return false;
}

bool AltairXToolChain::isPICDefaultForced() const { return true; }

bool AltairXToolChain::SupportsProfiling() const { return false; }

bool AltairXToolChain::hasBlocksRuntime() const { return false; }
