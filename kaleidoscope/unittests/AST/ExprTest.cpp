//===- ExprTest.cpp - Unit tests for the Kaleidoscope expression AST ------===//

#include "kaleidoscope/AST/Expr.h"

#include "llvm/Support/Casting.h"

#include "gtest/gtest.h"

using namespace kaleidoscope;

namespace {

// The Kind tag plus classof() is what makes LLVM's isa/cast/dyn_cast work
// on our hierarchy — the same mechanism every class hierarchy in LLVM uses
// instead of dynamic_cast.
//
// The S-expression printer is deliberately not tested here: it is test
// infrastructure (the lens the parser tests look through), not a stable
// format — like clang's -ast-dump output. The parser tests pin its behavior
// transitively.
TEST(ExprTest, SupportsLlvmStyleRtti) {
  NumberExpr Num(1);
  const Expr &E = Num;

  EXPECT_TRUE(llvm::isa<NumberExpr>(E));
  EXPECT_FALSE(llvm::isa<VariableExpr>(E));
  ASSERT_NE(llvm::dyn_cast<NumberExpr>(&E), nullptr);
  EXPECT_EQ(llvm::dyn_cast<NumberExpr>(&E)->getValue(), 1.0);
  EXPECT_EQ(llvm::dyn_cast<VariableExpr>(&E), nullptr);
}

} // namespace
