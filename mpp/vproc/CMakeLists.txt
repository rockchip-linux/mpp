# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp video process implement
# ----------------------------------------------------------------------------
add_library(mpp_vproc STATIC mpp_dec_vproc.cpp)
target_link_libraries(mpp_vproc vproc_rga vproc_iep mpp_base)

add_subdirectory(rga)
add_subdirectory(iep)
