# vim: syntax=cmake
include_directories(worker/inc)

# ----------------------------------------------------------------------------
# add mpp_hal implement
# ----------------------------------------------------------------------------
add_library(mpp_hal STATIC
    hal_task.cpp
    mpp_hal.cpp
    hal_regdrv.c
    )

set_target_properties(mpp_hal PROPERTIES FOLDER "mpp/hal")

add_subdirectory(dummy)

add_subdirectory(rkdec/h264d)

add_subdirectory(rkdec/h265d)

add_subdirectory(rkdec/vp9d)

add_subdirectory(rkdec/avsd)

# ----------------------------------------------------------------------------
# add hardware worker implement
# ----------------------------------------------------------------------------
add_subdirectory(worker/libvpu)
add_subdirectory(worker/libv4l2)

target_link_libraries(mpp_hal hal_h264d hal_h265d hal_vp9d hal_avsd hal_dummy)
