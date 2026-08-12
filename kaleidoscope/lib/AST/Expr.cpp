//===- Expr.cpp - Kaleidoscope expression AST printing --------------------===//

#include "kaleidoscope/AST/Expr.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace kaleidoscope;

// One switch over the Kind tag with llvm::cast<> per case — the standard
// LLVM shape for operating on a closed class hierarchy. The switch has no
// default so the compiler warns if a new Kind is added but not handled here.
llvm::raw_ostream &kaleidoscope::operator<<(llvm::raw_ostream &OS,
                                            const Expr &E) {
  switch (E.getKind()) {
  case Expr::Kind::Number:
    // %g prints the shortest form: "42" rather than "4.200000e+01".
    return OS << llvm::format("%g", llvm::cast<NumberExpr>(E).getValue());
  case Expr::Kind::Variable:
    return OS << llvm::cast<VariableExpr>(E).getName();
  case Expr::Kind::Binary: {
    const auto &B = llvm::cast<BinaryExpr>(E);
    return OS << '(' << B.getOp() << ' ' << B.getLHS() << ' ' << B.getRHS()
              << ')';
  }
  case Expr::Kind::Call: {
    const auto &C = llvm::cast<CallExpr>(E);
    OS << '(' << C.getCallee();
    for (const std::unique_ptr<Expr> &Arg : C.getArgs())
      OS << ' ' << *Arg;
    return OS << ')';
  }
  }
  llvm_unreachable("unknown expression kind");
}
