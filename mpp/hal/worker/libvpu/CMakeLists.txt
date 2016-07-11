# vim: syntax=cmake
# ----------------------------------------------------------------------------
# add libvpu implement
# ----------------------------------------------------------------------------
if(RKPLATFORM)
add_library(worker_vpu STATIC
    vpu.c
    )
endif(RKPLATFORM)
