# RUN: kaleidoscope < %s 2>&1 | FileCheck %s

# The same declaration rules a single module enforces in file mode hold
# across REPL lines (each line compiles into its own module; the session's
# prototype registry carries the rules over). And unlike file mode, an
# error only discards its line: the session — and everything defined
# before the error — survives, as the final call shows.

def f(x) x
def f(x) x + 1
# CHECK: error: redefinition of 'f'

extern f(a b)
# CHECK: error: conflicting types for 'f'

f(2)
# CHECK: 2
