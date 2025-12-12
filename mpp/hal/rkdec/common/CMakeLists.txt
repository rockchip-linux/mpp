# vim: syntax=cmake
include_directories(.)
# ----------------------------------------------------------------------------
# add rkvdec common file
# ----------------------------------------------------------------------------
set(HAL_RKVDEC_SRC
    vdpu_com.c
    vdpu34x_com.c
    vdpu382_com.c
    vdpu383_com.c
    vdpu384a_com.c
    vdpu38x_com.c
    )

add_library(hal_rkvdec_com OBJECT ${HAL_RKVDEC_SRC})

target_link_libraries(hal_rkvdec_com mpp_base)
set_target_properties(hal_rkvdec_com PROPERTIES FOLDER "mpp/hal")
