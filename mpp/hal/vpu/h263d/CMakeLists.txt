# vim: syntax=cmake
# hal h263 reg

set(HAL_H263D_HDR
   )

set(HAL_H263D_SRC
    hal_h263d_reg.c
    )

add_library(hal_h263d STATIC
    ${HAL_H263D_SRC} ${HAL_H263D_HDR}
    )
set_target_properties(hal_h263d PROPERTIES FOLDER "mpp/hal")

target_link_libraries(hal_h263d mpp_base)

