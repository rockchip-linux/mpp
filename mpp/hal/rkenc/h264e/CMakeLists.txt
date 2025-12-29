# vim: syntax=cmake
include_directories(.)
# hal/common for hal_bufs module
include_directories(../common/)
include_directories(../../common/h264/)
# mpp/codec/enc/h264 for h264 syntax
include_directories(../../../codec/enc/h264/)

# hal h264 encoder sourse
set(HAL_H264E_SRC)

add_hal_source(HAL_H264E_SRC HAVE_VEPU541 hal_h264e_vepu541.c)
add_hal_source(HAL_H264E_SRC HAVE_VEPU580 hal_h264e_vepu580.c)
add_hal_source(HAL_H264E_SRC HAVE_VEPU540C hal_h264e_vepu540c.c)
add_hal_source(HAL_H264E_SRC HAVE_VEPU510 hal_h264e_vepu510.c)
add_hal_source(HAL_H264E_SRC HAVE_VEPU511 hal_h264e_vepu511.c)

if (HAL_H264E_SRC)
    add_library(hal_h264e_vepu5xx OBJECT ${HAL_H264E_SRC})
endif()
