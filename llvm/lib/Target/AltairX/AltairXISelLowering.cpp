//===-- AltairXISelLowering.cpp - AltairX DAG Lowering Implementation -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that AltairX uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#include "AltairXISelLowering.h"

#include "AltairXCommon.h"
#include "AltairXMachineFunctionInfo.h"
#include "AltairXSubtarget.h"
#include "AltairXTargetMachine.h"

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/Debug.h"

#include <cassert>

using namespace llvm;

#define DEBUG_TYPE "altairx-isellower"

#include "AltairXGenCallingConv.inc"

static constexpr std::array<llvm::MVT, 3> SmallIntsMVT = {MVT::i8, MVT::i16,
                                                          MVT::i32};
static constexpr std::array<llvm::MVT, 4> AllIntsMVT = {MVT::i8, MVT::i16,
                                                        MVT::i32, MVT::i64};
static constexpr std::array<llvm::MVT, 2> AllFloatsMVT = {MVT::f32, MVT::f64};

static constexpr std::array<llvm::MVT, 6> AllMVT = {
    MVT::i8, MVT::i16, MVT::i32, MVT::i64, MVT::f32, MVT::f64};

static constexpr std::array<MCPhysReg, 8> GPRArgRegs = {
    AltairX::R1, AltairX::R2, AltairX::R3, AltairX::R4,
    AltairX::R5, AltairX::R6, AltairX::R7, AltairX::R8};
static constexpr std::array<MCPhysReg, 8> FP32ArgRegs = {
    AltairX::F0, AltairX::F1, AltairX::F2, AltairX::F3,
    AltairX::F4, AltairX::F5, AltairX::F6, AltairX::F7};
static constexpr std::array<MCPhysReg, 8> FP64ArgRegs = {
    AltairX::D0, AltairX::D1, AltairX::D2, AltairX::D3,
    AltairX::D4, AltairX::D5, AltairX::D6, AltairX::D7};

AltairXTargetLowering::AltairXTargetLowering(const TargetMachine &TM,
                                             const AltairXSubtarget &STI)
    : TargetLowering(TM), Subtarget(STI) {

  // Set up the register classes
  addRegisterClass(MVT::i8, &AltairX::GPIReg8RegClass);
  addRegisterClass(MVT::i16, &AltairX::GPIReg16RegClass);
  addRegisterClass(MVT::i32, &AltairX::GPIReg32RegClass);
  addRegisterClass(MVT::i64, &AltairX::GPIReg64RegClass);
  addRegisterClass(MVT::f32, &AltairX::FReg32RegClass);
  addRegisterClass(MVT::f64, &AltairX::FReg64RegClass);
  computeRegisterProperties(Subtarget.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(AltairX::R0);
  setSchedulingPreference(Sched::Hybrid);

  // Use i32 for setcc operations results (slt, sgt, ...).
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);

  // loadext/storetrunc f64 from/to f32 is not natively supported
  setLoadExtAction(ISD::EXTLOAD, MVT::f64, MVT::f32, LegalizeAction::Expand);
  setTruncStoreAction(MVT::f64, MVT::f32, LegalizeAction::Expand);

  // We cannot match this directly
  setCondCodeAction({ISD::SETO, ISD::SETUO}, AllFloatsMVT, LegalizeAction::Expand);

  // Constants
  setOperationAction(ISD::Constant, AllIntsMVT, LegalizeAction::Legal);
  // AXIMPR: This is the default, but could be improved with isFPImmLegal
  setOperationAction(ISD::ConstantFP, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::GlobalAddress, MVT::i64, LegalizeAction::Custom);
  //setOperationAction(ISD::GlobalTLSAddress, MVT::i64, LegalizeAction::Custom);
  setOperationAction(ISD::FrameIndex, MVT::i64, LegalizeAction::Expand);
  setOperationAction(ISD::JumpTable, MVT::i64, LegalizeAction::Custom);
  setOperationAction(ISD::ConstantPool, AllMVT, LegalizeAction::Custom);
  //setOperationAction(ISD::ExternalSymbol, MVT::i64, LegalizeAction::Custom);
  setOperationAction(ISD::BlockAddress, MVT::i64, LegalizeAction::Custom);
  //setOperationAction(PtrAuthGlobalAddress, MVT::i64, LegalizeAction::Custom);
  //setOperationAction(GLOBAL_OFFSET_TABLE, MVT::i64, LegalizeAction::Custom);
  //setOperationAction(FRAMEADDR, MVT::i64, LegalizeAction::Custom);
  //setOperationAction(RETURNADDR, MVT::i64, LegalizeAction::Custom);
  //setOperationAction(ADDROFRETURNADDR, MVT::i64, LegalizeAction::Custom);

  // AXIMPR: This can be matched, but they are hard to generate from high level code
  setOperationAction(ISD::SMUL_LOHI, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::UMUL_LOHI, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::SDIVREM, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::UDIVREM, AllIntsMVT, LegalizeAction::Expand);

  setOperationAction(ISD::FREM, AllFloatsMVT, LegalizeAction::Expand);

  setOperationAction(ISD::FPTRUNC_ROUND, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::FMA, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::FMAD, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::FCOPYSIGN, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::FGETSIGN, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::FCANONICALIZE, AllFloatsMVT, LegalizeAction::Expand);

  // AXIMPR: vector support to do
  // BUILD_VECTOR, INSERT_VECTOR_ELT, EXTRACT_VECTOR_ELT, CONCAT_VECTORS
  // INSERT_SUBVECTOR, EXTRACT_SUBVECTOR, VECTOR_DEINTERLEAVE, VECTOR_INTERLEAVE, VECTOR_REVERSE
  // VECTOR_SHUFFLE, VECTOR_SPLICE, SCALAR_TO_VECTOR, SPLAT_VECTOR, SPLAT_VECTOR_PARTS, STEP_VECTOR, VECTOR_COMPRESS

  setOperationAction(ISD::BSWAP, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::CTTZ, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::CTLZ, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::CTPOP, AllIntsMVT, LegalizeAction::Expand);

  setOperationAction(ISD::SELECT, AllMVT, LegalizeAction::Custom);
  //setOperationAction(ISD::VSELECT, AllMVT, LegalizeAction::Custom);
  setOperationAction(ISD::SELECT_CC, AllMVT, LegalizeAction::Custom);
  setOperationAction(ISD::SETCC, AllMVT, LegalizeAction::Custom);
  
  setOperationAction(ISD::SHL_PARTS, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::SRA_PARTS, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::SRL_PARTS, AllIntsMVT, LegalizeAction::Expand);

  setOperationAction(ISD::FP_TO_SINT, SmallIntsMVT, LegalizeAction::Promote);
  setOperationAction(ISD::SINT_TO_FP, SmallIntsMVT, LegalizeAction::Promote);
  setOperationAction(ISD::FP_TO_UINT, SmallIntsMVT, LegalizeAction::Promote);
  setOperationAction(ISD::UINT_TO_FP, SmallIntsMVT, LegalizeAction::Promote);
  setOperationAction(ISD::FP_TO_SINT, MVT::i64, LegalizeAction::Custom);
  setOperationAction(ISD::SINT_TO_FP, MVT::i64, LegalizeAction::Custom);
  setOperationAction(ISD::FP_TO_UINT, MVT::i64, LegalizeAction::Expand);
  setOperationAction(ISD::UINT_TO_FP, MVT::i64, LegalizeAction::Expand);

  setOperationAction(ISD::GET_ROUNDING, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::SET_ROUNDING, AllFloatsMVT, LegalizeAction::Expand);

  setOperationAction(ISD::FCOS, AllFloatsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::FPOW, AllFloatsMVT, LegalizeAction::Expand);

  setOperationAction(ISD::DYNAMIC_STACKALLOC, AllIntsMVT, Expand);
  setOperationAction({ISD::STACKSAVE, ISD::STACKRESTORE}, MVT::Other, Expand);

  setOperationAction(ISD::BR_CC, AllMVT, LegalizeAction::Custom);
  setOperationAction(ISD::BR_JT, MVT::Other, LegalizeAction::Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, LegalizeAction::Expand);
  setOperationAction(ISD::BRIND, MVT::Other, LegalizeAction::Custom);

  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Custom);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);

  // Set minimum and preferred function alignment, and loop alignment
  setMinFunctionAlignment(Align{4});
  setMinStackArgumentAlignment(Align{8});
  setPrefFunctionAlignment(Align{4});
  setPrefLoopAlignment(Align{1});
}

