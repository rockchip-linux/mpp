/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef RK_VDEC_KCFG_H
#define RK_VDEC_KCFG_H

#include "rk_type.h"
#include "mpp_err.h"

typedef void* MppVdecKcfg;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MPP_VDEC_KCFG_TYPE_INIT,
    MPP_VDEC_KCFG_TYPE_DEINIT,
    MPP_VDEC_KCFG_TYPE_RESET,
    MPP_VDEC_KCFG_TYPE_START,
    MPP_VDEC_KCFG_TYPE_STOP,
    MPP_VDEC_KCFG_TYPE_BUTT,
} MppVdecKcfgType;

MPP_RET mpp_vdec_kcfg_init(MppVdecKcfg *cfg, MppVdecKcfgType type);
MPP_RET mpp_vdec_kcfg_init_by_name(MppVdecKcfg *cfg, const char *name);
MPP_RET mpp_vdec_kcfg_deinit(MppVdecKcfg cfg);

MPP_RET mpp_vdec_kcfg_set_s32(MppVdecKcfg cfg, const char *name, RK_S32 val);
MPP_RET mpp_vdec_kcfg_set_u32(MppVdecKcfg cfg, const char *name, RK_U32 val);
MPP_RET mpp_vdec_kcfg_set_s64(MppVdecKcfg cfg, const char *name, RK_S64 val);
MPP_RET mpp_vdec_kcfg_set_u64(MppVdecKcfg cfg, const char *name, RK_U64 val);
MPP_RET mpp_vdec_kcfg_set_ptr(MppVdecKcfg cfg, const char *name, void *val);
MPP_RET mpp_vdec_kcfg_set_st(MppVdecKcfg cfg, const char *name, void *val);

MPP_RET mpp_vdec_kcfg_get_s32(MppVdecKcfg cfg, const char *name, RK_S32 *val);
MPP_RET mpp_vdec_kcfg_get_u32(MppVdecKcfg cfg, const char *name, RK_U32 *val);
MPP_RET mpp_vdec_kcfg_get_s64(MppVdecKcfg cfg, const char *name, RK_S64 *val);
MPP_RET mpp_vdec_kcfg_get_u64(MppVdecKcfg cfg, const char *name, RK_U64 *val);
MPP_RET mpp_vdec_kcfg_get_ptr(MppVdecKcfg cfg, const char *name, void **val);
MPP_RET mpp_vdec_kcfg_get_st(MppVdecKcfg cfg, const char *name, void *val);

void mpp_vdec_kcfg_show(MppVdecKcfg cfg);

#ifdef __cplusplus
}
#endif

#endif /* RK_VDEC_KCFG_H */
