//===- CodeGen.cpp - Kaleidoscope IR generation implementation ------------===//

#include "kaleidoscope/CodeGen/CodeGen.h"

#include "kaleidoscope/AST/Decl.h"
#include "kaleidoscope/AST/Expr.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>

using namespace kaleidoscope;

// Error messages borrow clang's diagnostic wording ("use of undeclared
// identifier", "redefinition of ...") so they read like a compiler's, not
// an interpreter's.
static llvm::Error makeCodeGenError(const llvm::Twine &Msg) {
  return llvm::createStringError(llvm::inconvertibleErrorCode(), Msg);
}

namespace {

// Per-declaration emission state, a miniature of clang's CodeGenFunction:
// the IRBuilder tracks where the next instruction is inserted, and
// NamedValues is the body's whole scope — parameter names to their
// llvm::Values. Cross-declaration lookups go through the module instead
// (Module::getFunction), so none of this state outlives one declaration.
class FunctionEmitter {
public:
  explicit FunctionEmitter(llvm::Module &M) : M(M), Builder(M.getContext()) {}

  llvm::Expected<llvm::Function *> emit(const FunctionDecl &D);

private:
  llvm::Expected<llvm::Value *> emitExpr(const Expr &E);

  llvm::Module &M;
  llvm::IRBuilder<> Builder;
  llvm::StringMap<llvm::Value *> NamedValues;
};

} // namespace

llvm::Expected<llvm::Value *> FunctionEmitter::emitExpr(const Expr &E) {
  switch (E.getKind()) {
  case Expr::Kind::Number:
    return llvm::ConstantFP::get(Builder.getDoubleTy(),
                                 llvm::cast<NumberExpr>(E).getValue());

  case Expr::Kind::Variable: {
    llvm::StringRef Name = llvm::cast<VariableExpr>(E).getName();
    if (llvm::Value *V = NamedValues.lookup(Name))
      return V;
    return makeCodeGenError("use of undeclared identifier '" + Name + "'");
  }

  case Expr::Kind::Binary: {
    const auto &B = llvm::cast<BinaryExpr>(E);
    auto LHS = emitExpr(B.getLHS());
    if (!LHS)
      return LHS.takeError();
    auto RHS = emitExpr(B.getRHS());
    if (!RHS)
      return RHS.takeError();

    // The value names ("addtmp", ...) follow the tutorial; they only make
    // the IR easier to read, and LLVM uniques them as needed.
    switch (B.getOp()) {
    case '+':
      return Builder.CreateFAdd(*LHS, *RHS, "addtmp");
    case '-':
      return Builder.CreateFSub(*LHS, *RHS, "subtmp");
    case '*':
      return Builder.CreateFMul(*LHS, *RHS, "multmp");
    case '<': {
      // fcmp produces an i1; convert it back to 0.0/1.0, the language's
      // only type. "ult" is an *unordered* comparison: true if either
      // operand is NaN — the tutorial's choice, kept as-is.
      llvm::Value *Cmp = Builder.CreateFCmpULT(*LHS, *RHS, "cmptmp");
      return Builder.CreateUIToFP(Cmp, Builder.getDoubleTy(), "booltmp");
    }
    }
    llvm_unreachable("parser accepted an unknown binary operator");
  }

  case Expr::Kind::Call: {
    const auto &C = llvm::cast<CallExpr>(E);
    llvm::Function *Callee = M.getFunction(C.getCallee());
    if (!Callee)
      return makeCodeGenError("call to undeclared function '" +
                              C.getCallee() + "'");
    if (C.getArgs().size() != Callee->arg_size())
      return makeCodeGenError(
          llvm::Twine(C.getArgs().size() < Callee->arg_size() ? "too few"
                                                              : "too many") +
          " arguments to call of '" + C.getCallee() + "'");

    llvm::SmallVector<llvm::Value *, 8> Args;
    for (const Expr *Arg : C.getArgs()) {
      auto V = emitExpr(*Arg);
      if (!V)
        return V.takeError();
      Args.push_back(*V);
    }
    return Builder.CreateCall(Callee, Args, "calltmp");
  }
  }
  llvm_unreachable("unknown expression kind");
}

llvm::Expected<llvm::Function *> FunctionEmitter::emit(const FunctionDecl &D) {
  // One function may be declared many times ('extern', a 'def' after an
  // 'extern') as long as the signatures agree; with every value a double,
  // the signature is just the parameter count.
  llvm::Function *F = M.getFunction(D.getName());
  bool Created = !F;
  if (F) {
    if (F->arg_size() != D.getParams().size())
      return makeCodeGenError("conflicting types for '" + D.getName() + "'");
  } else {
    auto *DoubleTy = llvm::Type::getDoubleTy(M.getContext());
    auto *FT = llvm::FunctionType::get(
        DoubleTy,
        llvm::SmallVector<llvm::Type *, 8>(D.getParams().size(), DoubleTy),
        /*isVarArg=*/false);
    F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                               D.getName(), M);
  }

  if (!D.hasBody())
    return F;

  if (!F->empty())
    return makeCodeGenError("redefinition of '" + D.getName() + "'");

  auto EmitBody = [&]() -> llvm::Error {
    // The body's parameter names come from this 'def', not from any
    // earlier declaration ("extern f(x)" then "def f(a) a" is fine). The
    // scope map is keyed by the *source* names: IR value names are not a
    // substitute, since LLVM silently uniquifies them ("def f(x x)" gets
    // arguments %x and %x1 — the map, not the renaming, catches the
    // duplicate).
    NamedValues.clear();
    for (auto [Arg, Name] : llvm::zip(F->args(), D.getParams())) {
      Arg.setName(Name);
      if (!NamedValues.insert({Name, &Arg}).second)
        return makeCodeGenError("redefinition of parameter '" + Name + "'");
    }

    Builder.SetInsertPoint(
        llvm::BasicBlock::Create(M.getContext(), "entry", F));
    auto Body = emitExpr(D.getBody());
    if (!Body)
      return Body.takeError();
    Builder.CreateRet(*Body);
    return llvm::Error::success();
  };

  if (llvm::Error E = EmitBody()) {
    // Leave no half-emitted function behind: erase what this call
    // created, but keep a pre-existing declaration.
    if (Created)
      F->eraseFromParent();
    else
      F->deleteBody();
    return std::move(E);
  }

  // Verification failing here is a bug in this file, not in the input.
  assert(!llvm::verifyFunction(*F, &llvm::errs()) &&
         "emitted invalid IR for a valid AST");
  return F;
}

llvm::Expected<llvm::Function *>
kaleidoscope::emitFunctionDecl(llvm::Module &M, const FunctionDecl &D) {
  return FunctionEmitter(M).emit(D);
}
