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
    MPP_SGLN_OS_ALLOCATOR,
    MPP_SGLN_MEM_POOL,
    /* hardware platform */
    MPP_SGLN_SOC,
    MPP_SGLN_PLATFORM,
    /* software platform */
    MPP_SGLN_RUNTIME,
    /* base module */
    MPP_SGLN_BUFFER,
    MPP_SGLN_META,
    MPP_SGLN_FRAME,
    MPP_SGLN_PACKET,
    /* system module */
    MPP_SGLN_KOBJ,
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

#define MPP_SINGLETON(id, name, init, deinit) \
    __attribute__((constructor)) \
    static void __mpp_singleton_add(void) { \
        MppSingletonInfo info = { \
            id, \
            name, \
            init, \
            deinit, \
        }; \
        mpp_singleton_add(&info); \
    }

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_singleton_add(MppSingletonInfo *info);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_SINGLETON_H__ */