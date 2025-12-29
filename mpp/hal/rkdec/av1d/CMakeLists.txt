# vim: syntax=cmake
include_directories(../../common/av1)

set(HAL_AV1D_SRC)

add_hal_source(HAL_AV1D_SRC HAVE_VDPU383 hal_av1d_vdpu383.c)
add_hal_source(HAL_AV1D_SRC HAVE_VDPU384B hal_av1d_vdpu384b.c)

if (HAL_AV1D_SRC)
    list(APPEND HAL_AV1D_SRC hal_av1d_com.c)
    add_library(hal_av1d_rkv OBJECT ${HAL_AV1D_SRC})
endif()
