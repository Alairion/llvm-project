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

using namespace llvm::opt;

namespace clang {
namespace driver {

namespace tools {
namespace altairx {

void Linker::ConstructJob(Compilation &C, const JobAction &JA,
                          const InputInfo &Output, const InputInfoList &Inputs,
                          const ArgList &Args,
                          const char *LinkingOutput) const {
  bool isLLD{};
  const char *exec = Args.MakeArgString(getToolChain().GetLinkerPath(&isLLD));

  llvm::opt::ArgStringList InputFileList;
  for(const auto& II : Inputs) {
    if(!II.isFilename()) {
      // This is a linker input argument.
      // We cannot mix input arguments and file names in a -filelist input, thus
      // we prematurely stop our list (remaining files shall be passed as
      // arguments).
      if(InputFileList.size() > 0)
        break;

      continue;
    }

    InputFileList.push_back(II.getFilename());
  }

  ArgStringList CmdArgs;
  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  Args.AddAllArgs(CmdArgs, options::OPT_L);
  AddLinkerInputs(getToolChain(), Inputs, Args, CmdArgs, JA);

  auto cmd = std::make_unique<Command>(
    JA, *this, ResponseFileSupport::AtFileUTF8(), exec, CmdArgs, Inputs, Output);
  cmd->setInputFileList(std::move(InputFileList));
  C.addCommand(std::move(cmd));
}

}
}

namespace toolchains {

AltairXToolChain::AltairXToolChain(const Driver &D, const llvm::Triple &Triple,
                                   const ArgList &Args)
    : ToolChain(D, Triple, Args) {}

bool AltairXToolChain::isPICDefault() const { return false; }

bool AltairXToolChain::isPIEDefault(const llvm::opt::ArgList &Args) const {
  return false;
}

bool AltairXToolChain::isPICDefaultForced() const { return false; }

Tool *AltairXToolChain::buildLinker() const {
  return new tools::altairx::Linker(*this);
}

} // namespace toolchains
} // namespace driver
} // namespace clang