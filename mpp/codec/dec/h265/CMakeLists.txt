# vim: syntax=cmake
set(H265D_PARSER_HDR
    h265d_defs.h
    h265d_parser.h
    h265d_codec.h
    )

set(H265D_PARSER_SRC
    h265d_parser.c
    h265d_ps.c
    h265d_refs.c
    h265d_sei.c
    h265d_parser2_syntax.c
    )

add_library(${CODEC_H265D} STATIC
    ${H265D_PARSER_SRC}
    ${H265D_PARSER_HDR}
    )

set_target_properties(${CODEC_H265D} PROPERTIES FOLDER "mpp/codec")
target_link_libraries(${CODEC_H265D} mpp_base)
