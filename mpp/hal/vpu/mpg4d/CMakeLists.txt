# vim: syntax=cmake

# hal mpeg4 decoder sourse
add_library(hal_mpeg4d_api OBJECT
    hal_m4vd_com.c
    hal_m4vd_api.c
    hal_m4vd_vdpu1.c
    hal_m4vd_vdpu2.c
    )
