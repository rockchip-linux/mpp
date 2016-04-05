# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp_dec implement
# ----------------------------------------------------------------------------
add_library(mpp_codec STATIC
    mpp_buf_slot.cpp
    mpp_enc.cpp
    mpp_dec.cpp
    mpp_parser.cpp
    )
set_target_properties(mpp_codec PROPERTIES FOLDER "mpp/codec")

add_subdirectory(dec)

add_subdirectory(enc)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dec/avs_impl/")
add_subdirectory(dec/avs_impl)
target_link_libraries(mpp_codec
                      codec_h264d
					  codec_vp9d
					  codec_h265d
					  codec_avsd
					  codec_avsd_impl
                      codec_dummy_enc
                      codec_dummy_dec
                      mpp)
else()
target_link_libraries(mpp_codec
                      codec_h264d
					  codec_vp9d
					  codec_h265d
					  codec_avsd
                      codec_dummy_enc
                      codec_dummy_dec
                      mpp)
endif()

