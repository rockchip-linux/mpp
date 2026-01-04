# vim: syntax=cmake
include_directories(../../common/av1/)

# hal av1d decoder sourse
set(HAL_AV1D_SRC)

add_hal_source(HAL_AV1D_SRC HAVE_AV1D_VPU hal_av1d_vdpu.c)

if (HAL_AV1D_SRC)
    add_library(hal_av1d_vpu OBJECT ${HAL_AV1D_SRC})
endif()
