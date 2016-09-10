# vim: syntax=cmake
set(JPEGD_PARSER_HDR
    jpegd_parser.h
    )

set(JPEGD_PARSER_SRC
    jpegd_parser.c
    )

add_library(${CODEC_JPEGD} STATIC
    ${JPEGD_PARSER_SRC}
    ${JPEGD_PARSER_HDR}
    )

set_target_properties(${CODEC_JPEGD} PROPERTIES FOLDER "mpp/codec")
target_link_libraries(${CODEC_JPEGD} mpp_base)

#add_subdirectory(test)
