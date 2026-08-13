# RUN: not toy %s 2>&1 | FileCheck %s

# A parse error goes to stderr and the driver exits non-zero — "not"
# inverts that so the test passes exactly when toy fails.

def 1(x) x
# CHECK: toy: error: expected function name in prototype
