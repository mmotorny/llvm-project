//===- toy.cpp - Kaleidoscope, built step by step -------------------------===//
//
// Step 1: the lexer.
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

#include <cctype>
#include <cstdlib>
#include <istream>
#include <iostream>
#include <print>
#include <string>
#include <variant>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

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
} // namespace tok

using Token = std::variant<tok::Eof, tok::Def, tok::Extern, tok::Identifier,
                           tok::Number, tok::Char>;

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

//===----------------------------------------------------------------------===//
// Driver
//===----------------------------------------------------------------------===//

// The standard trick for building a std::visit visitor out of lambdas: a
// struct that inherits every lambda's operator().
template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

// Temporary driver so this step is testable on its own: read stdin and print
// each token on its own line. Replaced by the parser driver in step 2.
int main() {
  Lexer Lex(std::cin);
  for (;;) {
    Token Tok = Lex.Next();
    if (std::holds_alternative<tok::Eof>(Tok)) {
      std::println("eof");
      return 0;
    }
    std::visit(Overloaded{
                   [](tok::Eof) {},
                   [](tok::Def) { std::println("def"); },
                   [](tok::Extern) { std::println("extern"); },
                   [](const tok::Identifier &I) {
                     std::println("identifier: {}", I.Name);
                   },
                   [](tok::Number N) { std::println("number: {}", N.Value); },
                   [](tok::Char C) { std::println("char: '{}'", C.Ch); },
               },
               Tok);
  }
}
