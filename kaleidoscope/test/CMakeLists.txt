# lit regression tests, wired up the way in-tree LLVM projects (and the
# mlir/examples/standalone template) do it: CMake instantiates
# lit.site.cfg.py.in with build-tree paths, and that generated file chains
# to the checked-in lit.cfg.py for everything path-independent.
configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

# `ninja check-kaleidoscope`: build the tools the tests run, then run lit.
add_lit_testsuite(check-kaleidoscope
  "Running the Kaleidoscope regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS FileCheck count not toy
)
set_target_properties(check-kaleidoscope PROPERTIES FOLDER "Kaleidoscope/Tests")
