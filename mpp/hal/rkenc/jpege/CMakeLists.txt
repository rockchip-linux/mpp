# vim: syntax=cmake
include_directories(.)
include_directories(../../common/jpeg/)
include_directories(../common/)

# hal jpeg encoder sourse
set(HAL_JPEGE_SRC)

add_hal_source(HAL_JPEGE_SRC HAVE_VEPU540C hal_jpege_vepu540c.c)
add_hal_source(HAL_JPEGE_SRC HAVE_VEPU511  hal_jpege_vepu511.c)
add_hal_source(HAL_JPEGE_SRC HAVE_VEPU720  hal_jpege_vpu720.c)

if (HAL_JPEGE_SRC)
    add_library(hal_jpege_rkv OBJECT ${HAL_JPEGE_SRC})
endif()
