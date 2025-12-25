# vim: syntax=cmake
include_directories(../../common/av1/)

# hal h263 decoder sourse
add_library(hal_av1d_vpu OBJECT
    hal_av1d_vdpu.c
    )
