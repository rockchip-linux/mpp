# vim: syntax=cmake
include_directories(.)
include_directories(../common)

# h264 decoder sourse
add_library(h264d_api OBJECT
    h264d_api.c
    h264d_parse.c
    h264d_slice.c
    h264d_sps.c
    h264d_pps.c
    h264d_scalist.c
    h264d_sei.c
    h264d_dpb.c
    h264d_init.c
    h264d_fill.c
    )
