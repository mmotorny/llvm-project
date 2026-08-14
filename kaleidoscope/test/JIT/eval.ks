# RUN: kaleidoscope < %s | FileCheck %s

# With input on stdin and no file argument, the driver runs its REPL:
# definitions print their IR as they are JIT-compiled, and bare
# expressions are executed, printing their value.

def sq(x) x * x
# CHECK: define double @sq(double %x)

sq(3)
# CHECK: 9

sq(1 + 1) * sq(2)
# CHECK: 16

# 'extern' resolves against the process running the REPL: this cos is
# the one in the C library the driver itself links against.
extern cos(x)
# CHECK: declare double @cos(double)

cos(0)
# CHECK: 1
