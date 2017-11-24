/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_META_H__
#define __MPP_META_H__

#include <stdint.h>
#include "rk_type.h"

#include "mpp_frame.h"
#include "mpp_packet.h"

#define FOURCC_META(a, b, c, d) ((RK_U32)(a) << 24  | \
                                ((RK_U32)(b) << 16) | \
                                ((RK_U32)(c) << 8)  | \
                                ((RK_U32)(d) << 0))

/*
 * Mpp Metadata definition
 *
 * Metadata is for information transmision in mpp.
 * Mpp task will contain two meta data:
 *
 * 1. Data flow metadata
 *    This metadata contains information of input / output data flow. For example
 *    A. decoder input side task the input packet must be defined and output frame
 *    may not be defined. Then decoder will try malloc or use committed buffer to
 *    complete decoding.
 *    B. decoder output side task
 *
 *
 * 2. Flow control metadata
 *
 */
typedef enum MppMetaDataType_e {
    /*
     * mpp meta data of data flow
     * reference counter will be used for these meta data type
     */
    TYPE_FRAME                  = FOURCC_META('m', 'f', 'r', 'm'),
    TYPE_PACKET                 = FOURCC_META('m', 'p', 'k', 't'),
    TYPE_BUFFER                 = FOURCC_META('m', 'b', 'u', 'f'),

    /* mpp meta data of normal data type */
    TYPE_S32                    = FOURCC_META('s', '3', '2', ' '),
    TYPE_S64                    = FOURCC_META('s', '6', '4', ' '),
    TYPE_PTR                    = FOURCC_META('p', 't', 'r', ' '),
} MppMetaType;

typedef enum MppMetaKey_e {
    /* data flow key */
    KEY_INPUT_FRAME             = FOURCC_META('i', 'f', 'r', 'm'),
    KEY_INPUT_PACKET            = FOURCC_META('i', 'p', 'k', 't'),
    KEY_OUTPUT_FRAME            = FOURCC_META('o', 'f', 'r', 'm'),
    KEY_OUTPUT_PACKET           = FOURCC_META('o', 'p', 'k', 't'),
    /* output motion information for motion detection */
    KEY_MOTION_INFO             = FOURCC_META('m', 'v', 'i', 'f'),
    KEY_HDR_INFO                = FOURCC_META('h', 'd', 'r', ' '),

    /* flow control key */
    KEY_INPUT_BLOCK             = FOURCC_META('i', 'b', 'l', 'k'),
    KEY_OUTPUT_BLOCK            = FOURCC_META('o', 'b', 'l', 'k'),
    KEY_INPUT_IDR_REQ           = FOURCC_META('i', 'i', 'd', 'r'),   /* input idr frame request flag */
    KEY_OUTPUT_INTRA            = FOURCC_META('o', 'i', 'd', 'r'),   /* output intra frame indicator */

    /* flow control key */
    KEY_WIDTH                   = FOURCC_META('w', 'd', 't', 'h'),
    KEY_HEIGHT                  = FOURCC_META('h', 'g', 'h', 't'),
    KEY_BITRATE                 = FOURCC_META('b', 'p', 's', ' '),
    KEY_BITRATE_UP              = FOURCC_META('b', 'p', 's', 'u'),
    KEY_BITRATE_LOW             = FOURCC_META('b', 'p', 's', 'l'),
    KEY_INPUT_FPS               = FOURCC_META('i', 'f', 'p', 's'),
    KEY_OUTPUT_FPS              = FOURCC_META('o', 'f', 'p', 's'),
    KEY_GOP                     = FOURCC_META('g', 'o', 'p', ' '),
    KEY_QP                      = FOURCC_META('q', 'p', ' ', ' '),
    KEY_QP_MIN                  = FOURCC_META('q', 'm', 'i', 'n'),
    KEY_QP_MAX                  = FOURCC_META('q', 'm', 'a', 'x'),
    KEY_QP_DIFF_RANGE           = FOURCC_META('q', 'd', 'i', 'f'),
    KEY_RC_MODE                 = FOURCC_META('r', 'c', 'm', 'o'),
} MppMetaKey;

typedef void* MppMeta;

#define mpp_meta_get(meta) mpp_meta_get_with_tag(meta, MODULE_TAG, __FUNCTION__)

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_meta_get_with_tag(MppMeta *meta, const char *tag, const char *caller);
MPP_RET mpp_meta_put(MppMeta meta);

MPP_RET mpp_meta_set_s32(MppMeta meta, MppMetaKey key, RK_S32 val);
MPP_RET mpp_meta_set_s64(MppMeta meta, MppMetaKey key, RK_S64 val);
MPP_RET mpp_meta_set_ptr(MppMeta meta, MppMetaKey key, void  *val);
MPP_RET mpp_meta_get_s32(MppMeta meta, MppMetaKey key, RK_S32 *val);
MPP_RET mpp_meta_get_s64(MppMeta meta, MppMetaKey key, RK_S64 *val);
MPP_RET mpp_meta_get_ptr(MppMeta meta, MppMetaKey key, void  **val);

MPP_RET mpp_meta_set_frame (MppMeta meta, MppMetaKey key, MppFrame  frame);
MPP_RET mpp_meta_set_packet(MppMeta meta, MppMetaKey key, MppPacket packet);
MPP_RET mpp_meta_set_buffer(MppMeta meta, MppMetaKey key, MppBuffer buffer);
MPP_RET mpp_meta_get_frame (MppMeta meta, MppMetaKey key, MppFrame  *frame);
MPP_RET mpp_meta_get_packet(MppMeta meta, MppMetaKey key, MppPacket *packet);
MPP_RET mpp_meta_get_buffer(MppMeta meta, MppMetaKey key, MppBuffer *buffer);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_META_H__*/
