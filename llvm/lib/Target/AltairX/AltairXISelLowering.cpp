//===-- AltairXISelLowering.cpp - AltairX DAG Lowering Implementation
//-----------===//
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

#include "AltairXCommon.h"
#include "AltairXISelLowering.h"
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

AltairXTargetLowering::AltairXTargetLowering(const TargetMachine &TM,
                                             const AltairXSubtarget &STI)
    : TargetLowering(TM), Subtarget(STI) {

  static constexpr std::array<llvm::MVT, 4> AllIntsMVT = {MVT::i8, MVT::i16, MVT::i32, MVT::i64};

  // Set up the register classes
  addRegisterClass(MVT::i8, &AltairX::GPIReg8RegClass);
  addRegisterClass(MVT::i16, &AltairX::GPIReg16RegClass);
  addRegisterClass(MVT::i32, &AltairX::GPIReg32RegClass);
  addRegisterClass(MVT::i64, &AltairX::GPIReg64RegClass);

  // Must, computeRegisterProperties - Once all of the register classes are
  // added, this allows us to compute derived properties we expose.
  computeRegisterProperties(Subtarget.getRegisterInfo());

  // setStackPointerRegisterToSaveRestore(AltairX::X2);

  setSchedulingPreference(Sched::RegPressure);
  

  // Use i32 for setcc operations results (slt, sgt, ...).
  setBooleanContents(ZeroOrOneBooleanContent);
  // setBooleanVectorContents(ZeroOrOneBooleanContent);

  setOperationAction(ISD::GlobalAddress, MVT::i32, LegalizeAction::Custom);
  setOperationAction(ISD::BlockAddress, MVT::i32, LegalizeAction::Custom);
  setOperationAction(ISD::ConstantPool, MVT::i32, LegalizeAction::Custom);

  setOperationAction(ISD::BRCOND, MVT::Other, Custom);
  // Decompose BR_CC in SETCC + BRCOND to later generate a cmp + bx
  setOperationAction(ISD::BR_CC, AllIntsMVT, LegalizeAction::Expand);
  setOperationAction(ISD::SELECT_CC, AllIntsMVT, LegalizeAction::Custom);

  setOperationAction(ISD::Constant, AllIntsMVT, LegalizeAction::Legal);

  // Set minimum and preferred function alignment, and loop alignment (log2)
  setMinFunctionAlignment(Align{1});
  setPrefFunctionAlignment(Align{1});
  setPrefLoopAlignment(Align{1});
}

const char *AltairXTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case AltairXISD::RET:
    return "AltairXISD::RET";
  case AltairXISD::BRCOND:
    return "AltairXISD::BRCOND";
  case AltairXISD::SCMP:
    return "AltairXISD::SCMP";
  case AltairXISD::CMOVE:
    return "AltairXISD::CMOVE";
  default:
    return nullptr;
  }
}

void AltairXTargetLowering::ReplaceNodeResults(
    SDNode *N, SmallVectorImpl<SDValue> &Results, SelectionDAG &DAG) const {
  llvm_unreachable("Don't know how to custom expand this!");
  // switch (N->getOpcode()) {
  // default:
  // }
}

//===----------------------------------------------------------------------===//
//@            Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

