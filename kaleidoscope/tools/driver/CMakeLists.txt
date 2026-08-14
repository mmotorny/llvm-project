# The driver executable: the directory is "driver", the binary takes the
# project's name — clang's layout exactly (clang/tools/driver builds
# `clang`). LineEditor is LLVM's REPL input library (prompt, line editing,
# history), the one clang-query uses.
# OrcJIT is the JIT the REPL evaluates through; Passes carries the -O
# pipeline; "native" expands to the host target's backend, which the JIT
# compiles with.
set(LLVM_LINK_COMPONENTS Core LineEditor OrcJIT Passes Support native)

add_llvm_executable(kaleidoscope
  driver.cpp
)
target_link_libraries(kaleidoscope
  PRIVATE kaleidoscopeAST kaleidoscopeCodeGen kaleidoscopeLex
          kaleidoscopeParse)
