# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

# Declaring a function again is fine (see extern-call.ks); giving it a
# second body is not.

def f(x) x
def f(x) x + 1
# CHECK: kaleidoscope: error: redefinition of 'f'
