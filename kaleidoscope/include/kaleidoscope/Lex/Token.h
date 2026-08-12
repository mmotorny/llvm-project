//===- Token.h - Kaleidoscope token ---------------------------------------===//
//
// Production-style token modeling, after clang's Token and MLIR's: one flat,
// trivially copyable class for every kind of token. A token is its kind plus
// its *spelling* — the exact source characters it was lexed from, borrowed
// from the source buffer, never owned. Lexing therefore allocates nothing.
//
// This replaces the earlier std::variant design and makes its opposite
// trade: the variant made wrong-payload access a compile error; here payload
// accessors are guarded by asserts, so the mistake is caught at runtime in
// asserting builds. In exchange tokens become uniform, copyable,
// allocation-free values — the properties production lexers optimize for.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_LEX_TOKEN_H
#define KALEIDOSCOPE_LEX_TOKEN_H

#include "kaleidoscope/Lex/TokenKinds.h"

#include "llvm/ADT/StringRef.h"

#include <cassert>

namespace kaleidoscope {

class Lexer;

class Token {
public:
  tok::TokenKind getKind() const { return Kind; }

  /// Predicates in clang's idiom: `if (Tok.is(tok::l_paren))`.
  bool is(tok::TokenKind K) const { return Kind == K; }
  bool isNot(tok::TokenKind K) const { return Kind != K; }
  template <typename... Ts> bool isOneOf(Ts... Ks) const {
    static_assert(sizeof...(Ts) > 0, "requires at least one kind");
    return (is(Ks) || ...);
  }

  /// The exact source characters this token was lexed from — a view into
  /// the source buffer, valid only as long as the buffer lives.
  llvm::StringRef getSpelling() const { return Spelling; }

  /// The identifier's name; asserts this is an identifier.
  llvm::StringRef getIdentifier() const {
    assert(is(tok::identifier) && "not an identifier");
    return Spelling;
  }

  /// The literal's numeric value, decoded from the spelling on demand — a
  /// token nobody asks never pays for the conversion. Asserts this is a
  /// number.
  double getNumber() const;

private:
  friend class Lexer; // Only the lexer mints tokens.
  Token(tok::TokenKind Kind, llvm::StringRef Spelling)
      : Kind(Kind), Spelling(Spelling) {}

  tok::TokenKind Kind;
  llvm::StringRef Spelling;
};

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEX_TOKEN_H
