# vim: syntax=cmake
include_directories(.)
include_directories(include)

# h264 encoder api
add_library(h264e_api OBJECT
    h264e_api_v2.c
    h264e_sps.c
    h264e_pps.c
    h264e_sei.c
    h264e_dpb.c
    h264e_slice.c
    )
