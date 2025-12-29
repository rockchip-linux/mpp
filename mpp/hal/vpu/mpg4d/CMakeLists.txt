# vim: syntax=cmake

# hal mpeg4 decoder sourse
set(HAL_MPEG4D_SRC
    hal_m4vd_com.c
    hal_m4vd_api.c
    )

add_hal_source(HAL_MPEG4D_SRC HAVE_VDPU1 hal_m4vd_vdpu1.c)
add_hal_source(HAL_MPEG4D_SRC HAVE_VDPU2 hal_m4vd_vdpu2.c)

if (HAL_MPEG4D_SRC)
    add_library(hal_mpeg4d_api OBJECT ${HAL_MPEG4D_SRC})
endif()
