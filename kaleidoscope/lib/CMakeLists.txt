# The compiler itself: a static library that the unit tests and (from step 2)
# the interactive driver link against. BUILDTREE_ONLY: never installed.
add_llvm_library(KaleidoscopeCore BUILDTREE_ONLY
  Lexer.cpp
)
