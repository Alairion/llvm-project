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
    // 23-bits pc-relative branch (bx, loop) 
    fixup_altairx_pcrel_br_imm23 = FirstTargetFixupKind,
    // 24-bits fixup for absolute jumps (call, jump, ...)
    fixup_altairx_call_imm24,

    // Marker
    LastTargetFixupKind,
    NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
  };
} // end namespace AArch64

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
  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override;

  // Target Fixup Interfaces
  unsigned getNumFixupKinds() const override;
  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override;
  const MCFixupKindInfo& getFixupKindInfo(MCFixupKind Kind) const override;

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
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
