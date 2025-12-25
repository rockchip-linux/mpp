# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp_dec implement
# ----------------------------------------------------------------------------
add_library(mpp_codec STATIC
    mpp_enc_impl.c
    mpp_enc_v2.c
    enc_impl.c
    mpp_dec_no_thread.c
    mpp_dec_normal.c
    mpp_dec.c
    mpp_parser.c
    )

add_library(mpp_rc OBJECT mpp_rc.c)

add_subdirectory(dec)

add_subdirectory(enc)

add_subdirectory(rc)

merge_objects("${CODEC_MPEG2D}" mpeg2d_api)
merge_objects("${CODEC_MPEG4D}" mpeg4d_api)
merge_objects("${CODEC_H263D}"  h263d_api)
merge_objects("${CODEC_H264D}"  h264d_api)
merge_objects("${CODEC_H265D}"  h265d_api)
merge_objects("${CODEC_VP8D}"   vp8d_api)
merge_objects("${CODEC_VP9D}"   vp9d_api)
merge_objects("${CODEC_AV1D}"   av1d_api)
merge_objects("${CODEC_AVSD}"   avsd_api)
merge_objects("${CODEC_AVS2D}"  avs2d_api)
merge_objects("${CODEC_JPEGD}"  jpegd_api)

merge_objects("${CODEC_H264E}"  h264e_api)
merge_objects("${CODEC_H265E}"  h265e_api)
merge_objects("${CODEC_JPEGE}"  jpege_api)
merge_objects("${CODEC_VP8E}"   vp8e_api)

target_link_libraries(mpp_codec
    mpp_rc
    enc_rc
    ${CODEC_AVSD}
    ${CODEC_AVS2D}
    ${CODEC_H263D}
    ${CODEC_H264D}
    ${CODEC_H265D}
    ${CODEC_MPEG2D}
    ${CODEC_MPEG4D}
    ${CODEC_VP8D}
    ${CODEC_VP9D}
    ${CODEC_JPEGD}
    ${CODEC_AV1D}
    ${CODEC_H264E}
    ${CODEC_H265E}
    ${CODEC_JPEGE}
    ${CODEC_VP8E}
    codec_dummy_enc
    codec_dummy_dec
    mpp_vproc
    mpp_base
    dec_common
    hal_common
    )
