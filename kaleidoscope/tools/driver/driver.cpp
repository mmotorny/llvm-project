//===- driver.cpp - Kaleidoscope compiler driver --------------------------===//
//
// The driver is the program a user actually runs; the phase libraries do
// the work. As of step 5 that work reaches execution: the driver can
// optimize the IR it emits (-O) and, interactively, run it.
//
// A Kaleidoscope program is a sequence of top-level entities: function
// definitions, extern declarations, and bare expressions to evaluate. The
// walkTopLevel loop below picks the rule for each the way the parser picks
// rules internally — by looking at one token:
//
//     toplevel ::= definition | external | expression
//
// A bare expression has no place of its own in IR — instructions only
// exist inside functions — so it is wrapped in an anonymous nullary
// function, the tutorial's __anon_expr device; *evaluating* the expression
// means calling that function.
//
// Modes, following clang-query's driver:
//
//  - Given a file (or "-" for a pipe): compile the whole buffer into one
//    module and print its IR, stopping at the first error. This is the
//    mode most lit tests use. -ast-dump (clang's flag) stops after the
//    parser and prints each entity's AST instead.
//
//  - Given no arguments: a read-evaluate-print loop (REPL) on
//    llvm::LineEditor (prompt, line editing, history). "Evaluate" is
//    literal, via ORC — LLVM's JIT ("just-in-time") compilation library,
//    which compiles IR to native code in-process: definitions are
//    compiled as they arrive and bare expressions are executed, printing
//    their value. Each line must be a complete top-level entity — a line
//    is lexed as its own buffer, so an entity cannot continue onto the
//    next line — and an error discards only that line.
//
//===----------------------------------------------------------------------===//

#include "kaleidoscope/AST/Decl.h"
#include "kaleidoscope/AST/Expr.h"
#include "kaleidoscope/CodeGen/CodeGen.h"
#include "kaleidoscope/Lex/Lexer.h"
#include "kaleidoscope/Parse/Parser.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/LineEditor/LineEditor.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

using namespace kaleidoscope;

static llvm::cl::opt<std::string> InputFilename(llvm::cl::Positional,
                                                llvm::cl::desc("<input file>"),
                                                llvm::cl::init(""));

static llvm::cl::opt<bool>
    ASTDump("ast-dump",
            llvm::cl::desc("Print each parsed entity's AST instead of "
                           "generating code"));

static llvm::cl::opt<bool>
    Optimize("O", llvm::cl::desc("Run the optimization passes (instcombine, "
                                 "reassociate, gvn, simplifycfg)"));

// Report an error; false so call sites can `return report(...)`.
static bool report(llvm::Error E) {
  llvm::WithColor::error(llvm::errs(), "kaleidoscope")
      << llvm::toString(std::move(E)) << '\n';
  return false;
}

// The tutorial's four-pass pipeline, run over each function:
// instcombine (peephole algebraic simplification), reassociate (reorder
// commutative chains into a canonical form, exposing repeats), gvn
// ("global value numbering": reuse recomputed values), and simplifycfg
// (tidy the block graph). Off by default, like clang at -O0: unoptimized
// output is the ground truth the CodeGen tests check.
static void optimize(llvm::Module &M) {
  // New-pass-manager boilerplate: analyses live in per-IR-level managers,
  // and passes request them lazily; the proxies let a function analysis
  // reach a module analysis and vice versa.
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;
  llvm::PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  llvm::FunctionPassManager FPM;
  FPM.addPass(llvm::InstCombinePass());
  FPM.addPass(llvm::ReassociatePass());
  FPM.addPass(llvm::GVNPass());
  FPM.addPass(llvm::SimplifyCFGPass());

  for (llvm::Function &F : M)
    if (!F.isDeclaration())
      FPM.run(F, FAM);
}

// Parse every top-level entity in Buffer, handing declarations to OnDecl
// and bare expressions to OnExpr (each returns false to stop). Stops at
// the first error (the grammar has no separators to resynchronize on):
// false if one occurred.
template <typename DeclFn, typename ExprFn>
static bool walkTopLevel(llvm::StringRef Buffer, llvm::BumpPtrAllocator &Alloc,
                         DeclFn OnDecl, ExprFn OnExpr) {
  Lexer Lex(Buffer);
  Parser P(Lex, Alloc);

  while (P.getCurToken().isNot(tok::eof)) {
    switch (P.getCurToken().getKind()) {
    case tok::kw_def: {
      auto D = P.parseDefinition();
      if (!D)
        return report(D.takeError());
      if (!OnDecl(*D))
        return false;
      break;
    }
    case tok::kw_extern: {
      auto D = P.parseExtern();
      if (!D)
        return report(D.takeError());
      if (!OnDecl(*D))
        return false;
      break;
    }
    default: {
      auto E = P.parseExpr();
      if (!E)
        return report(E.takeError());
      if (!OnExpr(*E))
        return false;
      break;
    }
    }
  }
  return true;
}

