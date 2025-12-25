# vim: syntax=cmake
include_directories(.)

# vp8 decoder source
add_library(vp8d_api OBJECT
    vp8d_api.c
    vp8d_parser.c
    )
