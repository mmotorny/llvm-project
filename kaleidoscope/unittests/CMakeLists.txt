# Suite target grouping all Kaleidoscope unit tests: `ninja KaleidoscopeTests`
# builds every test binary registered against it. Tests mirror lib/'s
# per-phase directories, as clang's unittests do.
add_custom_target(KaleidoscopeTests)
set_target_properties(KaleidoscopeTests PROPERTIES FOLDER "Kaleidoscope/Tests")

add_subdirectory(Lex)
