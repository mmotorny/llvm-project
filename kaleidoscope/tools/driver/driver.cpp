//===- driver.cpp - Kaleidoscope compiler driver --------------------------===//
//
// The driver is the program a user actually runs; the phase libraries do
// the work. As of step 4 "the work" is compilation to LLVM IR: every
// top-level entity is parsed and emitted into one llvm::Module. Under
// -ast-dump (clang's flag) the driver stops after parsing and prints each
// entity's AST instead — the form the parser lit tests check.
//
// A Kaleidoscope program is a sequence of top-level entities: function
// definitions, extern declarations, and bare expressions to evaluate. The
// loop below picks the rule for each the way the parser picks rules
// internally — by looking at one token:
//
//     toplevel ::= definition | external | expression
//
// A bare expression has no place of its own in IR — instructions only
// exist inside functions — so it is wrapped in an anonymous nullary
// function, the tutorial's __anon_expr device. Once there is a JIT (step
// 5), evaluating an expression means calling its anonymous function.
//
// Two modes, following clang-query's driver:
//
//  - Given a file (or "-" for a pipe): process the whole buffer, stopping
//    at the first error, and print the complete module at the end. This
//    is the mode lit tests use.
//  - Given no arguments on a terminal: an interactive
//    read-evaluate-print loop (REPL) via llvm::LineEditor, which supplies
//    the prompt, line editing, and history. Each line must be a complete
//    top-level entity — a line is lexed as its own buffer, so an entity
//    cannot continue onto the next line — and an error discards only that
//    line. Every line's IR goes into one session-long module, printed
//    entity by entity as it is emitted.
//
//===----------------------------------------------------------------------===//

#include "kaleidoscope/AST/Decl.h"
#include "kaleidoscope/AST/Expr.h"
#include "kaleidoscope/CodeGen/CodeGen.h"
#include "kaleidoscope/Lex/Lexer.h"
#include "kaleidoscope/Parse/Parser.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
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

static llvm::cl::opt<bool>
    ASTDump("ast-dump",
            llvm::cl::desc("Print each parsed entity's AST instead of "
                           "generating code"));

// Report an error; false so call sites can `return report(...)`.
static bool report(llvm::Error E) {
  llvm::WithColor::error(llvm::errs(), "kaleidoscope")
      << llvm::toString(std::move(E)) << '\n';
  return false;
}

// A name for a top-level expression's wrapper function that nothing else
// in M uses: "__anon_expr", "__anon_expr.1", ... The scan restarts per
// call, but each name found is then taken in M, so the sequence advances.
static llvm::StringRef uniqueAnonName(llvm::Module &M,
                                      llvm::BumpPtrAllocator &Alloc) {
  for (unsigned I = 0;; ++I) {
    llvm::SmallString<32> Name("__anon_expr");
    if (I) {
      Name += '.';
      Name += llvm::utostr(I);
    }
    if (!M.getNamedValue(Name))
      return llvm::StringRef(Name).copy(Alloc);
  }
}

// Parse every top-level entity in Buffer and either print its AST
// (-ast-dump) or emit it into M (printing each emitted function if
// PrintEachEntity — the REPL's mode; file mode prints the whole module
// once at the end instead). Stops at the first error (the grammar has no
// separators to resynchronize on): false if one occurred.
static bool handleBuffer(llvm::StringRef Buffer, llvm::Module &M,
                         bool PrintEachEntity) {
  Lexer Lex(Buffer);
  llvm::BumpPtrAllocator Alloc;
  Parser P(Lex, Alloc);

  while (P.getCurToken().isNot(tok::eof)) {
    FunctionDecl *D = nullptr;
    switch (P.getCurToken().getKind()) {
    case tok::kw_def: {
      auto Parsed = P.parseDefinition();
      if (!Parsed)
        return report(Parsed.takeError());
      D = *Parsed;
      break;
    }
    case tok::kw_extern: {
      auto Parsed = P.parseExtern();
      if (!Parsed)
        return report(Parsed.takeError());
      D = *Parsed;
      break;
    }
    default: {
      auto E = P.parseExpr();
      if (!E)
        return report(E.takeError());
      if (ASTDump) {
        llvm::outs() << **E << '\n';
        continue;
      }
      D = new (Alloc) FunctionDecl(uniqueAnonName(M, Alloc), {}, *E);
      break;
    }
    }

    if (ASTDump) {
      llvm::outs() << *D << '\n';
      continue;
    }
    auto F = emitFunctionDecl(M, *D);
    if (!F)
      return report(F.takeError());
    if (PrintEachEntity)
      llvm::outs() << **F;
  }
  return true;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);
  llvm::cl::ParseCommandLineOptions(argc, argv, "Kaleidoscope compiler\n");

  llvm::LLVMContext Ctx;
  llvm::Module M(InputFilename.empty() ? llvm::StringRef("<repl>")
                                       : llvm::StringRef(InputFilename),
                 Ctx);

  if (!InputFilename.empty()) {
    auto Buffer = llvm::MemoryBuffer::getFileOrSTDIN(InputFilename);
    if (std::error_code EC = Buffer.getError()) {
      llvm::WithColor::error(llvm::errs(), "kaleidoscope")
          << InputFilename << ": " << EC.message() << '\n';
      return 1;
    }
    if (!handleBuffer((*Buffer)->getBuffer(), M, /*PrintEachEntity=*/false))
      return 1;
    if (!ASTDump)
      llvm::outs() << M;
    return 0;
  }

  // LineEditor derives the "kaleidoscope> " prompt from the program name,
  // as clang-query's does.
  llvm::LineEditor LE("kaleidoscope");
  while (std::optional<std::string> Line = LE.readLine()) {
    handleBuffer(*Line, M, /*PrintEachEntity=*/true);
    llvm::outs().flush();
  }
  return 0;
}
