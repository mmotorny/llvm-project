# Suite target grouping all Kaleidoscope unit tests: `ninja KaleidoscopeTests`
# builds every test binary registered against it.
add_custom_target(KaleidoscopeTests)
set_target_properties(KaleidoscopeTests PROPERTIES FOLDER "Kaleidoscope/Tests")

add_unittest(KaleidoscopeTests KaleidoscopeLexerTest
  LexerTest.cpp
)
target_link_libraries(KaleidoscopeLexerTest PRIVATE KaleidoscopeCore)
