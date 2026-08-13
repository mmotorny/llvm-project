# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

# Each error case is its own file: the driver stops at the first parse
# error, so a file can exercise exactly one.

x +
# CHECK: kaleidoscope: error: expected an expression
