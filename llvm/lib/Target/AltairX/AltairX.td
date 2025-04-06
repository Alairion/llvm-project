//=== AltairX.td - Describe the AltairX Target Machine ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Top tablegen file that pulls in code from the LLVM framework along with
// code specific to this target.
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Calling Conv, Instruction Descriptions, Register information, etc
//===----------------------------------------------------------------------===//

include "AltairXSchedule.td"
include "AltairXRegisterInfo.td"
include "AltairXCallingConv.td"
include "AltairXInstrInfo.td"

def AltairXInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// AltairX processors supported
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic",  NoSchedModel, []>;

//===----------------------------------------------------------------------===//
// AltairX Target declaration
//===----------------------------------------------------------------------===//

def AltairX : Target {
  let InstructionSet = AltairXInstrInfo;
}
