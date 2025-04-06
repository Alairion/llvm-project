//===-- AltairXSchedule.td - AltairX Scheduling Definitions ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Functional units of the AltairX
//===----------------------------------------------------------------------===//
def ALU1  : FuncUnit;
//def ALU2  : FuncUnit;
//def LSU1  : FuncUnit;
//def LSU2  : FuncUnit;
//def FPU1  : FuncUnit;
//def FPU2  : FuncUnit;
//def DFPU1 : FuncUnit;
//def DFPU2 : FuncUnit;
//def EFU   : FuncUnit;
//def MDU   : FuncUnit;
//def BRU   : FuncUnit;

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for Cpu0
//===----------------------------------------------------------------------===//
def IIAlu1    : InstrItinClass;
//def II_CLO   : InstrItinClass;
//def II_CLZ   : InstrItinClass;
//def IILoad   : InstrItinClass;
//def IIStore  : InstrItinClass;
//def IIBranch : InstrItinClass;
//def IIPseudo : InstrItinClass;

//===----------------------------------------------------------------------===//
// Cpu0 Generic instruction itineraries.
//===----------------------------------------------------------------------===//
//@ http://llvm.org/docs/doxygen/html/structllvm_1_1InstrStage.html
def Cpu0GenericItineraries : ProcessorItineraries<[ALU1], [], [
////@2
    InstrItinData<IIAlu1              , [InstrStage<1,  [ALU1]>]>,
//  InstrItinData<II_CLO             , [InstrStage<1,  [ALU]>]>,
//  InstrItinData<II_CLZ             , [InstrStage<1,  [ALU]>]>,
//  InstrItinData<IILoad             , [InstrStage<3,  [ALU]>]>,
//  InstrItinData<IIStore            , [InstrStage<1,  [ALU]>]>,
//  InstrItinData<IIBranch           , [InstrStage<1,  [ALU]>]>
]>;


/// Define scheduler resources associated with def operands.
//def WriteIALU : SchedWrite;    // 32 or 64-bit integer ALU operations
def WriteJalr : SchedWrite;    // 32 or 64-bit integer ALU operations
//
///// Define scheduler resources associated with use operands.
//def ReadIALU : SchedRead;
def ReadJalr : SchedRead;