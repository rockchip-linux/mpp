/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_CFG_H__
#define __MPP_CFG_H__

#include "mpp_internal.h"

/* header size 128 byte */
typedef struct MppCfgInfoHead_t {
    char            version[116];
    RK_S32          info_size;
    RK_S32          info_count;
    RK_S32          node_count;
} MppCfgInfoHead;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET mpp_cfg_set_s32(MppCfgInfo *info, void *cfg, RK_S32 val);
MPP_RET mpp_cfg_get_s32(MppCfgInfo *info, void *cfg, RK_S32 *val);
MPP_RET mpp_cfg_set_u32(MppCfgInfo *info, void *cfg, RK_U32 val);
MPP_RET mpp_cfg_get_u32(MppCfgInfo *info, void *cfg, RK_U32 *val);
MPP_RET mpp_cfg_set_s64(MppCfgInfo *info, void *cfg, RK_S64 val);
MPP_RET mpp_cfg_get_s64(MppCfgInfo *info, void *cfg, RK_S64 *val);
MPP_RET mpp_cfg_set_u64(MppCfgInfo *info, void *cfg, RK_U64 val);
MPP_RET mpp_cfg_get_u64(MppCfgInfo *info, void *cfg, RK_U64 *val);
MPP_RET mpp_cfg_set_st(MppCfgInfo *info, void *cfg, void *val);
MPP_RET mpp_cfg_get_st(MppCfgInfo *info, void *cfg, void *val);
MPP_RET mpp_cfg_set_ptr(MppCfgInfo *info, void *cfg, void *val);
MPP_RET mpp_cfg_get_ptr(MppCfgInfo *info, void *cfg, void **val);

#define MPP_CFG_SET_S32(info, cfg, val) (mpp_cfg_set_s32)(info, cfg, val)
#define MPP_CFG_GET_S32(info, cfg, val) (mpp_cfg_get_s32)(info, cfg, (RK_S32 *)(val))
#define MPP_CFG_SET_U32(info, cfg, val) (mpp_cfg_set_u32)(info, cfg, val)
#define MPP_CFG_GET_U32(info, cfg, val) (mpp_cfg_get_u32)(info, cfg, (RK_U32 *)(val))
#define MPP_CFG_SET_S64(info, cfg, val) (mpp_cfg_set_s64)(info, cfg, val)
#define MPP_CFG_GET_S64(info, cfg, val) (mpp_cfg_get_s64)(info, cfg, (RK_S64 *)(val))
#define MPP_CFG_SET_U64(info, cfg, val) (mpp_cfg_set_u64)(info, cfg, val)
#define MPP_CFG_GET_U64(info, cfg, val) (mpp_cfg_get_u64)(info, cfg, (RK_U64 *)(val))
#define MPP_CFG_SET_St(info, cfg, val)  (mpp_cfg_set_st )(info, cfg, val)
#define MPP_CFG_GET_St(info, cfg, val)  (mpp_cfg_get_st )(info, cfg, (void *)(val))
#define MPP_CFG_SET_Ptr(info, cfg, val) (mpp_cfg_set_ptr)(info, cfg, val)
#define MPP_CFG_GET_Ptr(info, cfg, val) (mpp_cfg_get_ptr)(info, cfg, (void **)(val))

const char *strof_cfg_type(CfgType type);

#define CHECK_CFG_INFO(node, name, type) \
    check_cfg_info(node, name, type, __FUNCTION__)

MPP_RET check_cfg_info(MppCfgInfo *node, const char *name, CfgType type,
                       const char *func);

#ifdef  __cplusplus
}
#endif

#endif /*__MPP_CFG_H__*/
