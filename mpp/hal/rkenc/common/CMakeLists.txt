# vim: syntax=cmake
include_directories(.)

# hal vepu541 common module
add_library(hal_vepu5xx_common OBJECT
    vepu541_common.c
    vepu5xx_common.c
    vepu540c_common.c
    vepu580_tune.c
    vepu580_common.c
    vepu510_common.c
    vepu511_common.c
    )
