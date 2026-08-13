//===- toy.cpp - Kaleidoscope compiler driver -----------------------------===//
//
// The driver is the program a user actually runs; the phase libraries do
// the work. For now "the work" is parsing: each top-level entity is parsed
// and its AST printed back, which is exactly what the lit tests FileCheck.
// From step 4 on, this is where printed ASTs become printed LLVM IR.
//
// A Kaleidoscope program is a sequence of top-level entities: function
// definitions, extern declarations, and bare expressions to evaluate. The
// loop below picks the rule for each the way the parser picks rules
// internally — by looking at one token:
//
//     toplevel ::= definition | external | expression
//
// Two modes, following clang-query's driver:
//
//  - Given a file (or "-" for a pipe): parse the whole buffer, stopping at
//    the first error. This is the mode lit tests use.
//  - Given no arguments on a terminal: an interactive
//    read-evaluate-print loop (REPL) via llvm::LineEditor, which supplies
//    the prompt, line editing, and history. Each line must be a complete
//    top-level entity — a line is lexed as its own buffer, so an entity
//    cannot continue onto the next line — and an error discards only that
//    line.
//
//===----------------------------------------------------------------------===//

#include "kaleidoscope/AST/Decl.h"
#include "kaleidoscope/AST/Expr.h"
#include "kaleidoscope/Lex/Lexer.h"
#include "kaleidoscope/Parse/Parser.h"

#include "llvm/LineEditor/LineEditor.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <string>
#include <system_error>

using namespace kaleidoscope;

static llvm::cl::opt<std::string>
    InputFilename(llvm::cl::Positional, llvm::cl::desc("<input file>"),
                  llvm::cl::init(""));

// Print the parsed entity, or report the parse error; false on error.
template <typename T> static bool printOrReport(llvm::Expected<T *> Parsed) {
  if (!Parsed) {
    llvm::WithColor::error(llvm::errs(), "toy")
        << llvm::toString(Parsed.takeError()) << '\n';
    return false;
  }
  llvm::outs() << **Parsed << '\n';
  return true;
}

// Parse and print every top-level entity in Buffer, stopping at the first
// error (the grammar has no separators to resynchronize on): false if one
// occurred.
static bool handleBuffer(llvm::StringRef Buffer) {
  Lexer Lex(Buffer);
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  while (P.getCurToken().isNot(tok::eof)) {
    switch (P.getCurToken().getKind()) {
    case tok::kw_def:
      if (!printOrReport(P.parseDefinition()))
        return false;
      break;
    case tok::kw_extern:
      if (!printOrReport(P.parseExtern()))
        return false;
      break;
    default:
      if (!printOrReport(P.parseExpr()))
        return false;
      break;
    }
  }
  return true;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);
  llvm::cl::ParseCommandLineOptions(argc, argv, "Kaleidoscope compiler\n");

  if (!InputFilename.empty()) {
    auto Buffer = llvm::MemoryBuffer::getFileOrSTDIN(InputFilename);
    if (std::error_code EC = Buffer.getError()) {
      llvm::WithColor::error(llvm::errs(), "toy")
          << InputFilename << ": " << EC.message() << '\n';
      return 1;
    }
    return handleBuffer((*Buffer)->getBuffer()) ? 0 : 1;
  }

  llvm::LineEditor LE("toy");
  LE.setPrompt("ready> ");
  while (std::optional<std::string> Line = LE.readLine())
    handleBuffer(*Line);
  return 0;
}
