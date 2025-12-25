# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add vidoe process IEP (Image Enhancement Processor) implement
# ----------------------------------------------------------------------------
add_library(vproc_iep OBJECT
    iep.c
    )

add_subdirectory(test)
