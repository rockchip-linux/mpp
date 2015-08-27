# vim: syntax=cmake
# ----------------------------------------------------------------------------
# add mpp_hal implement
# ----------------------------------------------------------------------------
add_library(mpp_hal STATIC
    mpp_hal.cpp
    )

add_subdirectory(rkdec/h264d)

add_subdirectory(rkdec/h265d)

add_subdirectory(rkdec/vp9d)
