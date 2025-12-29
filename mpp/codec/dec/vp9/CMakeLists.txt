# vim: syntax=cmake
include_directories(.)

# vp9 decoder sourse
add_library(vp9d_api OBJECT
    vp9d_api.c
    vp9d_parser.c
    vp9d_codec.c
    )
