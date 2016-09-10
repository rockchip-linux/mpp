# vim: syntax=cmake
set(MPG4D_PARSER_HDR
    mpg4d_parser.h
    )

set(MPG4D_PARSER_SRC
    mpg4d_api.c
    mpg4d_parser.c
    )

add_library(${CODEC_MPEG4D} STATIC
    ${MPG4D_PARSER_SRC}
    ${MPG4D_PARSER_HDR}
    )

set_target_properties(${CODEC_MPEG4D} PROPERTIES FOLDER "mpp/codec")
target_link_libraries(${CODEC_MPEG4D} mpp_base)
