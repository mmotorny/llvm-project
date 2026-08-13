//===- Parser.cpp - Kaleidoscope parser implementation --------------------===//

#include "kaleidoscope/Parse/Parser.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <cassert>

using namespace kaleidoscope;

// Binary-operator precedence: higher binds tighter, so '*' groups before
// '+', and '<' groups last. A dedicated type rather than a naked int; the
// underlying values are spaced out because step 8 of the tutorial lets
// users declare new operators with numeric precedences in between. (Clang's
// fixed grammar affords it a dense named enum instead:
// clang/Basic/OperatorPrecedence.h's prec::Level.)
enum class Parser::Prec : int {
  Invalid = -1, // Not a binary operator: loses every comparison.
  Lowest = 0,   // parseExpr's starting bound: accepts any operator.
  Comparison = 10,     // <
  Additive = 20,       // +, -
  Multiplicative = 40, // *
};

// One step tighter than P: the recursion bound that encodes
// left-associativity in parseBinOpRHS.
Parser::Prec Parser::oneTighter(Prec P) {
  return Prec(static_cast<int>(P) + 1);
}

// Anything that is not a binary operator gets Invalid, which loses every
// precedence comparison — that one convention is what lets parseBinOpRHS's
// exit test be a single comparison.
Parser::Prec Parser::getPrecedence(const Token &Tok) {
  switch (Tok.getKind()) {
  case tok::less:
    return Prec::Comparison;
  case tok::plus:
  case tok::minus:
    return Prec::Additive;
  case tok::star:
    return Prec::Multiplicative;
  default:
    return Prec::Invalid;
  }
}

static llvm::Error makeParseError(const llvm::Twine &Msg) {
  return llvm::createStringError(llvm::inconvertibleErrorCode(), Msg);
}

bool Parser::consumeIf(tok::TokenKind K) {
  if (Cur.isNot(K))
    return false;
  advance();
  return true;
}

llvm::Expected<Parser::Prototype> Parser::parsePrototype() {
  if (Cur.isNot(tok::identifier))
    return makeParseError("expected function name in prototype");
  llvm::StringRef Name = Cur.getIdentifier();
  advance();

  if (!consumeIf(tok::l_paren))
    return makeParseError("expected '(' in prototype");

  // Parameters are bare identifiers separated only by whitespace — the
  // tutorial's grammar has no commas here, though calls do: "def f(a b)"
  // but "f(1, 2)".
  llvm::SmallVector<llvm::StringRef, 8> Params;
  while (Cur.is(tok::identifier)) {
    Params.push_back(Cur.getIdentifier());
    advance();
  }
  if (!consumeIf(tok::r_paren))
    return makeParseError("expected ')' in prototype");

  return Prototype{Name, llvm::ArrayRef(Params).copy(Alloc)};
}

llvm::Expected<FunctionDecl *> Parser::parseDefinition() {
  assert(Cur.is(tok::kw_def) && "parseDefinition called without 'def'");
  advance();

  auto Proto = parsePrototype();
  if (!Proto)
    return Proto.takeError();

  auto Body = parseExpr();
  if (!Body)
    return Body.takeError();
  return new (Alloc) FunctionDecl(Proto->Name, Proto->Params, *Body);
}

llvm::Expected<FunctionDecl *> Parser::parseExtern() {
  assert(Cur.is(tok::kw_extern) && "parseExtern called without 'extern'");
  advance();

  auto Proto = parsePrototype();
  if (!Proto)
    return Proto.takeError();
  return new (Alloc) FunctionDecl(Proto->Name, Proto->Params, /*Body=*/nullptr);
}

llvm::Expected<Expr *> Parser::parseExpr() {
  auto LHS = parsePrimary();
  if (!LHS)
    return LHS.takeError();
  return parseBinOpRHS(Prec::Lowest, *LHS);
}

llvm::Expected<Expr *> Parser::parsePrimary() {
  if (Cur.is(tok::number)) {
    double Value = Cur.getNumber();
    advance();
    return new (Alloc) NumberExpr(Value);
  }

  if (Cur.is(tok::identifier)) {
    // The spelling stays valid after advance(): it points into the source
    // buffer, not into the token.
    llvm::StringRef Name = Cur.getIdentifier();
    advance();

    // A bare identifier is a variable reference; one followed by '(' is a
    // call.
    if (!consumeIf(tok::l_paren))
      return new (Alloc) VariableExpr(Name);

    llvm::SmallVector<Expr *, 8> Args;
    if (!consumeIf(tok::r_paren)) {
      while (true) {
        auto Arg = parseExpr();
        if (!Arg)
          return Arg.takeError();
        Args.push_back(*Arg);

        if (consumeIf(tok::r_paren))
          break;
        if (!consumeIf(tok::comma))
          return makeParseError("expected ')' or ',' in argument list");
      }
    }
    return new (Alloc) CallExpr(Name, llvm::ArrayRef(Args).copy(Alloc));
  }

  if (consumeIf(tok::l_paren)) {
    auto E = parseExpr();
    if (!E)
      return E.takeError();
    if (!consumeIf(tok::r_paren))
      return makeParseError("expected ')'");
    // No ParenExpr node: the grouping's whole effect is already encoded in
    // the shape of the subtree.
    return E;
  }

  return makeParseError("expected an expression");
}

llvm::Expected<Expr *> Parser::parseBinOpRHS(Prec MinPrec, Expr *LHS) {
  while (true) {
    // Not an operator, or one that binds looser than we're allowed to
    // consume: our caller deals with it.
    Prec P = getPrecedence(Cur);
    if (P < MinPrec)
      return LHS;

    // The AST stores the operator as its source character; the punctuator's
    // one-char spelling is exactly that. (Clang similarly translates token
    // kinds into its own operator enum at this boundary.)
    char Op = Cur.getSpelling().front();
    advance();

    auto RHS = parsePrimary();
    if (!RHS)
      return RHS.takeError();

    // If the next operator binds tighter than this one — "x + y * 2" with
    // Op '+' and '*' pending — the right operand isn't just the primary, it
    // is everything that '*' claims. Recurse to let it claim it, bounding
    // it at oneTighter(P): that strictness is what makes equal-precedence
    // chains group left ("a - b + c" is "(a - b) + c").
    if (P < getPrecedence(Cur)) {
      RHS = parseBinOpRHS(oneTighter(P), *RHS);
      if (!RHS)
        return RHS.takeError();
    }

    LHS = new (Alloc) BinaryExpr(Op, LHS, *RHS);
  }
}
