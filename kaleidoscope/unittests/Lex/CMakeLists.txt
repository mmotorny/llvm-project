add_unittest(KaleidoscopeTests KaleidoscopeLexTests
  LexerTest.cpp
)
target_link_libraries(KaleidoscopeLexTests PRIVATE kaleidoscopeLex)
