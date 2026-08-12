//===- TokenKinds.cpp - Kaleidoscope token kinds --------------------------===//

#include "kaleidoscope/Lex/TokenKinds.h"

#include "llvm/Support/ErrorHandling.h"

const char *kaleidoscope::tok::getSpelling(TokenKind Kind) {
  switch (Kind) {
  case unknown:
  case eof:
  case identifier:
  case number:
    return nullptr;
  case kw_def:
    return "def";
  case kw_extern:
    return "extern";
  case l_paren:
    return "(";
  case r_paren:
    return ")";
  case comma:
    return ",";
  case plus:
    return "+";
  case minus:
    return "-";
  case star:
    return "*";
  case less:
    return "<";
  }
  llvm_unreachable("unknown token kind");
}
