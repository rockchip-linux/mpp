# vim: syntax=cmake
set(H265D_PARSER_HDR
    h265d_defs.h
    h265d_parser.h
    h265d_codec.h
    )

set(H265D_PARSER_SRC
    ../../../mpp_bitread.c
    h265d_parser.c
    h265d_ps.c
    h265d_refs.c
    h265d_sei.c
    h265d_parser2_syntax.c
    )

add_library(h265d_parser STATIC
	${H265D_PARSER_SRC} ${H265D_PARSER_HDR}
    )

target_link_libraries(h265d_parser osal)

add_subdirectory(test)
