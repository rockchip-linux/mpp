# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add kmpp basic components
# ----------------------------------------------------------------------------
add_library(kmpp_base STATIC
    kmpp_obj.c
    kmpp_frame.c
    kmpp_venc_cfg.c
    )

target_link_libraries(kmpp_base osal)
set_target_properties(kmpp_base PROPERTIES FOLDER "kmpp/base")

# unit test
add_subdirectory(test)
