# vim: syntax=cmake
include_directories(.)

# avs decoder api
set(AVSD_API

    )
set(AVSD_COMMON

    )

# avs decoder header
set(AVSD_HDR
    avsd_parse.h
    )

# avs decoder sourse
set(AVSD_SRC
    avsd_api.c
    avsd_parse.c
    )

add_library(codec_avsd STATIC
    ${AVSD_API}
    ${AVSD_COMMON}
    ${AVSD_HDR}
    ${AVSD_SRC}
    )

target_link_libraries(codec_avsd mpp_base)
set_target_properties(codec_avsd PROPERTIES FOLDER "mpp/codec")