# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

def f(x x) x
# CHECK: kaleidoscope: error: redefinition of parameter 'x'
