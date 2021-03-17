# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h265/)
include_directories(../common/)

# hal h265 header
set(HAL_H265E_HDR
    hal_h265e_vepu541_reg.h
    hal_h265e_vepu580_reg.h
    )

# hal h265 encoder sourse
set(HAL_H265E_SRC
    hal_h265e_vepu541.c
    hal_h265e_vepu580.c
    )

add_library(hal_h265e_vepu541 STATIC
            ${HAL_H265E_HDR}
            ${HAL_H265E_SRC}
            )

target_link_libraries(hal_h265e_vepu541 hal_h265e hal_common hal_vepu541_common)
set_target_properties(hal_h265e_vepu541 PROPERTIES FOLDER "mpp/hal")
