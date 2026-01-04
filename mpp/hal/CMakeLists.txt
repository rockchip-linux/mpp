# vim: syntax=cmake
include_directories(worker/inc)
include_directories(common)

# Macro to conditionally add source files
# Usage: add_hal_source(SRC_LIST CONDITION SOURCE_FILE)
macro(add_hal_source SRC_LIST CONDITION SOURCE_FILE)
    if (${CONDITION})
        list(APPEND ${SRC_LIST} ${SOURCE_FILE})
    endif()
endmacro()

# ----------------------------------------------------------------------------
# add hardware hal
# ----------------------------------------------------------------------------
add_subdirectory(vpu)
add_subdirectory(rkdec)
add_subdirectory(rkenc)
add_subdirectory(common)

merge_objects("${HAL_H263D}"    hal_h263d_api)
merge_objects("${HAL_JPEGD}"    hal_jpegd_api)
merge_objects("${HAL_MPEG2D}"   hal_mpeg2d_api)
merge_objects("${HAL_MPEG4D}"   hal_mpeg4d_api)
merge_objects("${HAL_VP8D}"     hal_vp8d_api)
merge_objects("${HAL_VP9D}"     hal_vp9d_api)
merge_objects("${HAL_AV1D}"     hal_av1d_com hal_av1d_rkv hal_av1d_vpu)
merge_objects("${HAL_AVSD}"     hal_avsd_api)
merge_objects("${HAL_AVS2D}"    hal_avs2d_api)
merge_objects("${HAL_H264D}"    hal_h264d_api)
merge_objects("${HAL_H265D}"    hal_h265d_api)

merge_objects("${HAL_H264E}"    hal_h264e_com hal_h264e_vepu5xx hal_h264e_vpu)
merge_objects("${HAL_H265E}"    hal_h265e_com hal_h265e_vepu5xx)
merge_objects("${HAL_JPEGE}"    hal_jpege_com hal_jpege_rkv hal_jpege_vpu)
merge_objects("${HAL_VP8E}"     hal_vp8e_api)

# ----------------------------------------------------------------------------
# add mpp_hal implement
# ----------------------------------------------------------------------------
add_library(mpp_hal STATIC
    mpp_enc_hal.c
    hal_task.c
    mpp_hal.c
    )

set_target_properties(mpp_hal PROPERTIES FOLDER "mpp/hal")

target_link_libraries(mpp_hal
    hal_common
    hal_vepu5xx_common
    hal_vepu_common
    hal_rkvdec_com
    ${HAL_AVSD}
    ${HAL_AVS2D}
    ${HAL_H263D}
    ${HAL_H264D}
    ${HAL_H265D}
    ${HAL_MPEG2D}
    ${HAL_MPEG4D}
    ${HAL_VP8D}
    ${HAL_VP9D}
    ${HAL_JPEGD}
    ${HAL_AV1D}
    ${HAL_H264E}
    ${HAL_JPEGE}
    ${HAL_H265E}
    ${HAL_VP8E}
    )
