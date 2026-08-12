//===- Parser.cpp - Kaleidoscope parser implementation --------------------===//

#include "kaleidoscope/Parse/Parser.h"

#include "llvm/Support/Error.h"

#include <variant>

using namespace kaleidoscope;

namespace {

// Binary operator precedence: higher binds tighter, so '*' groups before
// '+', and '<' groups last. Anything that is not a binary operator gets -1,
// which loses every precedence comparison.
int getPrecedence(const Token &Tok) {
  const auto *C = std::get_if<tok::Char>(&Tok);
  if (!C)
    return -1;
  switch (C->Ch) {
  case '<':
    return 10;
  case '+':
  case '-':
    return 20;
  case '*':
    return 40;
  default:
    return -1;
  }
}

llvm::Error makeParseError(const llvm::Twine &Msg) {
  return llvm::createStringError(llvm::inconvertibleErrorCode(), Msg);
}

} // namespace

bool Parser::consumeIf(char C) {
  const auto *Ch = std::get_if<tok::Char>(&Cur);
  if (!Ch || Ch->Ch != C)
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
  if (const auto *Num = std::get_if<tok::Number>(&Cur)) {
    double Value = Num->Value;
    advance();
    return std::make_unique<NumberExpr>(Value);
  }

  if (const auto *Id = std::get_if<tok::Identifier>(&Cur)) {
    std::string Name = Id->Name;
    advance();

    // A bare identifier is a variable reference; one followed by '(' is a
    // call.
    if (!consumeIf('('))
      return std::make_unique<VariableExpr>(std::move(Name));

    std::vector<std::unique_ptr<Expr>> Args;
    if (!consumeIf(')')) {
      while (true) {
        auto Arg = parseExpr();
        if (!Arg)
          return Arg.takeError();
        Args.push_back(std::move(*Arg));

        if (consumeIf(')'))
          break;
        if (!consumeIf(','))
          return makeParseError("expected ')' or ',' in argument list");
      }
    }
    return std::make_unique<CallExpr>(std::move(Name), std::move(Args));
  }

  if (consumeIf('(')) {
    auto E = parseExpr();
    if (!E)
      return E;
    if (!consumeIf(')'))
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

    char Op = std::get<tok::Char>(Cur).Ch;
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
