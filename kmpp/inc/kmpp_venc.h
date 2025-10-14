/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_VENC_H__
#define __KMPP_VENC_H__

#include "rk_venc_kcfg.h"

typedef void* KmppVenc;

#define KMPP_VENC_IOCTL_TABLE(prefix, IOC_CTX, IOC_IN_, IOC_OUT, IOC_IO_) \
    IOC_IN_(prefix, init,           MppVencKcfg)                \
    IOC_CTX(prefix, deinit)                                     \
    IOC_CTX(prefix, reset)                                      \
    IOC_CTX(prefix, start)                                      \
    IOC_CTX(prefix, stop)                                       \
    IOC_CTX(prefix, suspend)                                    \
    IOC_CTX(prefix, resume)                                     \
    IOC_IN_(prefix, get_cfg,        MppVencKcfg)                \
    IOC_IN_(prefix, set_cfg,        MppVencKcfg)                \
    IOC_IO_(prefix, encode,         KmppFrame,     KmppPacket)  \
    IOC_IN_(prefix, put_frm,        KmppFrame)                  \
    IOC_OUT(prefix, get_pkt,        KmppPacket)                 \
    IOC_IN_(prefix, put_pkt,        KmppPacket)

#define KMPP_OBJ_NAME               kmpp_venc
#define KMPP_OBJ_INTF_TYPE          KmppVenc
#define KMPP_OBJ_FUNC_IOCTL         KMPP_VENC_IOCTL_TABLE
#include "kmpp_obj_func.h"

#endif /*__KMPP_VENC_H__*/
