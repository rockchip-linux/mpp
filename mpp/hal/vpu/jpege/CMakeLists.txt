# vim: syntax=cmake
# hal jpeg reg

set(HAL_JPEGE_HDR
   )

set(HAL_JPEGE_SRC
    hal_jpege_hdr.c
    hal_jpege_api.c
    )

add_library(hal_jpege STATIC
    ${HAL_JPEGE_SRC} ${HAL_JPEGE_HDR}
    )

set_target_properties(hal_jpege PROPERTIES FOLDER "mpp/hal")
target_link_libraries(hal_jpege mpp_base)
