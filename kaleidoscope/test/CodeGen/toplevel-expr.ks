# RUN: kaleidoscope %s | FileCheck %s

# A bare expression is wrapped in an anonymous nullary function — IR has
# no instructions outside functions. Each wrapper gets a fresh name.
# The bodies below arrive pre-computed: IRBuilder folds operations on
# constants as it emits them, so no instructions remain.

1 + 2
# CHECK-LABEL: define double @__anon_expr()
# CHECK:         ret double 3.000000e+00

2 < 3
# CHECK-LABEL: define double @__anon_expr.1()
# CHECK:         ret double 1.000000e+00

def one() 1
one() + 1
# CHECK-LABEL: define double @__anon_expr.2()
# CHECK:         %calltmp = call double @one()
# CHECK-NEXT:    %addtmp = fadd double %calltmp, 1.000000e+00
# CHECK-NEXT:    ret double %addtmp
