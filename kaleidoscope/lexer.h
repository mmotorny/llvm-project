//===- lexer.h - Kaleidoscope step 1: the lexer ---------------------------===//
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

#ifndef KALEIDOSCOPE_LEXER_H
#define KALEIDOSCOPE_LEXER_H

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <istream>
#include <ostream>
#include <string>
#include <variant>

// One struct per token kind; a payload exists only on the kinds that have
// one. Operators and punctuation the lexer doesn't otherwise recognize
// ('+', '(', ',', ...) come through as Char.
namespace tok {
struct Eof {
  bool operator==(const Eof &) const = default;
};
struct Def {
  bool operator==(const Def &) const = default;
};
struct Extern {
  bool operator==(const Extern &) const = default;
};
struct Identifier {
  std::string Name;
  bool operator==(const Identifier &) const = default;
};
struct Number {
  double Value;
  bool operator==(const Number &) const = default;
};
struct Char {
  char Ch;
  bool operator==(const Char &) const = default;
};
} // namespace tok

using Token = std::variant<tok::Eof, tok::Def, tok::Extern, tok::Identifier,
                           tok::Number, tok::Char>;

// Printable for test-failure messages and debugging. Defined in namespace
// tok so argument-dependent lookup finds it for Token (a std::variant over
// tok:: types).
namespace tok {
inline std::ostream &operator<<(std::ostream &OS, const Token &Tok) {
  std::visit(
      [&](const auto &T) {
        using T2 = std::decay_t<decltype(T)>;
        if constexpr (std::is_same_v<T2, Eof>)
          OS << "eof";
        else if constexpr (std::is_same_v<T2, Def>)
          OS << "def";
        else if constexpr (std::is_same_v<T2, Extern>)
          OS << "extern";
        else if constexpr (std::is_same_v<T2, Identifier>)
          OS << "identifier " << T.Name;
        else if constexpr (std::is_same_v<T2, Number>)
          OS << "number " << T.Value;
        else if constexpr (std::is_same_v<T2, Char>)
          OS << "char '" << T.Ch << "'";
      },
      Tok);
  return OS;
}
} // namespace tok

class Lexer {
public:
  explicit Lexer(std::istream &In) : In(In) {}

  /// Consume characters from the stream and return the next token.
  Token Next() {
    // Skip any whitespace.
    while (std::isspace(LastChar))
      LastChar = In.get();

    if (std::isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
      std::string Name(1, char(LastChar));
      while (std::isalnum(LastChar = In.get()))
        Name += char(LastChar);

      // Keywords are just identifiers the lexer special-cases.
      if (Name == "def")
        return tok::Def{};
      if (Name == "extern")
        return tok::Extern{};
      return tok::Identifier{std::move(Name)};
    }

    if (std::isdigit(LastChar) || LastChar == '.') { // number: [0-9.]+
      std::string NumStr;
      do {
        NumStr += char(LastChar);
        LastChar = In.get();
      } while (std::isdigit(LastChar) || LastChar == '.');

      // Sloppy on purpose (accepts "1.2.3"); the tutorial leaves fixing
      // this as an exercise.
      return tok::Number{std::strtod(NumStr.c_str(), nullptr)};
    }

    if (LastChar == '#') {
      // Comment until end of line.
      do
        LastChar = In.get();
      while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return Next();
    }

    // Check for end of file. Don't eat the EOF.
    if (LastChar == EOF)
      return tok::Eof{};

    // Otherwise, hand the character through as-is.
    char ThisChar = char(LastChar);
    LastChar = In.get();
    return tok::Char{ThisChar};
  }

private:
  std::istream &In;
  // One character of lookahead: to know where a token like `fib` ends, the
  // lexer must read one character past it; that character belongs to the
  // *next* token, so it is held here between calls. Starts as ' ' so the
  // first Next() falls straight into the whitespace loop and reads for real.
  int LastChar = ' ';
};

#endif // KALEIDOSCOPE_LEXER_H
