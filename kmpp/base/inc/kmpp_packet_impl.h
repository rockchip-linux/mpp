/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_PACKET_IMPL_H__
#define __KMPP_PACKET_IMPL_H__

#include "kmpp_packet.h"

#define KMPP_PACKET_FLAG_EOS        (0x00000001)
#define KMPP_PACKET_FLAG_EXTRA_DATA (0x00000002)
#define KMPP_PACKET_FLAG_INTERNAL   (0x00000004)
#define KMPP_PACKET_FLAG_EXTERNAL   (0x00000008)
#define KMPP_PACKET_FLAG_INTRA      (0x00000010)
#define KMPP_PACKET_FLAG_PARTITION  (0x00000020)
#define KMPP_PACKET_FLAG_EOI        (0x00000040)

typedef struct RingBufPool_t {
    rk_u32 r_pos;
    rk_u32 w_pos;
    rk_u32 len;
    rk_u32 use_len;
    void *buf_base;
    MppBuffer buf;
    rk_u32 init_done;
    rk_u32 min_buf_size;
    rk_u32 l_r_pos;
    rk_u32 l_w_pos;
    rk_u32 max_use_len;
} RingBufPool;

typedef struct RingBuf_t {
    MppBuffer buf;
    void *buf_start;
    RingBufPool *ring_pool;
    rk_u32 start_offset;
    rk_u32 r_pos;
    rk_u32 use_len;
    rk_u32 size;
    rk_u32 cir_flag;
} RingBuf;

typedef struct KmppPacketImpl_t {
    const char *name;
    rk_s32 size;
    rk_s32 length;
    rk_s64 pts;
    rk_s64 dts;
    rk_u32 status;
    rk_u32 flag;
    rk_u32 temporal_id;
    KmppShmPtr data;
    KmppShmPtr pos;
    KmppShmPtr buffer;
    RingBuf buf;
} KmppPacketImpl;

typedef struct KmppPacketPriv_t {
    KmppMeta    meta;
} KmppPacketPriv;

#endif /* __KMPP_PACKET_IMPL_H__ */