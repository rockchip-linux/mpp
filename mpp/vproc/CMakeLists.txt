# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp video process implement
# ----------------------------------------------------------------------------
add_library(mpp_vproc STATIC mpp_dec_vproc.cpp mpp_vproc_dev.c)
target_link_libraries(mpp_vproc vproc_rga vproc_iep vproc_iep2 ${VPROC_VDPP} mpp_base)

add_subdirectory(rga)
add_subdirectory(iep)
add_subdirectory(iep2)
add_subdirectory(vdpp)