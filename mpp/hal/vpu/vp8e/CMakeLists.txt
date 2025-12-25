# vim: syntax=cmake
include_directories(../common/)

add_library(hal_vp8e_api OBJECT
    hal_vp8e_api_v2.c
    hal_vp8e_putbit.c
    hal_vp8e_table.c
    hal_vp8e_entropy.c
    hal_vp8e_base.c
    hal_vp8e_vepu1_v2.c
    hal_vp8e_vepu2_v2.c
    )
