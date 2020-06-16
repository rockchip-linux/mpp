# vim: syntax=cmake
include_directories(.)

# hal vepu541 common module
add_library(hal_vepu_common STATIC
            vepu_common.c vepu_common.h
            )

target_link_libraries(hal_vepu_common mpp_base)
set_target_properties(hal_vepu_common PROPERTIES FOLDER "mpp/hal")