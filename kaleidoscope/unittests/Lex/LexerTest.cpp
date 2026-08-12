//===- LexerTest.cpp - Unit tests for the Kaleidoscope lexer --------------===//

#include "kaleidoscope/Lex/Lexer.h"

#include "gtest/gtest.h"

#include <sstream>

namespace {

TEST(LexerTest, EmptyInput) {
  std::istringstream In("");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

TEST(LexerTest, WhitespaceOnly) {
  std::istringstream In("  \t\n  ");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// Sticky Eof is the documented contract (see Lexer::Next): running out of
// input is a stable condition, so lookahead code may always ask for another
// token without tracking whether the end was already seen.
TEST(LexerTest, EofIsSticky) {
  std::istringstream In("x");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"x"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

TEST(LexerTest, Keywords) {
  std::istringstream In("def extern");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Def{}));
  EXPECT_EQ(Lex.Next(), Token(tok::Extern{}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

TEST(LexerTest, IdentifierCarriesItsName) {
  std::istringstream In("fib x1");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"fib"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"x1"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// A keyword must match exactly; an identifier merely starting with one is
// still an identifier.
TEST(LexerTest, KeywordPrefixIsAnIdentifier) {
  std::istringstream In("definition externs");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"definition"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"externs"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

TEST(LexerTest, NumberCarriesItsValue) {
  std::istringstream In("1 40.5 .25");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Number{1}));
  EXPECT_EQ(Lex.Next(), Token(tok::Number{40.5}));
  EXPECT_EQ(Lex.Next(), Token(tok::Number{0.25}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// A number contains at most one dot (the tutorial's lexer would eat
// "1.2.3" as one malformed number). The second dot ends the token, and —
// starting a fresh token with '.' followed by a digit — begins a new
// fractional number. Rejecting the two adjacent numbers is the parser's
// job, not the lexer's.
TEST(LexerTest, SecondDotStartsANewNumber) {
  std::istringstream In("1.2.3");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Number{1.2}));
  EXPECT_EQ(Lex.Next(), Token(tok::Number{0.3}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// A dot with no digits around it is not a number.
TEST(LexerTest, LoneDotIsAChar) {
  std::istringstream In(".");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Char{'.'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// Operators and punctuation come through as plain characters.
TEST(LexerTest, OperatorsAndPunctuation) {
  std::istringstream In("(x + 1)");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Char{'('}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"x"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'+'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Number{1}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{')'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

TEST(LexerTest, CommentsAreSkippedToEndOfLine) {
  std::istringstream In("# ignored entirely\ndef");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Def{}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

TEST(LexerTest, CommentAtEndOfInput) {
  std::istringstream In("def # trailing comment");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Def{}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// No whitespace needed between tokens: the character classes separate them.
TEST(LexerTest, TokensNeedNoSpaces) {
  std::istringstream In("fib(x-1)");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"fib"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'('}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"x"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'-'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Number{1}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{')'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// A realistic multi-line program: comment, definition, extern declaration.
TEST(LexerTest, FibonacciSample) {
  std::istringstream In("# Compute the x'th fibonacci number.\n"
                        "def fib(x)\n"
                        "  fib(x-1) + fib(x-2)\n"
                        "\n"
                        "extern sin(arg)\n");
  Lexer Lex(In);

  EXPECT_EQ(Lex.Next(), Token(tok::Def{}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"fib"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'('}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"x"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{')'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"fib"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'('}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"x"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'-'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Number{1}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{')'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'+'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"fib"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'('}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"x"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'-'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Number{2}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{')'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Extern{}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"sin"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{'('}));
  EXPECT_EQ(Lex.Next(), Token(tok::Identifier{"arg"}));
  EXPECT_EQ(Lex.Next(), Token(tok::Char{')'}));
  EXPECT_EQ(Lex.Next(), Token(tok::Eof{}));
}

// Two lexers over two streams are fully independent — the state that the
// tutorial keeps in globals is per-instance here.
TEST(LexerTest, IndependentInstances) {
  std::istringstream A("def"), B("42");
  Lexer LexA(A), LexB(B);

  EXPECT_EQ(LexA.Next(), Token(tok::Def{}));
  EXPECT_EQ(LexB.Next(), Token(tok::Number{42}));
  EXPECT_EQ(LexA.Next(), Token(tok::Eof{}));
  EXPECT_EQ(LexB.Next(), Token(tok::Eof{}));
}

} // namespace
