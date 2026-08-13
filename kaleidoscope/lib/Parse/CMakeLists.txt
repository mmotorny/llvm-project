# The parsing phase, named after clang's clangParse: tokens in, AST out.
set(LLVM_LINK_COMPONENTS Support)

add_llvm_library(kaleidoscopeParse BUILDTREE_ONLY
  Parser.cpp

  LINK_LIBS
  kaleidoscopeAST
  kaleidoscopeLex
)
