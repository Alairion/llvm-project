//===- IntrinsicsAltairX.td - Defines AltairX intrinsics ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "altairx" in {

def int_altairx_syscall : Intrinsic<[], [llvm_vararg_ty], []>;

}