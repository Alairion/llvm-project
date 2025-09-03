add_llvm_component_library(LLVMAltairXDesc
  AltairXELFObjectWriter.cpp
  AltairXELFStreamer.cpp
  AltairXMCAsmBackend.cpp
  AltairXMCAsmInfo.cpp
  AltairXMCTargetDesc.cpp
  AltairXMCCodeEmitter.cpp
  AltairXInstPrinter.cpp
  AltairXOptionRecord.cpp

  LINK_COMPONENTS
  MC
  AltairXInfo
  Support

  ADD_TO_COMPONENT 
  AltairX
)

add_dependencies(LLVMAltairXDesc AltairXCommonTableGen)