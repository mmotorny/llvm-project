# Kaleidoscope working notes

Cross-session context lives here, never in the memory directory. Update
this file in the same PR as the work it describes.

## Project

Build the Kaleidoscope compiler (LLVM tutorial) in this fork to learn LLVM.
Maksym has not read the tutorial: explain each concept and LLVM convention
from zero in PRs and chat. Roadmap: `kaleidoscope/README.md`.

## Workflow

- Every change is a PR to `main` of `mmotorny/llvm-project` (never
  upstream), merged after Maksym's review.
- On review comments: change the code, or push back with in-tree evidence
  (grep clang/MLIR/LLVM; cite file:line). Reply in the PR threads.
  "Ptal" = fetch and address new comments.
- Branches: `kaleidoscope/NN-topic` (steps), `kaleidoscope/topic`
  (refactors). Delete after merge.

## Build & test

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

## Conventions (settled in review; don't relitigate silently)

- C++17, LLVM's standard. LLVM types over std: `StringRef`, `ArrayRef`,
  `Expected`, `raw_ostream`.
- clang layout and naming: `kaleidoscope<Phase>` libraries in
  `kaleidoscope/lib/<Phase>/`, headers in `include/kaleidoscope/<Phase>/`,
  tests in `unittests/<Phase>/`, CamelCase files, everything in
  `namespace kaleidoscope` (nested only when semantic: `tok`).
- Tokens: flat trivially copyable `Token` = kind + `StringRef` spelling
  into the caller's buffer; per-kind `tok::` enum including punctuators;
  assert-guarded accessors; lazy `getNumber()`. Lexer: `Cur`/`End`
  pointers over a `StringRef`; sticky `tok::eof`; keywords via
  `StringSwitch`.
- AST: no virtuals (protected non-virtual base dtor); nodes in the
  caller's `BumpPtrAllocator` via `new (Alloc)`; trivially destructible;
  `StringRef`/`ArrayRef` payloads (`ArrayRef::copy(Alloc)` for lists);
  operations are external kind-switches with `cast<>` and no `default:`;
  printing is the `print(raw_ostream&)` / `dump()` (`LLVM_DUMP_METHOD`) /
  `operator<<` trio; never assert on the dump format.
- Parser: recursive descent + precedence climbing (clang's
  `ParseRHSOfBinaryExpression`); one lookahead `Token` in the parser,
  lexer stays pull-only; errors via `llvm::Expected` with explicit
  `takeError()`; `Prec` enum opaque in the header, values in the .cpp;
  `BinaryExpr` op is `char` (step 8 adds user-defined operators).
- `static` for file-local functions; anonymous namespaces only for types.
  `assert` for programmer errors (builds always have assertions on).

## Review lessons

1. Grep for an existing LLVM helper before writing one (`ArrayRef::copy`,
   `llvm::to_string`, `llvm::isSpace` all existed).
2. No speculative API: add functions with their first consumer.
3. Implementation details stay out of headers.
4. Tests DAMP: each spells out its API calls; don't test print formats.
5. Verify design claims in-tree; cite, don't assert.

## Status 2026-08-13

Merged: lexer (#1), production tokens (#3), expression AST + parser (#2).
31 tests green. Next, step 3: `def`/`extern` parsing (Prototype/Function
nodes), `toy` driver (`kaleidoscope/tools/toy/`), lit+FileCheck tier
(`kaleidoscope/test/`) — consider two PRs (parsing; driver+lit). Then
step 4: LLVM IR codegen.
