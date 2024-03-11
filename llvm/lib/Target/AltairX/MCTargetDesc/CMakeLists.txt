add_llvm_component_library(LLVMAltairXDesc
  AltairXELFObjectWriter.cpp
  AltairXMCAsmBackend.cpp
  AltairXMCAsmInfo.cpp
  AltairXMCTargetDesc.cpp
  AltairXMCCodeEmitter.cpp
  AltairXInstPrinter.cpp

  LINK_COMPONENTS
  MC
  AltairXInfo
  Support

  ADD_TO_COMPONENT 
  AltairX
)

add_dependencies(LLVMAltairXDesc AltairXCommonTableGen)