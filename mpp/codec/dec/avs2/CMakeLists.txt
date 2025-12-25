# vim: syntax=cmake
include_directories(.)

# avs2 decoder sourse
add_library(avs2d_api OBJECT
    avs2d_api.c
    avs2d_parse.c
    avs2d_ps.c
    avs2d_dpb.c
    )
