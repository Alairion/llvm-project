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
enum class BRCondCode : std::uint32_t {
  EQ = 0b000,  // Equal
  NE = 0b001,  // Not equal
  LT = 0b010,  // Less
  GE = 0b011,  // Greater or equal
  LTU = 0b100, // Less (unsigned)
  GEU = 0b101, // Greater or equal (unsigned)
};

struct ConditionOperands {
  BRCondCode cc;
  MachineOperand left;
  MachineOperand right;
};

inline ConditionOperands
reverseCondition(const ConditionOperands &operands) noexcept {
  switch (operands.cc) {
  case BRCondCode::EQ:
    return {BRCondCode::NE, operands.left, operands.right};
  case BRCondCode::NE:
    return {BRCondCode::EQ, operands.left, operands.right};
  case BRCondCode::LTU:
    return {BRCondCode::LTU, operands.right, operands.left};
  case BRCondCode::GEU:
    return {BRCondCode::GEU, operands.right, operands.left};
  case BRCondCode::LT:
    return {BRCondCode::LT, operands.right, operands.left};
  case BRCondCode::GE:
    return {BRCondCode::GE, operands.right, operands.left};
  default:
    llvm_unreachable("Unknown codecode");
  }
}

// Natively supported condition codes for SCMP and FSCMP
enum class SCMPCondCode : uint32_t {
  EQ,  // Equal
  NE,  // Not Equal
  LT,  // Less (signed)
  LTU, // Less (unsigned), not supported by FSCMP
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
