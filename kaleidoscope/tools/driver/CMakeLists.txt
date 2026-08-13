# The driver executable: the directory is "driver", the binary takes the
# project's name — clang's layout exactly (clang/tools/driver builds
# `clang`). LineEditor is LLVM's REPL input library (prompt, line editing,
# history), the one clang-query uses.
set(LLVM_LINK_COMPONENTS LineEditor Support)

add_llvm_executable(kaleidoscope
  driver.cpp
)
target_link_libraries(kaleidoscope
  PRIVATE kaleidoscopeAST kaleidoscopeLex kaleidoscopeParse)
