# vim: syntax=cmake
include_directories(.)

# h264 / h265 common decoder sourse
add_library(dec_common OBJECT
    h2645d_sei.c
    )
