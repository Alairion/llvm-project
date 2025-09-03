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

  llvm::opt::ArgStringList inputFileList;
  for(const auto& inputFile : Inputs) {
    if(!inputFile.isFilename()) {
      // This is a linker input argument.
      // We cannot mix input arguments and file names in a -filelist input, thus
      // we prematurely stop our list (remaining files shall be passed as
      // arguments).
      if(inputFileList.size() > 0) {
        break;
      }

      continue;
    }

    inputFileList.push_back(inputFile.getFilename());
  }

  auto& toolchain = getToolChain();
  ArgStringList cmdArgs;
  cmdArgs.push_back("-Bstatic"); // altairx is only a VM for now, only full executables are supported
  toolchain.AddFilePathLibArgs(Args, cmdArgs);

  if(!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    if(toolchain.ShouldLinkCXXStdlib(Args)) {
      toolchain.AddCXXStdlibLibArgs(Args, cmdArgs);
    }

    if(Args.hasArg(options::OPT_pthread)) {
      cmdArgs.push_back("-lpthread");
      cmdArgs.push_back("--shared-memory");
    }

    cmdArgs.push_back("-lc");
    //cmdArgs.push_back("-lm");
    //AddRunTimeLibs(toolchain, toolchain.getDriver(), cmdArgs, Args);
  }

  cmdArgs.push_back("-o");
  cmdArgs.push_back(Output.getFilename());

  Args.AddAllArgs(cmdArgs, options::OPT_L);
  AddLinkerInputs(toolchain, Inputs, Args, cmdArgs, JA);

  auto cmd =
      std::make_unique<Command>(JA, *this, ResponseFileSupport::AtFileUTF8(),
                                exec, cmdArgs, Inputs, Output);
  cmd->setInputFileList(std::move(inputFileList));
  C.addCommand(std::move(cmd));
}

}
}

namespace toolchains {

AltairXToolChain::AltairXToolChain(const Driver &D, const llvm::Triple &Triple,
                                   const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  getProgramPaths().push_back(getDriver().Dir);

  auto SysRoot = getDriver().SysRoot;
  getFilePaths().push_back(SysRoot + "/lib");
  getLibraryPaths().push_back(SysRoot + "/lib");
}

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