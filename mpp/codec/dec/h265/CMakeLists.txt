# vim: syntax=cmake
include_directories(../common)

# h265 decoder source
add_library(h265d_api OBJECT
    h265d_api.c
    h265d_spliter.c
    h265d_flow.c
    h265d_vps.c
    h265d_sps.c
    h265d_pps.c
    h265d_sei.c
    h265d_dpb.c
    h265d_syntax.c
    )
