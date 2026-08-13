# RUN: toy %s | FileCheck %s

# The expression grammar, exercised as bare top-level entities. Newlines
# are just whitespace to the lexer, so what makes each line a separate
# entity is only that no line *continues* the previous one: none starts
# with a binary operator, and no line ending in an identifier is followed
# by one starting with '(' — juxtaposed, those would parse as a call.

42
# CHECK: 42

x
# CHECK-NEXT: x

# '*' binds tighter than '+' ...
x + y * 2
# CHECK-NEXT: (+ x (* y 2))

# ... parentheses override precedence ...
(x + y) * 2
# CHECK-NEXT: (* (+ x y) 2)

# ... equal precedence groups left to right ...
a - b + c
# CHECK-NEXT: (+ (- a b) c)

# ... and comparison binds loosest.
a + b < c * d
# CHECK-NEXT: (< (+ a b) (* c d))

fib(x - 1)
# CHECK-NEXT: (fib (- x 1))

f()
# CHECK-NEXT: (f)

f(a, b + 1, g(c))
# CHECK-NEXT: (f a (+ b 1) (g c))
