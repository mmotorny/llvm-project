# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

def f(x) y
# CHECK: kaleidoscope: error: use of undeclared identifier 'y'
