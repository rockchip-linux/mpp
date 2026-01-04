# vim: syntax=cmake

# hal jpeg decoder sourse
set(HAL_JPEGD_SRC
    hal_jpegd_common.c
    hal_jpegd_vpu7xx_com.c
    )

add_hal_source(HAL_JPEGD_SRC HAVE_VDPU2  hal_jpegd_vdpu2.c)
add_hal_source(HAL_JPEGD_SRC HAVE_VDPU1  hal_jpegd_vdpu1.c)
add_hal_source(HAL_JPEGD_SRC HAVE_VDPU720 hal_jpegd_rkv.c)
add_hal_source(HAL_JPEGD_SRC HAVE_VDPU730 hal_jpegd_vpu730.c)

if (HAL_JPEGD_SRC)
    add_library(hal_jpegd_api OBJECT ${HAL_JPEGD_SRC})
endif()
