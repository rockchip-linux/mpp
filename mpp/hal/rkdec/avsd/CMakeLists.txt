# vim: syntax=cmake

# hal avs decoder sourse

set(HAL_AVSD_SRC
    hal_avsd_api.c
    hal_avsd_base.c
    hal_avsd_plus.c
    )

add_hal_source(HAL_AVSD_SRC HAVE_VDPU1 hal_avsd_vdpu1.c)
add_hal_source(HAL_AVSD_SRC HAVE_VDPU2 hal_avsd_vdpu2.c)

if (HAL_AVSD_SRC)
    add_library(hal_avsd_api OBJECT ${HAL_AVSD_SRC})
endif()
