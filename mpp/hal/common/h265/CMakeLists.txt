# vim: syntax=cmake
include_directories(.)
include_directories(../../rkenc/h265e/)
include_directories(../../rkenc/common/)

# hal h265 header
set(HAL_H265E_HDR
    )

# hal h265 encoder sourse
set(HAL_H265E_SRC
    hal_h265e_api_v2.c
    )

add_library(hal_h265e STATIC
            ${HAL_H265E_HDR}
            ${HAL_H265E_SRC}
            )

set_target_properties(hal_h265e PROPERTIES FOLDER "mpp/hal")

target_link_libraries(hal_h265e hal_h265e_vepu541 mpp_base)
