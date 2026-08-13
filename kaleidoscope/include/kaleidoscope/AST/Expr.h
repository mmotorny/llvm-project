//===- Expr.h - Kaleidoscope expression AST -------------------------------===//
//
// The parser's output is an Abstract Syntax Tree: one node per language
// construct, with children for the construct's parts. "x + y * 2" becomes
//
//     BinaryExpr('+')
//     ├── VariableExpr("x")
//     └── BinaryExpr('*')
//         ├── VariableExpr("y")
//         └── NumberExpr(2)
//
// The tree *is* the meaning: operator precedence, grouping parentheses, and
// call boundaries are all encoded in its shape, so nothing downstream ever
// re-derives them from the source text.
//
// The hierarchy is modeled the way clang models its AST:
//
//  - No virtual functions anywhere — not even a destructor. Nodes live in
//    an ASTContext arena and are never deleted individually, so no vtable
//    is needed. Type identification is LLVM-style RTTI (a Kind tag plus
//    classof), which makes llvm::isa<>/cast<>/dyn_cast<> work — see
//    https://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html
//  - Operations over the tree (printing here; code generation later) are
//    one switch over the Kind tag, not virtual methods: a closed hierarchy
//    with open operations, so adding an operation touches no node class,
//    and a default-less switch is compiler-checked for exhaustiveness.
//  - Nodes own no memory: names are StringRefs borrowing the source buffer
//    (the same lifetime contract as token spellings), child links are plain
//    pointers to arena-owned nodes, and argument lists are ArrayRefs into
//    the arena.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_AST_EXPR_H
#define KALEIDOSCOPE_AST_EXPR_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace kaleidoscope {

/// Base class for all expression nodes.
class Expr {
public:
  enum class Kind { Number, Variable, Binary, Call };

  Kind getKind() const { return K; }

  /// Print this expression to OS as a prefix S-expression — "x + y * 2"
  /// prints as "(+ x (* y 2))" — which spells out the tree shape
  /// unambiguously. Not a stable format: for tests and debugging only.
  void print(llvm::raw_ostream &OS) const;

  /// Debugger convenience, in LLVM's tradition: print to llvm::errs() with
  /// a trailing newline. Call it from a debugger prompt.
  void dump() const;

protected:
  explicit Expr(Kind K) : K(K) {}
  // Non-virtual, and protected so nothing can `delete` a node through the
  // base class: nodes are arena-owned by the ASTContext.
  ~Expr() = default;

private:
  const Kind K;
};

/// A numeric literal, like "1.0".
class NumberExpr : public Expr {
public:
  explicit NumberExpr(double Value) : Expr(Kind::Number), Value(Value) {}

  double getValue() const { return Value; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Number; }

private:
  double Value;
};

/// A reference to a variable, like "x". The name borrows the source buffer,
/// like the token spelling it came from.
class VariableExpr : public Expr {
public:
  explicit VariableExpr(llvm::StringRef Name)
      : Expr(Kind::Variable), Name(Name) {}

  llvm::StringRef getName() const { return Name; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Variable; }

private:
  llvm::StringRef Name;
};

/// A binary operator application, like "x + y".
class BinaryExpr : public Expr {
public:
  BinaryExpr(char Op, Expr *LHS, Expr *RHS)
      : Expr(Kind::Binary), Op(Op), LHS(LHS), RHS(RHS) {}

  char getOp() const { return Op; }
  const Expr &getLHS() const { return *LHS; }
  const Expr &getRHS() const { return *RHS; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Binary; }

private:
  char Op;
  Expr *LHS;
  Expr *RHS;
};

/// A function call, like "fib(40)".
class CallExpr : public Expr {
public:
  CallExpr(llvm::StringRef Callee, llvm::ArrayRef<Expr *> Args)
      : Expr(Kind::Call), Callee(Callee), Args(Args) {}

  llvm::StringRef getCallee() const { return Callee; }
  llvm::ArrayRef<Expr *> getArgs() const { return Args; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Call; }

private:
  llvm::StringRef Callee;
  llvm::ArrayRef<Expr *> Args;
};

/// Sugar over Expr::print, LLVM-style (cf. operator<< for llvm::Value), so
/// expressions compose in raw_ostream chains.
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Expr &E) {
  E.print(OS);
  return OS;
}

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_AST_EXPR_H
