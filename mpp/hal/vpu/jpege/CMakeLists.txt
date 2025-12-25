# vim: syntax=cmake
include_directories(../common/)
include_directories(../../common/jpeg/)

add_library(hal_jpege_vpu OBJECT
    hal_jpege_base.c
    hal_jpege_vepu1_v2.c
    hal_jpege_vepu2_v2.c
    )
