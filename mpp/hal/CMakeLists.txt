# vim: syntax=cmake
include_directories(worker/inc)
include_directories(common)

# ----------------------------------------------------------------------------
# add hardware hal
# ----------------------------------------------------------------------------
add_subdirectory(vpu)
add_subdirectory(rkdec)
add_subdirectory(rkenc)
add_subdirectory(dummy)
add_subdirectory(rkdec/avsd)
add_subdirectory(common)

# ----------------------------------------------------------------------------
# add mpp_hal implement
# ----------------------------------------------------------------------------
add_library(mpp_hal STATIC
    hal_task.cpp
    mpp_hal.cpp
    )

set_target_properties(mpp_hal PROPERTIES FOLDER "mpp/hal")

# ----------------------------------------------------------------------------
# add hardware worker implement
# ----------------------------------------------------------------------------
add_subdirectory(worker/mpp_device)
add_subdirectory(worker/libv4l2)

target_link_libraries(mpp_hal
                      ${HAL_AVSD}
                      ${HAL_H263D}
                      ${HAL_H264D}
                      ${HAL_H265D}
                      ${HAL_MPEG2D}
                      ${HAL_MPEG4D}
                      ${HAL_VP8D}
                      ${HAL_VP9D}
                      ${HAL_JPEGD}
                      ${HAL_H264E}
                      ${HAL_JPEGE}
                      ${HAL_H265E}
                      ${HAL_VP8E}
                      hal_dummy
                      mpp_device
                      )
