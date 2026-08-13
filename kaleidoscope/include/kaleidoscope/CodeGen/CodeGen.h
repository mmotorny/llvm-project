//===- CodeGen.h - Kaleidoscope IR generation -----------------------------===//
//
// Code generation turns the AST into LLVM IR — the typed, SSA-form
// intermediate representation every LLVM frontend targets. From here on
// the work is shared: LLVM's optimizers and backends take the IR the rest
// of the way, which is the entire point of building on LLVM.
//
// The mapping is small because the language is small: every Kaleidoscope
// value is a double, so every function has type double(double, ...), a
// number literal is a double constant, and each operator is one
// floating-point instruction. The one wrinkle is '<': fcmp produces an i1
// (a one-bit boolean), which must be converted back to double (0.0 or
// 1.0) to stay inside the language's only type.
//
// The interface is one function: everything else — the IRBuilder that
// tracks the insertion point, the parameter-name scope — is per-call
// state, an implementation detail of CodeGen.cpp. Cross-declaration state
// lives in the llvm::Module itself: an earlier 'extern' or 'def' is found
// by name lookup in the module, never re-tracked on the side.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_CODEGEN_CODEGEN_H
#define KALEIDOSCOPE_CODEGEN_CODEGEN_H

#include "llvm/Support/Error.h"

namespace llvm {
class Function;
class Module;
} // namespace llvm

namespace kaleidoscope {

class FunctionDecl;

/// Emit D into M: a declaration (no body) for an 'extern', a full
/// definition for a 'def'. Returns the llvm::Function, or an error for
/// the cases the language rules out — an unknown variable or function in
/// the body, a call with the wrong number of arguments, or redefining a
/// function that already has a body.
llvm::Expected<llvm::Function *> emitFunctionDecl(llvm::Module &M,
                                                  const FunctionDecl &D);

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_CODEGEN_CODEGEN_H
