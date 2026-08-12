//===- Parser.cpp - Kaleidoscope parser implementation --------------------===//

#include "kaleidoscope/Parse/Parser.h"

#include "llvm/Support/Error.h"

using namespace kaleidoscope;

// Binary operator precedence: higher binds tighter, so '*' groups before
// '+', and '<' groups last. Anything that is not a binary operator gets -1,
// which loses every precedence comparison.
static int getPrecedence(const Token &Tok) {
  switch (Tok.getKind()) {
  case tok::less:
    return 10;
  case tok::plus:
  case tok::minus:
    return 20;
  case tok::star:
    return 40;
  default:
    return -1;
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

llvm::Expected<std::unique_ptr<Expr>> Parser::parseExpr() {
  auto LHS = parsePrimary();
  if (!LHS)
    return LHS;
  return parseBinOpRHS(0, std::move(*LHS));
}

llvm::Expected<std::unique_ptr<Expr>> Parser::parsePrimary() {
  if (Cur.is(tok::number)) {
    double Value = Cur.getNumber();
    advance();
    return std::make_unique<NumberExpr>(Value);
  }

  if (Cur.is(tok::identifier)) {
    std::string Name = Cur.getIdentifier().str();
    advance();

    // A bare identifier is a variable reference; one followed by '(' is a
    // call.
    if (!consumeIf(tok::l_paren))
      return std::make_unique<VariableExpr>(std::move(Name));

    std::vector<std::unique_ptr<Expr>> Args;
    if (!consumeIf(tok::r_paren)) {
      while (true) {
        auto Arg = parseExpr();
        if (!Arg)
          return Arg.takeError();
        Args.push_back(std::move(*Arg));

        if (consumeIf(tok::r_paren))
          break;
        if (!consumeIf(tok::comma))
          return makeParseError("expected ')' or ',' in argument list");
      }
    }
    return std::make_unique<CallExpr>(std::move(Name), std::move(Args));
  }

  if (consumeIf(tok::l_paren)) {
    auto E = parseExpr();
    if (!E)
      return E;
    if (!consumeIf(tok::r_paren))
      return makeParseError("expected ')'");
    // No ParenExpr node: the grouping's whole effect is already encoded in
    // the shape of the subtree.
    return E;
  }

  return makeParseError("expected an expression");
}

llvm::Expected<std::unique_ptr<Expr>>
Parser::parseBinOpRHS(int MinPrec, std::unique_ptr<Expr> LHS) {
  while (true) {
    // Not an operator, or one that binds looser than we're allowed to
    // consume: our caller deals with it.
    int Prec = getPrecedence(Cur);
    if (Prec < MinPrec)
      return std::move(LHS);

    // The AST stores the operator as its source character; the punctuator's
    // one-char spelling is exactly that. (Clang similarly translates token
    // kinds into its own operator enum at this boundary.)
    char Op = Cur.getSpelling().front();
    advance();

    auto RHS = parsePrimary();
    if (!RHS)
      return RHS;

    // If the next operator binds tighter than this one — "x + y * 2" with
    // Op '+' and '*' pending — the right operand isn't just the primary, it
    // is everything that '*' claims. Recurse to let it claim it.
    if (Prec < getPrecedence(Cur)) {
      RHS = parseBinOpRHS(Prec + 1, std::move(*RHS));
      if (!RHS)
        return RHS;
    }

    LHS = std::make_unique<BinaryExpr>(Op, std::move(LHS), std::move(*RHS));
  }
}
