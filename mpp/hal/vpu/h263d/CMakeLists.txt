# vim: syntax=cmake
# hal h263 reg

set(HAL_H263D_HDR
    hal_h263d_base.h
    hal_h263d_vdpu1.h
    hal_h263d_vdpu2.h
    hal_h263d_vdpu1_reg.h
    hal_h263d_vdpu2_reg.h
   )

set(HAL_H263D_SRC
    hal_h263d_api.c
    hal_h263d_vdpu1.c
    hal_h263d_vdpu2.c
    )

add_library(hal_h263d STATIC
    ${HAL_H263D_SRC} ${HAL_H263D_HDR}
    )
set_target_properties(hal_h263d PROPERTIES FOLDER "mpp/hal")

target_link_libraries(hal_h263d mpp_base)

