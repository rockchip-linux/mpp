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

add_subdirectory(vpu/h263d)

add_subdirectory(vpu/m2vd)

add_subdirectory(vpu/mpg4d)

add_subdirectory(vpu/vp8d)

add_subdirectory(vpu/jpegd)

add_subdirectory(rkenc/h264e)

# ----------------------------------------------------------------------------
# add hardware worker implement
# ----------------------------------------------------------------------------
add_subdirectory(worker/libvpu)
add_subdirectory(worker/libv4l2)

if(RKPLATFORM)
set(RKPLAT_VPU worker_vpu)
endif(RKPLATFORM)


target_link_libraries(mpp_hal
                      hal_avsd
                      hal_h263d
                      hal_h264d
                      hal_h265d
                      hal_m2vd
                      hal_mpg4d
                      hal_vp8d
                      hal_vp9d
		      hal_jpegd
                      hal_h264e		      
                      hal_dummy
                      ${RKPLAT_VPU}
                      )
