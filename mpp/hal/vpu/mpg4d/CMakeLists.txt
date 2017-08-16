# vim: syntax=cmake
# hal mpeg4 reg

set(HAL_MPEG4D_HDR
    hal_m4vd_com.h
    hal_m4vd_vdpu1.h
    hal_m4vd_vdpu2.h
    hal_m4vd_vdpu1_reg.h
    hal_m4vd_vdpu2_reg.h
    )

set(HAL_MPEG4D_SRC
    hal_m4vd_com.c
    hal_m4vd_api.c
    hal_m4vd_vdpu1.c
    hal_m4vd_vdpu2.c
    )

add_library(${HAL_MPEG4D} STATIC
    ${HAL_MPEG4D_SRC}
    ${HAL_MPEG4D_HDR}
    )

set_target_properties(${HAL_MPEG4D} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_MPEG4D} mpp_base)

#add_subdirectory(test)
