// AltairXInstrAliases.td - Target Description for AltairX Target - tablegen  //
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// ALU aliases
def : InstAlias<"move $dst, $src", (AddRIq GPIReg64:$dst, GPIReg64:$src, 0)>;
def : InstAlias<"move $dst, $src", (AddRId GPIReg32:$dst, GPIReg32:$src, 0)>;
def : InstAlias<"move $dst, $src", (AddRIw GPIReg16:$dst, GPIReg16:$src, 0)>;
def : InstAlias<"move $dst, $src", (AddRIb GPIReg8:$dst, GPIReg8:$src, 0)>;

// BRU aliases
def : InstAlias<"ret", (IndirectCall ZERO, R31)>;