// A name for a top-level expression's wrapper function that nothing else
// in M uses: "__anon_expr", "__anon_expr.1", ...
static void nextAnonName(unsigned N, llvm::SmallVectorImpl<char> &Name) {
  Name.clear();
  llvm::raw_svector_ostream OS(Name);
  OS << "__anon_expr";
  if (N)
    OS << '.' << N;
}

namespace {

// One interactive session. Every definition is JIT-compiled as it
// arrives; every bare expression is executed and its value printed.
//
// ORC owns each module it is given, so — unlike file mode's single
// module — every entity is emitted into a module of its own. What
// stitches them together is the prototype registry: a bodiless
// FunctionDecl per known function, re-declared into each fresh module so
// cross-line calls resolve (and arity/redefinition rules keep holding
// across lines, as the shared module makes them hold within a file).
class Repl {
public:
  static llvm::Expected<std::unique_ptr<Repl>> create();
  void run();

private:
  explicit Repl(std::unique_ptr<llvm::orc::LLJIT> JIT)
      : JIT(std::move(JIT)), Names(SessionAlloc) {}

  struct OwnedModule {
    std::unique_ptr<llvm::LLVMContext> Ctx;
    std::unique_ptr<llvm::Module> M;
  };
  OwnedModule makeModule();

  bool handleDecl(FunctionDecl *D);
  bool handleExpr(Expr *E);

  std::unique_ptr<llvm::orc::LLJIT> JIT;
  // The registry's decls outlive the per-line parse arenas, so their
  // names are re-saved here.
  llvm::BumpPtrAllocator SessionAlloc;
  llvm::StringSaver Names;
  struct ProtoInfo {
    FunctionDecl *Proto; // Bodiless: the interface only.
    bool Defined;        // Whether a 'def' provided the body yet.
  };
  llvm::StringMap<ProtoInfo> Protos;
  unsigned NumAnonExprs = 0;
};

} // namespace

llvm::Expected<std::unique_ptr<Repl>> Repl::create() {
  auto JIT = llvm::orc::LLJITBuilder().create();
  if (!JIT)
    return JIT.takeError();

  // Resolve symbols the JIT doesn't define against this very process:
  // "extern cos(x)" finds the libm already linked into the driver.
  auto ProcessSymbols =
      llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          (*JIT)->getDataLayout().getGlobalPrefix());
  if (!ProcessSymbols)
    return ProcessSymbols.takeError();
  (*JIT)->getMainJITDylib().addGenerator(std::move(*ProcessSymbols));

  return std::unique_ptr<Repl>(new Repl(std::move(*JIT)));
}

Repl::OwnedModule Repl::makeModule() {
  auto Ctx = std::make_unique<llvm::LLVMContext>();
  auto M = std::make_unique<llvm::Module>("<repl>", *Ctx);
  M->setDataLayout(JIT->getDataLayout());
  // Redeclare everything the session knows, so this module can call it.
  // Emitting a bodiless decl into a fresh module cannot fail.
  for (const auto &Entry : Protos)
    llvm::cantFail(emitFunctionDecl(*M, *Entry.getValue().Proto));
  return {std::move(Ctx), std::move(M)};
}

bool Repl::handleDecl(FunctionDecl *D) {
  // The rules a shared module enforces in file mode, enforced across
  // per-line modules by the registry instead.
  auto It = Protos.find(D->getName());
  if (It != Protos.end()) {
    if (It->second.Proto->getParams().size() != D->getParams().size())
      return report(llvm::createStringError(llvm::inconvertibleErrorCode(),
                                            "conflicting types for '" +
                                                D->getName() + "'"));
    if (D->hasBody() && It->second.Defined)
      return report(
          llvm::createStringError(llvm::inconvertibleErrorCode(),
                                  "redefinition of '" + D->getName() + "'"));
  }

  auto [Ctx, M] = makeModule();
  auto F = emitFunctionDecl(*M, *D);
  if (!F)
    return report(F.takeError());
  if (Optimize)
    optimize(*M);
  llvm::outs() << **F;

  if (It == Protos.end()) {
    llvm::SmallVector<llvm::StringRef, 8> Params;
    for (llvm::StringRef Param : D->getParams())
      Params.push_back(Names.save(Param));
    auto *Proto = new (SessionAlloc)
        FunctionDecl(Names.save(D->getName()),
                     llvm::ArrayRef(Params).copy(SessionAlloc), nullptr);
    It = Protos.insert({Proto->getName(), {Proto, false}}).first;
  }
  It->second.Defined |= D->hasBody();

  // Only a definition carries code; an extern's meaning is entirely in
  // the registry (and the process symbols it will resolve to).
  if (D->hasBody())
    if (llvm::Error Err = JIT->addIRModule(
            llvm::orc::ThreadSafeModule(std::move(M), std::move(Ctx))))
      return report(std::move(Err));
  return true;
}

