# RUN: kaleidoscope %s | FileCheck %s

# An extern emits only a declaration — the body comes from elsewhere at
# link time (cos's from libm). Note the printed declaration drops the
# parameter name: without a body there is nothing to refer to it.

extern cos(x)
# CHECK: declare double @cos(double)

def wave(x) cos(x) * cos(x)
# CHECK-LABEL: define double @wave(double %x)
# CHECK:         %calltmp = call double @cos(double %x)
# CHECK-NEXT:    %calltmp1 = call double @cos(double %x)
# CHECK-NEXT:    %multmp = fmul double %calltmp, %calltmp1
# CHECK-NEXT:    ret double %multmp

# Recursion needs no forward declaration: the callee is found in the
# module because its own 'def' put the declaration there before the body
# was emitted.
def fac(n) n * fac(n - 1)
# CHECK-LABEL: define double @fac(double %n)
# CHECK:         %subtmp = fsub double %n, 1.000000e+00
# CHECK-NEXT:    %calltmp = call double @fac(double %subtmp)
# CHECK-NEXT:    %multmp = fmul double %n, %calltmp
# CHECK-NEXT:    ret double %multmp
