//===- Decl.cpp - Kaleidoscope function declaration printing --------------===//

#include "kaleidoscope/AST/Decl.h"

#include "kaleidoscope/AST/Expr.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

using namespace kaleidoscope;

void FunctionDecl::print(llvm::raw_ostream &OS) const {
  OS << '(' << (hasBody() ? "def" : "extern") << " (" << getName();
  for (llvm::StringRef Param : getParams())
    OS << ' ' << Param;
  OS << ')';
  if (hasBody())
    OS << ' ' << getBody();
  OS << ')';
}

LLVM_DUMP_METHOD void FunctionDecl::dump() const {
  print(llvm::errs());
  llvm::errs() << '\n';
}
