# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

# Re-declaration must agree with what came before. All values are
# doubles, so the whole signature is the parameter count.

extern f(a b)
def f(x) x
# CHECK: kaleidoscope: error: conflicting types for 'f'
