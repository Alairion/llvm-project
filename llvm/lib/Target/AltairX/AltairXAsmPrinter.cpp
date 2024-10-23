//===-- AltairXAsmPrinter.cpp - AltairX LLVM Assembly Printer -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format CPU0 assembly language.
//
//===----------------------------------------------------------------------===//

#include "AltairXInstrInfo.h"
#include "AltairXTargetMachine.h"
#include "MCTargetDesc/AltairXInstPrinter.h"
#include "TargetInfo/AltairXTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "altairx-asm-printer"

namespace llvm {
class AltairXAsmPrinter : public AsmPrinter {
public:
  AltairXAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  virtual StringRef getPassName() const override {
    return "AltairX Assembly Printer";
  }

  virtual void SetupMachineFunction(MachineFunction &MF) override {
    AsmPrinter::SetupMachineFunction(MF);
    Subtarget = &MF.getSubtarget<AltairXSubtarget>();
  }

  // This function must be present as it is internally used by the
  // auto-generated function emitPseudoExpansionLowering to expand pseudo
  // instruction
  void EmitToStreamer(MCStreamer &S, const MCInst &Inst);
  // Auto-generated function in AltairXGenMCPseudoLowering.inc
  bool emitPseudoExpansionLowering(MCStreamer &OutStreamer,
                                   const MachineInstr *MI);

  // Emit a bundle or a single instruction to associated streamer
  void emitInstruction(const MachineInstr* MI) override;

private:
  void LowerInstruction(const MachineInstr *MI, MCInst &OutMI) const;
  MCOperand LowerOperand(const MachineOperand &MO) const;
  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;

  const AltairXSubtarget *Subtarget{};
};
} // namespace llvm

// Simple pseudo-instructions have their lowering (with expansion to real
// instructions) auto-generated.
#include "AltairXGenMCPseudoLowering.inc"
void AltairXAsmPrinter::EmitToStreamer(MCStreamer &S, const MCInst &Inst) {
  AsmPrinter::EmitToStreamer(*OutStreamer, Inst);
}

void AltairXAsmPrinter::emitInstruction(const MachineInstr *MI) {
  // Do any auto-generated pseudo lowerings.
  if (emitPseudoExpansionLowering(*OutStreamer, MI)) {
    return;
  }

  if (MI->isBundle()) {
    const MachineBasicBlock* MBB = MI->getParent();

    // temporary bundle, will be dropper after being emitted by OutStreamer
    MCInst bundle;
    bundle.setOpcode(AltairX::BUNDLE);
    std::array<MCInst, 2> content; // buffer for bundle instructions (max 2)
    auto nextContent = content.begin(); // used to fill content and bound-check

    for (auto MII = std::next(MI->getIterator()); // start at first BUNDLE MI
         MII->isInsideBundle() && MII != MBB->instr_end(); ++MII) {
      if (!MII->isDebugInstr() && !MII->isImplicitDef()) {
        assert(nextContent != content.end() &&
               "Bundle constains more than 2 instructions!");

        LowerInstruction(to_address(MII), *nextContent);
        bundle.addOperand(MCOperand::createInst(to_address(nextContent)));
        ++nextContent;
      }
    }

    EmitToStreamer(*OutStreamer, bundle);
  } else {
    MCInst TmpInst;
    LowerInstruction(MI, TmpInst);
    EmitToStreamer(*OutStreamer, TmpInst);
  }
}

void AltairXAsmPrinter::LowerInstruction(const MachineInstr *MI,
                                         MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp = LowerOperand(MO);
    if (MCOp.isValid()) {
      OutMI.addOperand(MCOp);
    }
  }
}

MCOperand AltairXAsmPrinter::LowerOperand(const MachineOperand &MO) const {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit()) {
      break;
    }
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_MachineBasicBlock:
    return LowerSymbolOperand(MO, MO.getMBB()->getSymbol());
  case MachineOperand::MO_GlobalAddress:
    return LowerSymbolOperand(MO, getSymbol(MO.getGlobal()));
  case MachineOperand::MO_BlockAddress:
    return LowerSymbolOperand(MO, GetBlockAddressSymbol(MO.getBlockAddress()));
  case MachineOperand::MO_ExternalSymbol:
    return LowerSymbolOperand(MO, GetExternalSymbolSymbol(MO.getSymbolName()));
  case MachineOperand::MO_ConstantPoolIndex:
    return LowerSymbolOperand(MO, GetCPISymbol(MO.getIndex()));
  case MachineOperand::MO_RegisterMask:
    break;
  default:
    llvm_unreachable("unknown operand type");
  }

  return MCOperand();
}

MCOperand AltairXAsmPrinter::LowerSymbolOperand(const MachineOperand &MO,
                                                MCSymbol *Sym) const {
  MCContext &Ctx = OutContext;

  const MCExpr *Expr =
      MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx);

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset()) {
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
    llvm_unreachable("LowerSymbolOperand is not supported yet");
  }

  return MCOperand::createExpr(Expr);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAltairXAsmPrinter() {
  RegisterAsmPrinter<AltairXAsmPrinter> X(getTheAltairXTarget());
}
