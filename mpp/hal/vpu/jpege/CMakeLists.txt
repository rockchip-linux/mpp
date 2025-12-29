# vim: syntax=cmake
include_directories(../common/)
include_directories(../../common/jpeg/)

set(HAL_JPEGE_SRC)

add_hal_source(HAL_JPEGE_SRC HAVE_VEPU1 hal_jpege_vepu1_v2.c)
add_hal_source(HAL_JPEGE_SRC HAVE_VEPU2 hal_jpege_vepu2_v2.c)

if (HAL_JPEGE_SRC)
    list(APPEND HAL_JPEGE_SRC hal_jpege_base.c)
    add_library(hal_jpege_vpu OBJECT ${HAL_JPEGE_SRC})
endif()