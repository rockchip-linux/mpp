/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_cfg"

#include <string.h>

#include "mpp_env.h"
#include "mpp_cfg.h"
#include "mpp_debug.h"

#define MPP_CFG_DBG_SET             (0x00000001)
#define MPP_CFG_DBG_GET             (0x00000002)

#define mpp_cfg_dbg(flag, fmt, ...) _mpp_dbg(mpp_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_cfg_dbg_set(fmt, ...)   mpp_cfg_dbg(MPP_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define mpp_cfg_dbg_get(fmt, ...)   mpp_cfg_dbg(MPP_CFG_DBG_GET, fmt, ## __VA_ARGS__)

#define CFG_TO_PTR(info, cfg)       ((char *)cfg + info->data_offset)
#define CFG_TO_s32_PTR(info, cfg)   ((RK_S32 *)CFG_TO_PTR(info, cfg))
#define CFG_TO_u32_PTR(info, cfg)   ((RK_U32 *)CFG_TO_PTR(info, cfg))
#define CFG_TO_s64_PTR(info, cfg)   ((RK_S64 *)CFG_TO_PTR(info, cfg))
#define CFG_TO_u64_PTR(info, cfg)   ((RK_U64 *)CFG_TO_PTR(info, cfg))
#define CFG_TO_ptr_PTR(info, cfg)   ((void **)CFG_TO_PTR(info, cfg))

#define CFG_TO_FLAG_PTR(info, cfg)  ((RK_U32 *)((char *)cfg + info->flag_offset))

static RK_U32 mpp_cfg_debug = 0;

const char *strof_cfg_type(CfgType type)
{
    static const char *cfg_type_names[] = {
        "RK_S32",
        "RK_U32",
        "RK_S64",
        "RK_U64",
        "struct",
        "void *",
        "unknown"
    };

    if (type < 0 || type >= CFG_FUNC_TYPE_BUTT)
        type = CFG_FUNC_TYPE_BUTT;

    return cfg_type_names[type];
}

static void show_cfg_info_err(MppCfgInfo *node, CfgType type, const char *func, const char *name)
{
    mpp_err("%s cfg %s expect %s input NOT %s\n", func, name,
            strof_cfg_type(node->data_type), strof_cfg_type(type));
}

static MPP_RET mpp_cfg_set(MppCfgInfo *info, void *cfg, void *val)
{
    if (memcmp((char *)cfg + info->data_offset, val, info->data_size)) {
        memcpy((char *)cfg + info->data_offset, val, info->data_size);
        *((RK_U32 *)((char *)cfg + info->flag_offset)) |= info->flag_value;
    }
    return MPP_OK;
}

static MPP_RET mpp_cfg_get(MppCfgInfo *info, void *cfg, void *val)
{
    memcpy(val, (char *)cfg + info->data_offset, info->data_size);
    return MPP_OK;
}

#define MPP_CFG_ACCESS(type, base_type) \
    MPP_RET mpp_cfg_set_##type(MppCfgInfo *info, void *cfg, base_type val) \
    { \
        base_type *dst = CFG_TO_##type##_PTR(info, cfg); \
        base_type old = dst[0]; \
        dst[0] = val; \
        if (!info->flag_value) { \
            mpp_cfg_dbg_set("%p + %d set " #type " change %d -> %d\n", cfg, info->data_offset, old, val); \
        } else { \
            if (old != val) { \
                mpp_cfg_dbg_set("%p + %d set " #type " update %d -> %d flag %d|%x\n", \
                                cfg, info->data_offset, old, val, info->flag_offset, info->flag_value); \
                CFG_TO_FLAG_PTR(info, cfg)[0] |= info->flag_value; \
            } else { \
                mpp_cfg_dbg_set("%p + %d set " #type " keep   %d\n", cfg, info->data_offset, old); \
            } \
        } \
        return MPP_OK; \
    } \
    MPP_RET mpp_cfg_get_##type(MppCfgInfo *info, void *cfg, base_type *val) \
    { \
        if (info && info->data_size) { \
            base_type *src = CFG_TO_##type##_PTR(info, cfg); \
            mpp_cfg_dbg_set("%p + %d get " #type " value  %d\n", cfg, info->data_offset, src[0]); \
            val[0] = src[0]; \
            return MPP_OK; \
        } \
        return MPP_NOK; \
    }

MPP_CFG_ACCESS(s32, RK_S32)
MPP_CFG_ACCESS(u32, RK_U32)
MPP_CFG_ACCESS(s64, RK_S64)
MPP_CFG_ACCESS(u64, RK_U64)
MPP_CFG_ACCESS(ptr, void *)

MPP_RET mpp_cfg_set_st(MppCfgInfo *info, void *cfg, void *val)
{
    return mpp_cfg_set(info, cfg, val);
}

MPP_RET mpp_cfg_get_st(MppCfgInfo *info, void *cfg, void *val)
{
    return mpp_cfg_get(info, cfg, val);
}

MPP_RET check_cfg_info(MppCfgInfo *node, const char *name, CfgType type,
                       const char *func)
{
    if (NULL == node) {
        mpp_err("%s: cfg %s is invalid\n", func, name);
        return MPP_NOK;
    }

    CfgType cfg_type = (CfgType)node->data_type;
    RK_S32 cfg_size = node->data_size;
    MPP_RET ret = MPP_OK;

    switch (type) {
    case CFG_FUNC_TYPE_St : {
        if (cfg_type != type) {
            show_cfg_info_err(node, type, func, name);
            ret = MPP_NOK;
        }
        if (cfg_size <= 0) {
            mpp_err("%s: cfg %s found invalid size %d\n", func, name, cfg_size);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_Ptr : {
        if (cfg_type != type) {
            show_cfg_info_err(node, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_S32 :
    case CFG_FUNC_TYPE_U32 : {
        if (cfg_size != sizeof(RK_S32)) {
            show_cfg_info_err(node, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_S64 :
    case CFG_FUNC_TYPE_U64 : {
        if (cfg_size != sizeof(RK_S64)) {
            show_cfg_info_err(node, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    default : {
        mpp_err("%s: cfg %s found invalid cfg type %d\n", func, name, type);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}
