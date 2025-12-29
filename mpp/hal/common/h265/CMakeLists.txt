# vim: syntax=cmake
include_directories(.)
include_directories(../../rkenc/h265e/)
include_directories(../../../codec/enc/h265/)

# hal h265 encoder common
add_library(hal_h265e_com OBJECT
    hal_h265e_stream_amend.c
    )
