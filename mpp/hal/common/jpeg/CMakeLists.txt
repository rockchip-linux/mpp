# vim: syntax=cmake
include_directories(.)
include_directories(../../rkenc/common/)
include_directories(../../rkenc/jpege/)
include_directories(../../vpu/jpege/)

# hal h264 header
set(HAL_JPEGE_HDR
    )

# hal h264 encoder sourse
set(HAL_JPEGE_SRC
    hal_jpege_api_v2.c
    hal_jpege_hdr.c
    )

add_library(hal_jpege STATIC
            ${HAL_JPEGE_HDR}
            ${HAL_JPEGE_SRC}
            )

target_link_libraries(hal_jpege hal_jpege_rkv hal_jpege_vpu mpp_base)
set_target_properties(hal_jpege PROPERTIES FOLDER "mpp/hal")
