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

#define mpp_cfg_dbg(flag, fmt, ...) mpp_dbg(mpp_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_cfg_dbg_set(fmt, ...)   mpp_cfg_dbg(MPP_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define mpp_cfg_dbg_get(fmt, ...)   mpp_cfg_dbg(MPP_CFG_DBG_GET, fmt, ## __VA_ARGS__)

static RK_U32 mpp_cfg_debug = 0;

static void show_cfg_entry_err(KmppEntry *node, ElemType type, const char *func, const char *name)
{
    mpp_err("%s cfg %s expect %s input NOT %s\n", func, name,
            strof_elem_type(node->tbl.elem_type), strof_elem_type(type));
}

MPP_RET check_cfg_entry(KmppEntry *node, const char *name, ElemType type,
                        const char *func)
{
    if (NULL == node) {
        mpp_err("%s: cfg %s is invalid\n", func, name);
        return MPP_NOK;
    }

    ElemType cfg_type = (ElemType)node->tbl.elem_type;
    RK_S32 cfg_size = node->tbl.elem_size;
    MPP_RET ret = MPP_OK;

    switch (type) {
    case ELEM_TYPE_st : {
        if (cfg_type != type) {
            show_cfg_entry_err(node, type, func, name);
            ret = MPP_NOK;
        }
        if (cfg_size <= 0) {
            mpp_err("%s: cfg %s found invalid size %d\n", func, name, cfg_size);
            ret = MPP_NOK;
        }
    } break;
    case ELEM_TYPE_ptr : {
        if (cfg_type != type) {
            show_cfg_entry_err(node, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    case ELEM_TYPE_s32 :
    case ELEM_TYPE_u32 : {
        if (cfg_size != sizeof(RK_S32)) {
            show_cfg_entry_err(node, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    case ELEM_TYPE_s64 :
    case ELEM_TYPE_u64 : {
        if (cfg_size != sizeof(RK_S64)) {
            show_cfg_entry_err(node, type, func, name);
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
