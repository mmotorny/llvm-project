//===- Decl.h - Kaleidoscope function declarations ------------------------===//
//
// The only thing Kaleidoscope declares is functions, in two forms:
//
//     def add(a b) a + b        a *definition*: interface plus body
//     extern cos(x)             a *declaration*: interface only, the body
//                               lives elsewhere (e.g. in libm)
//
// Both are one class here, FunctionDecl, with the body optional — clang's
// modeling: clang has no separate "prototype" node either, a FunctionDecl
// without a body *is* the prototype (cf. clang's FunctionDecl::hasBody()).
// The tutorial instead splits PrototypeAST from FunctionAST; one class is
// smaller and nothing yet needs the split.
//
// A function's interface is just its name and parameter names: every value
// in the language is a double, so there are no types to record, and arity
// is the parameter count.
//
// FunctionDecl follows the arena discipline of Expr (see Expr.h): allocated
// in a caller-owned BumpPtrAllocator, trivially destructible, owning no
// memory — names borrow the source buffer, the parameter list is an
// ArrayRef into the arena, the body a pointer to an arena-owned tree.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_AST_DECL_H
#define KALEIDOSCOPE_AST_DECL_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

#include <cassert>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace kaleidoscope {

class Expr;

/// A function: name, parameter names, and — for definitions — the body
/// expression. There is no return statement: the body's value is the
/// function's result.
class FunctionDecl {
public:
  FunctionDecl(llvm::StringRef Name, llvm::ArrayRef<llvm::StringRef> Params,
               Expr *Body)
      : Name(Name), Params(Params), Body(Body) {}

  llvm::StringRef getName() const { return Name; }
  llvm::ArrayRef<llvm::StringRef> getParams() const { return Params; }

  /// Definitions have a body; 'extern' declarations don't.
  bool hasBody() const { return Body != nullptr; }
  const Expr &getBody() const {
    assert(hasBody() && "'extern' declaration has no body");
    return *Body;
  }

  /// Print as a prefix S-expression, like Expr::print: "(def (add a b)
  /// (+ a b))", "(extern (cos x))". Not a stable format: for tests and
  /// debugging only.
  void print(llvm::raw_ostream &OS) const;

  /// Debugger convenience: print to llvm::errs() with a trailing newline.
  void dump() const;

private:
  llvm::StringRef Name;
  llvm::ArrayRef<llvm::StringRef> Params;
  Expr *Body;
};

/// Sugar over FunctionDecl::print, as for Expr.
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const FunctionDecl &D) {
  D.print(OS);
  return OS;
}

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_AST_DECL_H
