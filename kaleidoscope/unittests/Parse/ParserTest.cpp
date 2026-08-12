//===- ParserTest.cpp - Unit tests for the Kaleidoscope parser ------------===//

#include "kaleidoscope/Parse/Parser.h"

#include "kaleidoscope/AST/Expr.h"
#include "kaleidoscope/Lex/Lexer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include <sstream>

using namespace kaleidoscope;

namespace {

// Render an expression through its raw_ostream printer: "(+ x (* y 2))"
// spells out the tree shape, so string comparison checks the parse
// structure, not just that parsing succeeded.
std::string toString(const Expr &E) {
  std::string S;
  llvm::raw_string_ostream OS(S);
  OS << E;
  return S;
}

TEST(ParserTest, ParsesNumber) {
  std::istringstream In("42");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "42");
}

TEST(ParserTest, ParsesVariable) {
  std::istringstream In("x");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "x");
}

TEST(ParserTest, MultiplicationBindsTighterThanAddition) {
  std::istringstream In("x + y * 2");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(+ x (* y 2))");
}

TEST(ParserTest, EqualPrecedenceGroupsLeftToRight) {
  std::istringstream In("a - b + c");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(+ (- a b) c)");
}

TEST(ParserTest, ComparisonBindsLoosest) {
  std::istringstream In("a + b < c * d");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(< (+ a b) (* c d))");
}

TEST(ParserTest, ParenthesesOverridePrecedence) {
  std::istringstream In("(x + y) * 2");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(* (+ x y) 2)");
}

TEST(ParserTest, ParsesCallWithExpressionArguments) {
  std::istringstream In("fib(x - 1)");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(fib (- x 1))");
}

TEST(ParserTest, ParsesCallWithNoArguments) {
  std::istringstream In("f()");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(f)");
}

TEST(ParserTest, ParsesCallWithMultipleArguments) {
  std::istringstream In("f(a, b + 1, g(c))");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(f a (+ b 1) (g c))");
}

TEST(ParserTest, MissingRightOperandIsAnError) {
  std::istringstream In("x +");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected an expression");
}

TEST(ParserTest, UnclosedParenthesisIsAnError) {
  std::istringstream In("(x + y");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected ')'");
}

TEST(ParserTest, MissingArgumentSeparatorIsAnError) {
  std::istringstream In("f(a b)");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()),
            "expected ')' or ',' in argument list");
}

TEST(ParserTest, EmptyInputIsAnError) {
  std::istringstream In("");
  Lexer Lex(In);
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected an expression");
}

} // namespace
