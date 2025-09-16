//===- IntrinsicsAltairX.td - Defines AltairX intrinsics ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let TargetPrefix = "altairx" in {

class AltairXInt<list<LLVMType> ret_types, list<LLVMType> param_types,
              list<IntrinsicProperty> intr_properties = []>
    : Intrinsic<ret_types, param_types, intr_properties>,
      ClangBuiltin<!subst("int_altairx", "__builtin_altairx", NAME)>;

foreach i = 0...7 in {
  def int_altairx_syscall#i : AltairXInt<[llvm_i64_ty], !listsplat(llvm_i64_ty, !add(i, 1)), [ImmArg<ArgIndex<0>>]>;
}

}