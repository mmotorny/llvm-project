# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

(x + y
# CHECK: kaleidoscope: error: expected ')'
