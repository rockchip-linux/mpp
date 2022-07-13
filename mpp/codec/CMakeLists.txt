# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp_dec implement
# ----------------------------------------------------------------------------
add_library(mpp_codec STATIC
    mpp_enc_impl.cpp
    mpp_enc_v2.cpp
    enc_impl.cpp
    mpp_dec.cpp
    mpp_parser.cpp
    )

add_library(mpp_rc STATIC mpp_rc.cpp)

set_target_properties(mpp_codec PROPERTIES FOLDER "mpp/codec")

add_subdirectory(dec)

add_subdirectory(enc)

add_subdirectory(rc)

target_link_libraries(mpp_codec
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
                      ${CODEC_JPEGE}
                      ${CODEC_H265E}
                      ${CODEC_VP8E}
                      codec_dummy_enc
                      codec_dummy_dec
                      mpp_vproc
                      mpp_base)
