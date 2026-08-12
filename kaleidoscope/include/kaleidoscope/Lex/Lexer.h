//===- Lexer.h - Kaleidoscope step 1: the lexer ---------------------------===//
//
// A compiler never looks at raw characters directly. The first stage, the
// lexer (also called a tokenizer or scanner), groups characters into
// *tokens*: "def" is one token, the number "12.5" is one token, "(" is one
// token. Everything downstream (the parser) works with this token stream
// instead of characters.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_LEX_LEXER_H
#define KALEIDOSCOPE_LEX_LEXER_H

#include "kaleidoscope/Lex/Token.h"

#include "llvm/ADT/StringRef.h"

namespace kaleidoscope {

/// Produces the token stream for one source buffer.
///
/// Production-style, like clang lexing a MemoryBuffer: the lexer walks a
/// contiguous, caller-owned buffer with a cursor, and every token's spelling
/// is a slice of that buffer — lexing allocates nothing. (This is why the
/// earlier std::istream design had to go: a stream cannot hand out views of
/// input it has already consumed.)
class Lexer {
public:
  /// Buffer must outlive every token this lexer produces.
  explicit Lexer(llvm::StringRef Buffer)
      : Cur(Buffer.begin()), End(Buffer.end()) {}

  /// Lex and return the next token.
  ///
  /// At end of input returns tok::eof, and keeps returning it on further
  /// calls: running out of input is a stable condition rather than a
  /// one-shot event, so lookahead code may always ask for the next token
  /// without tracking whether it already saw the end.
  Token Next();

private:
  /// Make a token of kind Kind spanning [TokStart, Cur).
  Token formToken(tok::TokenKind Kind, const char *TokStart) {
    return Token(Kind, llvm::StringRef(TokStart, Cur - TokStart));
  }

  // Cursor into the caller's buffer: the characters in [Cur, End) are not
  // yet lexed.
  const char *Cur;
  const char *End;
};

} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEX_LEXER_H
