# One library per compiler phase, following clang's convention (clangLex,
# clangParse, clangAST, ...): lib/<Phase>/ builds kaleidoscope<Phase>.
add_subdirectory(Lex)
