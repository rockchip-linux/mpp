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

# libvdpp.so for hwpq
add_library(vdpp SHARED hwpq_vdpp_proc.c ../../mpp_info.c)
set_target_properties(vdpp PROPERTIES FOLDER "mpp/vproc/vdpp")
set_target_properties(vdpp PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(vdpp osal mpp_base kmpp_base vproc_vdpp m)
set_target_properties(vdpp PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(vdpp PROPERTIES CXX_VISIBILITY_PRESET default)
