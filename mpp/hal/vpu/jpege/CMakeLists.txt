# vim: syntax=cmake
# hal jpeg reg

include_directories(../common/)
include_directories(../../common/jpeg/)

set(HAL_JPEGE_HDR
    )

set(HAL_JPEGE_SRC
    hal_jpege_base.c
    hal_jpege_vepu1_v2.c
    hal_jpege_vepu2_v2.c
    )

add_library(hal_jpege_vpu STATIC
    ${HAL_JPEGE_SRC}
    ${HAL_JPEGE_HDR}
    )

set_target_properties(hal_jpege_vpu PROPERTIES FOLDER "mpp/hal")
target_link_libraries(hal_jpege_vpu hal_vepu_common mpp_base)
