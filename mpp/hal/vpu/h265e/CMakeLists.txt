# vim: syntax=cmake
# hal h265e reg
include_directories(../../common/h265/)
include_directories(../common/)

set(HAL_H265E_SRC
    hal_h265e_vepu22.c
    )

add_library(hal_h265e_vpu STATIC
    ${HAL_H265E_SRC}
    )

set_target_properties(hal_h265e_vpu PROPERTIES FOLDER "mpp/hal")
target_link_libraries(hal_h265e_vpu vproc_rga hal_h265e mpp_base)
