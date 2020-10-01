add_llvm_component_library(LLVMAltairXDesc
  AltairXMCAsmInfo.cpp
  AltairXMCTargetDesc.cpp
  AltairXInstPrinter.cpp

  LINK_COMPONENTS
  MC
  AltairXInfo
  Support

  ADD_TO_COMPONENT 
  AltairX
)

add_dependencies(LLVMAltairXDesc AltairXCommonTableGen)