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

  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,

  CCIfType<[i64], CCAssignToReg<[R1, R2, R3, R4, R5, R6, R7, R8]>>,
  CCIfType<[i64], CCAssignToStack<8, 8>>
]>;

// C Return Convention
def AltairX_CRetConv : CallingConv<[
  CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,

  CCIfType<[i64], CCAssignToReg<[R1, R2, R3, R4, R5, R6, R7, R8]>>,
  CCIfType<[i64], CCAssignToStack<8, 8>>
]>;

def AltairX_CalleeSavedRegs : CalleeSavedRegs<(add 
  (sequence "R%u", 9, 20)
)>;
