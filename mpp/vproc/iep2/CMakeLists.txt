# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add video process IEP2 (Image Enhancement Processor) implement
# ----------------------------------------------------------------------------
add_library(vproc_iep2 STATIC iep2.c iep2_ff.c iep2_gmv.c iep2_osd.c iep2_pd.c iep2_roi.c)
set_target_properties(vproc_iep2 PROPERTIES FOLDER "mpp/vproc/iep2")

add_subdirectory(test)
