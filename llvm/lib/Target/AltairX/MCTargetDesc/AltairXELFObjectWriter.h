//== AltairXELFObjectWriter.h  ------------------------------------*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_ELFOBJECTWRITER_ALTAIRXELFOBJECTWRITER_H
#define LLVM_LIB_TARGET_ALTAIRX_ELFOBJECTWRITER_ALTAIRXELFOBJECTWRITER_H

#include "llvm/MC/MCELFObjectWriter.h"

namespace llvm {

class AltairXELFObjectWriter : public MCELFObjectTargetWriter {
public:
  AltairXELFObjectWriter(std::uint8_t OSABI);

protected:
  std::uint32_t getRelocType(MCContext &Ctx, const MCValue &Target,
                             const MCFixup &Fixup, bool IsPCRel) const override;
};

}

#endif
