# RUN: not toy %s 2>&1 | FileCheck %s

(x + y
# CHECK: toy: error: expected ')'
