# RUN: kaleidoscope %s | FileCheck %s

# Every value is a double, so a definition becomes a function over
# doubles and each operator one floating-point instruction.

def add(a b) a + b
# CHECK-LABEL: define double @add(double %a, double %b)
# CHECK:       entry:
# CHECK-NEXT:    %addtmp = fadd double %a, %b
# CHECK-NEXT:    ret double %addtmp

def axpy(a x y) a * x + y
# CHECK-LABEL: define double @axpy(double %a, double %x, double %y)
# CHECK:         %multmp = fmul double %a, %x
# CHECK-NEXT:    %addtmp = fadd double %multmp, %y
# CHECK-NEXT:    ret double %addtmp

# '<' has no boolean to produce: the i1 from fcmp converts right back to
# 0.0-or-1.0, the language's only type.
def lt(a b) a < b
# CHECK-LABEL: define double @lt(double %a, double %b)
# CHECK:         %cmptmp = fcmp ult double %a, %b
# CHECK-NEXT:    %booltmp = uitofp i1 %cmptmp to double
# CHECK-NEXT:    ret double %booltmp
