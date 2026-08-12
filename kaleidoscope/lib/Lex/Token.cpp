//===- Token.cpp - Kaleidoscope token -------------------------------------===//

#include "kaleidoscope/Lex/Token.h"

using namespace kaleidoscope;

double Token::getNumber() const {
  assert(is(tok::number) && "not a number");
  double Value = 0;
  [[maybe_unused]] bool Failed = Spelling.getAsDouble(Value);
  assert(!Failed && "lexer formed a number that does not parse");
  return Value;
}
