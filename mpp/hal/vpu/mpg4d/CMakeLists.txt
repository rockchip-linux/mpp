# vim: syntax=cmake
# hal mpeg4 reg

set(HAL_MPEG4D_HDR
    )

set(HAL_MPEG4D_SRC
    hal_mpg4d_reg.c
    )

add_library(${HAL_MPEG4D} STATIC
    ${HAL_MPEG4D_SRC}
    ${HAL_MPEG4D_HDR}
    )

set_target_properties(${HAL_MPEG4D} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_MPEG4D} mpp_base)

#add_subdirectory(test)
