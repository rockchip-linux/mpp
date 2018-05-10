# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp video process implement
# ----------------------------------------------------------------------------
add_library(mpp_vproc STATIC mpp_dec_vproc.cpp)

add_subdirectory(rga)
add_subdirectory(iep)
