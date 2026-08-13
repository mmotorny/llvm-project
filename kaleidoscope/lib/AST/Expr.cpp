//===- Expr.cpp - Kaleidoscope expression AST printing --------------------===//

#include "kaleidoscope/AST/Expr.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace kaleidoscope;

// One switch over the Kind tag with llvm::cast<> per case — the standard
// LLVM shape for operating on a closed class hierarchy. The switch has no
// default so the compiler warns if a new Kind is added but not handled here.
void Expr::print(llvm::raw_ostream &OS) const {
  switch (getKind()) {
  case Kind::Number:
    // %g prints the shortest form: "42" rather than "4.200000e+01".
    OS << llvm::format("%g", llvm::cast<NumberExpr>(*this).getValue());
    return;
  case Kind::Variable:
    OS << llvm::cast<VariableExpr>(*this).getName();
    return;
  case Kind::Binary: {
    const auto &B = llvm::cast<BinaryExpr>(*this);
    OS << '(' << B.getOp() << ' ' << B.getLHS() << ' ' << B.getRHS() << ')';
    return;
  }
  case Kind::Call: {
    const auto &C = llvm::cast<CallExpr>(*this);
    OS << '(' << C.getCallee();
    for (const std::unique_ptr<Expr> &Arg : C.getArgs())
      OS << ' ' << *Arg;
    OS << ')';
    return;
  }
  }
  llvm_unreachable("unknown expression kind");
}

LLVM_DUMP_METHOD void Expr::dump() const {
  print(llvm::errs());
  llvm::errs() << '\n';
}
