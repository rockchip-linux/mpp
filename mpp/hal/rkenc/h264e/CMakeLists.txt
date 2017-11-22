# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)

# hal h264 header
set(HAL_H264E_HDR
    hal_h264e_rkv.h
    hal_h264e_rkv_dpb.h
    hal_h264e_rkv_stream.h
    hal_h264e_rkv_utils.h
    hal_h264e_rkv_nal.h
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_rkv.c
    hal_h264e_rkv_dpb.c
    hal_h264e_rkv_stream.c
    hal_h264e_rkv_utils.c
    hal_h264e_rkv_nal.c
    )

add_library(hal_h264e_rkv STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )

target_link_libraries(hal_h264e_rkv hal_h264e)
set_target_properties(hal_h264e_rkv PROPERTIES FOLDER "mpp/hal")
