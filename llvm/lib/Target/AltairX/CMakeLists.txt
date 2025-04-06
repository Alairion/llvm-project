add_llvm_component_group(AltairX)

set(LLVM_TARGET_DEFINITIONS AltairX.td)

tablegen(LLVM AltairXGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM AltairXGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM AltairXGenCodeEmitter.inc -gen-emitter)
tablegen(LLVM AltairXGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM AltairXGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM AltairXGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM AltairXGenCallingConv.inc -gen-callingconv)
tablegen(LLVM AltairXGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM AltairXGenMCPseudoLowering.inc -gen-pseudo-lowering)

add_public_tablegen_target(AltairXCommonTableGen)

add_llvm_target(AltairXCodeGen
  AltairXAsmPrinter.cpp
  AltairXFrameLowering.cpp
  AltairXInstrInfo.cpp
  AltairXISelLowering.cpp
  AltairXRegisterInfo.cpp
  AltairXSubtarget.cpp
  AltairXTargetObjectFile.cpp
  AltairXISelDAGToDAG.cpp
  AltairXTargetMachine.cpp
  
  LINK_COMPONENTS
  AltairXDesc
  AltairXInfo
  AsmPrinter
  CodeGen
  Core
  MC
  SelectionDAG
  Support
  Target

  ADD_TO_COMPONENT
  AltairX
)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
