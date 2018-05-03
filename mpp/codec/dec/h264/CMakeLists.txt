# vim: syntax=cmake
include_directories(.)

# h264 decoder api
set(H264D_API
    )

set(H264D_COMMON
    )

# h264 decoder header
set(H264D_HDR
    h264d_global.h
    h264d_parse.h
    h264d_slice.h
    h264d_sps.h
    h264d_pps.h
    h264d_scalist.h
    h264d_sei.h
    h264d_dpb.h
    h264d_init.h
    h264d_fill.h
    )

# h264 decoder sourse
set(H264D_SRC
    h264d_api.c
    h264d_parse.c
    h264d_slice.c
    h264d_sps.c
    h264d_pps.c
    h264d_scalist.c
    h264d_sei.c
    h264d_dpb.c
    h264d_init.c
    h264d_fill.c
    )


add_library(${CODEC_H264D} STATIC
    ${H264D_API}
    ${H264D_COMMON}
    ${H264D_HDR}
    ${H264D_SRC}
    )

target_link_libraries(${CODEC_H264D} mpp_base)
set_target_properties(${CODEC_H264D} PROPERTIES FOLDER "mpp/codec")

