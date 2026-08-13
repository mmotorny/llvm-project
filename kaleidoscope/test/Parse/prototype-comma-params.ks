# RUN: not toy %s 2>&1 | FileCheck %s

# Prototype parameters are separated by whitespace, not commas (though
# call arguments use commas).
def f(a, b) a
# CHECK: toy: error: expected ')' in prototype
