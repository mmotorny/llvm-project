# -*- Python -*-
#
# Configuration for lit, LLVM's regression test runner. Every file below
# this directory with a matching suffix is one test: lit runs the file's
# "# RUN:" lines as shell commands (with %s expanded to the file's path)
# and the test passes iff they all exit 0. The RUN lines pipe the
# compiler's output into FileCheck, which verifies it against the file's
# "# CHECK:" lines. Both directives hide in '#' comments, so a test is
# simultaneously a valid Kaleidoscope program and its own expected output.

import os

import lit.formats
from lit.llvm import llvm_config

config.name = "Kaleidoscope"
config.test_format = lit.formats.ShTest(execute_external=False)
config.suffixes = [".ks"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.kaleidoscope_obj_root

# Substitutions for the standard helper tools (FileCheck, not, ...) plus
# our own: bare "kaleidoscope" in a RUN line resolves to the just-built
# binary. (Substitution skips matches preceded by '/', so the word inside
# the test files' *paths* is never rewritten.)
llvm_config.use_default_substitutions()
llvm_config.add_tool_substitutions(["kaleidoscope"], [config.llvm_tools_dir])
