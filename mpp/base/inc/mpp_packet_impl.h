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

#include "mpp_meta.h"

#define MPP_PACKET_FLAG_EOS             (0x00000001)
#define MPP_PACKET_FLAG_EXTRA_DATA      (0x00000002)
#define MPP_PACKET_FLAG_INTERNAL        (0x00000004)
#define MPP_PACKET_FLAG_INTRA           (0x00000008)

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
    const char  *name;

    void        *data;
    void        *pos;
    size_t      size;
    size_t      length;

    RK_S64      pts;
    RK_S64      dts;

    RK_U32      flag;

    MppBuffer   buffer;
    MppMeta     meta;
} MppPacketImpl;

/*
 * mpp_packet_reset is only used internelly and should NOT be used outside
 */
MPP_RET mpp_packet_reset(MppPacketImpl *packet);

/* pointer check function */
MPP_RET check_is_mpp_packet(void *ptr);

#endif /*__MPP_PACKET_IMPL_H__*/
