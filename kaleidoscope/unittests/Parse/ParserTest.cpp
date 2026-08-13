//===- ParserTest.cpp - Unit tests for the Kaleidoscope parser ------------===//

#include "kaleidoscope/Parse/Parser.h"

#include "kaleidoscope/AST/Expr.h"
#include "kaleidoscope/Lex/Lexer.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ScopedPrinter.h"

#include "gtest/gtest.h"

using namespace kaleidoscope;

namespace {

TEST(ParserTest, ParsesNumber) {
  Lexer Lex("42");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "42");
}

TEST(ParserTest, ParsesVariable) {
  Lexer Lex("x");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "x");
}

TEST(ParserTest, MultiplicationBindsTighterThanAddition) {
  Lexer Lex("x + y * 2");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "(+ x (* y 2))");
}

TEST(ParserTest, EqualPrecedenceGroupsLeftToRight) {
  Lexer Lex("a - b + c");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "(+ (- a b) c)");
}

TEST(ParserTest, ComparisonBindsLoosest) {
  Lexer Lex("a + b < c * d");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "(< (+ a b) (* c d))");
}

TEST(ParserTest, ParenthesesOverridePrecedence) {
  Lexer Lex("(x + y) * 2");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "(* (+ x y) 2)");
}

TEST(ParserTest, ParsesCallWithExpressionArguments) {
  Lexer Lex("fib(x - 1)");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "(fib (- x 1))");
}

TEST(ParserTest, ParsesCallWithNoArguments) {
  Lexer Lex("f()");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "(f)");
}

TEST(ParserTest, ParsesCallWithMultipleArguments) {
  Lexer Lex("f(a, b + 1, g(c))");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_TRUE(bool(E));
  EXPECT_EQ(llvm::to_string(**E), "(f a (+ b 1) (g c))");
}

TEST(ParserTest, MissingRightOperandIsAnError) {
  Lexer Lex("x +");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected an expression");
}

TEST(ParserTest, UnclosedParenthesisIsAnError) {
  Lexer Lex("(x + y");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected ')'");
}

TEST(ParserTest, MissingArgumentSeparatorIsAnError) {
  Lexer Lex("f(a b)");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()),
            "expected ')' or ',' in argument list");
}

TEST(ParserTest, EmptyInputIsAnError) {
  Lexer Lex("");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected an expression");
}

} // namespace
