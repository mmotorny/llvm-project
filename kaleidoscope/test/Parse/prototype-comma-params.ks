# RUN: not kaleidoscope %s 2>&1 | FileCheck %s

# Prototype parameters are separated by whitespace, not commas (though
# call arguments use commas).
def f(a, b) a
# CHECK: kaleidoscope: error: expected ')' in prototype
