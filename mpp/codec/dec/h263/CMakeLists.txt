# vim: syntax=cmake
set(H263D_PARSER_HDR
    h263d_parser.h
    )

set(H263D_PARSER_SRC
    h263d_api.c
    h263d_parser.c
    )

add_library(codec_h263d STATIC
	${H263D_PARSER_SRC} ${H263D_PARSER_HDR}
    )
set_target_properties(codec_h263d PROPERTIES FOLDER "mpp/codec")

target_link_libraries(codec_h263d mpp_base)
