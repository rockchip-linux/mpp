# vim: syntax=cmake
set(MPG4D_PARSER_HDR
    mpg4d_parser.h
    )

set(MPG4D_PARSER_SRC
    mpg4d_api.c
    mpg4d_parser.c
    )

add_library(codec_mpg4d STATIC
	${MPG4D_PARSER_SRC} ${MPG4D_PARSER_HDR}
    )
set_target_properties(codec_mpg4d PROPERTIES FOLDER "mpp/codec")

target_link_libraries(codec_mpg4d mpp_base)
