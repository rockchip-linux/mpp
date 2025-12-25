# vim: syntax=cmake
include_directories(.)

# hal vepu541 common module
add_library(hal_vepu_common OBJECT
    vepu_common.c
    )