SDValue AltairXTargetLowering::LowerOperation(SDValue Op,
                                              SelectionDAG &DAG) const {
  LLVM_DEBUG(dbgs() << "Lowering: ");
  LLVM_DEBUG(Op.dump());

  switch (Op.getOpcode()) {
  case ISD::FP_TO_SINT:
    return LowerFP_TO_SINT(Op, DAG);
  case ISD::FP_TO_UINT:
    return LowerFP_TO_UINT(Op, DAG);
  case ISD::SINT_TO_FP:
    return LowerSINT_TO_FP(Op, DAG);
  case ISD::UINT_TO_FP:
    return LowerUINT_TO_FP(Op, DAG);
  case ISD::GlobalAddress:
    return LowerGlobalAddress(Op, DAG);
  case ISD::ConstantPool:
    return LowerConstantPool(Op, DAG);
  case ISD::BlockAddress:
    return LowerBlockAddress(Op, DAG);
  case ISD::JumpTable:
    return LowerJumpTable(Op, DAG);
  case ISD::SELECT:
    return LowerSELECT(Op, DAG);
  case ISD::SELECT_CC:
    return LowerSELECT_CC(Op, DAG);
  case ISD::SETCC:
    return LowerSETCC(Op, DAG);
  case ISD::BR_CC:
    return LowerBR_CC(Op, DAG);
  case ISD::BRIND:
    return LowerBRIND(Op, DAG);
  case ISD::VASTART:
    return LowerVASTART(Op, DAG);
  case ISD::VAARG:
    return LowerVAARG(Op, DAG);
  default:
    llvm_unreachable("unimplemented operand");
  }
}

const char *AltairXTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case AltairXISD::CONSTANTTOREG:
    return "AltairXISD::ConstantToReg";
  case AltairXISD::RET:
    return "AltairXISD::Ret";
  case AltairXISD::CALL:
    return "AltairXISD::Call";
  case AltairXISD::JUMP:
    return "AltairXISD::Jump";
  case AltairXISD::INDIRECT_CALL:
    return "AltairXISD::IndirectCall";
  case AltairXISD::INDIRECT_JUMP:
    return "AltairXISD::IndirectJump";
  case AltairXISD::INDIRECT_BRA:
    return "AltairXISD::IndirectBRA";
  case AltairXISD::CMP:
    return "AltairXISD::Cmp";
  case AltairXISD::FCMP:
    return "AltairXISD::FCmp";
  case AltairXISD::BRC:
    return "AltairXISD::BRC";
  case AltairXISD::SCMP:
    return "AltairXISD::SCmp";
  case AltairXISD::SBIT:
    return "AltairXISD::SBit";
  case AltairXISD::CMOVE:
    return "AltairXISD::CMove";
  case AltairXISD::FSCMP:
    return "AltairXISD::FSCmp";
  case AltairXISD::FCMOVE:
    return "AltairXISD::CMove";
  case AltairXISD::GAWRAPPER:
    return "AltairXISD::GAWrapper";
  case AltairXISD::ITOF:
    return "AltairXISD::ITOF";
  case AltairXISD::FTOI:
    return "AltairXISD::FTOI";
  case AltairXISD::VAARG:
    return "AltairXISD::VAARG";
  default:
    return nullptr;
  }
}

std::pair<unsigned, const TargetRegisterClass *>
AltairXTargetLowering::getRegForInlineAsmConstraint(
    const TargetRegisterInfo *RegInfo, StringRef Constraint, MVT VT) const {
  if(Constraint[0] == 'r') {
    switch (VT.SimpleTy) {
    case MVT::i8:
      return std::make_pair(0U, &AltairX::GPIReg8RegClass);
    case MVT::i16:
      return std::make_pair(0U, &AltairX::GPIReg16RegClass);
    case MVT::i32:
      return std::make_pair(0U, &AltairX::GPIReg32RegClass);
    case MVT::i64:
      return std::make_pair(0U, &AltairX::GPIReg64RegClass);
    default:
      break;
    }
  }

  return TargetLowering::getRegForInlineAsmConstraint(RegInfo, Constraint, VT);
}

namespace {

SDValue toValVT(SelectionDAG &DAG, SDValue Value, const CCValAssign &VA,
                const SDLoc &DL) {
  switch (VA.getLocInfo()) {
  case CCValAssign::Full:
    return Value; // identity
  case CCValAssign::BCvt:
    return DAG.getNode(ISD::BITCAST, DL, VA.getValVT(), Value);
  default:
    llvm_unreachable("Unknown location info");
  }
}

SDValue lowerFromRegLoc(SelectionDAG &DAG, SDValue Chain, const CCValAssign &VA,
                        const SDLoc &DL) {
  assert(VA.isRegLoc() && "lowerFromRegLoc called with non reg loc");
  if (VA.needsCustom()) {
    llvm_unreachable("Custom val not supported by Args Lowering");
  }

  const MVT type = VA.getLocVT();
  const auto *regClass = AltairXRegisterInfo::MVTRegClass(type);
  if (!regClass) {
    llvm_unreachable("Invalid MTV for argument");
  }

  MachineFunction &MF = DAG.getMachineFunction();
  // Transform the arguments in physical registers into virtual ones.
  const auto reg = MF.addLiveIn(VA.getLocReg(), regClass);
  SDValue arg = DAG.getCopyFromReg(Chain, DL, reg, type);
  return toValVT(DAG, arg, VA, DL);
}

// The caller is responsible for loading the full value if the argument is
// passed with CCValAssign::Indirect.
SDValue lowerFromMemLoc(SelectionDAG &DAG, SDValue Chain, const CCValAssign &VA,
                        const SDLoc &DL) {
  assert(VA.isMemLoc());

  switch (VA.getLocInfo()) {
  default:
    llvm_unreachable("Unexpected CCValAssign::LocInfo");
  case CCValAssign::Full:
  // case CCValAssign::Indirect:
  case CCValAssign::BCvt:
    break;
  }

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  EVT type = VA.getValVT();

  int index =
      MFI.CreateFixedObject(type.getStoreSize(), VA.getLocMemOffset(), true);
  SDValue frameIndex = DAG.getFrameIndex(index, MVT::i64);

  return DAG.getExtLoad(ISD::NON_EXTLOAD, DL, VA.getLocVT(), Chain, frameIndex,
                        MachinePointerInfo::getFixedStack(MF, index), type);
}

} // namespace

