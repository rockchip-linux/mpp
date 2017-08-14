include_directories(.)

set(HAL_VP8E_HDR
    hal_vp8e_debug.h
    hal_vp8e_table.h
    hal_vp8e_entropy.h
    hal_vp8e_putbit.h
    hal_vp8e_base.h
    hal_vp8e_vepu1.h
    hal_vp8e_vepu2.h
    hal_vp8e_vepu1_reg.h
    hal_vp8e_vepu2_reg.h
    )

set(HAL_VP8E_SRC
    hal_vp8e_api.c
    hal_vp8e_putbit.c
    hal_vp8e_table.c
    hal_vp8e_entropy.c
    hal_vp8e_base.c
    hal_vp8e_vepu1.c
    hal_vp8e_vepu2.c
    )

add_library(hal_vp8e STATIC ${HAL_VP8E_SRC})

set_target_properties(hal_vp8e PROPERTIES FOLDER "mpp/hal")
target_link_libraries(hal_vp8e mpp_base)
