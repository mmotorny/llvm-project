//===- Lexer.cpp - Kaleidoscope lexer implementation ----------------------===//

#include "kaleidoscope/Lex/Lexer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <istream>
#include <ostream>

using namespace kaleidoscope;

namespace kaleidoscope::tok {
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
} // namespace kaleidoscope::tok

Token Lexer::Next() {
  // Comments are whitespace as far as the parser is concerned, so skip both
  // together: any run of blanks, and everything from '#' to end of line.
  while (true) {
    if (std::isspace(In.peek()))
      In.get();
    else if (In.peek() == '#')
      do
        In.get();
      while (In.peek() != EOF && In.peek() != '\n' && In.peek() != '\r');
    else
      break;
  }

  if (std::isalpha(In.peek())) { // identifier: [a-zA-Z][a-zA-Z0-9]*
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

  if (std::isdigit(In.peek()) ||
      In.peek() == '.') { // number: [0-9]* ('.' [0-9]*)?
    std::string NumStr;
    while (std::isdigit(In.peek()))
      NumStr += char(In.get());
    if (In.peek() == '.') {
      NumStr += char(In.get());
      while (std::isdigit(In.peek()))
        NumStr += char(In.get());
    }
    if (NumStr == ".") // a lone dot is not a number after all
      return tok::Char{'.'};
    return tok::Number{std::strtod(NumStr.c_str(), nullptr)};
  }

  if (In.peek() == EOF)
    return tok::Eof{};

  // Otherwise, hand the character through as-is.
  return tok::Char{char(In.get())};
}
