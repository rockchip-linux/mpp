# vim: syntax=cmake
set(H263D_PARSER_HDR
    h263d_parser.h
    )

set(H263D_PARSER_SRC
    h263d_api.c
    h263d_parser.c
    )

add_library(${CODEC_H263D} STATIC
    ${H263D_PARSER_SRC} ${H263D_PARSER_HDR}
    )

set_target_properties(${CODEC_H263D} PROPERTIES FOLDER "mpp/codec")

target_link_libraries(${CODEC_H263D} mpp_base)
