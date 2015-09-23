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

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

static const char *module_name = MODULE_TAG;

static void setup_mpp_packet_name(MppPacketImpl *packet)
{
    packet->name = module_name;
}

static void check_mpp_packet_name(MppPacketImpl *packet)
{
    mpp_assert(packet->name == module_name);
}

MPP_RET mpp_packet_new(MppPacket *packet)
{
    if (NULL == packet) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = mpp_calloc(MppPacketImpl, 1);
    *packet = p;
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_NULL_PTR;
    }
    setup_mpp_packet_name(p);
    return MPP_OK;
}

MPP_RET mpp_packet_init(MppPacket *packet, void *data, size_t size)
{
    if (NULL == packet || NULL == data || 0 == size) {
        mpp_err_f("invalid input packet %p data %p size %d\n",
                  packet, data, size);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = mpp_packet_new(packet);
    if (ret) {
        mpp_err_f("new packet failed\n");
        return ret;
    }
    MppPacketImpl *p = (MppPacketImpl *)*packet;
    p->data = p->pos = data;
    p->size = size;

    return MPP_OK;
}

MPP_RET mpp_packet_copy(MppPacket *packet, const MppPacket src)
{
    if (NULL == packet || NULL == src) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    check_mpp_packet_name((MppPacketImpl *)src);
    size_t size = mpp_packet_get_size(src);
    void *data = mpp_malloc_size(void, size);
    if (NULL == data) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_MALLOC;
    }

    MPP_RET ret = mpp_packet_init(packet, data, size);
    if (MPP_OK == ret) {
        memcpy(data, mpp_packet_get_data(src), size);
        return MPP_OK;
    }

    mpp_err_f("malloc failed\n");
    mpp_free(data);
    return ret;
}

MPP_RET mpp_packet_deinit(MppPacket *packet)
{
    if (NULL == packet || NULL == *packet) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)(*packet);
    check_mpp_packet_name(p);
    if (p->buffer)
        mpp_buffer_put(p->buffer);

    mpp_free(p);
    *packet = NULL;
    return MPP_OK;
}

MPP_RET mpp_packet_set_eos(MppPacket packet)
{
    if (NULL == packet) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;
    check_mpp_packet_name(p);
    p->flag |= MPP_PACKET_FLAG_EOS;
    return MPP_OK;
}

RK_U32 mpp_packet_get_eos(MppPacket packet)
{
    if (NULL == packet) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;
    check_mpp_packet_name(p);
    return (p->flag & MPP_PACKET_FLAG_EOS) ? (1) : (0);
}

MPP_RET mpp_packet_set_extra_data(MppPacket packet)
{
    if (NULL == packet) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;
    check_mpp_packet_name(p);
    p->flag |= MPP_PACKET_FLAG_EXTRA_DATA;
    return MPP_OK;
}

MPP_RET mpp_packet_reset(MppPacketImpl *packet)
{
    if (NULL == packet) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    check_mpp_packet_name(packet);
    memset(packet, 0, sizeof(*packet));
    return MPP_OK;
}

MPP_RET mpp_packet_read(MppPacket buffer, size_t offset, void *data, size_t size)
{
    if (NULL == buffer || NULL == data) {
        mpp_err_f("invalid input: buffer %p data %p\n", buffer, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    void *src = mpp_packet_get_data(buffer);
    mpp_assert(src != NULL);
    memcpy(data, (char*)src + offset, size);
    return MPP_OK;
}

MPP_RET mpp_packet_write(MppPacket buffer, size_t offset, void *data, size_t size)
{
    if (NULL == buffer || NULL == data) {
        mpp_err_f("invalid input: buffer %p data %p\n", buffer, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    void *dst = mpp_packet_get_data(buffer);
    mpp_assert(dst != NULL);
    memcpy((char*)dst + offset, data, size);
    return MPP_OK;
}

/*
 * object access function macro
 */
#define MPP_PACKET_ACCESSORS(type, field) \
    type mpp_packet_get_##field(const MppPacket s) \
    { \
        check_mpp_packet_name((MppPacketImpl*)s); \
        return ((MppPacketImpl*)s)->field; \
    } \
    void mpp_packet_set_##field(MppPacket s, type v) \
    { \
        check_mpp_packet_name((MppPacketImpl*)s); \
        ((MppPacketImpl*)s)->field = v; \
    }

MPP_PACKET_ACCESSORS(void*,  data)
MPP_PACKET_ACCESSORS(size_t, size)
MPP_PACKET_ACCESSORS(void*,  pos)
MPP_PACKET_ACCESSORS(RK_S64, pts)
MPP_PACKET_ACCESSORS(RK_S64, dts)
MPP_PACKET_ACCESSORS(RK_U32, flag)
MPP_PACKET_ACCESSORS(MppBuffer, buffer)

