# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

def f(x)
# CHECK: kaleidoscope: error: expected an expression
