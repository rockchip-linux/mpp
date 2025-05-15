/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_BUFFER_IMPL_H__
#define __KMPP_BUFFER_IMPL_H__

#include "mpp_list.h"
#include "kmpp_obj.h"
#include "kmpp_buffer.h"

#define BUF_ST_NONE             (0)
#define BUF_ST_INIT             (1)
#define BUF_ST_INIT_TO_USED     (2)
#define BUF_ST_USED             (3)
#define BUF_ST_UNUSED           (4)
#define BUF_ST_USED_TO_DEINIT   (5)
#define BUF_ST_DEINIT_AT_GRP    (6)
#define BUF_ST_DEINIT_AT_SRV    (7)

/* buffer group share name storage for both name and allocator */
#define BUF_GRP_STR_BUF_SIZE    (64)

typedef struct KmppBufGrpCfgImpl_t {
    rk_u32              flag;
    rk_u32              count;
    rk_u32              size;
    KmppBufferMode      mode;
    rk_s32              fd;
    rk_s32              grp_id;
    rk_s32              used;
    rk_s32              unused;
    void                *device;
    KmppShmPtr          allocator;
    KmppShmPtr          name;
    void                *grp_impl;
} KmppBufGrpCfgImpl;

typedef struct KmppBufGrpPriv_t {
    KmppObj             obj;
    KmppBufGrpCfgImpl   *impl;
} KmppBufGrpPriv;

typedef struct KmppBufGrpImpl_t {
    /* group share config pointer */
    KmppShmPtr          cfg;
    KmppBufGrpCfg       cfg_ext;
    KmppBufGrpCfgImpl   *cfg_usr;
    KmppObj             obj;

    /* internal parameter set on used */
    rk_u32              flag;
    rk_u32              count;
    rk_u32              size;
    KmppBufferMode      mode;

    pthread_mutex_t     lock;
    struct list_head    list_srv;
    struct list_head    list_used;
    struct list_head    list_unused;
    rk_s32              count_used;
    rk_s32              count_unused;

    /* allocator */
    void*               heap;
    rk_s32              heap_fd;
    rk_s32              grp_id;
    rk_s32              buf_id;
    rk_s32              buf_cnt;
    rk_s32              buffer_count;

    /* status status */
    rk_s32              log_runtime_en;
    rk_s32              log_history_en;
    rk_s32              clear_on_exit;
    rk_s32              dump_on_exit;
    rk_s32              is_orphan;
    rk_s32              is_finalizing;
    rk_s32              is_default;

    /* string storage */
    rk_s32              name_offset;
    rk_u8               str_buf[BUF_GRP_STR_BUF_SIZE];

    /* buffer group config for internal usage */
    KmppBufGrpCfgImpl   cfg_int;
} KmppBufGrpImpl;

typedef struct KmppBufCfgImpl_t {
    rk_u32              size;
    rk_u32              offset;
    rk_u32              flag;
    rk_s32              fd;
    rk_s32              index;      /* index for external user buffer match */
    rk_s32              grp_id;
    rk_s32              buf_gid;
    rk_s32              buf_uid;

    void                *hnd;
    void                *dmabuf;
    void                *kdev;
    void                *dev;
    rk_u64              iova;
    void                *kptr;
    void                *kpriv;
    void                *kfp;
    rk_u64              uptr;
    rk_u64              upriv;
    rk_u64              ufp;
    KmppShmPtr          sptr;
    KmppShmPtr          group;
    void                *buf_impl;
} KmppBufCfgImpl;

typedef struct KmppBufPriv_t {
    KmppObj             obj;
    KmppBufCfgImpl      *impl;
} KmppBufPriv;

typedef struct KmppBufferImpl_t {
    KmppShmPtr          cfg;
    KmppBufCfg          cfg_ext;
    KmppBufCfgImpl      *cfg_usr;

    KmppBufGrpImpl      *grp;
    /* when grp is valid used grp lock else use srv lock */
    pthread_mutex_t     lock;
    struct list_head    list_status;
    KmppObj             obj;

    KmppDmaBuf          buf;
    void                *kptr;
    rk_u64              uaddr;
    rk_u32              size;

    rk_s32              grp_id;
    rk_s32              buf_gid;
    rk_s32              buf_uid;
    rk_s32              ref_cnt;

    rk_u32              status;
    rk_u32              discard;

    /* mutex for list_maps */
    void                *mutex_maps;
    /* list for list in KmppBufIovaMap */
    struct list_head    list_maps;

    KmppBufCfgImpl      cfg_int;
} KmppBufferImpl;

#endif /* __KMPP_BUF_GRP_IMPL_H__ */
