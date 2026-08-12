//===- Lexer.cpp - Kaleidoscope lexer implementation ----------------------===//

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
  while (std::isspace(In.peek()))
    In.get();

  int C = In.peek();

  if (std::isalpha(C)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
    std::string Name;
    while (std::isalnum(In.peek()))
      Name += char(In.get());

    // Keywords are just identifiers the lexer special-cases.
    if (Name == "def")
      return tok::Def{};
    if (Name == "extern")
      return tok::Extern{};
    return tok::Identifier{std::move(Name)};
  }

  if (std::isdigit(C) || C == '.') { // number: [0-9.]+
    std::string NumStr;
    while (std::isdigit(In.peek()) || In.peek() == '.')
      NumStr += char(In.get());

    // Sloppy on purpose (accepts "1.2.3"); the tutorial leaves fixing
    // this as an exercise.
    return tok::Number{std::strtod(NumStr.c_str(), nullptr)};
  }

  if (C == '#') {
    // Comment until end of line, then start over on the next line.
    while (In.peek() != EOF && In.peek() != '\n' && In.peek() != '\r')
      In.get();
    return Next();
  }

  if (C == EOF)
    return tok::Eof{};

  // Otherwise, hand the character through as-is.
  return tok::Char{char(In.get())};
}
