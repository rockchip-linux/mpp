# vim: syntax=cmake
# ----------------------------------------------------------------------------
# add mpp_device implement for hardware register transaction
# ----------------------------------------------------------------------------
if (RKPLATFORM)
add_library(mpp_device STATIC mpp_device.c)
endif(RKPLATFORM)
