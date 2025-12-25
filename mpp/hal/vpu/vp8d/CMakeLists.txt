# vim: syntax=cmake
include_directories(.)

# hal vp8 decoder sourse
add_library(hal_vp8d_api OBJECT
    hal_vp8d_api.c
    hal_vp8d_vdpu2.c
    hal_vp8d_vdpu1.c
    )
