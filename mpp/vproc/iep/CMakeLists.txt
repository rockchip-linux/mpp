# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add vidoe process IEP (Image Enhancement Processor) implement
# ----------------------------------------------------------------------------
add_library(vproc_iep STATIC iep.cpp)
set_target_properties(vproc_iep PROPERTIES FOLDER "mpp/vproc/iep")

add_subdirectory(test)
