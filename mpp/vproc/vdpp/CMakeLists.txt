# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add vdpp (Video Display Post Processor) implement
# ----------------------------------------------------------------------------
add_library(vproc_vdpp STATIC vdpp_api.c
                              vdpp.c
                              vdpp2.c
                              vdpp_common.c)
set_target_properties(vproc_vdpp PROPERTIES FOLDER "mpp/vproc/vdpp")

add_subdirectory(test)
