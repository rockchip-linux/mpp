# vim: syntax=cmake

# hal vp9 decoder sourse
set(HAL_VP9D_SRC
    hal_vp9d_api.c
    hal_vp9d_com.c
    hal_vp9d_rkv.c
    hal_vp9d_vdpu34x.c
    hal_vp9d_vdpu382.c
    hal_vp9d_vdpu383.c
    hal_vp9d_vdpu384b.c
    )

add_library(${HAL_VP9D} OBJECT ${HAL_VP9D_SRC})

target_link_libraries(${HAL_VP9D})
set_target_properties(${HAL_VP9D} PROPERTIES FOLDER "mpp/hal")

