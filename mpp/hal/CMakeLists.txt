# vim: syntax=cmake
# ----------------------------------------------------------------------------
# add mpp_hal implement
# ----------------------------------------------------------------------------
add_library(mpp_hal STATIC
    hal_task.cpp
    mpp_hal.cpp
    )

set_target_properties(mpp_hal PROPERTIES FOLDER "mpp/hal")

add_subdirectory(dummy)

add_subdirectory(rkdec/h264d)

add_subdirectory(rkdec/h265d)

add_subdirectory(rkdec/vp9d)

target_link_libraries(mpp_hal hal_h264d hal_dummy)