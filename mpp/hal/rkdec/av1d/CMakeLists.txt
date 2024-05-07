# vim: syntax=cmake
# hal av1 reg

include_directories(../../common/av1)

set(HAL_AV1D_HDR
    ../../common/av1/hal_av1d_common.h
    )

set(HAL_AV1D_SRC
    hal_av1d_vdpu383.c
    )

add_library(hal_av1d_rkv STATIC
            ${HAL_AV1D_SRC}
            ${HAL_AV1D_HDR}
            )

set_target_properties(hal_av1d_rkv PROPERTIES FOLDER "mpp/hal")
target_link_libraries(hal_av1d_rkv mpp_base)

#add_subdirectory(test)
