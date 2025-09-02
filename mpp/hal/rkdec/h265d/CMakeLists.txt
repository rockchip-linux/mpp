# vim: syntax=cmake
# hal h265 reg

set(HAL_H265D_SRC
    hal_h265d_api.c
    hal_h265d_com.c
    hal_h265d_rkv.c
    hal_h265d_vdpu34x.c
    hal_h265d_vdpu382.c
    hal_h265d_vdpu383.c
    hal_h265d_vdpu384a.c
    hal_h265d_vdpu384b.c
    )

add_library(${HAL_H265D} OBJECT ${HAL_H265D_SRC})

target_link_libraries(${HAL_H265D})
set_target_properties(${HAL_H265D} PROPERTIES FOLDER "mpp/hal")
