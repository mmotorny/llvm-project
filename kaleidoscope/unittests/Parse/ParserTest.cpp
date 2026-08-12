//===- ParserTest.cpp - Unit tests for the Kaleidoscope parser ------------===//

#include "kaleidoscope/Parse/Parser.h"

#include "kaleidoscope/AST/Expr.h"
#include "kaleidoscope/Lex/Lexer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

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
  Lexer Lex("42");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "42");
}

TEST(ParserTest, ParsesVariable) {
  Lexer Lex("x");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "x");
}

TEST(ParserTest, MultiplicationBindsTighterThanAddition) {
  Lexer Lex("x + y * 2");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(+ x (* y 2))");
}

TEST(ParserTest, EqualPrecedenceGroupsLeftToRight) {
  Lexer Lex("a - b + c");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(+ (- a b) c)");
}

TEST(ParserTest, ComparisonBindsLoosest) {
  Lexer Lex("a + b < c * d");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(< (+ a b) (* c d))");
}

TEST(ParserTest, ParenthesesOverridePrecedence) {
  Lexer Lex("(x + y) * 2");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(* (+ x y) 2)");
}

TEST(ParserTest, ParsesCallWithExpressionArguments) {
  Lexer Lex("fib(x - 1)");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(fib (- x 1))");
}

TEST(ParserTest, ParsesCallWithNoArguments) {
  Lexer Lex("f()");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(f)");
}

TEST(ParserTest, ParsesCallWithMultipleArguments) {
  Lexer Lex("f(a, b + 1, g(c))");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(toString(**E), "(f a (+ b 1) (g c))");
}

TEST(ParserTest, MissingRightOperandIsAnError) {
  Lexer Lex("x +");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected an expression");
}

TEST(ParserTest, UnclosedParenthesisIsAnError) {
  Lexer Lex("(x + y");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected ')'");
}

TEST(ParserTest, MissingArgumentSeparatorIsAnError) {
  Lexer Lex("f(a b)");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()),
            "expected ')' or ',' in argument list");
}

TEST(ParserTest, EmptyInputIsAnError) {
  Lexer Lex("");
  Parser P(Lex);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected an expression");
}

} // namespace
