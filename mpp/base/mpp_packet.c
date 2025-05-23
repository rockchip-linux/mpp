/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_packet"

#include <string.h>

#include "mpp_debug.h"
#include "mpp_mem_pool.h"
#include "mpp_singleton.h"

#include "mpp_meta_impl.h"
#include "mpp_packet_impl.h"

static const char *module_name = MODULE_TAG;
static MppMemPool pool_packet = NULL;

#define setup_mpp_packet_name(packet)   ((MppPacketImpl*)packet)->name = module_name;

static void mpp_packet_srv_init()
{
    if (pool_packet)
        return;

    pool_packet = mpp_mem_pool_init_f(module_name, sizeof(MppPacketImpl));
}

static void mpp_packet_srv_deinit()
{
    if (pool_packet) {
        mpp_mem_pool_deinit_f(pool_packet);
        pool_packet = NULL;
    }
}

MPP_SINGLETON(MPP_SGLN_PACKET, mpp_packet, mpp_packet_srv_init, mpp_packet_srv_deinit)

MPP_RET check_is_mpp_packet_f(void *packet, const char *caller)
{
    if (packet && ((MppPacketImpl*)packet)->name == module_name)
        return MPP_OK;

    mpp_err("MppPacket %p failed on check from %s\n", packet, caller);
    mpp_abort();
    return MPP_NOK;
}

MPP_RET mpp_packet_new(MppPacket *packet)
{
    if (NULL == packet) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl*)mpp_mem_pool_get_f(pool_packet);
    *packet = p;
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_NULL_PTR;
    }
    setup_mpp_packet_name(p);
    p->segment_buf_cnt = MPP_PKT_SEG_CNT_DEFAULT;

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
    if (NULL == packet || check_is_mpp_packet(src)) {
        mpp_err_f("found invalid input %p %p\n", packet, src);
        return MPP_ERR_UNKNOW;
    }

    *packet = NULL;

    MppPacketImpl *src_impl = (MppPacketImpl *)src;
    MppPacket pkt;
    MPP_RET ret = mpp_packet_new(&pkt);
    if (ret)
        return ret;

    /* copy the source data */
    memcpy(pkt, src_impl, sizeof(*src_impl));

    /* increase reference of meta data */
    if (src_impl->meta)
        mpp_meta_inc_ref(src_impl->meta);

    if (src_impl->buffer) {
        /* if source packet has buffer just create a new reference to buffer */
        mpp_buffer_inc_ref(src_impl->buffer);
    } else {
        /*
         * NOTE: only copy valid data
         */
        size_t length = mpp_packet_get_length(src);
        /*
         * due to parser may be read 32 bit interface so we must alloc more size
         * then real size to avoid read carsh
         */
        void *pos = mpp_malloc_size(void, length + 256);
        if (NULL == pos) {
            mpp_err_f("malloc failed, size %d\n", length);
            mpp_packet_deinit(&pkt);
            return MPP_ERR_MALLOC;
        }

        MppPacketImpl *p = (MppPacketImpl *)pkt;
        p->data = p->pos = pos;
        p->size = p->length = length;
        p->flag |= MPP_PACKET_FLAG_INTERNAL;

        if (length) {
            memcpy(pos, src_impl->pos, length);
            /*
             * clean more alloc byte to zero
             */
            memset((RK_U8*)pos + length, 0, 256);
        }
    }

    *packet = pkt;
    return MPP_OK;
}

MPP_RET mpp_packet_deinit(MppPacket *packet)
{
    if (NULL == packet || check_is_mpp_packet(*packet)) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppPacketImpl *p = (MppPacketImpl *)(*packet);

    /* release buffer reference */
    if (p->buffer)
        mpp_buffer_put(p->buffer);

    if (p->flag & MPP_PACKET_FLAG_INTERNAL)
        mpp_free(p->data);

    if (p->meta)
        mpp_meta_put(p->meta);

    MPP_FREE(p->segments_ext);

    if (p->release)
        p->release(p->release_ctx, p->release_arg);

    mpp_mem_pool_put_f(pool_packet, *packet);
    *packet = NULL;
    return MPP_OK;
}

