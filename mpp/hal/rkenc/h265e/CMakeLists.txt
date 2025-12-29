# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h265/)
include_directories(../common/)
# mpp/codec/enc/h265 for h265 syntax
include_directories(../../../codec/enc/h265/)

# hal h265 encoder sourse
set(HAL_H265E_SRC
    )

add_hal_source(HAL_H265E_SRC HAVE_VEPU541 hal_h265e_vepu541.c)
add_hal_source(HAL_H265E_SRC HAVE_VEPU580 hal_h265e_vepu580.c)
add_hal_source(HAL_H265E_SRC HAVE_VEPU540C hal_h265e_vepu540c.c)
add_hal_source(HAL_H265E_SRC HAVE_VEPU510 hal_h265e_vepu510.c)
add_hal_source(HAL_H265E_SRC HAVE_VEPU511 hal_h265e_vepu511.c)

if (HAL_H265E_SRC)
    add_library(hal_h265e_vepu5xx OBJECT ${HAL_H265E_SRC})
endif()
