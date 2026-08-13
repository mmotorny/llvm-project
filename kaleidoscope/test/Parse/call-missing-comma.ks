# RUN: not toy %s 2>&1 | FileCheck %s

f(a b)
# CHECK: toy: error: expected ')' or ',' in argument list
