//=== AltairXSelectionDAGInfo.cpp - AltairX ISELDAG Info --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AltairXSelectionDAGInfo.h"

#include "AltairXISelLowering.h"

using namespace llvm;

bool AltairXSelectionDAGInfo::isTargetMemoryOpcode(unsigned Opcode) const {
  return Opcode == AltairXISD::VAARG;
}
