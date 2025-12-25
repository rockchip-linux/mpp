# vim: syntax=cmake
include_directories(.)

# jpeg encoder sourse
add_library(jpege_api OBJECT
    jpege_api_v2.c
    )
