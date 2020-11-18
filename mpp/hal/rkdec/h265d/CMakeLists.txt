# vim: syntax=cmake
# hal h265 reg

set(HAL_H265D_SRC
    hal_h265d_api.c
    hal_h265d_com.c
    hal_h265d_rkv.c
    hal_h265d_vdpu34x.c
    )

add_library(${HAL_H265D} STATIC ${HAL_H265D_SRC})

set_target_properties(${HAL_H265D} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_H265D} vdpu34x_com mpp_base)
