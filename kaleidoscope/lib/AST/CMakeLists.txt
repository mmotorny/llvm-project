# The AST node definitions and their printing, named after clang's clangAST.
set(LLVM_LINK_COMPONENTS Support)

add_llvm_library(kaleidoscopeAST BUILDTREE_ONLY
  Expr.cpp
)