/// LowerFormalArguments - transform physical registers into virtual registers
/// and generate load operations for arguments places on the stack.
SDValue AltairXTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  assert((CallConv == CallingConv::C || CallConv == CallingConv::Fast) &&
         "Unsupported CallingConv to FORMAL_ARGS");

  MachineFunction &MF = DAG.getMachineFunction();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign> argLocs;
  CCState CCInfo{CallConv, IsVarArg, MF, argLocs, *DAG.getContext()};
  CCInfo.AnalyzeFormalArguments(Ins, AltairX_CCallingConv);

  // Used with varargs to acumulate store chains.
  std::vector<SDValue> outChains;
  if (IsVarArg && MF.getFrameInfo().hasVAStart()) {
    constexpr int64_t slotSize = 8; // all args are in 8 bytes slots
    const int32_t intRegCount = CCInfo.getFirstUnallocated(GPRArgRegs);
    const int32_t floatRegCount = CCInfo.getFirstUnallocated(FP64ArgRegs);

    // Offset of the first variable argument from stack pointer, and size of
    // the vararg save area. For now, the varargs save area is either zero or
    // large enough to hold a0-a7.
    // If all registers are allocated, then all varargs must be passed on the
    // stack and we don't need to save any argregs.
    MachineFrameInfo &frameInfo = MF.getFrameInfo();

    // Record the frame index of the first variable argument
    // which is a value necessary to VASTART.
    auto *info = MF.getInfo<AltairXMachineFunctionInfo>();
    info->VarArgsFrameIndex =
        frameInfo.CreateFixedObject(1, CCInfo.getStackSize(), true);
    info->VarArgsGPOffset = intRegCount * slotSize;
    info->VarArgsFPOffset =
        GPRArgRegs.size() * slotSize + floatRegCount * slotSize;
    info->RegSaveFrameIndex = frameInfo.CreateStackObject(
        GPRArgRegs.size() * slotSize + FP64ArgRegs.size() * slotSize, Align(8),
        false);

    // keeping live input value
    SmallVector<SDValue, 8> liveIntRegs;
    for (auto reg : ArrayRef<MCPhysReg>{GPRArgRegs}.slice(intRegCount)) {
      liveIntRegs.emplace_back(DAG.getCopyFromReg(
          Chain, DL, MF.addLiveIn(reg, &AltairX::GPIReg64RegClass), MVT::i64));
    }

    SmallVector<SDValue, 8> memOps;
    SDValue frameIndex = DAG.getFrameIndex(info->RegSaveFrameIndex, MVT::i64);
    uint64_t stackOffset = info->VarArgsGPOffset;
    for (auto &&val : liveIntRegs) {
      SDValue fi = DAG.getNode(ISD::ADD, DL, MVT::i64, frameIndex,
                               DAG.getIntPtrConstant(stackOffset, DL));
      const auto ptrInfo = MachinePointerInfo::getFixedStack(
          MF, info->RegSaveFrameIndex, stackOffset);
      SDValue store = DAG.getStore(val.getValue(1), DL, val, fi, ptrInfo);
      memOps.emplace_back(store);
      stackOffset += slotSize;
    }

    SmallVector<SDValue, 8> liveFloatRegs;
    for (auto reg : ArrayRef<MCPhysReg>{FP64ArgRegs}.slice(floatRegCount)) {
      liveFloatRegs.emplace_back(DAG.getCopyFromReg(
          Chain, DL, MF.addLiveIn(reg, &AltairX::FReg64RegClass), MVT::f64));
    }

    for (auto &&val : liveFloatRegs) {
      SDValue fi = DAG.getNode(ISD::ADD, DL, MVT::i64, frameIndex,
                               DAG.getIntPtrConstant(stackOffset, DL));
      const auto ptrInfo = MachinePointerInfo::getFixedStack(
          MF, info->RegSaveFrameIndex, stackOffset);
      SDValue store = DAG.getStore(val.getValue(1), DL, val, fi, ptrInfo);
      memOps.push_back(store);
      stackOffset += slotSize;
    }

    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, memOps);
  }

  for (CCValAssign &VA : argLocs) {
    if (VA.isRegLoc()) {
      InVals.push_back(lowerFromRegLoc(DAG, Chain, VA, DL));
    } else {
      InVals.push_back(lowerFromMemLoc(DAG, Chain, VA, DL));
    }
  }

  // All stores are grouped in one node to allow the matching between
  // the size of Ins and InVals. This only happens for vararg functions.
  if (!outChains.empty()) {
    outChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, outChains);
  }

  return Chain;
}

bool AltairXTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool isVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState ccInfo{CallConv, isVarArg, MF, RVLocs, Context};
  return ccInfo.CheckReturn(Outs, AltairX_CRetConv);
}

/// LowerCallResult - Lower the result values of a call into the
/// appropriate copies out of appropriate physical registers.
namespace {

SDValue LowerCallResult(SDValue Chain, SDValue Glue,
                        const SmallVectorImpl<CCValAssign> &RetLocs, SDLoc DL,
                        SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) {
  // Copy results out of physical registers.
  SmallVector<std::pair<std::int64_t, std::size_t>> retMemLocs;
  for (const CCValAssign &VA : RetLocs) {
    if (VA.isRegLoc()) {
      SDValue ret =
          DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getValVT(), Glue);
      Chain = ret.getValue(1);
      Glue = ret.getValue(2);
      InVals.push_back(ret);
    } else {
      assert(VA.isMemLoc() && "Must be memory location.");
      retMemLocs.emplace_back(VA.getLocMemOffset(), InVals.size());

      // Reserve space for this result.
      InVals.push_back(SDValue());
    }
  }

  // Copy results out of memory.
  SmallVector<SDValue, 4> memOpChains;
  for (auto [offset, index] : retMemLocs) {
    SDValue sp = DAG.getRegister(AltairX::R0, MVT::i64);
    SDValue offVal = DAG.getConstant(offset, DL, MVT::i64);
    SDValue loc = DAG.getNode(ISD::ADD, DL, MVT::i64, sp, offVal);
    SDValue load = DAG.getLoad(MVT::i64, DL, Chain, loc, MachinePointerInfo());

    InVals[index] = load;
    memOpChains.push_back(load.getValue(1));
  }

  // Transform all loads nodes into one single node because
  // all load nodes are independent of each other.
  if (!memOpChains.empty()) {
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, memOpChains);
  }

  return Chain;
}

} // namespace

