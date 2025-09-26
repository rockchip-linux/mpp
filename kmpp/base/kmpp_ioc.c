/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#include "kmpp_ioc.h"

typedef struct KmppIocImpl_t {
    /* object defintion index for ioctl functions */
    rk_u32              def;
    /* object defintion ioctl command */
    rk_u32              cmd;
    /*
     * flags for:
     * last in the batch / not last
     * block / non-block
     * return / non-return
     * sync / async
     * ack / non-ack
     * direct call / timer call
     */
    rk_u32              flags;
    /* ioc object id for input and output queue match */
    rk_u32              id;
    /* ioc context object */
    KmppShmPtr          ctx;
    /* input config object */
    KmppShmPtr          in;
    /* output return object */
    KmppShmPtr          out;
} KmppIocImpl;

#define KMPP_OBJ_NAME               kmpp_ioc
#define KMPP_OBJ_INTF_TYPE          KmppIoc
#define KMPP_OBJ_IMPL_TYPE          KmppIocImpl
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_KMPP_IOC
#define KMPP_OBJ_ENTRY_TABLE        KMPP_IOC_ENTRY_TABLE
#define KMPP_OBJ_MISMATCH_LOG_DISABLE
#include "kmpp_obj_helper.h"
