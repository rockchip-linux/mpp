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

#define setup_mpp_packet_name(packet) \
    ((MppPacketImpl*)packet)->name = module_name;

#define check_mpp_packet_name(packet) \
    do { \
        mpp_assert(((MppPacketImpl*)packet)->name == module_name); \
        if (((MppPacketImpl*)packet)->name != module_name) \
            mpp_err_f("packet %p failed on name checking\n"); \
    } while (0)

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
    if (NULL == packet) {
        mpp_err_f("invalid NULL input packet\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = mpp_packet_new(packet);
    if (ret) {
        mpp_err_f("new packet failed\n");
        return ret;
    }
    MppPacketImpl *p = (MppPacketImpl *)*packet;
    p->data = p->pos    = data;
    p->size = p->length = size;

    return MPP_OK;
}

MPP_RET mpp_packet_init_with_buffer(MppPacket *packet, MppBuffer buffer)
{
    if (NULL == packet || NULL == buffer) {
        mpp_err_f("invalid input packet %p buffer %p\n", packet, buffer);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = mpp_packet_new(packet);
    if (ret) {
        mpp_err_f("new packet failed\n");
        return ret;
    }
    MppPacketImpl *p = (MppPacketImpl *)*packet;
    p->data = p->pos    = mpp_buffer_get_ptr(buffer);
    p->size = p->length = mpp_buffer_get_size(buffer);
    p->buffer = buffer;
    mpp_buffer_inc_ref(buffer);

    return MPP_OK;
}

MPP_RET mpp_packet_copy_init(MppPacket *packet, const MppPacket src)
{
    if (NULL == packet || NULL == src) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *src_impl = (MppPacketImpl *)src;

    check_mpp_packet_name(src_impl);
    *packet = NULL;
    MppPacket pkt;
    MPP_RET ret = mpp_packet_new(&pkt);
    if (ret)
        return ret;

    if (src_impl->buffer) {
        /* if source packet has buffer just create a new reference to buffer */
        memcpy(pkt, src_impl, sizeof(*src_impl));
        mpp_buffer_inc_ref(src_impl->buffer);
        return MPP_OK;
    }

    size_t size = mpp_packet_get_size(src);
    void *data = mpp_malloc_size(void, size);
    if (NULL == data) {
        mpp_err_f("malloc failed, size %d\n", size);
        mpp_packet_deinit(&pkt);
        return MPP_ERR_MALLOC;
    }

    MppPacketImpl *p = (MppPacketImpl *)pkt;
    memcpy(p, src_impl, sizeof(*src_impl));
    p->data = p->pos = data;
    p->size = p->length = size;
    p->flag |= MPP_PACKET_FLAG_INTERNAL;
    if (size) {
        memcpy(data, src_impl->data, size);
    }
    *packet = pkt;
    return MPP_OK;
}

MPP_RET mpp_packet_deinit(MppPacket *packet)
{
    if (NULL == packet || NULL == *packet) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)(*packet);
    check_mpp_packet_name(p);

    /* release buffer reference */
    if (p->buffer)
        mpp_buffer_put(p->buffer);

    if (p->flag & MPP_PACKET_FLAG_INTERNAL) {
        mpp_free(p->data);
    }

    mpp_free(p);
    *packet = NULL;
    return MPP_OK;
}

void mpp_packet_set_pos(MppPacket packet, void *pos)
{
    MppPacketImpl *p = (MppPacketImpl *)packet;
    check_mpp_packet_name(p);
    p->pos    = pos;
    p->length = p->size - ((char *)pos - (char *)p->data);
    mpp_assert(p->data <= p->pos);
    mpp_assert(p->size >= p->length);
}

void *mpp_packet_get_pos(const MppPacket packet)
{
    MppPacketImpl *p = (MppPacketImpl *)packet;
    check_mpp_packet_name(p);
    return p->pos;
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
    setup_mpp_packet_name(packet);
    return MPP_OK;
}

void mpp_packet_set_buffer(MppPacket packet, MppBuffer buffer)
{
    if (NULL == packet) {
        mpp_err_f("found NULL input\n");
        return ;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;
    check_mpp_packet_name(p);
    if (p->buffer) {
        mpp_buffer_put(p->buffer);
    }
    p->buffer = buffer;
    if (buffer) {
        mpp_buffer_inc_ref(buffer);
    }
}

MppBuffer mpp_packet_get_buffer(const MppPacket packet)
{
    if (NULL == packet) {
        mpp_err_f("found NULL input\n");
        return NULL;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;
    check_mpp_packet_name(p);
    return p->buffer;
}

MPP_RET mpp_packet_read(MppPacket packet, size_t offset, void *data, size_t size)
{
    if (NULL == packet || NULL == data) {
        mpp_err_f("invalid input: packet %p data %p\n", packet, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    void *src = mpp_packet_get_data(packet);
    mpp_assert(src != NULL);
    memcpy(data, (char*)src + offset, size);
    return MPP_OK;
}

MPP_RET mpp_packet_write(MppPacket packet, size_t offset, void *data, size_t size)
{
    if (NULL == packet || NULL == data) {
        mpp_err_f("invalid input: packet %p data %p\n", packet, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    void *dst = mpp_packet_get_data(packet);
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
        check_mpp_packet_name(s); \
        return ((MppPacketImpl*)s)->field; \
    } \
    void mpp_packet_set_##field(MppPacket s, type v) \
    { \
        check_mpp_packet_name(s); \
        ((MppPacketImpl*)s)->field = v; \
    }

MPP_PACKET_ACCESSORS(void *, data)
MPP_PACKET_ACCESSORS(size_t, size)
MPP_PACKET_ACCESSORS(size_t, length)
MPP_PACKET_ACCESSORS(RK_S64, pts)
MPP_PACKET_ACCESSORS(RK_S64, dts)
MPP_PACKET_ACCESSORS(RK_U32, flag)

