//==-- AltairX.h  ------------------------------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_COMMON_H
#define LLVM_LIB_TARGET_ALTAIRX_COMMON_H

#include <cstdint>

#include "llvm/Support/ErrorHandling.h"
#include <llvm/CodeGen/MachineOperand.h>

namespace llvm::AltairX {

// Natively supported condition codes for BRC
enum class BRCondCode : uint32_t {
  EQ = 0b000,  // Equal (signed or ordered)
  NE = 0b001,  // Not equal (signed or ordered)
  LT = 0b010,  // Less (signed or ordered)
  GE = 0b011,  // Greater or equal (signed or ordered)
  EQU = 0b100,  // Equal (unsigned or unordered)
  NEU = 0b101,  // Not equal (unsigned or unordered)
  LTU = 0b110,  // Less (unsigned or unordered)
  GEU = 0b111,  // Greater or equal (unsigned or unordered)
};

// Natively supported condition codes for SCMP and FSCMP
enum class SCMPCondCode : uint32_t {
  EQ = 0b00,  // Equal
  NE = 0b01,  // Not Equal
  LT = 0b10,  // Less (signed)
  LTU = 0b11, // Less (unsigned), not supported by FSCMP
};

// in instruction descr TSFlags
enum InstFormat {
  InstFormatPseudo,
  InstFormatMoveImm18,
  InstFormatRegReg,
  InstFormatRegRegReg,
  InstFormatUnary,
  InstFormatALURegImm9,
  InstFormatALURegRegImm9,
  InstFormatMDURegImm9,
  InstFormatLSURegImm10,
  InstFormatFPURegImm16,
  InstFormatBRURelImm23,
  InstFormatBRURelImm24,
  InstFormatBRUAbsImm24,
  InstFormatBRUIndirect,
  InstFormatCMPRegReg,
  InstFormatCMPRegImm9,
};

} // namespace llvm::AltairX

#endif
