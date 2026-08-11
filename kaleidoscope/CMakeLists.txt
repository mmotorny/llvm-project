# Kaleidoscope builds as part of the LLVM tree, hooked in as an LLVM
# "external project" (see README.md for the configure line). That gives us
# LLVM's CMake machinery: add_unittest() below links LLVM's vendored
# googletest (llvm_gtest) plus the LLVMSupport library it depends on.

# Suite target grouping all Kaleidoscope unit tests: `ninja KaleidoscopeTests`
# builds every test binary registered against it.
add_custom_target(KaleidoscopeTests)
set_target_properties(KaleidoscopeTests PROPERTIES FOLDER "Kaleidoscope/Tests")

add_unittest(KaleidoscopeTests KaleidoscopeLexerTest
  lexer_test.cpp
)