SDValue
AltairXTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                 SmallVectorImpl<SDValue> &InVals) const {
  CLI.IsTailCall = false; // Do not support tail calls yet.

  auto &DAG = CLI.DAG;
  auto &DL = CLI.DL;

  SmallVector<CCValAssign, 16> argLocs;
  CCState inInfo{CLI.CallConv, CLI.IsVarArg, DAG.getMachineFunction(), argLocs,
                 *DAG.getContext()};
  inInfo.AnalyzeCallOperands(CLI.Outs, AltairX_CCallingConv);

  // Analyze return values to determine the number of bytes of stack required.
  SmallVector<CCValAssign, 16> retLocs;
  CCState ccInfo{CLI.CallConv, CLI.IsVarArg, DAG.getMachineFunction(), retLocs,
                 *DAG.getContext()};
  ccInfo.AllocateStack(inInfo.getStackSize(), Align(8));
  ccInfo.AnalyzeCallResult(CLI.Ins, AltairX_CRetConv);
  const auto stackSize = ccInfo.getStackSize();

  CLI.Chain = DAG.getCALLSEQ_START(CLI.Chain, stackSize, 0, DL);

  SmallVector<std::pair<Register, SDValue>, 8> regsToPass;
  SmallVector<SDValue, 12> memOpChains;

  SDValue stackPtr;
  // Walk the register/memloc assignments, inserting copies/loads.
  for (std::size_t i{}; i < argLocs.size(); ++i) {
    CCValAssign &VA = argLocs[i];
    SDValue arg = CLI.OutVals[i];

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    default:
      llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      arg = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), arg);
      break;
    case CCValAssign::AExt:
      [[fallthrough]];
    case CCValAssign::ZExt:
      arg = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), arg);
      break;
    }

    // Arguments that can be passed on register must be kept at
    // RegsToPass vector
    if (VA.isRegLoc()) {
      regsToPass.emplace_back(VA.getLocReg(), arg);
    } else {
      assert(VA.isMemLoc() && "Must be register or memory argument.");
      if (!stackPtr.getNode()) {
        stackPtr = DAG.getCopyFromReg(CLI.Chain, DL, AltairX::R0, MVT::i64);
      }
      // Calculate the stack position.
      SDValue offset = DAG.getIntPtrConstant(VA.getLocMemOffset(), DL);
      SDValue ptrOff = DAG.getNode(ISD::ADD, DL, MVT::i64, stackPtr, offset);
      SDValue store =
          DAG.getStore(CLI.Chain, DL, arg, ptrOff, MachinePointerInfo{});

      memOpChains.push_back(store);
      CLI.IsTailCall = false;
    }
  }

  // Transform all store nodes into one single node because
  // all store nodes are independent of each other.
  if (!memOpChains.empty()) {
    CLI.Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, memOpChains);
  }

  // Build a sequence of copy-to-reg nodes chained together with token
  // chain and flag operands which copy the outgoing args into registers.
  // The Glue in necessary since all emitted instructions must be
  // stuck together.
  SDValue glue{};
  for (auto [reg, val] : regsToPass) {
    CLI.Chain = DAG.getCopyToReg(CLI.Chain, DL, reg, val, glue);
    glue = CLI.Chain.getValue(1);
  }

  // If the callee is a GlobalAddress node (quite common, every direct call is)
  // turn it into a TargetGlobalAddress node so that legalize doesn't hack it.
  // Likewise ExternalSymbol -> TargetExternalSymbol.
  bool isDirect = true;
  if (auto *global = dyn_cast<GlobalAddressSDNode>(CLI.Callee); global) {
    CLI.Callee = DAG.getTargetGlobalAddress(global->getGlobal(), DL, MVT::i64);
  } else if (auto *ext = dyn_cast<ExternalSymbolSDNode>(CLI.Callee); ext) {
    CLI.Callee = DAG.getTargetExternalSymbol(ext->getSymbol(), MVT::i64);
  } else {
    isDirect = false;
  }

  // Branch + Link = #chain, #target_address, #opt_in_flags...
  //             = Chain, Callee, Reg#1, Reg#2, ...
  // Returns a chain & a glue for retval copy to use.
  SmallVector<SDValue, 8> ops;
  ops.push_back(CLI.Chain);
  ops.push_back(CLI.Callee);
  for (auto &&[loc, arg] : regsToPass) {
    ops.push_back(DAG.getRegister(loc, arg.getValueType()));
  }

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  // Add a register mask operand representing the call-preserved registers.
  const auto *TRI = Subtarget.getRegisterInfo();
  const uint32_t *mask =
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CLI.CallConv);
  assert(mask && "Missing call preserved mask for calling convention");
  ops.push_back(DAG.getRegisterMask(mask));

  if (glue.getNode()) {
    ops.push_back(glue);
  }

  CLI.Chain =
      DAG.getNode(isDirect ? AltairXISD::CALL : AltairXISD::INDIRECT_CALL, DL,
                  NodeTys, ops);
  glue = CLI.Chain.getValue(1);

  // Create the CALLSEQ_END node.
  CLI.Chain = DAG.getCALLSEQ_END(CLI.Chain, stackSize, 0, glue, DL);
  glue = CLI.Chain.getValue(1);

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  if (CLI.IsTailCall) {
    return CLI.Chain;
  }

  return LowerCallResult(CLI.Chain, glue, retLocs, DL, DAG, InVals);
}

SDValue
AltairXTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                                   bool IsVarArg,
                                   const SmallVectorImpl<ISD::OutputArg> &Outs,
                                   const SmallVectorImpl<SDValue> &OutVals,
                                   const SDLoc &DL, SelectionDAG &DAG) const {
  // CCValAssign - represent the assignment of the return value to a location.
  // CCState - Info about the registers and stack slots.
  // Analyze outgoing return values.
  SmallVector<CCValAssign> retLocs;
  CCState ccInfo{CallConv, IsVarArg, DAG.getMachineFunction(), retLocs,
                 *DAG.getContext()};
  ccInfo.AnalyzeReturn(Outs, AltairX_CRetConv);

  SDValue flag;
  SmallVector<SDValue> retOps;
  retOps.push_back(Chain); // Operand #0 = Chain (updated below)

  // Copy the result values into the output registers.
  for (std::size_t i{}; i != retLocs.size(); ++i) {
    CCValAssign &VA = retLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");
    assert(!VA.needsCustom() && "Custom val assignment not supported");

    SDValue ret = OutVals[i];
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::ZExt:
    case CCValAssign::AExt:
      ret = DAG.getZExtOrTrunc(ret, DL, VA.getLocVT());
      break;
    case CCValAssign::SExt:
      ret = DAG.getSExtOrTrunc(ret, DL, VA.getLocVT());
      break;
    default:
      llvm_unreachable("Unknown loc info!");
    }

    // Guarantee that all emitted copies are stuck together
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), ret, flag);
    flag = Chain.getValue(1);
    retOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  // Update chain and glue.
  retOps[0] = Chain;
  if (flag.getNode()) {
    retOps.push_back(flag);
  }

  return DAG.getNode(AltairXISD::RET, DL, MVT::Other, retOps);
}

EVT AltairXTargetLowering::getSetCCResultType(const DataLayout &, LLVMContext &,
                                              EVT VT) const {
  if (VT.isVector()) {
    llvm_unreachable("no bool vec for now!");
  }
  // booleans are i8
  return MVT::i8;
}

MachineBasicBlock *
AltairXTargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                                   MachineBasicBlock *MBB) const {

  switch(MI.getOpcode()) {
  case AltairX::VAARG:
    return EmitVAARGWithCustomInserter(MI, MBB);
  default:
    llvm_unreachable("Unexpected instruction type to insert");
  }

  return MBB;
}