bool Repl::handleExpr(Expr *E) {
  // Unique per session, not per module: every wrapper lands in the same
  // JIT dylib, where a second "__anon_expr" would be a duplicate symbol.
  llvm::SmallString<32> AnonName;
  nextAnonName(NumAnonExprs++, AnonName);

  auto [Ctx, M] = makeModule();
  FunctionDecl Anon(AnonName, /*Params=*/{}, E);
  auto F = emitFunctionDecl(*M, Anon);
  if (!F)
    return report(F.takeError());
  if (Optimize)
    optimize(*M);

  if (llvm::Error Err = JIT->addIRModule(
          llvm::orc::ThreadSafeModule(std::move(M), std::move(Ctx))))
    return report(std::move(Err));

  // Materialize (compile to native code), then call. The lookup is what
  // triggers compilation of everything the expression needs.
  auto Addr = JIT->lookup(AnonName);
  if (!Addr)
    return report(Addr.takeError());
  double Value = Addr->toPtr<double (*)()>()();
  llvm::outs() << llvm::format("%g", Value) << '\n';
  return true;
}

void Repl::run() {
  llvm::LineEditor LE("kaleidoscope");
  while (std::optional<std::string> Line = LE.readLine()) {
    llvm::BumpPtrAllocator Alloc;
    walkTopLevel(
        *Line, Alloc, [&](FunctionDecl *D) { return handleDecl(D); },
        [&](Expr *E) { return handleExpr(E); });
    llvm::outs().flush();
  }
}

// A name for a top-level expression's wrapper in file mode, where M is
// the uniqueness domain.
static llvm::StringRef uniqueAnonName(llvm::Module &M,
                                      llvm::BumpPtrAllocator &Alloc) {
  llvm::SmallString<32> Name;
  for (unsigned I = 0;; ++I) {
    nextAnonName(I, Name);
    if (!M.getNamedValue(Name))
      return llvm::StringRef(Name).copy(Alloc);
  }
}

static int runOnFile() {
  auto Buffer = llvm::MemoryBuffer::getFileOrSTDIN(InputFilename);
  if (std::error_code EC = Buffer.getError()) {
    llvm::WithColor::error(llvm::errs(), "kaleidoscope")
        << InputFilename << ": " << EC.message() << '\n';
    return 1;
  }

  llvm::LLVMContext Ctx;
  llvm::Module M(InputFilename, Ctx);
  llvm::BumpPtrAllocator Alloc;

  bool OK = walkTopLevel((*Buffer)->getBuffer(), Alloc,
                         [&](FunctionDecl *D) {
                           if (ASTDump) {
                             llvm::outs() << *D << '\n';
                             return true;
                           }
                           auto F = emitFunctionDecl(M, *D);
                           return F ? true : report(F.takeError());
                         },
                         [&](Expr *E) {
                           if (ASTDump) {
                             llvm::outs() << *E << '\n';
                             return true;
                           }
                           auto *D = new (Alloc)
                               FunctionDecl(uniqueAnonName(M, Alloc),
                                            /*Params=*/{}, E);
                           auto F = emitFunctionDecl(M, *D);
                           return F ? true : report(F.takeError());
                         });
  if (!OK)
    return 1;

  if (!ASTDump) {
    if (Optimize)
      optimize(M);
    llvm::outs() << M;
  }
  return 0;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);
  llvm::cl::ParseCommandLineOptions(argc, argv, "Kaleidoscope compiler\n");

  if (!InputFilename.empty())
    return runOnFile();

  if (ASTDump) {
    // Parse-only REPL: no JIT to set up.
    llvm::LineEditor LE("kaleidoscope");
    while (std::optional<std::string> Line = LE.readLine()) {
      llvm::BumpPtrAllocator Alloc;
      walkTopLevel(
          *Line, Alloc,
          [](FunctionDecl *D) {
            llvm::outs() << *D << '\n';
            return true;
          },
          [](Expr *E) {
            llvm::outs() << *E << '\n';
            return true;
          });
      llvm::outs().flush();
    }
    return 0;
  }

  // The JIT compiles for the machine it runs on: pull in this target's
  // backend (the only one the interactive mode ever needs).
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  auto R = Repl::create();
  if (!R) {
    report(R.takeError());
    return 1;
  }
  (*R)->run();
  return 0;
}
