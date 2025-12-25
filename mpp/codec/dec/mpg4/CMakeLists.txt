# vim: syntax=cmake

# mpeg4 decoder sourse
add_library(mpeg4d_api OBJECT
    mpg4d_api.c
    mpg4d_parser.c
    )
