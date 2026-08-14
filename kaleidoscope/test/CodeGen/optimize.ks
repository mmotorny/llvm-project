# RUN: kaleidoscope -O %s | FileCheck %s
# RUN: kaleidoscope %s | FileCheck --check-prefix=RAW %s

# -O runs the tutorial's four function passes. The tutorial's own
# showcase: reassociation canonicalizes the two operand orders of "+3"
# into the same expression, and GVN then computes it once. Without -O
# (the RAW run) all three operations survive verbatim.

def test(x) (1 + 2 + x) * (x + (1 + 2))
# CHECK-LABEL: define double @test(double %x)
# CHECK:         %addtmp = fadd double %x, 3.000000e+00
# CHECK-NEXT:    %multmp = fmul double %addtmp, %addtmp
# CHECK-NEXT:    ret double %multmp

# RAW-LABEL: define double @test(double %x)
# RAW:         %addtmp = fadd double 3.000000e+00, %x
# RAW-NEXT:    %addtmp1 = fadd double %x, 3.000000e+00
# RAW-NEXT:    %multmp = fmul double %addtmp, %addtmp1
# RAW-NEXT:    ret double %multmp

# Instcombine knows x * 1.0 is x for every double (including NaNs), so
# the body reduces to returning the parameter.
def mulid(x) x * 1
# CHECK-LABEL: define double @mulid(double %x)
# CHECK:         ret double %x

# RAW-LABEL: define double @mulid(double %x)
# RAW:         %multmp = fmul double %x, 1.000000e+00
# RAW-NEXT:    ret double %multmp
