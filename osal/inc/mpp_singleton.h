/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_SINGLETON_H
#define MPP_SINGLETON_H

#include "mpp_sgln_base.h"

#define MPP_SGLN_MAX_CNT 64
#define MPP_SGLN_NO_ID_MAX_CNT 128

typedef enum MppSingletonId_e {
    MPP_SGLN_BASE           = 0,
    /* osal base module */
    MPP_SGLN_OS_LOG         = MPP_SGLN_BASE,
    MPP_SGLN_OS_MEM,
    MPP_SGLN_TRACE,
    MPP_SGLN_DMABUF,
    MPP_SGLN_OS_ALLOCATOR,
    MPP_SGLN_MEM_POOL,
    /* hardware platform */
    MPP_SGLN_SOC,
    MPP_SGLN_PLATFORM,
    MPP_SGLN_SERVER,
    MPP_SGLN_CLUSTER,
    /* software platform */
    MPP_SGLN_RUNTIME,
    MPP_SGLN_SYS,
    MPP_SGLN_RING,
    /* kernel module (MUST before userspace module) */
    MPP_SGLN_KOBJ,
    MPP_SGLN_KMPP_IOC,
    MPP_SGLN_KMPP_BUF_GRP_CFG,
    MPP_SGLN_KMPP_BUF_GRP,
    MPP_SGLN_KMPP_BUF_CFG,
    MPP_SGLN_KMPP_BUFFER,
    MPP_SGLN_KMPP_META,
    MPP_SGLN_KMPP_FRAME,
    MPP_SGLN_KMPP_PACKET,
    MPP_SGLN_KMPP_VENC_CFG,
    MPP_SGLN_KMPP_VENC,
    MPP_SGLN_KMPP_VDEC_CFG,
    MPP_SGLN_KMPP_VDEC,
    /* userspace base module */
    MPP_SGLN_BUFFER,
    MPP_SGLN_META,
    MPP_SGLN_FRAME,
    MPP_SGLN_PACKET,
    /* userspace system module */
    MPP_SGLN_SYS_CFG,
    MPP_SGLN_ENC_CFG,
    MPP_SGLN_DEC_CFG,
    MPP_SGLN_ENC_RC_API,
} MppSingletonId;

#define MPP_SINGLETON(id, name, init, deinit) \
    MPP_SGLN_DEF(mpp_singleton_add, id, name, init, deinit)

#define MPP_MODULE_ADD(name, init, deinit) \
    MPP_MODULE_DEF(mpp_singleton_add, name, init, deinit)

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_singleton_add(MppSglnInfo *info, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* MPP_SINGLETON_H */
