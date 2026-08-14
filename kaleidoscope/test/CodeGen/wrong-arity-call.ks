# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

extern cos(x)
cos(1, 2)
# CHECK: kaleidoscope: error: too many arguments to call of 'cos'
