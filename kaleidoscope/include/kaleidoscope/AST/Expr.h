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
// Kaleidoscope expressions come in exactly four kinds: numeric literals,
// variable references, binary operator applications, and function calls.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_AST_EXPR_H
#define KALEIDOSCOPE_AST_EXPR_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace kaleidoscope {

/// Base class for all expression nodes.
///
/// Type identification uses LLVM-style RTTI — a Kind tag plus a classof()
/// on each subclass — rather than C++'s dynamic_cast. This is how the whole
/// LLVM tree does it, and it makes llvm::isa<>, llvm::cast<> and
/// llvm::dyn_cast<> work on Expr (see
/// https://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html).
class Expr {
public:
  enum class Kind { Number, Variable, Binary, Call };

  virtual ~Expr() = default;

  Kind getKind() const { return K; }

protected:
  explicit Expr(Kind K) : K(K) {}

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

/// A reference to a variable, like "x".
class VariableExpr : public Expr {
public:
  explicit VariableExpr(std::string Name)
      : Expr(Kind::Variable), Name(std::move(Name)) {}

  const std::string &getName() const { return Name; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Variable; }

private:
  std::string Name;
};

/// A binary operator application, like "x + y".
class BinaryExpr : public Expr {
public:
  BinaryExpr(char Op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS)
      : Expr(Kind::Binary), Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

  char getOp() const { return Op; }
  const Expr &getLHS() const { return *LHS; }
  const Expr &getRHS() const { return *RHS; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Binary; }

private:
  char Op;
  std::unique_ptr<Expr> LHS, RHS;
};

/// A function call, like "fib(40)".
class CallExpr : public Expr {
public:
  CallExpr(std::string Callee, std::vector<std::unique_ptr<Expr>> Args)
      : Expr(Kind::Call), Callee(std::move(Callee)), Args(std::move(Args)) {}

  const std::string &getCallee() const { return Callee; }
  const std::vector<std::unique_ptr<Expr>> &getArgs() const { return Args; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Call; }

private:
  std::string Callee;
  std::vector<std::unique_ptr<Expr>> Args;
};

/// Print an expression as a prefix S-expression — "x + y * 2" prints as
/// "(+ x (* y 2))" — which spells out the tree shape unambiguously. Used by
/// tests and for debugging.
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Expr &E);

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_AST_EXPR_H