void mpp_packet_set_pos(MppPacket packet, void *pos)
{
    if (check_is_mpp_packet(packet))
        return;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    size_t offset = (RK_U8 *)pos - (RK_U8 *)p->data;
    size_t diff = (RK_U8 *)pos - (RK_U8 *)p->pos;

    /*
     * If set pos is a simple update on original buffer update the length
     * If set pos setup a new buffer reset length to size - offset
     * This will avoid assert on change "data" in mpp_packet
     */
    if (diff <= p->length)
        p->length -= diff;
    else
        p->length = p->size - offset;

    p->pos = pos;
    mpp_assert(p->data <= p->pos);
    mpp_assert(p->size >= p->length);
}

void *mpp_packet_get_pos(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return NULL;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    return p->pos;
}

MPP_RET mpp_packet_set_eos(MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return MPP_ERR_UNKNOW;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    p->flag |= MPP_PACKET_FLAG_EOS;
    return MPP_OK;
}

MPP_RET mpp_packet_clr_eos(MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return MPP_ERR_UNKNOW;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    p->flag &= ~MPP_PACKET_FLAG_EOS;
    return MPP_OK;
}

RK_U32 mpp_packet_get_eos(MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return 0;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    return (p->flag & MPP_PACKET_FLAG_EOS) ? (1) : (0);
}

MPP_RET mpp_packet_set_extra_data(MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return MPP_ERR_UNKNOW;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    p->flag |= MPP_PACKET_FLAG_EXTRA_DATA;
    return MPP_OK;
}

MPP_RET mpp_packet_reset(MppPacketImpl *packet)
{
    if (check_is_mpp_packet(packet))
        return MPP_ERR_UNKNOW;

    void *data = packet->data;
    size_t size = packet->size;

    memset(packet, 0, sizeof(*packet));

    packet->data = data;
    packet->pos  = data;
    packet->size = size;
    setup_mpp_packet_name(packet);
    mpp_packet_reset_segment(packet);
    return MPP_OK;
}

void mpp_packet_set_buffer(MppPacket packet, MppBuffer buffer)
{
    if (check_is_mpp_packet(packet))
        return;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    if (p->buffer != buffer) {
        if (buffer)
            mpp_buffer_inc_ref(buffer);

        if (p->buffer)
            mpp_buffer_put(p->buffer);

        p->buffer = buffer;
    }
}

MppBuffer mpp_packet_get_buffer(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return NULL;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    return p->buffer;
}

RK_S32 mpp_packet_has_meta(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return 0;

    MppPacketImpl *p = (MppPacketImpl *)packet;

    return (NULL != p->meta);
}

MppMeta mpp_packet_get_meta(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return NULL;

    MppPacketImpl *p = (MppPacketImpl *)packet;
    if (NULL == p->meta)
        mpp_meta_get(&p->meta);

    return p->meta;
}

MPP_RET mpp_packet_set_status(MppPacket packet, MppPacketStatus status)
{
    if (check_is_mpp_packet(packet))
        return MPP_ERR_UNKNOW;

    MppPacketImpl *p = (MppPacketImpl *)packet;

    p->status.val = status.val;
    return MPP_OK;
}

MPP_RET mpp_packet_get_status(MppPacket packet, MppPacketStatus *status)
{
    if (check_is_mpp_packet(packet)) {
        status->val = 0;
        return MPP_ERR_UNKNOW;
    }

    MppPacketImpl *p = (MppPacketImpl *)packet;

    status->val = p->status.val;
    return MPP_OK;
}

RK_U32 mpp_packet_is_partition(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return 0;

    MppPacketImpl *p = (MppPacketImpl *)packet;

    return (p->status.partition) || (p->flag & MPP_PACKET_FLAG_PARTITION);
}

RK_U32 mpp_packet_is_soi(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return 0;

    MppPacketImpl *p = (MppPacketImpl *)packet;

    return p->status.soi;
}

RK_U32 mpp_packet_is_eoi(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return 0;

    MppPacketImpl *p = (MppPacketImpl *)packet;

    return (p->status.eoi) || (p->flag & MPP_PACKET_FLAG_EOI);
}

