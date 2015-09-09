# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp_dec implement
# ----------------------------------------------------------------------------
add_library(mpp_codec STATIC
    mpp_buf_slot.cpp
    mpp_enc.cpp
    mpp_dec.cpp
    )
set_target_properties(mpp_codec PROPERTIES FOLDER "mpp/codec")

add_subdirectory(dec)

add_subdirectory(enc)

target_link_libraries(mpp_codec
                      codec_h264d
                      codec_dummy_enc
                      codec_dummy_dec
                      mpp)

