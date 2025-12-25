# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h265/)
include_directories(../common/)
# mpp/codec/enc/h265 for h265 syntax
include_directories(../../../codec/enc/h265/)

# hal h265 encoder sourse
add_library(hal_h265e_vepu5xx OBJECT
    hal_h265e_vepu541.c
    hal_h265e_vepu580.c
    hal_h265e_vepu540c.c
    hal_h265e_vepu510.c
    hal_h265e_vepu511.c
    )
