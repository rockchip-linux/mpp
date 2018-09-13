# vim: syntax=cmake
# hal h265e reg

set(HAL_H265E_SRC
    hal_h265e_api.c
    hal_h265e_vepu22.c
    )

add_library(${HAL_H265E} STATIC
    ${HAL_H265E_SRC}
    )

set_target_properties(${HAL_H265E} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_H265E} mpp_base)
