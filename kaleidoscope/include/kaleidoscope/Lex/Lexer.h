//===- Lexer.h - Kaleidoscope step 1: the lexer ---- ---------------------------===//
//
// A compiler never looks at raw characters directly. The first stage, the
// lexer (also called a tokenizer or scanner), groups characters into
// *tokens*: "def" is one token, the number "12.5" is one token, "(" is one
// token. Everything downstream (the parser, in step 2) works with this token
// stream instead of characters.
//
// Design note: the official tutorial keeps the lexer state in globals and a
// function-local static. We deviate: a Lexer instance owns all of its state
// and reads from any std::istream, and a token is a std::variant where each
// kind carries its own payload — so the payload for the wrong kind can't
// even be named, and independent Lexer instances are safe to use from
// different threads.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_LEX_LEXER_H
#define KALEIDOSCOPE_LEX_LEXER_H

#include <iosfwd>
#include <string>
#include <variant>

// Everything lives in the project namespace, clang-style: one namespace for
// the whole project regardless of directory (clang::), with rare semantic
// nested namespaces — tok below parallels clang::tok.
namespace kaleidoscope {

// One struct per token kind; a payload exists only on the kinds that have
// one. Operators and punctuation the lexer doesn't otherwise recognize
// ('+', '(', ',', ...) come through as Char.
namespace tok {
struct Eof {};
struct Def {};
struct Extern {};
struct Identifier {
  std::string Name;
};
struct Number {
  double Value;
};
struct Char {
  char Ch;
};

// Equality, needed by the tests (and by std::variant's operator==). Written
// by hand: defaulted comparisons arrive only in C++20, and we build as
// C++17 to match LLVM.
inline bool operator==(const Eof &, const Eof &) { return true; }
inline bool operator==(const Def &, const Def &) { return true; }
inline bool operator==(const Extern &, const Extern &) { return true; }
inline bool operator==(const Identifier &L, const Identifier &R) {
  return L.Name == R.Name;
}
inline bool operator==(const Number &L, const Number &R) {
  return L.Value == R.Value;
}
inline bool operator==(const Char &L, const Char &R) { return L.Ch == R.Ch; }
} // namespace tok

using Token = std::variant<tok::Eof, tok::Def, tok::Extern, tok::Identifier,
                           tok::Number, tok::Char>;

// Printable for test-failure messages and debugging. Declared in namespace
// tok so argument-dependent lookup finds it for Token (a std::variant over
// tok:: types).
namespace tok {
std::ostream &operator<<(std::ostream &OS, const Token &Tok);
} // namespace tok

class Lexer {
public:
  explicit Lexer(std::istream &In) : In(In) {}

  /// Consume characters from the stream and return the next token.
  ///
  /// At end of input returns tok::Eof, and keeps returning it on further
  /// calls. Like the underlying stream's eof state, running out of input is
  /// a stable condition rather than a one-shot event, so callers (the
  /// parser's lookahead in particular) may always ask for the next token
  /// without tracking whether they already saw the end.
  Token Next();

private:
  // The stream doubles as the lexer's one character of lookahead: peek()
  // lets Next() see where a token ends without consuming the character
  // that starts the next one.
  std::istream &In;
};

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEX_LEXER_H
