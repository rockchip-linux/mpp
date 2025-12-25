# vim: syntax=cmake
include_directories(.)

# mpeg2 decoder source
add_library(mpeg2d_api OBJECT
    m2vd_api.c
    m2vd_parser.c
    )
