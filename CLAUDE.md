# Kaleidoscope — Claude's working notes

This file is Claude's cross-session context, kept in the repo by Maksym's
request so it stays visible and reviewable. Claude: keep it current — update
it in the same PR as the work it describes, and do not use the hidden
per-user memory directory.

## What this project is

A learning project: build the Kaleidoscope compiler (the toy language from
the official LLVM tutorial) inside this fork, as a vehicle for learning LLVM
and its idioms. Maksym has not read the tutorial — explain every concept
from zero in PR descriptions and chat, including the "why" behind each LLVM
convention. The roadmap is the checklist in `kaleidoscope/README.md`.

## Workflow

- Every change lands as a PR against `main` of `mmotorny/llvm-project`
  (**never** upstream `llvm/llvm-project`), merged only after Maksym's
  review: `gh pr create --repo mmotorny/llvm-project --base main`.
- Address review comments with code changes, or with evidence-backed
  pushback — always verified against this tree (grep clang/MLIR/LLVM, cite
  file:line), never from memory. Reply in the PR threads; Maksym says
  "Ptal" when a round of comments is ready.
- Branches: `kaleidoscope/NN-topic` for tutorial steps,
  `kaleidoscope/topic` for refactors. Delete branches after merge.

## Build & test

Configured once as an LLVM external project (Release + assertions, X86
only, Homebrew clang; build dir `build/` at repo root):

```
cmake -S llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DLLVM_EXTERNAL_PROJECTS=kaleidoscope \
  -DLLVM_EXTERNAL_KALEIDOSCOPE_SOURCE_DIR="$PWD/kaleidoscope" \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
ninja -C build KaleidoscopeTests
build/tools/kaleidoscope/unittests/{Lex,AST,Parse}/Kaleidoscope*Tests
```

## Design conventions (settled through review — don't relitigate silently)

- **C++17**, matching LLVM's own standard. Prefer LLVM vocabulary types
  (`StringRef`, `ArrayRef`, `Expected`, `raw_ostream`) over newer std
  equivalents; learning them is part of the goal.
- **Layout and naming mirror clang**: per-phase libraries `kaleidoscopeLex`
  / `kaleidoscopeAST` / `kaleidoscopeParse` in `kaleidoscope/lib/<Phase>/`,
  headers in `include/kaleidoscope/<Phase>/`, tests in
  `unittests/<Phase>/`, CamelCase files, everything in `namespace
  kaleidoscope` (nested namespaces only when semantic, e.g. `tok`).
- **Tokens** are production-model (after clang/MLIR): flat trivially
  copyable `Token` = kind + spelling (`StringRef` borrowing the source
  buffer; zero allocation), per-kind `tok::` enum including punctuators
  (`kw_def`, `l_paren`, ...), assert-guarded accessors, lazy number
  decoding. Lexer walks a caller-owned `StringRef` buffer (`Cur`/`End`
  pointers); sticky `tok::eof`; keywords via `StringSwitch`.
- **AST** is clang-shaped: vtable-free nodes (no virtuals at all; protected
  non-virtual base dtor) allocated in a caller-owned
  `llvm::BumpPtrAllocator` (`new (Alloc) NumberExpr(...)` — LLVM's stock
  placement-new; no context-class wrapper), trivially destructible, payloads
  are `StringRef`/`ArrayRef` views (`ArrayRef::copy(Alloc)` for arg lists).
  Operations over the tree are external kind-switches with `llvm::cast<>`
  and no `default:` (LLVM-style RTTI: Kind tag + `classof`). Printing is
  the `print(raw_ostream&)` / `dump()` (`LLVM_DUMP_METHOD`) / `operator<<`
  trio; the S-expression format is test infrastructure, never asserted on
  directly.
- **Parser**: recursive descent + precedence climbing (clang's
  `ParseRHSOfBinaryExpression` shape), one parser-owned lookahead token
  (lexer stays pull-only), errors via `llvm::Expected` with explicit
  `takeError()` propagation, `enum class Prec` opaque in the header with
  values/rationale in the .cpp. `BinaryExpr` stores the op as `char`
  (deliberate: step 8 makes the operator set user-extensible).
- **Style points**: `static` for file-local functions, anonymous namespaces
  only for types (LLVM standard, opposite of Chromium); `assert` for
  programmer-error contracts (we always build with assertions on);
  `[[maybe_unused]]` for assert-only variables.

## Hard-won review lessons (Maksym checks these)

1. **Grep the tree before writing any utility** — `ArrayRef::copy`,
   `llvm::to_string`, `llvm::isSpace` all existed while Claude hand-rolled
   them.
2. **No speculative API** (YAGNI): unused functions were removed twice
   (`tok::getSpelling`, `ASTContext`). Reintroduce with their first real
   consumer.
3. **Implementation details out of headers** (opaque enum trick when a
   private type must be named).
4. **Tests are DAMP, not DRY**; each test spells out its API calls; never
   test dump formats.
5. Every design claim gets verified in-tree and cited, not asserted from
   memory.

## Status (2026-08-13)

- Merged: step 1 lexer (#1), production-token migration (#3), step 2
  expression AST + parser (#2). 31 tests green (17 Lex / 1 AST / 13 Parse).
- **Next: step 3** — parse `def` prototypes/definitions and `extern`
  declarations (new AST nodes: Prototype, Function), the interactive driver
  `toy.cpp` (likely `kaleidoscope/tools/toy/`), and the promised **lit +
  FileCheck test tier** (`kaleidoscope/test/` with `lit.cfg`) now that a
  driver will exist — clang/MLIR test parsers via lit, not gtest. Consider
  splitting into two PRs (parsing first; driver + lit second).
- Then step 4: codegen to LLVM IR (against this tree's libraries).
