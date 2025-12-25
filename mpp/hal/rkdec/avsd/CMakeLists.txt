# vim: syntax=cmake

# hal avs decoder sourse
add_library(hal_avsd_api OBJECT
    hal_avsd_api.c
    hal_avsd_base.c
    hal_avsd_vdpu1.c
    hal_avsd_vdpu2.c
    hal_avsd_plus.c
    )
