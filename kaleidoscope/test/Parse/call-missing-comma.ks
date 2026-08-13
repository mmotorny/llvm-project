# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

f(a b)
# CHECK: kaleidoscope: error: expected ')' or ',' in argument list
