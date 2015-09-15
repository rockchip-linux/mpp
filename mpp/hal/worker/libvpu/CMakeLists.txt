# vim: syntax=cmake
# ----------------------------------------------------------------------------
# add libvpu implement
# ----------------------------------------------------------------------------
if(ANDROID)
add_library(vpu STATIC
    vpu.c
    )
endif(ANDROID)