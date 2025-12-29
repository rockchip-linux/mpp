# vim: syntax=cmake

# hal h263 decoder sourse
set(HAL_H263D_SRC
    hal_h263d_api.c
    )

add_hal_source(HAL_H263D_SRC HAVE_VDPU1 hal_h263d_vdpu1.c)
add_hal_source(HAL_H263D_SRC HAVE_VDPU2 hal_h263d_vdpu2.c)

if (HAL_H263D_SRC)
    add_library(hal_h263d_api OBJECT ${HAL_H263D_SRC})
endif()
