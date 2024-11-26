#ifndef LLVM_LIB_TARGET_ALTAIRX_COMMON_H
#define LLVM_LIB_TARGET_ALTAIRX_COMMON_H

#include <cstdint>

#include "llvm/Support/ErrorHandling.h"
#include <llvm/CodeGen/MachineOperand.h>

namespace llvm::AltairX {

// Natively supported condition codes for BRC
enum class BRCondCode : std::uint32_t {
  NE = 0b000,  // Not equal
  EQ = 0b100,  // Equal
  LT = 0b010,  // Less
  GE = 0b110,  // Greater or equal
  LTS = 0b001, // Less (signed)
  GES = 0b101, // Greater or equal (signed)
};

struct ConditionOperands
{
  BRCondCode cc;
  MachineOperand left;
  MachineOperand right;
};

inline ConditionOperands reverseCondition(const ConditionOperands& operands) noexcept {
  switch (operands.cc) {
  case BRCondCode::NE:
    return {BRCondCode::EQ, operands.left, operands.right};
  case BRCondCode::EQ:
    return {BRCondCode::NE, operands.left, operands.right};
  case BRCondCode::LT:
    return {BRCondCode::LT, operands.right, operands.left};
  case BRCondCode::GE:
    return {BRCondCode::GE, operands.right, operands.left};
  case BRCondCode::LTS:
    return {BRCondCode::LTS, operands.right, operands.left};
  case BRCondCode::GES:
    return {BRCondCode::GES, operands.right, operands.left};
  default:
    llvm_unreachable("Unknown codecode");
  }
}

enum class SCMPCondCode : std::uint32_t {
  EQ = 0b0001, // Equal
  NE = 0b1001, // Not Equal
  LT = 0b0101, // Less (signed)
  LTU = 0b1101, // Less (unsigned)
};

// in instruction descr TSFlags
enum InstFormat {
  InstFormatPseudo = 0,
  InstFormatMoveImm18 = 1,
  InstFormatRegReg = 2,
  InstFormatRegRegShift = 3,
  InstFormatALURegImm9 = 4,
  InstFormatLSURegImm10 = 5,
  InstFormatLSURegImm16 = 6,
  InstFormatFPURegImm16 = 7,
  InstFormatBRURelImm23 = 8,
  InstFormatBRURelImm24 = 9,
  InstFormatBRUAbsImm24 = 10,
  InstFormatBRUIndirect = 11,
};

}

#endif