MachineBasicBlock *AltairXTargetLowering::EmitVAARGWithCustomInserter(
    MachineInstr &MI, MachineBasicBlock *MBB) const {

  // Operands to this pseudo-instruction:
  // 0  ) destination address (reg)
  // 1-2) va_list* (addrimm, c.f. AltairXDAGToDAGISel::selectAddrImm)
  //      1) base
  //      2) offset
  // 3  ) size (in bytes) of vaarg type (=vaarg second arg)
  // 4  ) 0 = int (gp_offset) ; 1 = float (fp_offset)
  // 5  ) alignment of type
  // 6  ) FR (implicit-def)
  // struct va_list {
  //   i32 gp_offset
  //   i32 fp_offset
  //   ptr overflow_area
  //   ptr reg_save_area
  // }
  // sizeof(va_list) = 24
  // alignment(va_list) = 8

  assert(MI.getNumOperands() == 7 && "VAARG must have 8 operands!");

  MachineFunction &func = *MBB->getParent();
  const TargetInstrInfo &instInfo = *Subtarget.getInstrInfo();
  MachineRegisterInfo &regInfo = func.getRegInfo();
  const TargetRegisterClass *addrRegClass = getRegClassFor(MVT::i64);
  const MIMetadata metadata{MI};

  const Register destReg = MI.getOperand(0).getReg();
  const MachineOperand& base = MI.getOperand(1);
  const MachineOperand& offset = MI.getOperand(2);
  const uint64_t argSize = MI.getOperand(3).getImm();
  const uint64_t argMode = MI.getOperand(4).getImm();
  const uint64_t argAlign = MI.getOperand(5).getImm();
  const uint64_t alignedArgSize =
      alignTo(argSize, Align(std::max(argAlign, 8ull)));

  // Memory reference
  assert(MI.hasOneMemOperand() && "VAARG must have single memoperand");
  auto *oldMMO = MI.memoperands().front();
  // Clone the MMO into two separate MMOs for loading and storing
  auto *loadOnlyMMO = func.getMachineMemOperand(
      oldMMO, oldMMO->getFlags() & ~MachineMemOperand::MOStore);
  auto *storeOnlyMMO = func.getMachineMemOperand(
      oldMMO, oldMMO->getFlags() & ~MachineMemOperand::MOLoad);

  constexpr int64_t slotSize = 8; // all args are in 8 bytes slots
  constexpr uint64_t intRegsCount = 8;
  constexpr uint64_t floatRegsCount = 8;
  const bool useFPOffset = (argMode == 1);
  const uint64_t maxOffset = intRegsCount * slotSize +
                             (useFPOffset ? floatRegsCount * slotSize : 0);

  // First emit code to check if gp_offset (or fp_offset) is below the bound.
  // If so, pull the argument from reg_save_area. (branch to offsetMBB)
  // If not, pull from overflow_area. (branch to overflowMBB)

  // Registers for the PHI in endMBB
  // Argument address computed by offsetMBB
  Register offsetDestReg = regInfo.createVirtualRegister(addrRegClass);
  // Argument address computed by overflowMBB
  Register overflowDestReg = regInfo.createVirtualRegister(addrRegClass);

  const BasicBlock *llvmBB = MBB->getBasicBlock();
  auto *overflowMBB = func.CreateMachineBasicBlock(llvmBB);
  auto *offsetMBB = func.CreateMachineBasicBlock(llvmBB);
  auto *endMBB = func.CreateMachineBasicBlock(llvmBB);

  // Insert the new basic blocks
  auto it = std::next(MBB->getIterator());
  func.insert(it, offsetMBB);
  func.insert(it, overflowMBB);
  func.insert(it, endMBB);

  // Transfer the remainder of MBB and its successor edges to endMBB.
  endMBB->splice(endMBB->begin(), MBB, std::next(MI.getIterator()), MBB->end());
  endMBB->transferSuccessorsAndUpdatePHIs(MBB);

  // Make offsetMBB and overflowMBB successors of MBB
  MBB->addSuccessor(offsetMBB);
  MBB->addSuccessor(overflowMBB);

  // endMBB is a successor of both offsetMBB and overflowMBB
  offsetMBB->addSuccessor(endMBB);
  overflowMBB->addSuccessor(endMBB);

  // Load the offset value into a register
  Register offsetReg = regInfo.createVirtualRegister(addrRegClass);
  BuildMI(MBB, metadata, instInfo.get(AltairX::LoadRIdZX64), offsetReg)
      .add(base)
      .addDisp(offset, useFPOffset ? 4ull : 0ull)
      .setMemRefs(loadOnlyMMO);

  // Check if there is enough room left to pull this argument.
  Register maxOffsetReg = regInfo.createVirtualRegister(addrRegClass);
  BuildMI(MBB, metadata, instInfo.get(AltairX::ConstantToRegq), maxOffsetReg)
    .addImm(maxOffset + slotSize - alignedArgSize);

  // Branch to "overflowMBB" if offset >= max
  // Fall through to "offsetMBB" otherwise
  BuildMI(MBB, metadata, instInfo.get(AltairX::BRCq))
      .addMBB(overflowMBB)
      .addImm(static_cast<int64_t>(ISD::CondCode::SETUGE))
      .addReg(offsetReg)
      .addReg(maxOffsetReg);

  // Read the reg_save_area address.
  Register regSaveReg = regInfo.createVirtualRegister(addrRegClass);
  BuildMI(offsetMBB, metadata, instInfo.get(AltairX::LoadRIq), regSaveReg)
      .add(base)
      .addDisp(offset, 16ull)
      .setMemRefs(loadOnlyMMO);

  // Add the offset to the reg_save_area to get the final address.
  BuildMI(offsetMBB, metadata, instInfo.get(AltairX::AddRRq), offsetDestReg)
      .addReg(offsetReg)
      .addReg(regSaveReg);

  // Compute the offset for the next argument
  Register nextOffsetReg = regInfo.createVirtualRegister(addrRegClass);
  BuildMI(offsetMBB, metadata, instInfo.get(AltairX::AddRIq), nextOffsetReg)
      .addReg(offsetReg)
      .addImm(useFPOffset ? 16ull : 8ull);

  // Store it back into the va_list.
  BuildMI(offsetMBB, metadata, instInfo.get(AltairX::StoreRIdT64))
      .addReg(nextOffsetReg)
      .add(base)
      .addDisp(offset, useFPOffset ? 4ull : 0ull)
      .setMemRefs(storeOnlyMMO);

  // Jump to endMBB
  BuildMI(offsetMBB, metadata, instInfo.get(AltairX::BRA)).addMBB(endMBB);

  // Emit code to use overflow area
  // Load the overflow_area address into a register.
  Register overflowAddrReg = regInfo.createVirtualRegister(addrRegClass);
  BuildMI(overflowMBB, metadata, instInfo.get(AltairX::LoadRIq),
          overflowAddrReg)
      .add(base)
      .addDisp(offset, 8)
      .setMemRefs(loadOnlyMMO);

  // Note: align address here if needed later!
  BuildMI(overflowMBB, metadata, instInfo.get(TargetOpcode::COPY),
    overflowDestReg)
      .addReg(overflowAddrReg);

  // Compute the next overflow address after this argument.
  // (the overflow address should be kept 8-byte aligned)
  Register nextAddrReg = regInfo.createVirtualRegister(addrRegClass);
  BuildMI(overflowMBB, metadata, instInfo.get(AltairX::AddRIq), nextAddrReg)
      .addReg(overflowDestReg)
      .addImm(alignedArgSize);

  // Store the new overflow address.
  BuildMI(overflowMBB, metadata, instInfo.get(AltairX::StoreRIq))
      .addReg(nextAddrReg)
      .add(base)
      .addDisp(offset, 8)
      .setMemRefs(storeOnlyMMO);

  // emit the PHI to the front of endMBB.
  BuildMI(*endMBB, endMBB->begin(), metadata, instInfo.get(AltairX::PHI),
          destReg)
      .addReg(offsetDestReg)
      .addMBB(offsetMBB)
      .addReg(overflowDestReg)
      .addMBB(overflowMBB);

  // Erase the pseudo instruction
  MI.eraseFromParent();

  return endMBB;
}

SDValue AltairXTargetLowering::LowerFP_TO_SINT(SDValue Op,
                                               SelectionDAG &DAG) const {
  assert(Op.getSimpleValueType() == MVT::i64);

  const MVT ftype = Op.getOperand(0).getSimpleValueType();
  assert((ftype == MVT::f32 || ftype == MVT::f64));

  const SDLoc dl{Op};
  auto ftoi = DAG.getNode(AltairXISD::FTOI, dl, MVT::f64, Op.getOperand(0));
  return DAG.getNode(ISD::BITCAST, dl, MVT::i64, ftoi);
}

