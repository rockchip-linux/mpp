# vim: syntax=cmake

# hal h263 decoder sourse
add_library(hal_h263d_api OBJECT
    hal_h263d_api.c
    hal_h263d_vdpu1.c
    hal_h263d_vdpu2.c
    )