/// LowerFormalArguments - transform physical registers into virtual registers
/// and generate load operations for arguments places on the stack.
SDValue AltairXTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {

  static constexpr MCPhysReg GPRArgRegs[] = {
      AltairX::R1, AltairX::R2, AltairX::R3, AltairX::R4,
      AltairX::R5, AltairX::R6, AltairX::R7, AltairX::R8};

  assert(CallConv == CallingConv::C &&
         "Unsupported CallingConv to FORMAL_ARGS");

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign> argLocs;
  CCState CCInfo{CallConv, IsVarArg, DAG.getMachineFunction(), argLocs,
                 *DAG.getContext()};
  CCInfo.AnalyzeFormalArguments(Ins, AltairX_CCallingConv);
  
  // Calculate the amount of stack space that we need to allocate to store
  // byval and variadic arguments that are passed in registers.
  // We need to know this before we allocate the first byval or variadic
  // argument, as they will be allocated a stack slot below the CFA (Canonical
  // Frame Address, the stack pointer at entry to the function).
  unsigned int argRegBegin = AltairX::R1;
  for (CCValAssign& val : argLocs) {
    if (CCInfo.getInRegsParamsProcessed() >= CCInfo.getInRegsParamsCount()) {
      break;
    }

    if (!Ins[val.getValNo()].Flags.isByVal()) {
      continue;
    }

    assert(val.isMemLoc() && "unexpected byval pointer in reg");

    unsigned int begin{};
    unsigned int end{};
    CCInfo.getInRegsParamInfo(CCInfo.getInRegsParamsProcessed(), begin, end);

    argRegBegin = std::min(argRegBegin, begin);
    CCInfo.nextInRegsParam();
  }
  CCInfo.rewindByValRegsInfo();
  
  if (IsVarArg && MFI.hasVAStart()) {
    const auto reg = CCInfo.getFirstUnallocated(GPRArgRegs);
    if (reg != std::size(GPRArgRegs)) {
      argRegBegin = std::min(argRegBegin, static_cast<unsigned int>(GPRArgRegs[reg]));
    }
  }

  auto origArg = MF.getFunction().arg_begin();
  for (CCValAssign& VA : argLocs) {
    unsigned int argIdx{};
    if (Ins[VA.getValNo()].isOrigArg()) {
      std::advance(origArg, Ins[VA.getValNo()].getOrigArgIndex() - argIdx);
      argIdx = Ins[VA.getValNo()].getOrigArgIndex();
    }

    // Arguments stored in registers.
    if (VA.isRegLoc()) {
      if(VA.needsCustom()) {
        llvm_unreachable("Custom val not supported by Args Lowering");
      }

      const MVT type = VA.getLocVT();
      const auto* RC = AltairXRegisterInfo::MVTRegClass(type);
      if(!RC) {
        llvm_unreachable("Invalid MTV for argument");
      }

      // Transform the arguments in physical registers into virtual ones.
      const auto reg = MF.addLiveIn(VA.getLocReg(), RC);
      SDValue arg = DAG.getCopyFromReg(Chain, DL, reg, type);

      // If this is an 8, 16 or 32 bits value, it is received as a 64 bits int.
      // Insert an assert[sz]ext to capture this, then
      // truncate to the right size.
      switch (VA.getLocInfo()) {
      case CCValAssign::Full:
        break;
      case CCValAssign::BCvt:
        arg = DAG.getNode(ISD::BITCAST, DL, VA.getValVT(), arg);
        break;
      case CCValAssign::SExt:
        arg = DAG.getNode(ISD::AssertSext, DL, type, arg,
                          DAG.getValueType(VA.getValVT()));
        arg = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), arg);
        break;
      case CCValAssign::ZExt:
        arg = DAG.getNode(ISD::AssertZext, DL, type, arg,
                          DAG.getValueType(VA.getValVT()));
        arg = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), arg);
        break;
      case CCValAssign::AExt:
        arg = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), arg);
        break;
      default:
        llvm_unreachable("Unknown location info");
      }

      InVals.push_back(arg);
    } else {
      // sanity check
      assert(VA.isMemLoc());
      assert(VA.getValVT() != MVT::i64 && "i64 should already be lowered");
    }
  }

  return Chain;
}

//===----------------------------------------------------------------------===//
//@              Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

bool AltairXTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool isVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, MF, RVLocs, Context);
  return CCInfo.CheckReturn(Outs, AltairX_CRetConv);
}

/// LowerMemOpCallTo - Store the argument to the stack.
SDValue AltairXTargetLowering::LowerMemOpCallTo(SDValue Chain, SDValue Arg,
                                                const SDLoc &dl,
                                                SelectionDAG &DAG,
                                                const CCValAssign &VA,
                                                ISD::ArgFlagsTy Flags) const {
  llvm_unreachable("Cannot store arguments to stack");
}

/// LowerCallResult - Lower the result values of a call into the
/// appropriate copies out of appropriate physical registers.
SDValue AltairXTargetLowering::LowerCallResult(
    SDValue Chain, SDValue InFlag, CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals, bool isThisReturn,
    SDValue ThisVal) const {
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins, AltairX_CRetConv);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign VA = RVLocs[i];

    // Pass 'this' value directly from the argument to return value, to avoid
    // reg unit interference
    if (i == 0 && isThisReturn) {
      assert(!VA.needsCustom() && VA.getLocVT() == MVT::i32 &&
             "unexpected return calling convention register assignment");
      InVals.push_back(ThisVal);
      continue;
    }

    SDValue Val;
    if (VA.needsCustom()) {
      llvm_unreachable("Vector and floating point values not supported yet");
    } else {
      Val =
          DAG.getCopyFromReg(Chain, dl, VA.getLocReg(), VA.getLocVT(), InFlag);
      Chain = Val.getValue(1);
      InFlag = Val.getValue(2);
    }

    switch (VA.getLocInfo()) {
    default:
      llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full:
      break;
    case CCValAssign::BCvt:
      Val = DAG.getNode(ISD::BITCAST, dl, VA.getValVT(), Val);
      break;
    }

    InVals.push_back(Val);
  }

  return Chain;
}