MPP_RET mpp_packet_read(MppPacket packet, size_t offset, void *data, size_t size)
{
    if (check_is_mpp_packet(packet) || NULL == data) {
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
    if (check_is_mpp_packet(packet) || NULL == data) {
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

MPP_RET mpp_packet_copy(MppPacket dst, MppPacket src)
{
    if (check_is_mpp_packet(dst) || check_is_mpp_packet(src)) {
        mpp_err_f("invalid input: dst %p src %p\n", dst, src);
        return MPP_ERR_UNKNOW;
    }

    MppPacketImpl *dst_impl = (MppPacketImpl *)dst;
    MppPacketImpl *src_impl = (MppPacketImpl *)src;

    memcpy(dst_impl->pos, src_impl->pos, src_impl->length);
    dst_impl->length = src_impl->length;

    if (src_impl->segment_nb)
        mpp_packet_copy_segment_info(dst, src);

    return MPP_OK;
}

MPP_RET mpp_packet_append(MppPacket dst, MppPacket src)
{
    if (check_is_mpp_packet(dst) || check_is_mpp_packet(src)) {
        mpp_err_f("invalid input: dst %p src %p\n", dst, src);
        return MPP_ERR_UNKNOW;
    }

    MppPacketImpl *dst_impl = (MppPacketImpl *)dst;
    MppPacketImpl *src_impl = (MppPacketImpl *)src;

    memcpy((RK_U8 *)dst_impl->pos + dst_impl->length, src_impl->pos,
           src_impl->length);

    if (src_impl->segment_nb) {
        MppPktSeg *segs = src_impl->segments;
        RK_U32 offset = dst_impl->length;
        RK_U32 i;

        for (i = 0; i < src_impl->segment_nb; i++, segs++) {
            mpp_packet_add_segment_info(dst, segs->type, offset, segs->len);
            offset += segs->len;
        }
    }

    dst_impl->length += src_impl->length;
    return MPP_OK;
}

void mpp_packet_reset_segment(MppPacket packet)
{
    MppPacketImpl *p = (MppPacketImpl *)packet;

    p->segment_nb = 0;
    p->segment_buf_cnt = MPP_PKT_SEG_CNT_DEFAULT;
    memset(p->segments_def, 0, sizeof(p->segments_def));
    p->segments = NULL;
    MPP_FREE(p->segments_ext);
}

void mpp_packet_set_segment_nb(MppPacket packet, RK_U32 segment_nb)
{
    MppPacketImpl *p = (MppPacketImpl *)packet;
    MppPktSeg *segs = p->segments;
    RK_S32 i;

    if (segment_nb >= p->segment_nb || !segs)
        return;

    if (!segment_nb) {
        mpp_packet_reset_segment(packet);
        return;
    }

    /* truncate segment member and drop later segment info */
    if (segment_nb <= MPP_PKT_SEG_CNT_DEFAULT) {
        if (p->segments_ext) {
            memcpy(p->segments_def, segs, sizeof(*segs) * segment_nb);
            segs = p->segments_def;
            p->segments = segs;
            MPP_FREE(p->segments_ext);
        }

        p->segment_buf_cnt = MPP_PKT_SEG_CNT_DEFAULT;
    }

    /* relink segment info */
    for (i = 0; i < (RK_S32)segment_nb - 1; i++)
        segs[i].next = &segs[i + 1];

    segs[segment_nb - 1].next = NULL;

    p->segment_nb = segment_nb;
}

MPP_RET mpp_packet_add_segment_info(MppPacket packet, RK_S32 type, RK_S32 offset, RK_S32 len)
{
    MppPacketImpl *p = (MppPacketImpl *)packet;
    RK_U32 old_buf_cnt = p->segment_buf_cnt;
    RK_U32 segment_nb  = p->segment_nb;
    MppPktSeg *seg_buf = p->segments;

    if (segment_nb >= old_buf_cnt) {
        RK_U32 i;

        /* realloc segment info buffer. default 8 segments */
        old_buf_cnt *= 2;

        if (NULL == p->segments_ext) {
            seg_buf = mpp_calloc(MppPktSeg, old_buf_cnt);
            if (seg_buf)
                memcpy(seg_buf, p->segments_def, sizeof(p->segments_def));
        } else {
            seg_buf = mpp_realloc(p->segments_ext, MppPktSeg, old_buf_cnt);
        }

        if (NULL == seg_buf)
            return MPP_NOK;

        for (i = 0; i < segment_nb - 1; i++)
            seg_buf[i].next = &seg_buf[i + 1];

        p->segments_ext = seg_buf;
        p->segments = seg_buf;
        p->segment_buf_cnt = old_buf_cnt;
    } else {
        if (NULL == seg_buf) {
            seg_buf = p->segments_def;
            p->segments = seg_buf;
        }
    }

    mpp_assert(seg_buf);
    seg_buf += segment_nb;
    seg_buf->index  = segment_nb;
    seg_buf->type   = type;
    seg_buf->offset = offset;
    seg_buf->len    = len;
    seg_buf->next   = NULL;

    if (segment_nb)
        seg_buf[-1].next = seg_buf;

    p->segment_nb++;
    mpp_assert(p->segment_nb <= p->segment_buf_cnt);

    return MPP_OK;
}

void mpp_packet_copy_segment_info(MppPacket dst, MppPacket src)
{
    MppPacketImpl *dst_impl = (MppPacketImpl *)dst;
    MppPacketImpl *src_impl = (MppPacketImpl *)src;

    mpp_packet_reset_segment(dst);

    if (src_impl->segment_nb) {
        MppPktSeg *src_segs = src_impl->segments;
        MppPktSeg *dst_segs = NULL;
        RK_U32 segment_nb = src_impl->segment_nb;
        RK_U32 i;

        dst_impl->segment_nb = segment_nb;
        dst_impl->segment_buf_cnt = src_impl->segment_buf_cnt;

        if (segment_nb <= MPP_PKT_SEG_CNT_DEFAULT) {
            dst_segs = dst_impl->segments_def;

            memcpy(dst_segs, src_segs, sizeof(*dst_segs) * segment_nb);
        } else {
            dst_segs = mpp_calloc(MppPktSeg, dst_impl->segment_buf_cnt);

            mpp_assert(dst_segs);
            dst_impl->segments_ext = dst_segs;
            memcpy(dst_segs, src_segs, sizeof(*dst_segs) * segment_nb);
        }

        for (i = 0; i < segment_nb - 1; i++)
            dst_segs[i].next = &dst_segs[i + 1];

        dst_impl->segments = dst_segs;
    }
}

const MppPktSeg *mpp_packet_get_segment_info(const MppPacket packet)
{
    if (check_is_mpp_packet(packet))
        return NULL;

    MppPacketImpl *p = (MppPacketImpl *)packet;

    if (!p->segment_nb)
        return NULL;

    return (const MppPktSeg *)p->segments;
}

void mpp_packet_set_release(MppPacket packet, ReleaseCb release, void *ctx, void *arg)
{
    if (check_is_mpp_packet(packet))
        return;

    MppPacketImpl *p = (MppPacketImpl *)packet;

    p->release = release;
    p->release_ctx = ctx;
    p->release_arg = arg;
}

/*
 * object access function macro
 */
#define MPP_PACKET_ACCESSORS(type, field) \
    type mpp_packet_get_##field(const MppPacket s) \
    { \
        check_is_mpp_packet(s); \
        return ((MppPacketImpl*)s)->field; \
    } \
    void mpp_packet_set_##field(MppPacket s, type v) \
    { \
        check_is_mpp_packet(s); \
        ((MppPacketImpl*)s)->field = v; \
    }

#define MPP_PACKET_ACCESSOR_GET(type, field) \
    type mpp_packet_get_##field(const MppPacket s) \
    { \
        check_is_mpp_packet(s); \
        return ((MppPacketImpl*)s)->field; \
    }

MPP_PACKET_ACCESSORS(void *, data)
MPP_PACKET_ACCESSORS(size_t, size)
MPP_PACKET_ACCESSORS(size_t, length)
MPP_PACKET_ACCESSORS(RK_S64, pts)
MPP_PACKET_ACCESSORS(RK_S64, dts)
MPP_PACKET_ACCESSORS(RK_U32, flag)
MPP_PACKET_ACCESSORS(MppTask, task)
MPP_PACKET_ACCESSOR_GET(RK_U32, segment_nb)
