# RUN: not toy %s 2>&1 | FileCheck %s

def 1(x) x
# CHECK: toy: error: expected function name in prototype
