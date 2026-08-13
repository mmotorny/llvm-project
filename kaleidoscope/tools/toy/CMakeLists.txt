# The driver executable, named "toy" after the tutorial's. LineEditor is
# LLVM's REPL input library (prompt, line editing, history), the one
# clang-query uses.
set(LLVM_LINK_COMPONENTS LineEditor Support)

add_llvm_executable(toy
  toy.cpp
)
target_link_libraries(toy
  PRIVATE kaleidoscopeAST kaleidoscopeLex kaleidoscopeParse)
