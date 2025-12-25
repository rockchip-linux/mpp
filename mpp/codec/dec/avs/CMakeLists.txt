# vim: syntax=cmake
include_directories(.)

# avs decoder sourse
add_library(avsd_api OBJECT
    avsd_api.c
    avsd_parse.c
    )
