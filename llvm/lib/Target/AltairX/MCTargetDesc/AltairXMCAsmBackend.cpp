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

bool AltairXMCAsmBackend::writeNopData(raw_ostream &OS, std::uint64_t Count,
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
      MCFixupKindInfo{"fixup_altairx_pcrel24lo", 8, 24, pcrel},
      MCFixupKindInfo{"fixup_altairx_pcrel24hi", 8, 24, pcrel},
      MCFixupKindInfo{"fixup_altairx_abs24lo", 8, 24, 0},
      MCFixupKindInfo{"fixup_altairx_abs24hi", 8, 24, 0},
      MCFixupKindInfo{"fixup_altairx_moveix9lo", 11, 9, 0},
      MCFixupKindInfo{"fixup_altairx_moveix9hi24", 8, 24, 0},
      MCFixupKindInfo{"fixup_altairx_moveix10lo", 10, 10, 0},
      MCFixupKindInfo{"fixup_altairx_moveix10hi24", 8, 24, 0},
      MCFixupKindInfo{"fixup_altairx_moveix18lo", 8, 18, 0},
      MCFixupKindInfo{"fixup_altairx_moveix18hi24", 8, 24, 0},
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

template <std::uint32_t N, std::uint64_t Align = 1>
constexpr std::uint64_t getMoveIXLowSignedValue(std::uint64_t Value) noexcept {
  // total size: N + MoveIX imm size (24)
  constexpr std::uint32_t size = N + 24;
  constexpr std::uint64_t mask = (1ull << (N - 1ull)) - 1ull;
  constexpr std::uint64_t signmask = 1ull << size;

  // sanity checks
  if (!isInt<size>(static_cast<std::int64_t>(Value))) {
    llvm_unreachable("fixup value out of range");
  }
  if(Value % Align != 0) {
    llvm_unreachable("fixup value bad alignement");
  }

  Value /= Align; // align value
  // put sign at bit N, and low (N - 1) bits in this instruction
  return ((Value & signmask) >> 24) | (Value & mask);
}

template <std::uint32_t N, std::uint64_t Align = 1>
std::uint64_t getMoveIXHighSignedValue(std::uint64_t Value) noexcept {
  constexpr std::uint32_t size = N + 24;
  if (!isInt<size>(static_cast<std::int64_t>(Value))) {
    llvm_unreachable("fixup value out of range");
  }
  if(Value % Align != 0) {
    llvm_unreachable("fixup value bad alignement");
  }

  Value /= Align; // align value
  // put inversed bits [N - 1; N + 22] in moveix
  return (Value >> (N - 1)) ^ 0x00FFFFFFull;
}

template <std::uint32_t N, std::uint64_t Align = 1>
constexpr std::uint64_t getMoveIXLowUnsignedValue(std::uint64_t Value) noexcept
{
  // total size: N + MoveIX imm size (24)
  constexpr std::uint32_t size = N + 24;
  constexpr std::uint64_t mask = (1ull << (N - 1ull)) - 1ull;

  // sanity checks
  if(!isUInt<size>(Value)) {
    llvm_unreachable("fixup value out of range");
  }
  if(Value % Align != 0) {
    llvm_unreachable("fixup value bad alignement");
  }

  Value /= Align; // align value
  return Value & mask;
}

template <std::uint32_t N, std::uint64_t Align = 1>
std::uint64_t getMoveIXHighUnsignedValue(std::uint64_t Value) noexcept
{
  constexpr std::uint32_t size = N + 24;

  if(!isUInt<size>(Value)) {
    llvm_unreachable("fixup value out of range");
  }
  if(Value % Align != 0) {
    llvm_unreachable("fixup value bad alignement");
  }

  Value /= Align; // align value
  return Value >> N;
}

} // namespace

std::uint64_t
AltairXMCAsmBackend::adjustImmValue(MCFixupKind kind,
                                    std::uint64_t Value) noexcept {

  switch (static_cast<AltairX::Fixups>(kind)) {
  case AltairX::fixup_altairx_pcrel24lo:
    return getMoveIXLowSignedValue<24, 4>(Value);
  case AltairX::fixup_altairx_pcrel24hi:
    return getMoveIXHighSignedValue<24, 4>(Value);
  case AltairX::fixup_altairx_abs24lo:
    return getMoveIXLowUnsignedValue<24, 4>(Value);
  case AltairX::fixup_altairx_abs24hi:
    return getMoveIXHighUnsignedValue<24, 4>(Value);
  case AltairX::fixup_altairx_moveix9lo:
    return getMoveIXLowSignedValue<9>(Value);
  case AltairX::fixup_altairx_moveix9hi:
    return getMoveIXHighSignedValue<9>(Value);
  case AltairX::fixup_altairx_moveix10lo:
    return getMoveIXLowSignedValue<10>(Value);
  case AltairX::fixup_altairx_moveix10hi:
    return getMoveIXHighSignedValue<10>(Value);
  case AltairX::fixup_altairx_moveix18lo:
    return getMoveIXLowSignedValue<18>(Value);
  case AltairX::fixup_altairx_moveix18hi:
    return getMoveIXHighSignedValue<18>(Value);
  default:
    llvm_unreachable("Unknown fixup kind!");
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

  auto fixed = AltairXMCAsmBackend::adjustImmValue(kind, Value);
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