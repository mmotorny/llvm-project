# Kaleidoscope builds as part of the LLVM tree, hooked in as an LLVM
# "external project" (see README.md for the configure line). That gives us
# LLVM's CMake machinery: add_unittest() below links LLVM's vendored
# googletest (llvm_gtest) plus the LLVMSupport library it depends on.

# Suite target grouping all Kaleidoscope unit tests: `ninja KaleidoscopeTests`
# builds every test binary registered against it.
add_custom_target(KaleidoscopeTests)
set_target_properties(KaleidoscopeTests PROPERTIES FOLDER "Kaleidoscope/Tests")

# DISABLE_PCH_REUSE: LLVM shares LLVMSupport's precompiled header with
# dependent targets, but that PCH is built as C++17 and our C++23 translation
# units cannot consume it.
add_unittest(KaleidoscopeTests KaleidoscopeLexerTest DISABLE_PCH_REUSE
  lexer_test.cpp
)

# LLVM builds as C++17; this project deliberately uses C++23.
set_target_properties(KaleidoscopeLexerTest PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
)
