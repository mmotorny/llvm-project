# RUN: toy %s | FileCheck %s

# The driver parses each top-level entity in order and prints its AST on
# one line, so CHECK-NEXT lines interleaved with the source pin the output
# to the entity right above them.

def fib(x)
  fib(x - 1) + fib(x - 2)
# CHECK: (def (fib x) (+ (fib (- x 1)) (fib (- x 2))))

extern cos(x)
# CHECK-NEXT: (extern (cos x))

def add(a b) a + b
# CHECK-NEXT: (def (add a b) (+ a b))

# A bare expression is a top-level entity too.
cos(1) < add(2, 3) * 4
# CHECK-NEXT: (< (cos 1) (* (add 2 3) 4))
