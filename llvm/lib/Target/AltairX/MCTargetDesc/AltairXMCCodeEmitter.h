//===-- AltairXMCCodeEmitter.h - AltairX Asm Info ------------------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the AltairXMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_MCCODEEMITTER_ALTAIRXMCCODEEMITTER_H
#define LLVM_LIB_TARGET_ALTAIRX_MCCODEEMITTER_ALTAIRXMCCODEEMITTER_H

#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCFixup.h"

#include <cstdint>

namespace llvm {

class MCInstrInfo;
class MCContext;
class MCOperand;

class AltairXMCCodeEmitter : public MCCodeEmitter {
public:
  explicit AltairXMCCodeEmitter(const MCInstrInfo &instrInfo,
                                MCContext &context)
      : MCII{instrInfo}, MCC{context} {}

  // For most instructions, works for regs, imms and expression
  // This may apply fixup to value for moveix support!
  std::uint64_t getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const;

  // For imm only operands (e.g. condcode operand type)
  std::uint64_t getImmOpValue(const MCInst &MI, std::uint32_t OpIdx,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const;

  // For imm value of moveix, MI flags will be used to determine the fixup type
  // to generate using the instruction type
  std::uint64_t getMoveIXOpValue(const MCInst &MI, std::uint32_t OpIdx,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  // For imm value of pcrel branches (brtarget operand type)
  std::uint64_t getBRTargetOpValue(const MCInst &MI, std::uint32_t OpIdx,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo &STI) const;

  // For imm value of absolute branches (brtarget operand type)
  std::uint64_t getCallTargetOpValue(const MCInst& MI, std::uint32_t OpIdx,
                                     SmallVectorImpl<MCFixup>& Fixups,
                                     const MCSubtargetInfo& STI) const;

protected:
  void encodeInstruction(const MCInst &Inst, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

private:
  MCFixupKind getImmFixupFor(const MCInst& MI) const;
  MCFixupKind getMoveIXFixupFor(const MCInst& MI) const;

  // getBinaryCodeForInstr - TableGen'erated function for getting the
  // binary encoding for an instruction.
  std::uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                      SmallVectorImpl<MCFixup> &Fixups,
                                      const MCSubtargetInfo &STI) const;

  const MCInstrInfo &MCII;
  MCContext &MCC;
};

} // namespace llvm

#endif // end LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXMCASMINFO_H
