//===--- AltairX.cpp - Implement AltairX target feature support ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements AltairXTargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "AltairX.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

ArrayRef<const char *> AltairXTargetInfo::getGCCRegNames() const {
  static const char *const GCCRegNames[] = {
      // Integer registers
      "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",  "r8",  "r9",
      "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17", "r18", "r19",
      "r20", "r21", "r22", "r23", "r24", "r25", "r26", "r27", "r28", "r29",
      "r30", "r31", "r31", "r32", "r33", "r34", "r35", "r36", "r37", "r38",
      "r39", "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47", "r48",
      "r49", "r50", "r51", "r52", "r53", "r54", "r55", "r56", "r57", "r58",
      "r59", "r60", "r61", "r62", "r63",
  };

  return {GCCRegNames};
}

ArrayRef<TargetInfo::GCCRegAlias> AltairXTargetInfo::getGCCRegAliases() const {
  static const TargetInfo::GCCRegAlias GCCRegAliases[] = {
      {{"sp"}, "r0"},

      {{"a0"}, "r1"},   {{"a1"}, "r2"},    {{"a2"}, "r3"},   {{"a3"}, "r4"},
      {{"a4"}, "r5"},   {{"a5"}, "r6"},    {{"a6"}, "r7"},   {{"a7"}, "r8"},

      {{"s0"}, "r9"},   {{"s1"}, "r10"},   {{"s2"}, "r11"},  {{"s3"}, "r12"},
      {{"s4"}, "r13"},  {{"s5"}, "r14"},   {{"s6"}, "r15"},  {{"s7"}, "r16"},
      {{"s8"}, "r17"},  {{"s9"}, "r18"},   {{"s10"}, "r19"},

      {{"t0"}, "r20"},  {{"t1"}, "r21"},   {{"t2"}, "r22"},  {{"t3"}, "r23"},
      {{"t4"}, "r24"},  {{"t5"}, "r25"},   {{"t6"}, "r26"},  {{"t7"}, "r27"},
      {{"t8"}, "r28"},  {{"t9"}, "r29"},

      {{"t10"}, "r30"}, {{"lr"}, "r31"},   {{"n0 "}, "r32"}, {{"n1 "}, "r33"},
      {{"n2 "}, "r34"}, {{"n3 "}, "r35"},  {{"n4 "}, "r36"}, {{"n5 "}, "r37"},
      {{"n6 "}, "r38"}, {{"n7 "}, "r39"},  {{"n8 "}, "r40"}, {{"n9 "}, "r41"},
      {{"n10"}, "r42"}, {{"n11"}, "r43"},  {{"n12"}, "r44"}, {{"n13"}, "r45"},
      {{"n14"}, "r46"}, {{"n15"}, "r47"},  {{"n16"}, "r48"}, {{"n17"}, "r49"},
      {{"n18"}, "r50"}, {{"n19"}, "r51"},  {{"n20"}, "r52"}, {{"n21"}, "r53"},
      {{"n22"}, "r54"}, {{"n23"}, "r55"},  {{"acc"}, "r56"}, {{"ba1"}, "r57"},
      {{"ba2"}, "r58"}, {{"bf1"}, "r59"},  {{"bf2"}, "r60"}, {{"bl1"}, "r61"},
      {{"bl2"}, "r62"}, {{"zero"}, "r63"},
  };

  return {GCCRegAliases};
}

AltairXTargetInfo::AltairXTargetInfo(const llvm::Triple& Triple, const TargetOptions&)
  : TargetInfo(Triple)
{
  // Description string has to be kept in sync with backend string at
  // llvm/lib/Target/AltairX/AltairXTargetMachine.cpp
  resetDataLayout(
    "e" // Little endian
    "-m:e" // ELF name mangling
    "-p:64:64:64:64" // 64-bit pointers, 64-bit aligned
    "-i64:64" // 64-bit integers, 64 bit aligned
    "-n8:16:32:64" // 8, 16, 32 and 64 bits native integers
    "-S64" // 64-bit natural stack alignment
  );

  SuitableAlign = 128; // Minimum valid align for any type

  PointerWidth = 64;
  PointerAlign = 64;
  BoolWidth = 8;
  BoolAlign = 8;
  IntWidth = 32;
  IntAlign = 32;
  LongWidth = 64;
  LongAlign = 64;
  MaxAtomicPromoteWidth = 64;
  MaxAtomicInlineWidth = 64;
}

void AltairXTargetInfo::getTargetDefines(const LangOptions &Opts,
                                       MacroBuilder &Builder) const {
  // Define the __ALTAIRX__ macro when building for this target
  Builder.defineMacro("__ALTAIRX__");
}

static constexpr Builtin::Info BuiltinInfo[] = {
#define BUILTIN(ID, TYPE, ATTRS)                                               \
  {#ID, TYPE, ATTRS, nullptr, HeaderDesc::NO_HEADER, ALL_LANGUAGES},
#define TARGET_BUILTIN(ID, TYPE, ATTRS, FEATURE)                               \
  {#ID, TYPE, ATTRS, FEATURE, HeaderDesc::NO_HEADER, ALL_LANGUAGES},
#include "clang/Basic/BuiltinsAltairX.def"
};

ArrayRef<Builtin::Info> AltairXTargetInfo::getTargetBuiltins() const {
  return BuiltinInfo;
}

bool AltairXTargetInfo::validateAsmConstraint(
    const char *&Name, TargetInfo::ConstraintInfo &info) const {
  switch (*Name) {
  case 'r': // general purpose
    info.setAllowsRegister();
    return true;
  default:
    break;
  }
  return false;
}

std::string AltairXTargetInfo::convertConstraint(const char *&Constraint) const {
  if (*Constraint == 'C') {
    return std::string("^") + std::string(Constraint++, 2);
  }

  return std::string(1, *Constraint);
}