SDValue
AltairXTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                 SmallVectorImpl<SDValue> &InVals) const {
  llvm_unreachable("Cannot lower call");
}

/// HandleByVal - Every parameter *after* a byval parameter is passed
/// on the stack.  Remember the next parameter register to allocate,
/// and then confiscate the rest of the parameter registers to insure
/// this.
void AltairXTargetLowering::HandleByVal(CCState *State, unsigned int& Size,
                                        Align Align) const {
  /*
// Byval (as with any stack) slots are always at least 4 byte aligned.

unsigned Reg = State->AllocateReg(GPRArgRegs);
if (!Reg)
  return;

unsigned AlignInRegs = Align / 4;
unsigned Waste = (AltairX::X4 - Reg) % AlignInRegs;
for (unsigned i = 0; i < Waste; ++i)
  Reg = State->AllocateReg(GPRArgRegs);

if (!Reg)
  return;

unsigned Excess = 4 * (AltairX::X4 - Reg);

// Special case when NSAA != SP and parameter size greater than size of
// all remained GPR regs. In that case we can't split parameter, we must
// send it to stack. We also must set NCRN to X4, so waste all
// remained registers.
const unsigned NSAAOffset = State->getNextStackOffset();
if (NSAAOffset != 0 && Size > Excess) {
  while (State->AllocateReg(GPRArgRegs))
    ;
  return;
}

// First register for byval parameter is the first register that wasn't
// allocated before this method call, so it would be "reg".
// If parameter is small enough to be saved in range [reg, r4), then
// the end (first after last) register would be reg + param-size-in-regs,
// else parameter would be splitted between registers and stack,
// end register would be r4 in this case.
unsigned ByValRegBegin = Reg;
unsigned ByValRegEnd = std::min<unsigned>(Reg + Size / 4, AltairX::X4);
State->addInRegsParamInfo(ByValRegBegin, ByValRegEnd);
// Note, first register is allocated in the beginning of function already,
// allocate remained amount of registers we need.
for (unsigned i = Reg + 1; i != ByValRegEnd; ++i)
  State->AllocateReg(GPRArgRegs);
// A byval parameter that is split between registers and memory needs its
// size truncated here.
// In the case where the entire structure fits in registers, we set the
// size in memory to zero.
Size = std::max<int>(Size - Excess, 0);
*/
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
  CCState CCInfo{CallConv, IsVarArg, DAG.getMachineFunction(), retLocs,
    *DAG.getContext()};

  CCInfo.AnalyzeReturn(Outs, AltairX_CRetConv);

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
  if(flag.getNode()) {
    retOps.push_back(flag);
  }

  return DAG.getNode(AltairXISD::RET, DL, MVT::Other, retOps);
}

//===----------------------------------------------------------------------===//
//  Lower helper functions
//===----------------------------------------------------------------------===//

SDValue AltairXTargetLowering::getGlobalAddressWrapper(
    SDValue GA, const GlobalValue *GV, SelectionDAG &DAG) const {
  llvm_unreachable("Unhandled global variable");
}

//===----------------------------------------------------------------------===//
//  Misc Lower Operation implementation
//===----------------------------------------------------------------------===//

SDValue AltairXTargetLowering::LowerGlobalAddress(SDValue Op,
                                                  SelectionDAG &DAG) const {
  llvm_unreachable("Unsupported global address");
}

SDValue AltairXTargetLowering::LowerConstantPool(SDValue Op,
                                                 SelectionDAG &DAG) const {
  llvm_unreachable("Unsupported constant pool");
}

SDValue AltairXTargetLowering::LowerBlockAddress(SDValue Op,
                                                 SelectionDAG &DAG) const {
  llvm_unreachable("Unsupported block address");
}

SDValue AltairXTargetLowering::LowerReturnAddr(SDValue Op,
                                               SelectionDAG &DAG) const {
  return SDValue();
}

