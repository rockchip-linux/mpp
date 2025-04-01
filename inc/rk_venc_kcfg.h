/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __RK_VENC_KCFG_H__
#define __RK_VENC_KCFG_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef void* MppVencKcfg;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MPP_VENC_KCFG_TYPE_INIT,
    MPP_VENC_KCFG_TYPE_DEINIT,
    MPP_VENC_KCFG_TYPE_RESET,
    MPP_VENC_KCFG_TYPE_START,
    MPP_VENC_KCFG_TYPE_STOP,
    MPP_VENC_KCFG_TYPE_BUTT,
} MppVencKcfgType;

MPP_RET mpp_venc_kcfg_init(MppVencKcfg *cfg, MppVencKcfgType type);
MPP_RET mpp_venc_kcfg_init_by_name(MppVencKcfg *cfg, const char *name);
MPP_RET mpp_venc_kcfg_deinit(MppVencKcfg cfg);

MPP_RET mpp_venc_kcfg_set_s32(MppVencKcfg cfg, const char *name, RK_S32 val);
MPP_RET mpp_venc_kcfg_set_u32(MppVencKcfg cfg, const char *name, RK_U32 val);
MPP_RET mpp_venc_kcfg_set_s64(MppVencKcfg cfg, const char *name, RK_S64 val);
MPP_RET mpp_venc_kcfg_set_u64(MppVencKcfg cfg, const char *name, RK_U64 val);
MPP_RET mpp_venc_kcfg_set_ptr(MppVencKcfg cfg, const char *name, void *val);
MPP_RET mpp_venc_kcfg_set_st(MppVencKcfg cfg, const char *name, void *val);

MPP_RET mpp_venc_kcfg_get_s32(MppVencKcfg cfg, const char *name, RK_S32 *val);
MPP_RET mpp_venc_kcfg_get_u32(MppVencKcfg cfg, const char *name, RK_U32 *val);
MPP_RET mpp_venc_kcfg_get_s64(MppVencKcfg cfg, const char *name, RK_S64 *val);
MPP_RET mpp_venc_kcfg_get_u64(MppVencKcfg cfg, const char *name, RK_U64 *val);
MPP_RET mpp_venc_kcfg_get_ptr(MppVencKcfg cfg, const char *name, void **val);
MPP_RET mpp_venc_kcfg_get_st(MppVencKcfg cfg, const char *name, void *val);

void mpp_venc_kcfg_show(MppVencKcfg cfg);

#ifdef __cplusplus
}
#endif

#endif /*__RK_VENC_KCFG_H__*/
