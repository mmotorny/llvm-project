# RUN: not toy %s 2>&1 | FileCheck %s

def f x
# CHECK: toy: error: expected '(' in prototype
