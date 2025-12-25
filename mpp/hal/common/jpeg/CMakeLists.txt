# vim: syntax=cmake
include_directories(.)
include_directories(../../rkenc/common/)
include_directories(../../rkenc/jpege/)
include_directories(../../vpu/jpege/)

# hal jpeg encoder common
add_library(hal_jpege_com OBJECT
    hal_jpege_hdr.c
    )

# hal jpeg encoder sourse
add_library(hal_jpege_api OBJECT
    hal_jpege_api_v2.c
    )