SDValue AltairXTargetLowering::LowerFP_TO_UINT(SDValue Op,
                                               SelectionDAG &DAG) const {
  assert(Op.getSimpleValueType() == MVT::i64);

  const MVT ftype = Op.getOperand(0).getSimpleValueType();
  assert((ftype == MVT::f32 || ftype == MVT::f64));

  const SDLoc dl{Op};
  auto ftoi = DAG.getNode(AltairXISD::FTOI, dl, MVT::f64, Op.getOperand(0));
  return DAG.getNode(ISD::BITCAST, dl, MVT::i64, ftoi);
}

SDValue AltairXTargetLowering::LowerSINT_TO_FP(SDValue Op,
                                               SelectionDAG &DAG) const {
  assert(Op.getOperand(0).getSimpleValueType() == MVT::i64);

  const MVT ftype = Op.getSimpleValueType();
  assert((ftype == MVT::f32 || ftype == MVT::f64));

  const SDLoc dl{Op};
  auto bitcast = DAG.getNode(ISD::BITCAST, dl, MVT::f64, Op.getOperand(0));
  return DAG.getNode(AltairXISD::ITOF, dl, ftype, bitcast);
}

SDValue AltairXTargetLowering::LowerUINT_TO_FP(SDValue Op,
                                               SelectionDAG &DAG) const {
  assert(Op.getOperand(0).getSimpleValueType() == MVT::i64);

  const MVT ftype = Op.getSimpleValueType();
  assert((ftype == MVT::f32 || ftype == MVT::f64));

  const SDLoc dl{Op};
  auto bitcast = DAG.getNode(ISD::BITCAST, dl, MVT::f64, Op.getOperand(0));
  return DAG.getNode(AltairXISD::ITOF, dl, ftype, bitcast);
}

namespace {

SDValue getGlobalAddress(SelectionDAG &DAG, const GlobalAddressSDNode *addr,
                         EVT type, uint32_t flags = 0) {
  return DAG.getTargetGlobalAddress(addr->getGlobal(), SDLoc{addr}, type, 0,
                                    flags);
}

static SDValue getGlobalAddress(SelectionDAG &DAG,
                                const BlockAddressSDNode *block, EVT type,
                                uint32_t flags = 0) {
  return DAG.getTargetBlockAddress(block->getBlockAddress(), type,
                                   block->getOffset(), flags);
}

static SDValue getGlobalAddress(SelectionDAG &DAG,
                                const ConstantPoolSDNode *pool, EVT type,
                                uint32_t flags = 0) {
  return DAG.getTargetConstantPool(pool->getConstVal(), type, pool->getAlign(),
                                   pool->getOffset(), flags);
}

static SDValue getGlobalAddress(SelectionDAG &DAG, const JumpTableSDNode *jt,
                                EVT type, uint32_t flags = 0) {
  return DAG.getTargetJumpTable(jt->getIndex(), type, flags);
}

} // namespace

template <typename NodeT>
SDValue
AltairXTargetLowering::getGlobalAddressWrapper(SelectionDAG &DAG,
                                               const NodeT *node) const {
  const EVT type = getPointerTy(DAG.getDataLayout());
  const SDLoc dl{node};

  // We may need to handle code model here
  SDValue addr = getGlobalAddress(DAG, node, type);
  return DAG.getNode(AltairXISD::GAWRAPPER, dl, type, addr);
}

SDValue AltairXTargetLowering::LowerGlobalAddress(SDValue Op,
                                                  SelectionDAG &DAG) const {
  return getGlobalAddressWrapper(DAG, cast<GlobalAddressSDNode>(Op));
}

SDValue AltairXTargetLowering::LowerConstantPool(SDValue Op,
                                                 SelectionDAG &DAG) const {
  return getGlobalAddressWrapper(DAG, cast<ConstantPoolSDNode>(Op));
}

SDValue AltairXTargetLowering::LowerBlockAddress(SDValue Op,
                                                 SelectionDAG &DAG) const {
  return getGlobalAddressWrapper(DAG, cast<BlockAddressSDNode>(Op));
}

SDValue AltairXTargetLowering::LowerJumpTable(SDValue Op,
                                              SelectionDAG &DAG) const {
  return getGlobalAddressWrapper(DAG, cast<JumpTableSDNode>(Op));
}

namespace {

// Convert to target constant so this instruction won't be selected by
// the tablegen pattern (set regclass:$rd, immpat:$imm)
// In some cases the left operand must be a constant value. But this is not
// natively supported. Example: "a > 5" becomes "5 < a".
// This is a simple helper fonction that returns a new node to materialize
// the constant, only if it is a constant!
SDValue promoteConstant(SelectionDAG &DAG, SDLoc dl, SDValue node) {
  if (node.getOpcode() == ISD::Constant) {
    const auto type = node.getValueType();

    if (auto *iconstant = dyn_cast<ConstantSDNode>(node); iconstant) {
      auto val = DAG.getTargetConstant(iconstant->getAPIntValue(), dl, type);
      return DAG.getNode(AltairXISD::CONSTANTTOREG, dl, type, val);
      //} else if (auto *fconstant = dyn_cast<ConstantFPSDNode>(node);
      //fconstant) {
      //  auto val = DAG.getTargetConstantFP(fconstant->getValueAPF(), dl,
      //  type); return DAG.getNode(AltairXISD::CONSTANTTOREG, dl, type, val);
    } else {
      llvm_unreachable("Expected constant SDNode");
    }
  }

  return node;
}

std::optional<SDValue> MatchesSBit(SelectionDAG &DAG, SDLoc dl,
                                   SDValue comparedNode, SDValue andNode,
                                   ISD::CondCode cc) {
  if (!isTrueWhenEqual(cc)) {
    return std::nullopt;
  }

  if (andNode.getOpcode() != ISD::AND) {
    return std::nullopt;
  }

  // seteq(i, and(i, j)) -> sbit(i, j)
  // seteq(j, and(i, j)) -> sbit(i, j)
  if (comparedNode == andNode.getOperand(0) ||
      comparedNode == andNode.getOperand(1)) {
    return DAG.getNode(AltairXISD::SBIT, dl, MVT::i8, andNode.getOperand(0),
                       andNode.getOperand(1));
  }

  return std::nullopt;
}

struct SETCCOperands {
  SDValue &left;
  SDValue &right;
  AltairX::SCMPCondCode cc{};
  bool needFlip{}; // true if "xor 1" must be added
};

std::optional<SETCCOperands> computeSETCCOperands(SDValue &Left, SDValue &Right,
                                                  ISD::CondCode CC) {
  // AXIMPR: support NaN
  const auto ltu_value = Left.getSimpleValueType().isFloatingPoint()
                             ? AltairX::SCMPCondCode::LT
                             : AltairX::SCMPCondCode::LTU;

  switch (CC) {
  // Natively supported cases, this will be matched by tablegen patterns as-is
  case ISD::SETOEQ: // equal
    [[fallthrough]];
  case ISD::SETUEQ:
    [[fallthrough]];
  case ISD::SETEQ:
    return SETCCOperands{Left, Right, AltairX::SCMPCondCode::EQ, false};
  case ISD::SETONE: // not equal
    [[fallthrough]];
  case ISD::SETUNE:
    [[fallthrough]];
  case ISD::SETNE:
    return SETCCOperands{Left, Right, AltairX::SCMPCondCode::NE, false};
  case ISD::SETOLT: // less than
    [[fallthrough]];
  case ISD::SETULT:
    [[fallthrough]];
  case ISD::SETLT:
    return SETCCOperands{Left, Right, AltairX::SCMPCondCode::LT, false};
  // Cases that need at least one modification
  case ISD::SETOGT: // ordered >
    return SETCCOperands{Right, Left, AltairX::SCMPCondCode::LT, false};
  case ISD::SETOGE: // ordered >=
    return SETCCOperands{Left, Right, AltairX::SCMPCondCode::LT, true};
  case ISD::SETOLE: // ordered <=
    return SETCCOperands{Right, Left, AltairX::SCMPCondCode::LT, true};
  case ISD::SETUGT: // unsigned >
    return SETCCOperands{Right, Left, ltu_value, false};
  case ISD::SETUGE: // unsigned >=
    return SETCCOperands{Left, Right, ltu_value, true};
  case ISD::SETULE: // unsigned <=
    return SETCCOperands{Right, Left, ltu_value, true};
  case ISD::SETGT: // signed >
    return SETCCOperands{Right, Left, AltairX::SCMPCondCode::LT, false};
  case ISD::SETGE: // signed >=
    return SETCCOperands{Left, Right, AltairX::SCMPCondCode::LT, true};
  case ISD::SETLE: // signed <=
    return SETCCOperands{Right, Left, AltairX::SCMPCondCode::LT, true};
  default:
    llvm_unreachable("Unsupported ISD::CondCode");
    break;
  }
}

} // namespace