namespace {

AltairX::CondCode toAltairXCondCode(ISD::CondCode value) {
  switch(value) {
  case ISD::SETUEQ:
    return AltairX::CondCode::EQ;
  case ISD::SETUGT:
    return AltairX::CondCode::G;
  case ISD::SETUGE:
    return AltairX::CondCode::GE;
  case ISD::SETULT:
    return AltairX::CondCode::L;
  case ISD::SETULE:
    return AltairX::CondCode::LE;
  case ISD::SETUNE:
    return AltairX::CondCode::NE;
  case ISD::SETEQ:
    return AltairX::CondCode::EQ;
  case ISD::SETGT:
    return AltairX::CondCode::GS;
  case ISD::SETGE:
    return AltairX::CondCode::GES;
  case ISD::SETLT:
    return AltairX::CondCode::LS;
  case ISD::SETLE:
    return AltairX::CondCode::LES;
  case ISD::SETNE:
    return AltairX::CondCode::NE;
  default:
    llvm_unreachable("Unsupported ISD::CondCode");
    break;
  }
}

AltairX::SCMPCondCode computeSelectOperands(SDValue &Left, SDValue &Right,
                                            SDValue &TVal, SDValue &FVal,
                                            ISD::CondCode CC) {
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

  switch(CC) {
  case ISD::SETUEQ: [[fallthrough]];
  case ISD::SETEQ: // nothing to do
    return AltairX::SCMPCondCode::EQ;
  case ISD::SETUNE: [[fallthrough]];
  case ISD::SETNE:
    std::swap(TVal, FVal);
    return AltairX::SCMPCondCode::EQ;
  case ISD::SETUGT:
    std::swap(Left, Right);
    return AltairX::SCMPCondCode::L;
  case ISD::SETUGE:
    std::swap(TVal, FVal);
    return AltairX::SCMPCondCode::L;
  case ISD::SETULT: // nothing to do
    return AltairX::SCMPCondCode::L;
  case ISD::SETULE:
    std::swap(TVal, FVal);
    std::swap(Left, Right);
    return AltairX::SCMPCondCode::L;
  case ISD::SETGT:
    std::swap(Left, Right);
    return AltairX::SCMPCondCode::LS;
  case ISD::SETGE:
    std::swap(TVal, FVal);
    return AltairX::SCMPCondCode::LS;
  case ISD::SETLT: // nothing to do
    return AltairX::SCMPCondCode::LS;
  case ISD::SETLE:
    std::swap(TVal, FVal);
    std::swap(Left, Right);
    return AltairX::SCMPCondCode::LS;
  default:
    llvm_unreachable("Unsupported ISD::CondCode");
    break;
  }
}

}

SDValue AltairXTargetLowering::LowerSELECT_CC(SDValue Op,
                                              SelectionDAG &DAG) const {
  SDValue left = Op.getOperand(0);
  SDValue right = Op.getOperand(1);
  SDValue tval = Op.getOperand(2);
  SDValue fval = Op.getOperand(3);
  const ISD::CondCode cc = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  
  const auto nativecc = static_cast<std::uint64_t>(
      computeSelectOperands(left, right, tval, fval, cc));
  const auto type = tval.getValueType();
  assert(type == fval.getValueType());

  SDLoc dl{Op};
  SDValue ccval = DAG.getConstant(nativecc, dl, MVT::i64);
  SDValue scmp = DAG.getNode(AltairXISD::SCMP, dl, MVT::i64, left, right, ccval);
  return DAG.getNode(AltairXISD::CMOVE, dl, type, fval, tval, scmp);
}

SDValue AltairXTargetLowering::LowerBRCOND(SDValue Op,
                                           SelectionDAG &DAG) const {
  SDValue chain = Op.getOperand(0);
  SDValue cond = Op.getOperand(1);
  SDValue dest = Op.getOperand(2);

  if(cond->getOpcode() == ISD::SETCC) {
    const ISD::CondCode cc = cast<CondCodeSDNode>(cond->getOperand(2))->get();
    const auto nativecc = static_cast<std::uint64_t>(toAltairXCondCode(cc));

    SDLoc DL{cond};
    SDValue ccval = DAG.getConstant(nativecc, DL, MVT::i32);
    return DAG.getNode(AltairXISD::BRCOND, DL, MVT::Other, chain, dest, ccval, cond);
  }

  llvm_unreachable("Unsupported BRCOND conditional operand");
}

SDValue AltairXTargetLowering::LowerOperation(SDValue Op,
                                              SelectionDAG &DAG) const {
  LLVM_DEBUG(dbgs() << "Lowering: ");
  LLVM_DEBUG(Op.dump());

  switch (Op.getOpcode()) {
  case ISD::BRCOND:
    return LowerBRCOND(Op, DAG);
  case ISD::SELECT_CC:
    return LowerSELECT_CC(Op, DAG);
  case ISD::GlobalAddress:
    return LowerGlobalAddress(Op, DAG);
  case ISD::BlockAddress:
    return LowerBlockAddress(Op, DAG);
  case ISD::ConstantPool:
    return LowerConstantPool(Op, DAG);
  case ISD::RETURNADDR:
    return LowerReturnAddr(Op, DAG);
  default:
    llvm_unreachable("unimplemented operand");
  }
}
