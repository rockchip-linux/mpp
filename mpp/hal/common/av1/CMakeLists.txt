# vim: syntax=cmake
include_directories(../../rkdec/av1d/)
include_directories(../../rkdec/inc/)
include_directories(../../vpu/av1d/)

add_library(hal_av1d_com OBJECT
    film_grain_noise_table.c
    )