SDValue AltairXTargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  // i8 = setcc left, right, seteq:ch
  // a == b -> se(a, b)
  // a != b -> sen(a, b)
  // a < b  -> slt(a, b)
  // a > b  -> slt(b, a)
  // a <= b -> slt(b, a) xor 1
  // a >= b -> slt(a, b) xor 1

  SDValue left = Op.getOperand(0);
  SDValue right = Op.getOperand(1);
  const ISD::CondCode cc = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  SDLoc dl{Op};

  const auto type = left.getSimpleValueType();
  if (type.isInteger()) {
    // seteq(and(i, j), j) -> sbit(i, j)
    // seteq(i, and(i, j)) -> sbit(i, j)
    if (auto node{MatchesSBit(DAG, dl, left, right, cc)}; node) {
      return *node;
    }
    if (auto node{MatchesSBit(DAG, dl, right, left, cc)}; node) {
      return *node;
    }
  }

  // generic case
  auto operands = computeSETCCOperands(left, right, cc);
  if (!operands) { // natively supported
    return Op;
  }

  // left must not be constant!
  SDValue realLeft = promoteConstant(DAG, dl, operands->left);
  SDValue ccval =
    DAG.getConstant(static_cast<uint64_t>(operands->cc), dl, MVT::i32);
  SDValue scmp;
  if (type.isFloatingPoint()) {
    scmp = DAG.getNode(AltairXISD::FSCMP, dl, MVT::i8, realLeft,
                       operands->right, ccval);
  } else {
    scmp = DAG.getNode(AltairXISD::SCMP, dl, MVT::i8, realLeft, operands->right,
                       ccval);
  }

  if (operands->needFlip) {
    return DAG.getNode(ISD::XOR, dl, MVT::i8, scmp,
                       DAG.getConstant(1, dl, MVT::i8));
  }

  return scmp;
}

namespace {

struct SelectSetCCOperands {
  SDValue &left;
  SDValue &right;
  AltairX::SCMPCondCode cc{};
};

struct SelectCMoveOperands {
  SDValue &trueVal;
  SDValue &falseVal;
};

struct SelectCCOperands {
  SelectSetCCOperands setcc;
  SelectCMoveOperands cmove;
};

SelectCCOperands computeSelectCCOperands(SDValue &Left, SDValue &Right,
                                         SDValue &TVal, SDValue &FVal,
                                         ISD::CondCode CC) {
  // AXIMPR: support NaN
  const auto ltu_value = Left.getSimpleValueType().isFloatingPoint()
                             ? AltairX::SCMPCondCode::LT
                             : AltairX::SCMPCondCode::LTU;

  // cmove(f, t, cond): f = t if (cond != 0)

  // left != right ? t : f:
  // cmove(t, f, left == right)
  // (3 != 2 ? t : f) -> t <- (3 == 2 ? f : t)
  // (3 != 3 ? t : f) -> f <- (3 == 3 ? f : t)
  // (3 != 4 ? t : f) -> t <- (3 == 4 ? f : t)

  // left > right ? t : f:
  // cmove(f, t, right < left)
  // (3 > 2 ? t : f) -> t <- (2 < 3 ? t : f)
  // (3 > 3 ? t : f) -> f <- (3 < 3 ? t : f)
  // (3 > 4 ? t : f) -> f <- (4 < 3 ? t : f)

  // left <= right ? t : f:
  // cmove(t, f, right < left)
  // (3 <= 2 ? t : f) -> f <- (2 < 3 ? f : t)
  // (3 <= 3 ? t : f) -> t <- (3 < 3 ? f : t)
  // (3 <= 4 ? t : f) -> t <- (4 < 3 ? f : t)

  // left >= right ? t : f:
  // cmove(t, f, left < right)
  // (3 >= 2 ? t : f) -> t <- (3 < 2 ? f : t)
  // (3 >= 3 ? t : f) -> t <- (3 < 3 ? f : t)
  // (3 >= 4 ? t : f) -> f <- (3 < 4 ? f : t)

  switch (CC) {
  // Natively supported cases, this will be matched by tablegen patterns as-is
  case ISD::SETOEQ: // equal
    [[fallthrough]];
  case ISD::SETUEQ:
    [[fallthrough]];
  case ISD::SETEQ:
    return {{Left, Right, AltairX::SCMPCondCode::EQ}, {TVal, FVal}};
  case ISD::SETONE: // not equal
    [[fallthrough]];
  case ISD::SETUNE:
    [[fallthrough]];
  case ISD::SETNE:
    return {{Left, Right, AltairX::SCMPCondCode::NE}, {TVal, FVal}};
  case ISD::SETOLT: // less than
    [[fallthrough]];
  case ISD::SETULT:
    [[fallthrough]];
  case ISD::SETLT:
    return {{Left, Right, AltairX::SCMPCondCode::LT}, {TVal, FVal}};
  // Cases that need at least one modification
  case ISD::SETOGT: // ordered >
    return {{Right, Left, AltairX::SCMPCondCode::LT}, {TVal, FVal}};
  case ISD::SETOGE: // ordered >=
    return {{Left, Right, AltairX::SCMPCondCode::LT}, {FVal, TVal}};
  case ISD::SETOLE: // ordered <=
    return {{Right, Left, AltairX::SCMPCondCode::LT}, {FVal, TVal}};
  case ISD::SETUGT: // unsigned >
    return {{Right, Left, ltu_value}, {TVal, FVal}};
  case ISD::SETUGE: // unsigned >=
    return {{Left, Right, ltu_value}, {FVal, TVal}};
  case ISD::SETULE: // unsigned <=
    return {{Right, Left, ltu_value}, {FVal, TVal}};
  case ISD::SETGT: // signed >
    return {{Right, Left, AltairX::SCMPCondCode::LT}, {TVal, FVal}};
  case ISD::SETGE: // signed >=
    return {{Left, Right, AltairX::SCMPCondCode::LT}, {FVal, TVal}};
  case ISD::SETLE: // signed <=
    return {{Right, Left, AltairX::SCMPCondCode::LT}, {FVal, TVal}};
  default:
    llvm_unreachable("Unsupported ISD::CondCode");
    break;
  }
}

} // namespace

