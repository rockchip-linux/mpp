# vim: syntax=cmake
# ----------------------------------------------------------------------------
# add libvpu implement
# ----------------------------------------------------------------------------
if(RKPLATFORM)
add_library(vpu STATIC
    vpu.c
    )
endif(RKPLATFORM)
