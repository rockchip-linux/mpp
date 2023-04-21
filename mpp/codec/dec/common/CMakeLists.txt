# vim: syntax=cmake
include_directories(.)

set(DEC_COMMON_HDR
    h2645d_sei.h
    )

# h264 decoder sourse
set(DEC_COMMON_SRC
    h2645d_sei.c
    )


set(DEC_COMMON dec_common)

add_library(${DEC_COMMON} STATIC
    ${DEC_COMMON_HDR}
    ${DEC_COMMON_SRC}
    )

target_link_libraries(${DEC_COMMON} mpp_base)
set_target_properties(${DEC_COMMON} PROPERTIES FOLDER "mpp/codec/dec/common")

