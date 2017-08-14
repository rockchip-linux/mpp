# This file setup the enable flag of all supported codecs

# AVS decoder
option(ENABLE_AVSD   "Enable avs decoder" ON)
if( ENABLE_AVSD AND
    EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/codec/dec/avs" )
    set(HAVE_AVSD true)
    set(CODEC_AVSD codec_avsd)
    set(HAL_AVSD hal_avsd)
    add_definitions(-DHAVE_AVSD)
endif()

# H.263 decoder
option(ENABLE_H263D  "Enable h.263 decoder" ON)
if( ENABLE_H263D )
    set(HAVE_H263D true)
    set(CODEC_H263D codec_h263d)
    set(HAL_H263D hal_h263d)
    add_definitions(-DHAVE_H263D)
endif()

# H.264 decoder
option(ENABLE_H264D  "Enable h.264 decoder" ON)
if( ENABLE_H264D )
    set(HAVE_H264D true)
    set(CODEC_H264D codec_h264d)
    set(HAL_H264D hal_h264d)
    add_definitions(-DHAVE_H264D)
endif()

# H.265 decoder
option(ENABLE_H265D  "Enable h.265 decoder" ON)
if( ENABLE_H265D )
    set(HAVE_H265D true)
    set(CODEC_H265D codec_h265d)
    set(HAL_H265D hal_h265d)
    add_definitions(-DHAVE_H265D)
endif()

# mpeg2 decoder
option(ENABLE_MPEG2D "Enable mpeg2 decoder" ON)
if( ENABLE_MPEG2D )
    set(HAVE_MPEG2D true)
    set(CODEC_MPEG2D codec_mpeg2d)
    set(HAL_MPEG2D hal_mpeg2d)
    add_definitions(-DHAVE_MPEG2D)
endif()

# mpeg4 decoder
option(ENABLE_MPEG4D "Enable mpeg4 decoder" ON)
if( ENABLE_MPEG4D )
    set(HAVE_MPEG4D true)
    set(CODEC_MPEG4D codec_mpeg4d)
    set(HAL_MPEG4D hal_mpeg4d)
    add_definitions(-DHAVE_MPEG4D)
endif()

# VP8 decoder
option(ENABLE_VP8D   "Enable vp8 decoder" ON)
if( ENABLE_VP8D )
    set(HAVE_VP8D true)
    set(CODEC_VP8D codec_vp8d)
    set(HAL_VP8D hal_vp8d)
    add_definitions(-DHAVE_VP8D)
endif()

# VP9 decoder
option(ENABLE_VP9D   "Enable vp9 decoder" ON)
if( ENABLE_VP9D )
    set(HAVE_VP9D true)
    set(CODEC_VP9D codec_vp9d)
    set(HAL_VP9D hal_vp9d)
    add_definitions(-DHAVE_VP9D)
endif()

# jpeg decoder
option(ENABLE_JPEGD  "Enable jpeg decoder" ON)
if( ENABLE_JPEGD )
    set(HAVE_JPEGD true)
    set(CODEC_JPEGD codec_jpegd)
    set(HAL_JPEGD hal_jpegd)
    add_definitions(-DHAVE_JPEGD)
endif()

# H.264 encoder
option(ENABLE_H264E  "Enable h.264 encoder" ON)
if( ENABLE_H264E )
    set(HAVE_H264E true)
    set(CODEC_H264E codec_h264e)
    set(HAL_H264E hal_h264e)
    add_definitions(-DHAVE_H264E)
endif()

# jpeg encoder
option(ENABLE_JPEGE  "Enable jpeg encoder" ON)
if( ENABLE_JPEGE )
    set(HAVE_JPEGE true)
    set(CODEC_JPEGE codec_jpege)
    set(HAL_JPEGE hal_jpege)
    add_definitions(-DHAVE_JPEGE)
endif()

# h265 encoder
option(ENABLE_H265E  "Enable h265 encoder" ON)
if( ENABLE_H265E )
    set(HAVE_H265E true)
    set(CODEC_H265E codec_h265e)
    set(HAL_H265E hal_h265e)
    add_definitions(-DHAVE_H265E)
endif()

# vp8 encoder
option(ENABLE_VP8E "Enable vp8 encoder" ON)
if( ENABLE_VP8E )
    set(HAVE_VP8E true)
    set(CODEC_VP8E codec_vp8e)
    set(HAL_VP8E hal_vp8e)
    add_definitions(-DHAVE_VP8E)
endif()
