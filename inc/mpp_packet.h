/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __MPP_PACKET_H__
#define __MPP_PACKET_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef void* MppPacket;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MppPacket interface
 *
 * mpp_packet_init = mpp_packet_new + mpp_packet_set_data + mpp_packet_set_size
 */
MPP_RET mpp_packet_new(MppPacket *packet);
MPP_RET mpp_packet_init(MppPacket *packet, void *data, size_t size);
MPP_RET mpp_packet_deinit(MppPacket *packet);


void    mpp_packet_set_data(MppPacket packet, void *data);
void*   mpp_packet_get_data(const MppPacket packet);
void    mpp_packet_set_size(MppPacket packet, size_t size);
size_t  mpp_packet_get_size(const MppPacket packet);

void    mpp_packet_set_pos(MppPacket packet, void *pos);
void*   mpp_packet_get_pos(const MppPacket packet);

void    mpp_packet_set_pts(MppPacket packet, RK_S64 pts);
RK_S64  mpp_packet_get_pts(const MppPacket packet);
void    mpp_packet_set_dts(MppPacket packet, RK_S64 dts);
RK_S64  mpp_packet_get_dts(const MppPacket packet);

void    mpp_packet_set_flag(MppPacket packet, RK_U32 flag);
RK_U32  mpp_packet_get_flag(const MppPacket packet);


MPP_RET mpp_packet_set_eos(MppPacket packet);
MPP_RET mpp_packet_set_extra_data(MppPacket packet);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_PACKET_H__*/
