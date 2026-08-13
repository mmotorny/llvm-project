# Kaleidoscope builds as part of the LLVM tree, hooked in as an LLVM
# "external project" (see README.md for the configure line). That gives us
# LLVM's CMake machinery: add_llvm_library/add_unittest, and LLVM's vendored
# googletest (llvm_gtest) plus the LLVMSupport library it depends on.
#
# The directory layout mirrors LLVM's own (LLVM's build system enforces one
# target per directory):
#   include/kaleidoscope/  public headers
#   lib/                   the compiler phases, one library each
#   tools/                 executables — the toy driver
#   unittests/             googletest unit tests
#   test/                  lit + FileCheck regression tests

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(unittests)
add_subdirectory(test)
