# vim: syntax=cmake
# hal jpeg reg

set(HAL_JPEGD_HDR
    hal_jpegd_common.h
    hal_jpegd_base.h
    hal_jpegd_vpu7xx_com.h
    )

set(HAL_JPEGD_SRC
    hal_jpegd_api.c
    hal_jpegd_common.c
    hal_jpegd_vdpu2.c
    hal_jpegd_vdpu1.c
    hal_jpegd_vpu7xx_com.c
    hal_jpegd_rkv.c
    hal_jpegd_vpu730.c
    )

add_library(hal_jpegd STATIC
    ${HAL_JPEGD_SRC} ${HAL_JPEGD_HDR}
    )

set_target_properties(hal_jpegd PROPERTIES FOLDER "mpp/hal")

    target_link_libraries(hal_jpegd mpp_base)

#add_subdirectory(test)
