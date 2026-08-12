//===- ExprTest.cpp - Unit tests for the Kaleidoscope expression AST ------===//

#include "kaleidoscope/AST/Expr.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include <memory>
#include <utility>
#include <vector>

using namespace kaleidoscope;

namespace {

// Render an expression through its raw_ostream printer.
std::string toString(const Expr &E) {
  std::string S;
  llvm::raw_string_ostream OS(S);
  OS << E;
  return S;
}

TEST(ExprTest, PrintsNumberInShortestForm) {
  EXPECT_EQ(toString(NumberExpr(42)), "42");
  EXPECT_EQ(toString(NumberExpr(40.5)), "40.5");
}

TEST(ExprTest, PrintsVariableAsItsName) {
  EXPECT_EQ(toString(VariableExpr("x")), "x");
}

TEST(ExprTest, PrintsBinaryAsPrefixSExpression) {
  BinaryExpr Sum('+', std::make_unique<VariableExpr>("x"),
                 std::make_unique<NumberExpr>(1));

  EXPECT_EQ(toString(Sum), "(+ x 1)");
}

TEST(ExprTest, PrintsCallWithSpaceSeparatedArgs) {
  std::vector<std::unique_ptr<Expr>> Args;
  Args.push_back(std::make_unique<VariableExpr>("x"));
  Args.push_back(std::make_unique<NumberExpr>(2));
  CallExpr Call("f", std::move(Args));

  EXPECT_EQ(toString(Call), "(f x 2)");
}

TEST(ExprTest, PrintsNoArgCallAsBareParens) {
  EXPECT_EQ(toString(CallExpr("f", {})), "(f)");
}

// The Kind tag plus classof() is what makes LLVM's isa/cast/dyn_cast work
// on our hierarchy — the same mechanism every class hierarchy in LLVM uses
// instead of dynamic_cast.
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