SDValue AltairXTargetLowering::LowerSELECT(SDValue Op,
                                           SelectionDAG &DAG) const {
  SDLoc dl{Op};
  SDValue cond = Op.getOperand(0);
  SDValue tval = Op.getOperand(1);
  SDValue fval = Op.getOperand(2);

  const auto type = Op.getValueType();
  // left must not be constant!
  SDValue realLeft = promoteConstant(DAG, dl, fval);

  if (type.isFloatingPoint()) {
    return DAG.getNode(AltairXISD::FCMOVE, dl, type, realLeft, cond, tval);
  }

  return DAG.getNode(AltairXISD::CMOVE, dl, type, realLeft, cond, tval);
}

SDValue AltairXTargetLowering::LowerSELECT_CC(SDValue Op,
                                              SelectionDAG &DAG) const {
  SDValue left = Op.getOperand(0);
  SDValue right = Op.getOperand(1);
  SDValue tval = Op.getOperand(2);
  SDValue fval = Op.getOperand(3);
  const ISD::CondCode cc = cast<CondCodeSDNode>(Op.getOperand(4))->get();

  SDLoc dl{Op};
  const auto cmptype = left.getValueType();
  const auto outtype = tval.getValueType();

  auto [setccOps, cmoveOps] =
      computeSelectCCOperands(left, right, tval, fval, cc);

  // left must not be constant!
  SDValue realLeft = promoteConstant(DAG, dl, setccOps.left);
  SDValue ccval =
      DAG.getConstant(static_cast<uint64_t>(setccOps.cc), dl, MVT::i32);

  SDValue setcc;
  if (cmptype.isFloatingPoint()) {
    setcc = DAG.getNode(AltairXISD::FSCMP, dl, MVT::i8, realLeft,
                        setccOps.right, ccval);
  } else {
    setcc = DAG.getNode(AltairXISD::SCMP, dl, MVT::i8, realLeft, setccOps.right,
                        ccval);
  }

  if (outtype.isFloatingPoint()) {
    return DAG.getNode(AltairXISD::FCMOVE, dl, Op.getValueType(),
                       cmoveOps.falseVal, setcc, cmoveOps.trueVal);
  }

  return DAG.getNode(AltairXISD::CMOVE, dl, Op.getValueType(),
                     cmoveOps.falseVal, setcc, cmoveOps.trueVal);

}

SDValue AltairXTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue chain = Op.getOperand(0);
  const ISD::CondCode cc = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue left = Op.getOperand(2);
  SDValue right = Op.getOperand(3);
  SDValue dest = Op.getOperand(4);

  SDLoc dl{Op};
  const auto type = left.getValueType();

  // BRC becomes [F]BRC[size] that supports all predicates
  // It is expanded by AltairXBranchPatcher pass
  auto ccval = DAG.getConstant(cc, dl, MVT::i32);
  return DAG.getNode(AltairXISD::BRC, dl, MVT::Other, chain, dest, ccval,
    left, right);
}

SDValue AltairXTargetLowering::LowerBRIND(SDValue Op, SelectionDAG &DAG) const {
  SDLoc dl{Op};
  SDValue chain = Op.getOperand(0);
  SDValue dest = Op.getOperand(1);

  return DAG.getNode(AltairXISD::INDIRECT_JUMP, dl, MVT::Other, chain, dest);
}

SDValue AltairXTargetLowering::LowerVASTART(SDValue Op,
                                            SelectionDAG &DAG) const {
  auto &func = DAG.getMachineFunction();
  SDLoc dl{Op};
  SDValue chain = Op.getOperand(0);
  SDValue ptr = Op.getOperand(1);
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  auto *info = func.getInfo<AltairXMachineFunctionInfo>();
  SmallVector<SDValue, 4> memOps;

  // Store gp_offset
  memOps.emplace_back(DAG.getStore(
      chain, dl, DAG.getConstant(info->VarArgsGPOffset, dl, MVT::i32), ptr,
      MachinePointerInfo(SV)));

  // Store fp_offset
  ptr = DAG.getNode(ISD::ADD, dl, MVT::i64, ptr,
                    DAG.getConstant(4, dl, MVT::i64));
  memOps.emplace_back(DAG.getStore(
      chain, dl, DAG.getConstant(info->VarArgsFPOffset, dl, MVT::i32), ptr,
      MachinePointerInfo(SV, 4)));

  // Store ptr to overflow_arg_area
  ptr = DAG.getNode(ISD::ADD, dl, MVT::i64, ptr,
                    DAG.getConstant(4, dl, MVT::i64));
  SDValue vaFI = DAG.getFrameIndex(info->VarArgsFrameIndex, MVT::i64);
  memOps.emplace_back(
      DAG.getStore(chain, dl, vaFI, ptr, MachinePointerInfo(SV, 8)));

  // Store ptr to reg_save_area.
  ptr = DAG.getNode(ISD::ADD, dl, MVT::i64, ptr,
                    DAG.getConstant(8, dl, MVT::i64));
  SDValue rsFI = DAG.getFrameIndex(info->RegSaveFrameIndex, MVT::i64);
  memOps.emplace_back(
      DAG.getStore(chain, dl, rsFI, ptr, MachinePointerInfo(SV, 16)));

  return DAG.getNode(ISD::TokenFactor, dl, MVT::Other, memOps);
}

namespace {

SDValue getAlignedValue(SelectionDAG &DAG, SDValue value, Align align) {
  SDLoc dl{value};

  SDValue incrVal = DAG.getConstant(align.value() - 1, dl, MVT::i64);
  SDValue incr = DAG.getNode(ISD::ADD, dl, MVT::i64, value, incrVal);

  SDValue alignVal =
      DAG.getSignedConstant(-static_cast<int64_t>(align.value()), dl, MVT::i64);
  return DAG.getNode(ISD::AND, dl, MVT::i64, incr, alignVal);
}

} // namespace

SDValue AltairXTargetLowering::LowerVAARG(SDValue Op, SelectionDAG &DAG) const {
  SDNode *node = Op.getNode();
  SDLoc dl{Op};

  const EVT type = node->getValueType(0);
  SDValue chain = node->getOperand(0);
  SDValue ptr = node->getOperand(1);
  const Value* value = cast<SrcValueSDNode>(node->getOperand(2))->getValue();
  const uint64_t align = Op.getConstantOperandVal(3);

  const uint8_t mode = type.isFloatingPoint() ? 1 : 0;
  const uint64_t size = DAG.getDataLayout().getTypeAllocSize(
      type.getTypeForEVT(*DAG.getContext()));
  // Decide which area this value should be read from
  const MachinePointerInfo ptrInfo{value};

  // VAARG returns two values: Variable Argument Address, Chain
  SDVTList vts = DAG.getVTList(getPointerTy(DAG.getDataLayout()), MVT::Other);
  std::array<SDValue, 5> ops = {chain, ptr,
                                DAG.getTargetConstant(size, dl, MVT::i64),
                                DAG.getTargetConstant(mode, dl, MVT::i8),
                                DAG.getTargetConstant(align, dl, MVT::i64)};
  SDValue vaarg = DAG.getMemIntrinsicNode(
      AltairXISD::VAARG, dl, vts, ops, MVT::i64, ptrInfo, std::nullopt,
      MachineMemOperand::MOLoad | MachineMemOperand::MOStore);
  chain = vaarg.getValue(1);

  // Load the next argument and return it
  return DAG.getLoad(type, dl, chain, vaarg, MachinePointerInfo{});
}

