# vim: syntax=cmake

# hal jpeg decoder sourse
add_library(hal_jpegd_api OBJECT
    hal_jpegd_api.c
    hal_jpegd_common.c
    hal_jpegd_vdpu2.c
    hal_jpegd_vdpu1.c
    hal_jpegd_vpu7xx_com.c
    hal_jpegd_rkv.c
    hal_jpegd_vpu730.c
    )
