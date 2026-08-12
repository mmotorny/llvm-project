# The lexing phase, named after clang's clangLex. BUILDTREE_ONLY: never
# installed.
set(LLVM_LINK_COMPONENTS Support)

add_llvm_library(kaleidoscopeLex BUILDTREE_ONLY
  Lexer.cpp
  Token.cpp
)
