# vim: syntax=cmake
include_directories(../../rkdec/av1d/)
include_directories(../../rkdec/inc/)
include_directories(../../vpu/av1d/)

# hal h264 header
set(HAL_AV1D_HDR
    hal_av1d_common.h
    )

# hal av1 decoder sourse
set(HAL_AV1D_SRC
    hal_av1d_api_v2.c
    film_grain_noise_table.c
    )

add_library(${HAL_AV1D} STATIC
            ${HAL_AV1D_HDR}
            ${HAL_AV1D_SRC}
            )

target_link_libraries(${HAL_AV1D} hal_av1d_rkv hal_av1d_vpu mpp_base)
set_target_properties(${HAL_AV1D} PROPERTIES FOLDER "mpp/hal")
