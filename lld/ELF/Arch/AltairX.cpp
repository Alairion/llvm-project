//===- AltairX.cpp --------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "OutputSections.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/BinaryFormat/ELF.h"

#include <iostream>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {

class AltairX final : public TargetInfo {
public:
  AltairX(Ctx &ctx);
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const std::uint8_t *loc) const override;
  void relocate(std::uint8_t *loc, const Relocation &rel,
                std::uint64_t val) const override;
  void writeBits(std::uint8_t *loc, std::uint32_t locOff, std::uint64_t val,
                 std::uint32_t valOff, std::uint32_t size) const;
};
} // namespace

AltairX::AltairX(Ctx &ctx) : TargetInfo{ctx} {

}

RelExpr AltairX::getRelExpr(RelType type, const Symbol &s,
                            const uint8_t *loc) const {
  switch (type) {
  case R_ALTAIRX_NONE:
    return R_NONE;
  case R_ALTAIRX_PCREL23LO:
    return R_GOTREL;
  case R_ALTAIRX_PCREL23HI:
    return R_GOTREL;
  case R_ALTAIRX_PCREL24LO:
    return R_GOTREL;
  case R_ALTAIRX_PCREL24HI:
    return R_GOTREL;
  case R_ALTAIRX_ABS24LO:
    return R_ABS;
  case R_ALTAIRX_ABS24HI:
    return R_ABS;
  case R_ALTAIRX_MOVEIX9LO:
    return R_ABS;
  case R_ALTAIRX_MOVEIX9HI:
    return R_ABS;
  case R_ALTAIRX_MOVEIX10LO:
    return R_ABS;
  case R_ALTAIRX_MOVEIX10HI:
    return R_ABS;
  case R_ALTAIRX_MOVEIX18LO:
    return R_ABS;
  case R_ALTAIRX_MOVEIX18HI:
    return R_ABS;
  default:
    error(getErrorLoc(ctx, loc) + "unknown relocation (" + Twine(type) +
          ") against symbol " + toStr(ctx, s));
    return R_NONE;
  }
}

void AltairX::writeBits(std::uint8_t *loc, std::uint32_t locOff, std::uint64_t val,
               std::uint32_t valOff, std::uint32_t size) const {
  const std::uint32_t tmp = read32(ctx, loc);
  const auto sizeMask = ((1ull << size) - 1ull);
  const auto patch =
      static_cast<std::uint32_t>(((val >> valOff) & sizeMask) << locOff);
  write32(ctx, loc, tmp | patch); // assume zeroed bits at mask location
}

void AltairX::relocate(std::uint8_t *loc, const Relocation &rel,
                       std::uint64_t val) const {
  switch (rel.type) {
  case R_ALTAIRX_NONE:
    break;
  case R_ALTAIRX_PCREL23LO:
    writeBits(loc, 9, val, 0, 24);
    break;
  case R_ALTAIRX_PCREL23HI:
    writeBits(loc, 8, val, 23, 24);
    break;
  case R_ALTAIRX_PCREL24LO:
    writeBits(loc, 8, val, 0, 24);
    break;
  case R_ALTAIRX_PCREL24HI:
    writeBits(loc, 8, val, 24, 24);
    break;
  case R_ALTAIRX_ABS24LO:
    writeBits(loc, 8, val, 0, 24);
    break;
  case R_ALTAIRX_ABS24HI:
    writeBits(loc, 8, val, 24, 24);
    break;
  case R_ALTAIRX_MOVEIX9LO:
    writeBits(loc, 11, val, 0, 9);
    break;
  case R_ALTAIRX_MOVEIX9HI:
    writeBits(loc, 8, val, 9, 24);
    break;
  case R_ALTAIRX_MOVEIX10LO:
    writeBits(loc, 10, val, 0, 10);
    break;
  case R_ALTAIRX_MOVEIX10HI:
    writeBits(loc, 8, val, 10, 24);
    break;
  case R_ALTAIRX_MOVEIX18LO:
    writeBits(loc, 8, val, 0, 18);
    break;
  case R_ALTAIRX_MOVEIX18HI:
    writeBits(loc, 8, val, 18, 24);
    break;
  default:
    llvm_unreachable("unknown relocation");
  }
}

void elf::setAltairXTargetInfo(Ctx &ctx) {
  ctx.target.reset(new AltairX{ctx});
}
