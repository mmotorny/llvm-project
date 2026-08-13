# Kaleidoscope working notes

Cross-session context lives here, never in the memory directory. Update
this file in the same PR as the work it describes.

## Project

Build the Kaleidoscope compiler (LLVM tutorial) in this fork to learn LLVM.
Maksym has not read the tutorial: explain each concept and LLVM convention
from zero in PRs and chat. Roadmap, build, and test commands:
`kaleidoscope/README.md`.

## Workflow

- Every change is a PR to `main` of `mmotorny/llvm-project` (never
  upstream), merged after Maksym's review.
- On review comments: change the code, or push back with in-tree evidence
  (grep clang/MLIR/LLVM; cite file:line). Reply in the PR threads.
  "Ptal" = fetch and address new comments.
- Delete branches after merge.

## Conventions

- Model everything on what clang/MLIR/LLVM do in this tree.
- Grep for an existing LLVM helper before writing one; no speculative API;
  implementation details out of headers; tests DAMP; don't test print
  formats.
