/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __RK_MPP_CFG_H__
#define __RK_MPP_CFG_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef void* MppSysCfg;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_sys_cfg_get(MppSysCfg *cfg);
MPP_RET mpp_sys_cfg_put(MppSysCfg cfg);
MPP_RET mpp_sys_cfg_ioctl(MppSysCfg cfg);

MPP_RET mpp_sys_cfg_set_s32(MppSysCfg cfg, const char *name, RK_S32 val);
MPP_RET mpp_sys_cfg_set_u32(MppSysCfg cfg, const char *name, RK_U32 val);
MPP_RET mpp_sys_cfg_set_s64(MppSysCfg cfg, const char *name, RK_S64 val);
MPP_RET mpp_sys_cfg_set_u64(MppSysCfg cfg, const char *name, RK_U64 val);
MPP_RET mpp_sys_cfg_set_ptr(MppSysCfg cfg, const char *name, void *val);

MPP_RET mpp_sys_cfg_get_s32(MppSysCfg cfg, const char *name, RK_S32 *val);
MPP_RET mpp_sys_cfg_get_u32(MppSysCfg cfg, const char *name, RK_U32 *val);
MPP_RET mpp_sys_cfg_get_s64(MppSysCfg cfg, const char *name, RK_S64 *val);
MPP_RET mpp_sys_cfg_get_u64(MppSysCfg cfg, const char *name, RK_U64 *val);
MPP_RET mpp_sys_cfg_get_ptr(MppSysCfg cfg, const char *name, void **val);

void mpp_sys_cfg_show(void);

#ifdef __cplusplus
}
#endif

#endif /*__RK_MPP_CFG_H__*/
