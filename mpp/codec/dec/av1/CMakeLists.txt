# vim: syntax=cmake
include_directories(.)

# av1 decoder sourse
add_library(av1d_api OBJECT
    av1d_api.c
    av1d_parser.c
    av1d_parser2_syntax.c
    av1d_cbs.c
    av1d_probs.c
    av1_entropymode.c
    )
