//=== AltairXCallingConv.td - Calling Conventions for AltairX --*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===--------------------------------------------------------------------------===//
// This describes the calling conventions for AltairX architecture.
//===--------------------------------------------------------------------------===//

// C calling convention
def AltairX_CCallingConv : CallingConv<[
  // Handles byval parameters. Tell LLVM what the size and alignment
  // requirements of each stack slot are
  CCIfByVal<CCPassByVal<8, 8>>,

  CCIfType<[i8], CCAssignToReg<[R1b, R2b, R3b, R4b, R5b, R6b, R7b, R8b]>>,
  CCIfType<[i16], CCAssignToReg<[R1w, R2w, R3w, R4w, R5w, R6w, R7w, R8w]>>,
  CCIfType<[i32], CCAssignToReg<[R1d, R2d, R3d, R4d, R5d, R6d, R7d, R8d]>>,
  CCIfType<[i64], CCAssignToReg<[R1, R2, R3, R4, R5, R6, R7, R8]>>,
  CCIfType<[i8], CCAssignToStack<1, 8>>,
  CCIfType<[i16], CCAssignToStack<2, 8>>,
  CCIfType<[i32], CCAssignToStack<4, 8>>,
  CCIfType<[i64], CCAssignToStack<8, 8>>,

  CCIfType<[f32], CCAssignToReg<[F0, F1, F2, F3, F4, F5, F6, F7]>>,
  CCIfType<[f64], CCAssignToReg<[D0, D1, D2, D3, D4, D5, D6, D7]>>,
  CCIfType<[f32], CCAssignToStack<4, 8>>,
  CCIfType<[f64], CCAssignToStack<8, 8>>,
]>;

// C Return Convention
def AltairX_CRetConv : CallingConv<[
  CCIfType<[i8], CCAssignToReg<[R1b]>>,
  CCIfType<[i16], CCAssignToReg<[R1w]>>,
  CCIfType<[i32], CCAssignToReg<[R1d]>>,
  CCIfType<[i64], CCAssignToReg<[R1]>>,

  CCIfType<[f32], CCAssignToReg<[F0, F1, F2, F3, F4, F5, F6, F7]>>,
  CCIfType<[f64], CCAssignToReg<[D0, D1, D2, D3, D4, D5, D6, D7]>>,
]>;

def AltairX_CalleeSavedRegs : CalleeSavedRegs<(add 
  (sequence "R%u", 9, 19),
  (sequence "F%u", 8, 19),
  R30, // FP
  R31 // LR
)>;
