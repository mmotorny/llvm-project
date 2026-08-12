//===- LexerTest.cpp - Unit tests for the Kaleidoscope lexer --------------===//

#include "kaleidoscope/Lex/Lexer.h"

#include "llvm/ADT/StringRef.h"

#include "gtest/gtest.h"

using namespace kaleidoscope;

namespace {

TEST(LexerTest, EmptyInput) {
  Lexer Lex("");

  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

TEST(LexerTest, WhitespaceOnly) {
  Lexer Lex("  \t\n  ");

  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// Sticky eof is the documented contract (see Lexer::Next): running out of
// input is a stable condition, so lookahead code may always ask for another
// token without tracking whether the end was already seen.
TEST(LexerTest, EofIsSticky) {
  Lexer Lex("x");

  EXPECT_TRUE(Lex.Next().is(tok::identifier));
  EXPECT_TRUE(Lex.Next().is(tok::eof));
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

TEST(LexerTest, Keywords) {
  Lexer Lex("def extern");

  Token Def = Lex.Next();
  EXPECT_TRUE(Def.is(tok::kw_def));
  EXPECT_EQ(Def.getSpelling(), "def");

  Token Extern = Lex.Next();
  EXPECT_TRUE(Extern.is(tok::kw_extern));
  EXPECT_EQ(Extern.getSpelling(), "extern");

  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

TEST(LexerTest, IdentifierCarriesItsName) {
  Lexer Lex("fib x1");

  EXPECT_EQ(Lex.Next().getIdentifier(), "fib");
  EXPECT_EQ(Lex.Next().getIdentifier(), "x1");
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// A keyword must match exactly; an identifier merely starting with one is
// still an identifier.
TEST(LexerTest, KeywordPrefixIsAnIdentifier) {
  Lexer Lex("definition externs");

  EXPECT_EQ(Lex.Next().getIdentifier(), "definition");
  EXPECT_EQ(Lex.Next().getIdentifier(), "externs");
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

TEST(LexerTest, NumberCarriesItsValue) {
  Lexer Lex("1 40.5 .25");

  EXPECT_EQ(Lex.Next().getNumber(), 1.0);
  EXPECT_EQ(Lex.Next().getNumber(), 40.5);
  EXPECT_EQ(Lex.Next().getNumber(), 0.25);
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// A number contains at most one dot. The second dot ends the token, and —
// starting a fresh token with '.' followed by a digit — begins a new
// fractional number. Rejecting the two adjacent numbers is the parser's
// job, not the lexer's.
TEST(LexerTest, SecondDotStartsANewNumber) {
  Lexer Lex("1.2.3");

  Token First = Lex.Next();
  EXPECT_EQ(First.getSpelling(), "1.2");
  EXPECT_EQ(First.getNumber(), 1.2);

  Token Second = Lex.Next();
  EXPECT_EQ(Second.getSpelling(), ".3");
  EXPECT_EQ(Second.getNumber(), 0.3);

  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// A dot with no digits around it is not a number — and not any other known
// token either.
TEST(LexerTest, LoneDotIsUnknown) {
  Lexer Lex(".");

  Token Dot = Lex.Next();
  EXPECT_TRUE(Dot.is(tok::unknown));
  EXPECT_EQ(Dot.getSpelling(), ".");

  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// Every punctuator is its own kind — the parser will compare named
// constants, never raw characters.
TEST(LexerTest, PunctuatorsHaveDistinctKinds) {
  Lexer Lex("( ) , + - * <");

  EXPECT_TRUE(Lex.Next().is(tok::l_paren));
  EXPECT_TRUE(Lex.Next().is(tok::r_paren));
  EXPECT_TRUE(Lex.Next().is(tok::comma));
  EXPECT_TRUE(Lex.Next().is(tok::plus));
  EXPECT_TRUE(Lex.Next().is(tok::minus));
  EXPECT_TRUE(Lex.Next().is(tok::star));
  EXPECT_TRUE(Lex.Next().is(tok::less));
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

TEST(LexerTest, CommentsAreSkippedToEndOfLine) {
  Lexer Lex("# ignored entirely\ndef");

  EXPECT_TRUE(Lex.Next().is(tok::kw_def));
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

TEST(LexerTest, CommentAtEndOfInput) {
  Lexer Lex("def # trailing comment");

  EXPECT_TRUE(Lex.Next().is(tok::kw_def));
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// No whitespace needed between tokens: the character classes separate them.
TEST(LexerTest, TokensNeedNoSpaces) {
  Lexer Lex("fib(x-1)");

  EXPECT_TRUE(Lex.Next().is(tok::identifier));
  EXPECT_TRUE(Lex.Next().is(tok::l_paren));
  EXPECT_TRUE(Lex.Next().is(tok::identifier));
  EXPECT_TRUE(Lex.Next().is(tok::minus));
  EXPECT_TRUE(Lex.Next().is(tok::number));
  EXPECT_TRUE(Lex.Next().is(tok::r_paren));
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// The zero-copy property that motivates this design: a token's spelling is
// a view into the original buffer, not a copy of it.
TEST(LexerTest, SpellingIsAViewIntoTheBuffer) {
  llvm::StringRef Buffer = "def fib";
  Lexer Lex(Buffer);

  EXPECT_EQ(Lex.Next().getSpelling().data(), Buffer.data());
  EXPECT_EQ(Lex.Next().getSpelling().data(), Buffer.data() + 4);
}

TEST(LexerTest, IsOneOfMatchesAnyListedKind) {
  Lexer Lex("+");
  Token Plus = Lex.Next();

  EXPECT_TRUE(Plus.isOneOf(tok::plus, tok::minus));
  EXPECT_FALSE(Plus.isOneOf(tok::star, tok::less));
}

// A realistic multi-line program: comment, definition, extern declaration.
TEST(LexerTest, FibonacciSample) {
  Lexer Lex("# Compute the x'th fibonacci number.\n"
            "def fib(x)\n"
            "  fib(x-1) + fib(x-2)\n"
            "\n"
            "extern sin(arg)\n");

  EXPECT_TRUE(Lex.Next().is(tok::kw_def));
  EXPECT_EQ(Lex.Next().getIdentifier(), "fib");
  EXPECT_TRUE(Lex.Next().is(tok::l_paren));
  EXPECT_EQ(Lex.Next().getIdentifier(), "x");
  EXPECT_TRUE(Lex.Next().is(tok::r_paren));
  EXPECT_EQ(Lex.Next().getIdentifier(), "fib");
  EXPECT_TRUE(Lex.Next().is(tok::l_paren));
  EXPECT_EQ(Lex.Next().getIdentifier(), "x");
  EXPECT_TRUE(Lex.Next().is(tok::minus));
  EXPECT_EQ(Lex.Next().getNumber(), 1.0);
  EXPECT_TRUE(Lex.Next().is(tok::r_paren));
  EXPECT_TRUE(Lex.Next().is(tok::plus));
  EXPECT_EQ(Lex.Next().getIdentifier(), "fib");
  EXPECT_TRUE(Lex.Next().is(tok::l_paren));
  EXPECT_EQ(Lex.Next().getIdentifier(), "x");
  EXPECT_TRUE(Lex.Next().is(tok::minus));
  EXPECT_EQ(Lex.Next().getNumber(), 2.0);
  EXPECT_TRUE(Lex.Next().is(tok::r_paren));
  EXPECT_TRUE(Lex.Next().is(tok::kw_extern));
  EXPECT_EQ(Lex.Next().getIdentifier(), "sin");
  EXPECT_TRUE(Lex.Next().is(tok::l_paren));
  EXPECT_EQ(Lex.Next().getIdentifier(), "arg");
  EXPECT_TRUE(Lex.Next().is(tok::r_paren));
  EXPECT_TRUE(Lex.Next().is(tok::eof));
}

// Two lexers over two buffers are fully independent.
TEST(LexerTest, IndependentInstances) {
  Lexer LexA("def"), LexB("42");

  EXPECT_TRUE(LexA.Next().is(tok::kw_def));
  EXPECT_EQ(LexB.Next().getNumber(), 42.0);
  EXPECT_TRUE(LexA.Next().is(tok::eof));
  EXPECT_TRUE(LexB.Next().is(tok::eof));
}

} // namespace
