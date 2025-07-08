/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_SINGLETON_H__
#define __MPP_SINGLETON_H__

#include "rk_type.h"

typedef enum MppSingletonId_e {
    MPP_SGLN_BASE           = 0,
    /* osal base module */
    MPP_SGLN_OS_LOG         = MPP_SGLN_BASE,
    MPP_SGLN_OS_MEM,
    MPP_SGLN_TRACE,
    MPP_SGLN_OS_ALLOCATOR,
    MPP_SGLN_MEM_POOL,
    /* hardware platform */
    MPP_SGLN_SOC,
    MPP_SGLN_PLATFORM,
    MPP_SGLN_SERVER,
    /* software platform */
    MPP_SGLN_RUNTIME,
    /* kernel module (MUST before userspace module) */
    MPP_SGLN_KOBJ,
    MPP_SGLN_KMPP_BUFFER,
    MPP_SGLN_KMPP_META,
    MPP_SGLN_KMPP_FRAME,
    MPP_SGLN_KMPP_PACKET,
    MPP_SGLN_KMPP_VENC_CFG,
    /* userspace base module */
    MPP_SGLN_BUFFER,
    MPP_SGLN_META,
    MPP_SGLN_FRAME,
    MPP_SGLN_PACKET,
    /* userspace system module */
    MPP_SGLN_ENC_CFG,
    MPP_SGLN_DEC_CFG,
    MPP_SGLN_DEC_RC_API,

    /* max count for start init process */
    MPP_SGLN_MAX_CNT,
} MppSingletonId;

typedef struct MppSingletonInfo_t {
    MppSingletonId  id;
    const char      *name;
    void            (*init)(void);
    void            (*deinit)(void);
} MppSingletonInfo;

#define SNGL_TO_STR(x)  #x
#define SNGL_TO_FUNC(x) __mpp_singleton_add_##x
/* warning: constructor priorities from 0 to 100 are reserved for the implementation */
#define SNGL_BASE_ID    101
#define MPP_SINGLETON(id, name, init, deinit) \
    /* increase id from base id to avoid compiler warning */ \
    __attribute__((constructor(SNGL_BASE_ID + id))) \
    static void SNGL_TO_FUNC(name)(void) { \
        MppSingletonInfo info = { \
            id, \
            SNGL_TO_STR(name), \
            init, \
            deinit, \
        }; \
        mpp_singleton_add(&info, __FUNCTION__); \
    }

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_singleton_add(MppSingletonInfo *info, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_SINGLETON_H__ */
