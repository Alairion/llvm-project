#ifndef LLVM_LIB_TARGET_ALTAIRX_COMMON_H
#define LLVM_LIB_TARGET_ALTAIRX_COMMON_H

#include <cstdint>

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

enum class SCMPCondCode : std::uint32_t {
  EQ = 0b0001, // Equal
  L = 0b1001, // Less
  LS = 0b0101, // Less (signed)
  AND = 0b1101, // Both not 0
};

}

#endif
