# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)
include_directories(../../../codec/enc/h264/)
include_directories(../common/)

# hal h264 encoder sourse
set(HAL_H264E_SRC)

add_hal_source(HAL_H264E_SRC HAVE_VEPU2 hal_h264e_vepu2_v2.c)
add_hal_source(HAL_H264E_SRC HAVE_VEPU1 hal_h264e_vepu1_v2.c)

if (HAL_H264E_SRC)
    list(APPEND HAL_H264E_SRC hal_h264e_vpu_tbl.c hal_h264e_vepu_v2.c)
    add_library(hal_h264e_vpu OBJECT ${HAL_H264E_SRC})
endif()
