//===- ASTContext.h - Kaleidoscope AST allocation ---------------*- C++ -*-===//
//
// Following clang's ASTContext: AST nodes are allocated in an arena owned by
// the context — `new (Ctx) NumberExpr(42)` — and never deleted individually;
// the whole tree's memory is released when the context dies. This is the
// piece that lets nodes carry no virtual functions at all: nobody ever
// deletes through the base class, so no virtual destructor is needed, and
// the nodes stay vtable-free exactly like clang's Stmt/Expr.
//
// The corollary (enforced with a static_assert in the allocator): node
// destructors never run, so nodes must be trivially destructible. Payloads
// that would own memory are stored as arena- or buffer-backed views
// (llvm::StringRef, llvm::ArrayRef) instead.
//
//===----------------------------------------------------------------------===//

#ifndef KALEIDOSCOPE_AST_ASTCONTEXT_H
#define KALEIDOSCOPE_AST_ASTCONTEXT_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Allocator.h"

#include <cstddef>
#include <memory>
#include <type_traits>

namespace kaleidoscope {

class ASTContext {
public:
  void *allocate(size_t Bytes, size_t Align) {
    return Alloc.Allocate(Bytes, Align);
  }

  /// Copy Src into the arena and return a view of the copy — how variable
  /// -length payloads (a call's argument list) become part of the tree
  /// without owning memory.
  template <typename T> llvm::ArrayRef<T> copyArray(llvm::ArrayRef<T> Src) {
    static_assert(std::is_trivially_copyable_v<T> &&
                      std::is_trivially_destructible_v<T>,
                  "arena-copied elements are memcpy'd and never destroyed");
    if (Src.empty())
      return {};
    T *Mem = static_cast<T *>(allocate(Src.size() * sizeof(T), alignof(T)));
    std::uninitialized_copy(Src.begin(), Src.end(), Mem);
    return llvm::ArrayRef<T>(Mem, Src.size());
  }

private:
  llvm::BumpPtrAllocator Alloc;
};

} // namespace kaleidoscope

/// Arena placement-new for AST nodes: `new (Ctx) NumberExpr(42)`. Mirrors
/// clang's operator new(size_t, const ASTContext &).
inline void *operator new(size_t Bytes, kaleidoscope::ASTContext &Ctx,
                          size_t Align = alignof(std::max_align_t)) {
  return Ctx.allocate(Bytes, Align);
}

/// Matching placement-delete; called by the compiler only if a constructor
/// throws mid-`new`. Arena memory is not individually reclaimed.
inline void operator delete(void *, kaleidoscope::ASTContext &, size_t) {}

#endif // KALEIDOSCOPE_AST_ASTCONTEXT_H
