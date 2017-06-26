# vim: syntax=cmake
# hal jpeg reg

set(HAL_JPEGE_HDR
    )

set(HAL_JPEGE_SRC
    hal_jpege_hdr.c
    hal_jpege_api.c
    hal_jpege_vepu1.c
    hal_jpege_vepu2.c
    )

add_library(${HAL_JPEGE} STATIC
    ${HAL_JPEGE_SRC}
    ${HAL_JPEGE_HDR}
    )

set_target_properties(${HAL_JPEGE} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_JPEGE} mpp_base)
