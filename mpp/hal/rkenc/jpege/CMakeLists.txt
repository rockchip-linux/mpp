# vim: syntax=cmake
include_directories(.)
include_directories(../../common/jpeg/)
include_directories(../common/)

# hal jpeg header
set(HAL_JPEGE_HDR
    hal_jpege_vepu540c_reg.h
    )

# hal jpeg encoder sourse
set(HAL_JPEGE_SRC
    hal_jpege_vepu540c.c
    )

add_library(hal_jpege_rkv STATIC
            ${HAL_JPEGE_HDR}
            ${HAL_JPEGE_SRC}
            )

target_link_libraries(hal_jpege_rkv  hal_jpege hal_common hal_vepu541_common)
set_target_properties(hal_jpege_rkv  PROPERTIES FOLDER "mpp/hal")
