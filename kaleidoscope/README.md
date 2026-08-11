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

All the code lives in a single file, `toy.cpp`, which grows with each step —
so each PR's diff is exactly what the step adds.

We follow the tutorial's *structure* but not its style: where the tutorial
uses globals and function-local statics for simplicity, we keep state in
classes (e.g. the lexer is a `Lexer` instance over a `std::istream`) and use
C++23 idioms (`std::variant` tokens, `std::println`) where they make the code
safer or clearer.

## Building and running

Steps 1–2 (lexer, parser) need only a C++ compiler. We build as C++23 (LLVM
itself requires only C++17, but permits newer standards):

```
clang++ -std=c++23 toy.cpp -o toy
echo 'def fib(x) fib(x-1)+fib(x-2)' | ./toy
```

From step 3 on (LLVM IR generation) the build line will link against LLVM
libraries; the instructions here will be updated then.

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
