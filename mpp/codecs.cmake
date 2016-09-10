# This file setup the enable flag of all supported codecs

if( NOT DEFINED DISABLE_AVSD AND
    EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/codec/dec/avs" )
    set(HAVE_AVSD true)
    set(CODEC_AVSD codec_avsd)
    set(HAL_AVSD hal_avsd)
    add_definitions(-DHAVE_AVSD)
endif()

# H.263 decoder
if( NOT DEFINED DISABLE_H263D )
    set(HAVE_H263D true)
    set(CODEC_H263D codec_h263d)
    set(HAL_H263D hal_h263d)
    add_definitions(-DHAVE_H263D)
endif()

# H.264 decoder
if( NOT DEFINED DISABLE_H264D )
    set(HAVE_H264D true)
    set(CODEC_H264D codec_h264d)
    set(HAL_H264D hal_h264d)
    add_definitions(-DHAVE_H264D)
endif()

# H.265 decoder
if( NOT DEFINED DISABLE_H265D )
    set(HAVE_H265D true)
    set(CODEC_H265D codec_h265d)
    set(HAL_H265D hal_h265d)
    add_definitions(-DHAVE_H265D)
endif()

# mpeg2 decoder
if( NOT DEFINED DISABLE_MPEG2D )
    set(HAVE_MPEG2D true)
    set(CODEC_MPEG2D codec_mpeg2d)
    set(HAL_MPEG2D hal_mpeg2d)
    add_definitions(-DHAVE_MPEG2D)
endif()

# mpeg4 decoder
if( NOT DEFINED DISABLE_MPEG4D )
    set(HAVE_MPEG4D true)
    set(CODEC_MPEG4D codec_mpeg4d)
    set(HAL_MPEG4D hal_mpeg4d)
    add_definitions(-DHAVE_MPEG4D)
endif()

# VP8 decoder
if( NOT DEFINED DISABLE_VP8D )
    set(HAVE_VP8D true)
    set(CODEC_VP8D codec_vp8d)
    set(HAL_VP8D hal_vp8d)
    add_definitions(-DHAVE_VP8D)
endif()

# VP9 decoder
if( NOT DEFINED DISABLE_VP9D )
    set(HAVE_VP9D true)
    set(CODEC_VP9D codec_vp9d)
    set(HAL_VP9D hal_vp9d)
    add_definitions(-DHAVE_VP9D)
endif()

# jpeg decoder
if( NOT DEFINED DISABLE_JPEGD )
    set(HAVE_JPEGD true)
    set(CODEC_JPEGD codec_jpegd)
    set(HAL_JPEGD hal_jpegd)
    add_definitions(-DHAVE_JPEGD)
endif()

# H.264 encoder
if( NOT DEFINED DISABLE_H264E )
    set(HAVE_H264E true)
    set(CODEC_H264E codec_h264e)
    set(HAL_H264E hal_h264e)
    add_definitions(-DHAVE_H264E)
endif()

# jpeg encoder
if( NOT DEFINED DISABLE_JPEGE )
    set(HAVE_JPEGE true)
    set(CODEC_JPEGE codec_jpege)
    set(HAL_JPEGE hal_jpege)
    add_definitions(-DHAVE_JPEGE)
endif()
