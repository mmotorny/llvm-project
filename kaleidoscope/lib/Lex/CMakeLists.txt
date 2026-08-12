# The lexing phase, named after clang's clangLex. BUILDTREE_ONLY: never
# installed.
add_llvm_library(kaleidoscopeLex BUILDTREE_ONLY
  Lexer.cpp
)
