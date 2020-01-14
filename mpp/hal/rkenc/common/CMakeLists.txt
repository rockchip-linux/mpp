# vim: syntax=cmake
include_directories(.)

# hal vepu541 common module
add_library(hal_vepu541_common STATIC
            vepu541_common.c
            )

target_link_libraries(hal_vepu541_common mpp_base)
set_target_properties(hal_vepu541_common PROPERTIES FOLDER "mpp/hal/vepu541")
