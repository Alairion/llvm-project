//===-- AltairXMCTargetDesc.h - AltairX Target Descriptions ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides AltairX specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXMCTARGETDESC_H
#define LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXMCTARGETDESC_H

#include <cstdint>

// Defines symbolic names for AltairX registers. This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "AltairXGenRegisterInfo.inc"

// Defines symbolic names for the AltairX instructions.
#define GET_INSTRINFO_ENUM
#include "AltairXGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "AltairXGenSubtargetInfo.inc"

#endif // end LLVM_LIB_TARGET_ALTAIRX_MCTARGETDESC_ALTAIRXMCTARGETDESC_H
