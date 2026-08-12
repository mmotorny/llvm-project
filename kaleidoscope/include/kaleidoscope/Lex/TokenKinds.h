//===- TokenKinds.h - Kaleidoscope token kinds ----------------------------===//
//
// One enumerator per token kind, production-style: every keyword and every
// punctuator gets its own kind (clang's tok::kw_int, tok::l_paren), so the
// parser compares named constants — `Tok.is(tok::l_paren)` — instead of
// matching raw characters, and switches over operators exhaustively.
//
// Clang generates this enum (and the spelling table, and keyword
// recognition) from an X-macro file, TokenKinds.def, because it has ~500
// kinds. At our size the plain enum is clearer; if the kind list ever grows
// past what a screen holds, the .def file is the known upgrade path.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_LEX_TOKENKINDS_H
#define KALEIDOSCOPE_LEX_TOKENKINDS_H

namespace kaleidoscope::tok {

enum TokenKind : unsigned short {
  unknown, // A character the lexer doesn't recognize.
  eof,     // End of input.

  identifier, // fib, x
  number,     // 1.25

  // Keywords.
  kw_def,
  kw_extern,

  // Punctuators.
  l_paren, // (
  r_paren, // )
  comma,   // ,
  plus,    // +
  minus,   // -
  star,    // *
  less,    // <
};

/// The fixed spelling of Kind — "(" for l_paren, "def" for kw_def — or
/// nullptr for the kinds whose spelling varies per token (identifier,
/// number) or that have none (eof, unknown).
const char *getSpelling(TokenKind Kind);

} // namespace kaleidoscope::tok

#endif // KALEIDOSCOPE_LEX_TOKENKINDS_H
