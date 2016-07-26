# vim: syntax=cmake
set(JPEGD_PARSER_HDR
    jpegd_parser.h
    )

set(JPEGD_PARSER_SRC
    jpegd_parser.c
    )
add_library(codec_jpegd STATIC
	${JPEGD_PARSER_SRC} ${JPEGD_PARSER_HDR}
    )

set_target_properties(codec_jpegd PROPERTIES FOLDER "mpp/codec")

target_link_libraries(codec_jpegd mpp_base)

#add_subdirectory(test)
