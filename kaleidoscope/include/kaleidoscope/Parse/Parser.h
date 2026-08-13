//===- Parser.h - Kaleidoscope parser -------------------------------------===//
//
// The parser turns the lexer's token stream into an AST. It is a
// *recursive-descent* parser: one member function per grammar rule, each
// consuming exactly the tokens of that rule and returning its AST node.
// Rules that contain expressions call back into parseExpr() — that mutual
// recursion is what lets "f(1 + g(x))" nest arbitrarily deep.
//
// Binary operators use the operator-precedence technique from the LLVM
// tutorial: parseBinOpRHS() eats operator/operand pairs left to right, and
// precedence numbers decide whether "x + y * 2" groups as "(x + y) * 2"
// (it doesn't) or "x + (y * 2)" (it does).
//
// Parse errors are reported with llvm::Expected<T>, LLVM's error-or-value
// return type: callers must check for the error state before touching the
// value (asserting builds enforce this at runtime), which makes forgetting
// error handling loud instead of silent.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_PARSE_PARSER_H
#define KALEIDOSCOPE_PARSE_PARSER_H

#include "kaleidoscope/AST/Expr.h"
#include "kaleidoscope/Lex/Lexer.h"

#include "llvm/Support/Error.h"

namespace kaleidoscope {

class ASTContext;

class Parser {
public:
  /// Parsed nodes are allocated in (and owned by) Ctx.
  /// Note: reads the first token from Lex immediately (see Cur below).
  Parser(Lexer &Lex, ASTContext &Ctx) : Lex(Lex), Ctx(Ctx), Cur(Lex.Next()) {}

  /// expression ::= primary (binop primary)*
  llvm::Expected<Expr *> parseExpr();

private:
  /// Binary-operator precedence, as a dedicated type so a precedence can't
  /// be confused with (or silently converted from) an unrelated integer.
  /// The underlying values are spaced out because step 8 of the tutorial
  /// lets users declare new operators with numeric precedences in between.
  /// (Clang's fixed grammar affords it a dense named enum instead:
  /// clang/Basic/OperatorPrecedence.h's prec::Level.)
  enum class Prec : int {
    Invalid = -1, // Not a binary operator: loses every comparison.
    Lowest = 0,   // parseExpr's starting bound: accepts any operator.
    Comparison = 10,     // <
    Additive = 20,       // +, -
    Multiplicative = 40, // *
  };

  static Prec getPrecedence(const Token &Tok);

  /// One step tighter than P: the recursion bound that encodes
  /// left-associativity in parseBinOpRHS.
  static Prec oneTighter(Prec P) {
    return Prec(static_cast<int>(P) + 1);
  }

  /// primary ::= number
  ///           | identifier
  ///           | identifier '(' (expression (',' expression)*)? ')'
  ///           | '(' expression ')'
  llvm::Expected<Expr *> parsePrimary();

  /// Parse "(binop primary)*" continuations of LHS, as long as the next
  /// operator binds at least as tightly as MinPrec.
  llvm::Expected<Expr *> parseBinOpRHS(Prec MinPrec, Expr *LHS);

  void advance() { Cur = Lex.Next(); }

  /// If the current token is of kind K, consume it and return true;
  /// otherwise leave it in place and return false.
  bool consumeIf(tok::TokenKind K);

  Lexer &Lex;
  ASTContext &Ctx;
  // One token of lookahead: the parser picks the grammar rule to apply by
  // inspecting the current token without consuming it.
  Token Cur;
};

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_PARSE_PARSER_H
