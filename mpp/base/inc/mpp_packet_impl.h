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

#ifndef __MPP_PACKET_IMPL_H__
#define __MPP_PACKET_IMPL_H__

#include "mpp_packet.h"

#define MPP_PACKET_FLAG_EOS             (0x00000001)
#define MPP_PACKET_FLAG_EXTRA_DATA      (0x00000002)
#define MPP_PACKET_FLAG_INTERNAL        (0x00000004)

#define MPP_PKT_SEG_CNT_DEFAULT         8

typedef union MppPacketStatus_t {
    RK_U32  val;
    struct {
        RK_U32  eos         : 1;
        RK_U32  extra_data  : 1;
        RK_U32  internal    : 1;
        /* packet is inputed on reset mark as discard */
        RK_U32  discard     : 1;

        /* for slice input output */
        RK_U32  partition   : 1;
        RK_U32  soi         : 1;
        RK_U32  eoi         : 1;
    };
} MppPacketStatus;

/*
 * mpp_packet_imp structure
 *
 * data     : pointer
 * size     : total buffer size
 * offset   : valid data start offset
 * length   : valid data length
 * pts      : packet pts
 * dts      : packet dts
 */
typedef struct MppPacketImpl_t {
    const char      *name;

    void            *data;
    void            *pos;
    size_t          size;
    size_t          length;

    RK_S64          pts;
    RK_S64          dts;

    MppPacketStatus status;
    RK_U32          flag;

    MppBuffer       buffer;
    MppMeta         meta;
    MppTask         task;

    RK_U32          segment_nb;
    RK_U32          segment_buf_cnt;
    MppPktSeg       segments_def[MPP_PKT_SEG_CNT_DEFAULT];
    MppPktSeg       *segments_ext;
    MppPktSeg       *segments;
} MppPacketImpl;

#ifdef __cplusplus
extern "C" {
#endif
/*
 * mpp_packet_reset is only used internelly and should NOT be used outside
 */
MPP_RET mpp_packet_reset(MppPacketImpl *packet);
MPP_RET mpp_packet_copy(MppPacket dst, MppPacket src);
MPP_RET mpp_packet_append(MppPacket dst, MppPacket src);

MPP_RET mpp_packet_set_status(MppPacket packet, MppPacketStatus status);
MPP_RET mpp_packet_get_status(MppPacket packet, MppPacketStatus *status);
void    mpp_packet_set_task(MppPacket packet, MppTask task);
MppTask mpp_packet_get_task(MppPacket packet);

void    mpp_packet_reset_segment(MppPacket packet);
void    mpp_packet_set_segment_nb(MppPacket packet, RK_U32 segment_nb);
MPP_RET mpp_packet_add_segment_info(MppPacket packet, RK_S32 type, RK_S32 offset, RK_S32 len);
void    mpp_packet_copy_segment_info(MppPacket dst, MppPacket src);

/* pointer check function */
MPP_RET check_is_mpp_packet(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_PACKET_IMPL_H__*/
