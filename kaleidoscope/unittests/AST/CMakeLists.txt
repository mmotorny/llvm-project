add_unittest(KaleidoscopeTests KaleidoscopeASTTests
  ExprTest.cpp
)
target_link_libraries(KaleidoscopeASTTests PRIVATE kaleidoscopeAST)
