# vim: syntax=cmake
include_directories(.)
include_directories(include)

# h264 encoder api
set(H264E_API
    )
set(H264E_COMMON
    )

# h264 encoder header
set(H264E_HDR
    include/h264e_codec.h
    )

# h264 encoder sourse
set(H264E_SRC
    src/h264e_api.c
    )


add_library(${CODEC_H264E} STATIC
    ${H264E_API}
    ${H264E_COMMON}
    ${H264E_HDR}
    ${H264E_SRC}
    )

target_link_libraries(${CODEC_H264E} mpp_rc mpp_base)
set_target_properties(${CODEC_H264E} PROPERTIES FOLDER "mpp/codec")
