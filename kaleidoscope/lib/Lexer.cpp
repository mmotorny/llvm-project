//===- Lexer.cpp - Kaleidoscope lexer implementation -- ----------------------===//

#include "kaleidoscope/Lexer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <istream>
#include <ostream>

namespace tok {
std::ostream &operator<<(std::ostream &OS, const Token &Tok) {
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

Token Lexer::Next() {
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
