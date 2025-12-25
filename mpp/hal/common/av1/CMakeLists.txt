# vim: syntax=cmake
include_directories(../../rkdec/av1d/)
include_directories(../../rkdec/inc/)
include_directories(../../vpu/av1d/)

add_library(hal_av1d_com OBJECT
    film_grain_noise_table.c
    )

# hal av1 decoder sourse
add_library(hal_av1d_api OBJECT
    hal_av1d_api_v2.c
    )
