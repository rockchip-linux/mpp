# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp_dec implement
# ----------------------------------------------------------------------------
add_library(mpp_codec STATIC
    mpp_enc.cpp
    mpp_controller.cpp
    mpp_dec.cpp
    mpp_parser.cpp
    )
set_target_properties(mpp_codec PROPERTIES FOLDER "mpp/codec")

add_subdirectory(dec)

add_subdirectory(enc)

target_link_libraries(mpp_codec
                      ${CODEC_AVSD}
                      codec_h263d
                      codec_h264d
                      codec_h265d
                      codec_m2vd
                      codec_mpg4d
                      codec_vp8d
                      codec_vp9d
                      codec_jpegd
                      codec_h264e
                      codec_jpege
                      codec_dummy_enc
                      codec_dummy_dec
                      mpp_base)
