/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __RK_VENC_KCFG_H__
#define __RK_VENC_KCFG_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef void* KmppVenccfg;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KMPP_VENC_CFG_TYPE_INIT,
    KMPP_VENC_CFG_TYPE_DEINIT,
    KMPP_VENC_CFG_TYPE_RESET,
    KMPP_VENC_CFG_TYPE_START,
    KMPP_VENC_CFG_TYPE_STOP,
    KMPP_VENC_CFG_TYPE_BUTT,
} MppVencKcfgType;

MPP_RET kmpp_venc_cfg_init(KmppVenccfg *cfg, MppVencKcfgType type);
MPP_RET kmpp_venc_cfg_init_by_name(KmppVenccfg *cfg, const char *name);
MPP_RET kmpp_venc_cfg_deinit(KmppVenccfg cfg);

MPP_RET kmpp_venc_cfg_set_s32(KmppVenccfg cfg, const char *name, RK_S32 val);
MPP_RET kmpp_venc_cfg_set_u32(KmppVenccfg cfg, const char *name, RK_U32 val);
MPP_RET kmpp_venc_cfg_set_s64(KmppVenccfg cfg, const char *name, RK_S64 val);
MPP_RET kmpp_venc_cfg_set_u64(KmppVenccfg cfg, const char *name, RK_U64 val);
MPP_RET kmpp_venc_cfg_set_ptr(KmppVenccfg cfg, const char *name, void *val);
MPP_RET kmpp_venc_cfg_set_st(KmppVenccfg cfg, const char *name, void *val);

MPP_RET kmpp_venc_cfg_get_s32(KmppVenccfg cfg, const char *name, RK_S32 *val);
MPP_RET kmpp_venc_cfg_get_u32(KmppVenccfg cfg, const char *name, RK_U32 *val);
MPP_RET kmpp_venc_cfg_get_s64(KmppVenccfg cfg, const char *name, RK_S64 *val);
MPP_RET kmpp_venc_cfg_get_u64(KmppVenccfg cfg, const char *name, RK_U64 *val);
MPP_RET kmpp_venc_cfg_get_ptr(KmppVenccfg cfg, const char *name, void **val);
MPP_RET kmpp_venc_cfg_get_st(KmppVenccfg cfg, const char *name, void *val);

void kmpp_venc_cfg_show(KmppVenccfg cfg);

#ifdef __cplusplus
}
#endif

#endif /*__RK_VENC_KCFG_H__*/
