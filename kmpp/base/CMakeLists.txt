# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add kmpp basic components
# ----------------------------------------------------------------------------
add_library(kmpp_base OBJECT
    kmpp_ioc.c
    kmpp_obj.c
    kmpp_meta.c
    kmpp_frame.c
    kmpp_buffer.c
    kmpp_venc_cfg.c
    kmpp_packet.c
    )

set_target_properties(kmpp_base PROPERTIES FOLDER "kmpp/base")

# unit test
add_subdirectory(test)
