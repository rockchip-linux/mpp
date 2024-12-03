# vim: syntax=cmake
include_directories(.)
include_directories(../../common/jpeg/)
include_directories(../common/)

# hal jpeg header
set(HAL_JPEGE_HDR
    hal_jpege_vepu540c_reg.h
    hal_jpege_vepu511_reg.h
    hal_jpege_vpu720_reg.h
    hal_jpege_vpu720.h
    )

# hal jpeg encoder sourse
set(HAL_JPEGE_SRC
    hal_jpege_vepu540c.c
    hal_jpege_vepu511.c
    hal_jpege_vpu720.c
    )

add_library(hal_jpege_rkv STATIC
            ${HAL_JPEGE_HDR}
            ${HAL_JPEGE_SRC}
            )

target_link_libraries(hal_jpege_rkv  hal_jpege_com hal_common hal_vepu541_common)
set_target_properties(hal_jpege_rkv  PROPERTIES FOLDER "mpp/hal")
