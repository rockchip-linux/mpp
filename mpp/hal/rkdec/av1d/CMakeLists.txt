# vim: syntax=cmake
include_directories(../../common/av1)

add_library(hal_av1d_rkv OBJECT
    hal_av1d_com.c
    hal_av1d_vdpu383.c
    hal_av1d_vdpu384b.c
    )
