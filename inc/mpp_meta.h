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
    TYPE_FRAME                  = 'mfrm',
    TYPE_PACKET                 = 'mpkt',
    TYPE_BUFFER                 = 'mbuf',

    /* mpp meta data of normal data type */
    TYPE_S32                    = 's32 ',
    TYPE_S64                    = 's64 ',
    TYPE_PTR                    = 'ptr ',
} MppMetaType;

typedef enum MppMetaKey_e {
    /* data flow key */
    KEY_INPUT_FRAME             = 'ifrm',
    KEY_INPUT_PACKET            = 'ipkt',
    KEY_OUTPUT_FRAME            = 'ofrm',
    KEY_OUTPUT_PACKET           = 'opkt',
    KEY_MOTION_INFO             = 'mvif',   /* output motion information for motion detection */

    /* flow control key */
    KEY_INPUT_BLOCK             = 'iblk',
    KEY_OUTPUT_BLOCK            = 'oblk',
    KEY_INPUT_IDR_REQ           = 'iidr',   /* input idr frame request flag */
    KEY_OUTPUT_INTRA            = 'oidr',   /* output intra frame indicator */

    /* flow control key */
    KEY_WIDTH                   = 'wdth',
    KEY_HEIGHT                  = 'hght',
    KEY_BITRATE                 = 'bps ',
    KEY_BITRATE_UP              = 'bpsu',
    KEY_BITRATE_LOW             = 'bpsl',
    KEY_INPUT_FPS               = 'ifps',
    KEY_OUTPUT_FPS              = 'ofps',
    KEY_GOP                     = 'gop ',
    KEY_QP                      = 'qp  ',
    KEY_QP_MIN                  = 'qmin',
    KEY_QP_MAX                  = 'qmax',
    KEY_QP_DIFF_RANGE           = 'qdif',
    KEY_RC_MODE                 = 'rcmo',
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
