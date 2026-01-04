# vim: syntax=cmake

# hal mpeg2 decoder sourse
set(HAL_M2VD_SRC)

add_hal_source(HAL_M2VD_SRC HAVE_VDPU1 hal_m2vd_vdpu1.c)
add_hal_source(HAL_M2VD_SRC HAVE_VDPU2 hal_m2vd_vdpu2.c)

if(HAL_M2VD_SRC)
   add_library(hal_mpeg2d_api OBJECT ${HAL_M2VD_SRC})
endif()
