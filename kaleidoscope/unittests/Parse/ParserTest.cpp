//===- ParserTest.cpp - Unit tests for the Kaleidoscope parser ------------===//
//
// Grammar behavior — what parses to what tree, and which inputs are
// errors — is tested in test/Parse/*.ks, through the toy driver. That is
// LLVM's convention: lit tests for anything observable through a tool,
// unit tests only for C++ API contracts a tool can't reach (llvm's
// TestingGuide, "Unit tests"; cf. clang, whose parser is tested by
// clang/test/Parser, not gtest).
//
//===----------------------------------------------------------------------===//

#include "kaleidoscope/Parse/Parser.h"

#include "kaleidoscope/Lex/Lexer.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Error.h"

#include "gtest/gtest.h"

using namespace kaleidoscope;

namespace {

// The driver never calls parseExpr() at end of input (it checks
// getCurToken() first), so this contract — a clean error, not a crash —
// is invisible to lit and stays a unit test.
TEST(ParserTest, EmptyInputIsAnError) {
  Lexer Lex("");
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  auto E = P.parseExpr();
  ASSERT_FALSE(bool(E));
  EXPECT_EQ(llvm::toString(E.takeError()), "expected an expression");
}

} // namespace
