# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

# Unlike recursion (the callee's own 'def' declares it before its body),
# a call to a never-declared name has nothing to bind to.

def f(x) g(x)
# CHECK: kaleidoscope: error: call to undeclared function 'g'
