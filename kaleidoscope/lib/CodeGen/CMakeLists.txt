# The IR-generation phase, named after clang's clangCodeGen: AST in,
# LLVM IR out. Core is the LLVM component with the IR classes themselves
# (Module, Function, IRBuilder).
set(LLVM_LINK_COMPONENTS Core Support)

add_llvm_library(kaleidoscopeCodeGen BUILDTREE_ONLY
  CodeGen.cpp

  LINK_LIBS
  kaleidoscopeAST
)
