# vim: syntax=cmake
# hal mpeg4 reg

set(HAL_MPEG4D_HDR
   )

set(HAL_MPEG4D_SRC
    hal_mpg4d_reg.c
    )

add_library(hal_mpg4d STATIC
    ${HAL_MPEG4D_SRC} ${HAL_MPEG4D_HDR}
    )
set_target_properties(hal_mpg4d PROPERTIES FOLDER "mpp/hal")

target_link_libraries(hal_mpg4d mpp_base)

#add_subdirectory(test)
