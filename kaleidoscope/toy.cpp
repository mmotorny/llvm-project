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
//===----------------------------------------------------------------------===//

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] (a character's own ASCII value) for
// one-character tokens it doesn't know, like '+' or '(', and one of these
// negative values for everything it does know. Distinguishing them is why the
// token type is int rather than this enum.
enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5,
};

// Some tokens carry a value with them: for tok_identifier the actual name,
// for tok_number the numeric value. The lexer leaves them in these globals
// each time it returns one of those tokens.
static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

/// gettok - Return the next token from standard input.
static int gettok() {
  // The last character read but not yet consumed. Starts as ' ' so the first
  // call falls straight into the whitespace-skipping loop and reads for real.
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getchar();

  if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;

    // Keywords are just identifiers the lexer special-cases.
    if (IdentifierStr == "def")
      return tok_def;
    if (IdentifierStr == "extern")
      return tok_extern;
    return tok_identifier;
  }

  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    // Sloppy on purpose (accepts "1.2.3"); the tutorial leaves fixing this
    // as an exercise.
    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  if (LastChar == '#') {
    // Comment until end of line.
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  // Check for end of file. Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

//===----------------------------------------------------------------------===//
// Driver
//===----------------------------------------------------------------------===//

// Temporary driver so this step is testable on its own: read stdin and print
// each token on its own line. Replaced by the parser driver in step 2.
int main() {
  while (true) {
    int Tok = gettok();
    switch (Tok) {
    case tok_eof:
      printf("eof\n");
      return 0;
    case tok_def:
      printf("def\n");
      break;
    case tok_extern:
      printf("extern\n");
      break;
    case tok_identifier:
      printf("identifier: %s\n", IdentifierStr.c_str());
      break;
    case tok_number:
      printf("number: %g\n", NumVal);
      break;
    default:
      printf("char: '%c'\n", (char)Tok);
      break;
    }
  }
}
