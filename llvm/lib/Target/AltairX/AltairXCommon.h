#ifndef LLVM_LIB_TARGET_ALTAIRX_COMMON_H
#define LLVM_LIB_TARGET_ALTAIRX_COMMON_H

#include <cstdint>

#include "llvm/Support/ErrorHandling.h"

namespace llvm::AltairX {

enum class CondCode : std::uint32_t {
  NE = 0b0000, // Not equal
  EQ = 0b1000, // Equal
  L = 0b0100, // Less
  LE = 0b1100, // Less or equal
  G = 0b0010, // Greater
  GE = 0b1010, // Greater or equal
  LS = 0b0110, // Less (signed)
  LES = 0b1110, // Less or equal (signed)
  GS = 0b0001, // Greater (signed)
  GES = 0b1001 // Greater or equal (signed)
};

inline CondCode reverseCondCode(CondCode cc) noexcept {
  switch (cc) {
  case CondCode::NE:
    return CondCode::EQ;
  case CondCode::EQ:
    return CondCode::NE;
  case CondCode::L:
    return CondCode::GE;
  case CondCode::LE:
    return CondCode::G;
  case CondCode::G:
    return CondCode::LE;
  case CondCode::GE:
    return CondCode::L;
  case CondCode::LS:
    return CondCode::GES;
  case CondCode::LES:
    return CondCode::GS;
  case CondCode::GS:
    return CondCode::LES;
  case CondCode::GES:
    return CondCode::LS;
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
  InstFormatBRURelImm24 = 8,
  InstFormatBRUAbsImm24 = 9,
  InstFormatBRUSpecial = 10,
};

}

#endif
