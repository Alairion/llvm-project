//== AltairXELFObjectWriter.cxx  ------------------------------------*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AltairXELFObjectWriter.h"
#include "AltairXMCAsmBackend.h"

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCFixup.h"

namespace llvm {

AltairXELFObjectWriter::AltairXELFObjectWriter(std::uint8_t OSABI)
    : MCELFObjectTargetWriter{true, OSABI, ELF::EM_AltairX, true} {}

std::uint32_t AltairXELFObjectWriter::getRelocType(MCContext &Ctx,
                                                   const MCValue &Target,
                                                   const MCFixup &Fixup,
                                                   bool IsPCRel) const {
  const auto kind = Fixup.getTargetKind();
  if(kind >= FirstLiteralRelocationKind) {
    return kind - FirstLiteralRelocationKind;
  }

  switch(kind)
  {
  case AltairX::fixup_altairx_pcrel_br_imm23:
    return ELF::R_ALTAIRX_PCRELBR23;
  case AltairX::fixup_altairx_call_imm24:
    return ELF::R_ALTAIRX_CALL24;
  case AltairX::fixup_altairx_moveix9lo:
    return ELF::R_ALTAIRX_MOVEIX9LO;
  case AltairX::fixup_altairx_moveix9hi24:
    return ELF::R_ALTAIRX_MOVEIX9HI24;
  case AltairX::fixup_altairx_moveix10lo:
    return ELF::R_ALTAIRX_MOVEIX10LO;
  case AltairX::fixup_altairx_moveix10hi24:
    return ELF::R_ALTAIRX_MOVEIX10HI24;
  default:
    return ELF::R_ALTAIRX_NONE;
  }
}

} // namespace llvm