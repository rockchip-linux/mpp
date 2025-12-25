# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add video process IEP2 (Image Enhancement Processor) implement
# ----------------------------------------------------------------------------
add_library(vproc_iep2 OBJECT
    iep2.c
    iep2_ff.c
    iep2_gmv.c
    iep2_osd.c
    iep2_pd.c
    iep2_roi.c
    )

add_subdirectory(test)
