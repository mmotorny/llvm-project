//===- Lexer.cpp - Kaleidoscope lexer implementation ----------------------===//

#include "kaleidoscope/Lex/Lexer.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSwitch.h"

using namespace kaleidoscope;
using llvm::isAlnum;
using llvm::isAlpha;
using llvm::isDigit;
using llvm::isSpace;

// Keywords are identifiers with reserved spellings. Clang drives this from
// its TokenKinds.def and a hash table of interned identifiers; a
// StringSwitch is the right size for two keywords.
static tok::TokenKind classifyIdentifier(llvm::StringRef Spelling) {
  return llvm::StringSwitch<tok::TokenKind>(Spelling)
      .Case("def", tok::kw_def)
      .Case("extern", tok::kw_extern)
      .Default(tok::identifier);
}

Token Lexer::Next() {
  // Skip whitespace and comments ('#' to end of line) together: both are
  // equally invisible to the parser.
  while (Cur != End) {
    if (isSpace(*Cur))
      ++Cur;
    else if (*Cur == '#')
      while (Cur != End && *Cur != '\n' && *Cur != '\r')
        ++Cur;
    else
      break;
  }

  const char *TokStart = Cur;

  if (Cur == End)
    return formToken(tok::eof, TokStart);

  char C = *Cur;

  if (isAlpha(C)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
    do
      ++Cur;
    while (Cur != End && isAlnum(*Cur));
    llvm::StringRef Spelling(TokStart, Cur - TokStart);
    return Token(classifyIdentifier(Spelling), Spelling);
  }

  if (isDigit(C) || C == '.') { // number: [0-9]* ('.' [0-9]*)?
    while (Cur != End && isDigit(*Cur))
      ++Cur;
    if (Cur != End && *Cur == '.') {
      ++Cur;
      while (Cur != End && isDigit(*Cur))
        ++Cur;
    }
    // A lone '.' contains no digits and is not a number after all.
    if (Cur - TokStart == 1 && C == '.')
      return formToken(tok::unknown, TokStart);
    return formToken(tok::number, TokStart);
  }

  ++Cur;
  switch (C) {
  case '(':
    return formToken(tok::l_paren, TokStart);
  case ')':
    return formToken(tok::r_paren, TokStart);
  case ',':
    return formToken(tok::comma, TokStart);
  case '+':
    return formToken(tok::plus, TokStart);
  case '-':
    return formToken(tok::minus, TokStart);
  case '*':
    return formToken(tok::star, TokStart);
  case '<':
    return formToken(tok::less, TokStart);
  default:
    return formToken(tok::unknown, TokStart);
  }
}
