# vim: syntax=cmake
# hal jpeg reg

include_directories(../common/)

set(HAL_JPEGE_HDR
    )

set(HAL_JPEGE_SRC
    hal_jpege_hdr.c
    hal_jpege_base.c
    hal_jpege_api_v2.c
    hal_jpege_vepu1_v2.c
    hal_jpege_vepu2_v2.c
    )

add_library(${HAL_JPEGE} STATIC
    ${HAL_JPEGE_SRC}
    ${HAL_JPEGE_HDR}
    )

set_target_properties(${HAL_JPEGE} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_JPEGE} hal_vepu_common mpp_base)
