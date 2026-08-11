//===- lexer_test.cpp - Unit tests for the Kaleidoscope lexer -------------===//

#include "lexer.h"

#include "gtest/gtest.h"

#include <sstream>
#include <vector>

namespace {

using Tokens = std::vector<Token>;

// Run the lexer over an in-memory string and collect every token up to and
// including Eof. Feeding a std::istringstream (rather than stdin) is exactly
// why Lexer takes a std::istream.
Tokens LexAll(const std::string &Source) {
  std::istringstream In(Source);
  Lexer Lex(In);
  Tokens Toks;
  do {
    Toks.push_back(Lex.Next());
  } while (!std::holds_alternative<tok::Eof>(Toks.back()));
  return Toks;
}

TEST(LexerTest, EmptyInput) {
  EXPECT_EQ(LexAll(""), Tokens({tok::Eof{}}));
}

TEST(LexerTest, WhitespaceOnly) {
  EXPECT_EQ(LexAll("  \t\n  "), Tokens({tok::Eof{}}));
}

TEST(LexerTest, Keywords) {
  EXPECT_EQ(LexAll("def extern"), Tokens({tok::Def{}, tok::Extern{}, tok::Eof{}}));
}

TEST(LexerTest, IdentifierCarriesItsName) {
  EXPECT_EQ(LexAll("fib x1"), Tokens({tok::Identifier{"fib"},
                                      tok::Identifier{"x1"}, tok::Eof{}}));
}

// A keyword must match exactly; an identifier merely starting with one is
// still an identifier.
TEST(LexerTest, KeywordPrefixIsAnIdentifier) {
  EXPECT_EQ(LexAll("definition externs"),
            Tokens({tok::Identifier{"definition"}, tok::Identifier{"externs"},
                    tok::Eof{}}));
}

TEST(LexerTest, NumberCarriesItsValue) {
  EXPECT_EQ(LexAll("1 40.5 .25"), Tokens({tok::Number{1}, tok::Number{40.5},
                                          tok::Number{0.25}, tok::Eof{}}));
}

// Known quirk, kept from the tutorial on purpose: the lexer greedily eats
// digits and dots, and strtod stops at the second dot, so "1.2.3" becomes
// the single number 1.2. This test documents the behavior; fixing it is a
// tutorial exercise.
TEST(LexerTest, MultipleDotsQuirk) {
  EXPECT_EQ(LexAll("1.2.3"), Tokens({tok::Number{1.2}, tok::Eof{}}));
}

// Operators and punctuation come through as plain characters.
TEST(LexerTest, OperatorsAndPunctuation) {
  EXPECT_EQ(LexAll("(x + 1)"),
            Tokens({tok::Char{'('}, tok::Identifier{"x"}, tok::Char{'+'},
                    tok::Number{1}, tok::Char{')'}, tok::Eof{}}));
}

TEST(LexerTest, CommentsAreSkippedToEndOfLine) {
  EXPECT_EQ(LexAll("# ignored entirely\ndef"),
            Tokens({tok::Def{}, tok::Eof{}}));
}

TEST(LexerTest, CommentAtEndOfInput) {
  EXPECT_EQ(LexAll("def # trailing comment"), Tokens({tok::Def{}, tok::Eof{}}));
}

// No whitespace needed between tokens: the character classes separate them.
TEST(LexerTest, TokensNeedNoSpaces) {
  EXPECT_EQ(LexAll("fib(x-1)"),
            Tokens({tok::Identifier{"fib"}, tok::Char{'('},
                    tok::Identifier{"x"}, tok::Char{'-'}, tok::Number{1},
                    tok::Char{')'}, tok::Eof{}}));
}

// A realistic program: the running example from the README.
TEST(LexerTest, FibonacciSample) {
  EXPECT_EQ(LexAll("# Compute the x'th fibonacci number.\n"
                   "def fib(x)\n"
                   "  fib(x-1) + fib(x-2)\n"
                   "\n"
                   "extern sin(arg)\n"),
            Tokens({tok::Def{}, tok::Identifier{"fib"}, tok::Char{'('},
                    tok::Identifier{"x"}, tok::Char{')'},
                    tok::Identifier{"fib"}, tok::Char{'('},
                    tok::Identifier{"x"}, tok::Char{'-'}, tok::Number{1},
                    tok::Char{')'}, tok::Char{'+'}, tok::Identifier{"fib"},
                    tok::Char{'('}, tok::Identifier{"x"}, tok::Char{'-'},
                    tok::Number{2}, tok::Char{')'}, tok::Extern{},
                    tok::Identifier{"sin"}, tok::Char{'('},
                    tok::Identifier{"arg"}, tok::Char{')'}, tok::Eof{}}));
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
