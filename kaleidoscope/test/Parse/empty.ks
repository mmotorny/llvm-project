# RUN: toy %s | count 0

# A file with no top-level entities — only comments and whitespace — is a
# valid, empty program: no output, successful exit. ("count 0" fails if
# its input has more than zero lines.)
