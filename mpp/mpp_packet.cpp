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

#define MODULE_TAG "mpp_packet"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

MPP_PACKET_ACCESSORS(MppPacketImpl, void*,  data)
MPP_PACKET_ACCESSORS(MppPacketImpl, size_t, size)
MPP_PACKET_ACCESSORS(MppPacketImpl, size_t, offset)
MPP_PACKET_ACCESSORS(MppPacketImpl, size_t, length)
MPP_PACKET_ACCESSORS(MppPacketImpl, RK_S64, pts)
MPP_PACKET_ACCESSORS(MppPacketImpl, RK_S64, dts)
MPP_PACKET_ACCESSORS(MppPacketImpl, RK_U32, flag)

MPP_RET mpp_packet_init(MppPacket *packet, void *data, size_t size)
{
    if (NULL == packet || NULL == data || 0 == size) {
        mpp_err("mpp_packet_init invalid input packet %p data %p size %d\n",
                packet, data, size);
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = mpp_calloc(MppPacketImpl, 1);
    if (NULL == p) {
        mpp_err("mpp_packet_init malloc failed\n");
        return MPP_ERR_NULL_PTR;
    }

    p->data = data;
    p->size = size;

    *packet = p;

    return MPP_OK;
}

MPP_RET mpp_packet_deinit(MppPacket *packet)
{
    if (NULL == packet || NULL == packet) {
        mpp_err("mpp_packet_deinit found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_free(*packet);
    *packet = NULL;
    return MPP_OK;
}

MPP_RET mpp_packet_set_eos(MppPacket packet)
{
    if (NULL == packet) {
        mpp_err("mpp_packet_set_eos found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;
    p->flag |= MPP_PACKET_FLAG_EOS;
    return MPP_OK;
}

MPP_RET mpp_packet_set_extra_data(MppPacket packet)
{
    if (NULL == packet) {
        mpp_err("mpp_packet_set_extra_data found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;
    p->flag |= MPP_PACKET_FLAG_EXTRA_DATA;
    return MPP_OK;
}

