/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef KMPP_VDEC_H
#define KMPP_VDEC_H

#include "rk_vdec_kcfg.h"

typedef void* KmppVdec;

#define KMPP_VDEC_IOCTL_TABLE(prefix, IOC_CTX, IOC_IN_, IOC_OUT, IOC_IO_) \
    IOC_IN_(prefix, init,           MppVdecKcfg)                \
    IOC_CTX(prefix, deinit)                                     \
    IOC_CTX(prefix, reset)                                      \
    IOC_CTX(prefix, start)                                      \
    IOC_CTX(prefix, stop)                                       \
    IOC_IN_(prefix, get_cfg,        MppVdecKcfg)                \
    IOC_IN_(prefix, set_cfg,        MppVdecKcfg)                \
    IOC_IN_(prefix, get_rt_cfg,     MppVdecKcfg)                \
    IOC_IN_(prefix, set_rt_cfg,     MppVdecKcfg)                \
    IOC_IO_(prefix, decode,         KmppPacket,     KmppFrame)  \
    IOC_IN_(prefix, put_pkt,        KmppPacket)                 \
    IOC_OUT(prefix, get_frm,        KmppFrame)                  \
    IOC_IN_(prefix, put_frm,        KmppFrame)

#define KMPP_OBJ_NAME               kmpp_vdec
#define KMPP_OBJ_INTF_TYPE          KmppVdec
#define KMPP_OBJ_FUNC_IOCTL         KMPP_VDEC_IOCTL_TABLE
#include "kmpp_obj_func.h"

#endif /* KMPP_VDEC_H */
