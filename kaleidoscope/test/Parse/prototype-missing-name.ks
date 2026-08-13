# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

def 1(x) x
# CHECK: kaleidoscope: error: expected function name in prototype
