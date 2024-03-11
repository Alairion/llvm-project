//===-- AltairXMCAsmBackend.cpp - AltairX MC Asm Backend -------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the AltairXMCAsmBackend class.
//
//===----------------------------------------------------------------------===//

#include "AltairXMCAsmBackend.h"

#include "AltairXELFObjectWriter.h"

#include "llvm/ADT/StringSwitch.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

#include <cassert>

namespace llvm {

std::unique_ptr<MCObjectTargetWriter>
AltairXMCAsmBackend::createObjectTargetWriter() const {
  return std::make_unique<AltairXELFObjectWriter>(ELF::ELFOSABI_STANDALONE); // replace later ?
}

bool AltairXMCAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count,
                                       const MCSubtargetInfo*) const {
  OS.write_zeros(llvm::alignTo(Count, 4)); // No-Op is just moveix 0, so 0
  return true;
}

// Target Fixup Interfaces
unsigned AltairXMCAsmBackend::getNumFixupKinds() const {
  return AltairX::NumTargetFixupKinds;
}

std::optional<MCFixupKind>
AltairXMCAsmBackend::getFixupKind(StringRef Name) const {
  return StringSwitch<std::optional<MCFixupKind>>(Name)
#define ELF_RELOC(Name, Value)                                                 \
  .Case(#Name, MCFixupKind(FirstLiteralRelocationKind + Value))
#include "llvm/BinaryFormat/ELFRelocs/AltairX.def"
#undef ELF_RELOC
  .Default(std::nullopt);
}

const MCFixupKindInfo &
AltairXMCAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  static const auto pcrel = MCFixupKindInfo::FKF_IsPCRel;
  static const std::array<MCFixupKindInfo, AltairX::NumTargetFixupKinds> infos{
      // This table *must* be in the order that the fixup_* kinds are defined
      // Name, Offset (bits), Size (bits), Flags
      MCFixupKindInfo{"fixup_altairx_pcrel_br_imm23", 9, 23, pcrel},
      MCFixupKindInfo{"fixup_altairx_call_imm24", 8, 24, 0},
  };

  // Fixup kinds from .reloc directive do not require any extra processing.
  if (Kind >= FirstLiteralRelocationKind) {
    return MCAsmBackend::getFixupKindInfo(FK_NONE);
  }

  if (Kind < FirstTargetFixupKind) {
    return MCAsmBackend::getFixupKindInfo(Kind);
  }

  assert(static_cast<std::uint32_t>(Kind - FirstTargetFixupKind) <
             getNumFixupKinds() &&
         "Invalid kind!");

  return infos[Kind - FirstTargetFixupKind];
}

namespace {

std::uint64_t adjustFixupValue(const MCFixup &Fixup, const MCValue &Target,
                               std::uint64_t Value, MCContext &Ctx,
                               bool IsResolved) {
  const auto signedValue = static_cast<std::int64_t>(Value);

  switch(Fixup.getKind())
  {
  case AltairX::fixup_altairx_pcrel_br_imm23:
    if(signedValue % 4 != 0) {
      Ctx.reportError(Fixup.getLoc(), "fixup value bad alignement");
    }
    if(!isInt<25>(signedValue)) {
      Ctx.reportError(Fixup.getLoc(), "fixup value out of range");
    }
    return Value >> 2;
  default:
    llvm_unreachable("Unknown fixup kind!");
  }
}

}


void AltairXMCAsmBackend::applyFixup(const MCAssembler &Asm,
                                     const MCFixup &Fixup,
                                     const MCValue &Target,
                                     MutableArrayRef<char> Data,
                                     std::uint64_t Value, bool IsResolved,
                                     const MCSubtargetInfo *STI) const {
  if(!Value) {
    return; // Doesn't change encoding.
  }

  const auto kind = Fixup.getKind();
  if(kind >= FirstLiteralRelocationKind) {
    return;
  }

  MCContext& context = Asm.getContext();
  const auto& info = getFixupKindInfo(Fixup.getKind());
  const auto offset = Fixup.getOffset();
  constexpr std::uint32_t opcodeSize = 4;
  assert(offset + opcodeSize <= Data.size() && "Invalid fixup offset!");

  auto fixed = adjustFixupValue(Fixup, Target, Value, context, IsResolved);
  fixed <<= info.TargetOffset;
  // mask in the bits from the fixup value
  for (std::uint32_t i{}; i != opcodeSize; ++i) {
    Data[offset + i] |= static_cast<std::uint8_t>((fixed >> (i * 8)) & 0xFFull);
  }
}

// Target Relaxation Interfaces
bool AltairXMCAsmBackend::fixupNeedsRelaxation(
    const MCFixup &Fixup, uint64_t Value, const MCRelaxableFragment *DF,
    const MCAsmLayout &Layout) const {
  return false;
}

} // namespace llvm