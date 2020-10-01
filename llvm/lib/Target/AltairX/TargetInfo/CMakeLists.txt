add_llvm_component_library(LLVMAltairXInfo
  AltairXTargetInfo.cpp
  
  LINK_COMPONENTS
  Support
  
  ADD_TO_COMPONENT 
  AltairX
)

add_dependencies(LLVMAltairXInfo AltairXCommonTableGen)