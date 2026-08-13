# Suite target grouping all Kaleidoscope unit tests: `ninja KaleidoscopeTests`
# builds every test binary registered against it. Tests mirror lib/'s
# per-phase directories, as clang's unittests do.
add_custom_target(KaleidoscopeTests)
set_target_properties(KaleidoscopeTests PROPERTIES FOLDER "Kaleidoscope/Tests")

# No Parse/ here: the parser is tested entirely through the driver, in
# test/Parse/ — clang's convention (its parser has no gtest coverage
# either; grammar behavior is clang/test/Parser lit tests).
add_subdirectory(AST)
add_subdirectory(Lex)
