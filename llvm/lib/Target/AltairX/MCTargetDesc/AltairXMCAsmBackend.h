//===-- AltairXMCAsmBackend.h - AltairX MC Asm Backend ---------*- C++ -*--===//
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

#ifndef LLVM_LIB_TARGET_ALTAIRX_MCASMBACKEND_ALTAIRXMCASMBACKEND_H
#define LLVM_LIB_TARGET_ALTAIRX_MCASMBACKEND_ALTAIRXMCASMBACKEND_H

#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCFixup.h"

namespace llvm {

class Target;
class MCSubtargetInfo;
class MCRegisterInfo;
class MCTargetOptions;

namespace AltairX {
enum Fixups {
  // 24-bits pc-relative branch (bx, loop)
  fixup_altairx_pcrel24lo = FirstTargetFixupKind,
  fixup_altairx_pcrel24hi,
  // 24-bits fixup for absolute jumps (call, jump, ...)
  fixup_altairx_abs24lo,
  fixup_altairx_abs24hi,

  // MoveIX fixups:
  // These fixups are in two parts, the low N-bits part that goes in the main
  // instruction, and the high 24-bits that goes in moveix.
  // If the imm value is sign-extended, the sign-bit must be place in the main
  // instruction immediate and the moveix value inverted.
  // With for exemple, sign-extended N-bits long imm will be handled like this:
  //   tmp = sext iN imm to i64
  //   tmp[N - 1; N + 22] ^= imm24 from moveix
  // Example: "add a, b, 0xF00FF0FF; moveix" (imm is -267390721)
  // add will carry a 9bits imm, with the sign (bit 31) + 8 bits:
  // 0xF00FF0FF -> 0x1FF
  // moveix will carry the remaining 24-bits, inverted:
  // 0xF00FF0FF -> ~(0xF00FF0FF >> 8) -> 0x0FF00F
  // At runtime the processor will do the following:
  //   tmp = sext i9 (0x1FF) to i64 = 0xFFFF'FFFF'FFFF'FFFF
  //   tmp[8; 30] ^= 0x0FF00F -> FFFF'FFFF'F00F'F0FF
  fixup_altairx_moveix9lo, // most ALU/MDU instructions
  fixup_altairx_moveix9hi,
  fixup_altairx_moveix10lo, // LSU instructions
  fixup_altairx_moveix10hi,
  fixup_altairx_moveix18lo, // MoveI
  fixup_altairx_moveix18hi,

  // Marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
} // namespace AltairX

class AltairXMCAsmBackend : public MCAsmBackend {
public:
  explicit AltairXMCAsmBackend(const Target &T,
                               const MCSubtargetInfo &Subtarget,
                               const MCRegisterInfo &RegInfo,
                               const MCTargetOptions &TargetOptions)
      : MCAsmBackend{support::endianness::little}, Target{T}, STI{Subtarget},
        MRI{RegInfo}, Options{TargetOptions} {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override;
  bool writeNopData(raw_ostream &OS, std::uint64_t Count,
                    const MCSubtargetInfo *STI) const override;

  // Target Fixup Interfaces
  unsigned getNumFixupKinds() const override;
  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override;
  const MCFixupKindInfo& getFixupKindInfo(MCFixupKind Kind) const override;

  // generate the right imm for instruction according to "kind"
  static std::uint64_t adjustImmValue(MCFixupKind kind,
                                      std::uint64_t Value) noexcept;


  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  std::uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override;

  // Target Relaxation Interfaces
  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override;



private:
  const Target &Target;
  const MCSubtargetInfo &STI;
  const MCRegisterInfo &MRI;
  const MCTargetOptions &Options;
};

} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXMCASMINFO_H
