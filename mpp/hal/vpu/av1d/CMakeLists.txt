# vim: syntax=cmake
# hal jpeg reg

set(HAL_AV1D_HDR
    hal_av1d_common.h
    )

set(HAL_AV1D_SRC
    hal_av1d_api.c
    hal_av1d_vdpu.c
    film_grain_noise_table.c
    )

add_library(${HAL_AV1D} STATIC
            ${HAL_AV1D_SRC}
            ${HAL_AV1D_HDR}
            )

set_target_properties(${HAL_AV1D} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_AV1D} mpp_base)

#add_subdirectory(test)
