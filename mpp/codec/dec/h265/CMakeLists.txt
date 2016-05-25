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

add_library(codec_h265d STATIC
	${H265D_PARSER_SRC} ${H265D_PARSER_HDR}
    )

set_target_properties(codec_h265d PROPERTIES FOLDER "mpp/codec")

target_link_libraries(codec_h265d mpp_base)

add_subdirectory(test)
