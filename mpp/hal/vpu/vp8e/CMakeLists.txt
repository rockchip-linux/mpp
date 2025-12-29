# vim: syntax=cmake
include_directories(../common/)

set(HAL_VP8E_SRC
    hal_vp8e_api_v2.c
    hal_vp8e_putbit.c
    hal_vp8e_table.c
    hal_vp8e_entropy.c
    hal_vp8e_base.c
    )

add_hal_source(HAL_VP8E_SRC HAVE_VEPU1 hal_vp8e_vepu1_v2.c)
add_hal_source(HAL_VP8E_SRC HAVE_VEPU2 hal_vp8e_vepu2_v2.c)

if (HAL_VP8E_SRC)
    add_library(hal_vp8e_api OBJECT ${HAL_VP8E_SRC})
endif()
