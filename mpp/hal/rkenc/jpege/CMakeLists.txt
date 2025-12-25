# vim: syntax=cmake
include_directories(.)
include_directories(../../common/jpeg/)
include_directories(../common/)

# hal jpeg encoder sourse
add_library(hal_jpege_rkv OBJECT
    hal_jpege_vepu540c.c
    hal_jpege_vepu511.c
    hal_jpege_vpu720.c
    )
