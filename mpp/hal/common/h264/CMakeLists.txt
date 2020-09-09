# vim: syntax=cmake
include_directories(.)
include_directories(../../rkenc/common/)
include_directories(../../rkenc/h264e/)
include_directories(../../vpu/h264e/)

# hal h264 header
set(HAL_H264E_HDR
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_api_v2.c
    )

add_library(hal_h264e STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )

target_link_libraries(hal_h264e hal_h264e_rkv hal_h264e_vpu mpp_base)
set_target_properties(hal_h264e PROPERTIES FOLDER "mpp/hal")
