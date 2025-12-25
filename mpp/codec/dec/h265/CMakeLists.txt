# vim: syntax=cmake
include_directories(../common)

# h265 decoder source
add_library(h265d_api OBJECT
    h265d_parser.c
    h265d_ps.c
    h265d_refs.c
    h265d_sei.c
    h265d_parser2_syntax.c
    )
