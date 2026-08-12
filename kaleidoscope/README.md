# Kaleidoscope

A from-scratch implementation of the Kaleidoscope toy language, following the
official LLVM tutorial (https://llvm.org/docs/tutorial/), built step by step as
a learning project. Each step lands as a reviewed PR in this fork.

Kaleidoscope is a tiny language: the only datatype is a 64-bit floating point
number, so there are no type declarations anywhere. It has function
definitions (`def`), declarations of external functions (`extern`), arithmetic
expressions, and (in later steps) if/then/else, loops, user-defined operators,
and mutable variables. Example of what a finished program looks like:

```
# Compute the x'th fibonacci number.
def fib(x)
  if x < 3 then
    1
  else
    fib(x-1) + fib(x-2)

fib(40)
```

The directory layout and library naming mirror clang's (LLVM's build system
enforces one target per directory, so a flat layout isn't even possible):
each compiler phase is a static library `kaleidoscope<Phase>` — like
`clangLex`, `clangParse`, `clangAST` in clang — with matching header and
test directories:

- `include/kaleidoscope/<Phase>/` — public headers (`Lex/Lexer.h`, ...)
- `lib/<Phase>/` — the implementation (`Lex/` builds `kaleidoscopeLex`, ...)
- `unittests/<Phase>/` — googletest unit tests (`Lex/LexerTest.cpp`, ...)

Each step adds its stage across those three directories, so each PR's diff
is exactly what the step adds. The interactive driver, `toy.cpp`, arrives in
step 2 once there is a parser to drive.

We follow the tutorial's *structure* but not its style: where the tutorial
uses globals and function-local statics for simplicity, we keep state in
classes and model data the way the production compilers in this repo do —
tokens, for example, follow clang's design (a flat `Token` of kind +
spelling borrowed from the source buffer; every keyword and punctuator its
own `tok::` kind) rather than the tutorial's int-or-global scheme. We build
as C++17, the same standard as the LLVM tree we're part of, and prefer
LLVM's own vocabulary types (`llvm::Expected`, `llvm::StringRef`, ...) over
newer standard-library equivalents — learning them is part of learning
LLVM.

## Building and testing

Kaleidoscope is wired into the LLVM build as an LLVM *external project*, the
same way in-tree LLVM code is built and tested. One-time configure from the
repo root (Release with assertions is the standard development
configuration; X86 is the only backend we need):

```
cmake -S llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DLLVM_EXTERNAL_PROJECTS=kaleidoscope \
  -DLLVM_EXTERNAL_KALEIDOSCOPE_SOURCE_DIR="$PWD/kaleidoscope" \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

(`LLVM_EXTERNAL_PROJECTS` registers the project; the `SOURCE_DIR` flag says
where it lives, needed because external projects are otherwise expected at
`llvm/tools/<name>` and ours sits at the repository root.)

Build and run the unit tests (the first build also compiles the LLVMSupport
library and LLVM's vendored googletest, which LLVM unit tests link against):

```
ninja -C build KaleidoscopeTests
build/tools/kaleidoscope/unittests/Lex/KaleidoscopeLexTests
```

Testing follows LLVM's own two-tier convention: **googletest unit tests**
for C++ APIs (what we have now), and — once the compiler emits IR in step
3+ — **lit + FileCheck** regression tests that run the compiler on sample
programs and check its output.

## Progress

- [x] Step 1: Lexer — turn the raw character stream into tokens
- [ ] Step 2: AST and expression parser
- [ ] Step 3: Parser for functions and the interactive driver loop
- [ ] Step 4: Code generation to LLVM IR
- [ ] Step 5: Optimization passes and JIT compilation
- [ ] Step 6: Control flow — if/then/else
- [ ] Step 7: Control flow — for loops
- [ ] Step 8: User-defined operators
- [ ] Step 9: Mutable variables
- [ ] Step 10: Compiling to object files
