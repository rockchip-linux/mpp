/*
 * Copyright 2020 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MODULE_TAG "mpp_cfg"

#include <string.h>

#include "mpp_cfg.h"
#include "mpp_debug.h"

const char *cfg_type_names[] = {
    "RK_S32",
    "RK_U32",
    "RK_S64",
    "RK_U64",
    "void *",
    "struct",
};

static void show_cfg_info_err(MppCfgInfoNode *node, CfgType type, const char *func)
{
    mpp_err("%s cfg %s expect %s input NOT %s\n", func, node->name,
            cfg_type_names[node->data_type],
            cfg_type_names[type]);
}

MPP_RET mpp_cfg_set(MppCfgInfoNode *info, void *cfg, void *val)
{
    if (memcmp((char *)cfg + info->data_offset, val, info->data_size)) {
        memcpy((char *)cfg + info->data_offset, val, info->data_size);
        *((RK_U32 *)((char *)cfg + info->flag_offset)) |= info->flag_value;
    }
    return MPP_OK;
}

MPP_RET mpp_cfg_get(MppCfgInfoNode *info, void *cfg, void *val)
{
    memcpy(val, (char *)cfg + info->data_offset, info->data_size);
    return MPP_OK;
}

MPP_RET mpp_cfg_set_s32(MppCfgInfoNode *info, void *cfg, RK_S32 val)
{
    return mpp_cfg_set(info, cfg, &val);
}

MPP_RET mpp_cfg_get_s32(MppCfgInfoNode *info, void *cfg, RK_S32 *val)
{
    return mpp_cfg_get(info, cfg, val);
}

MPP_RET mpp_cfg_set_u32(MppCfgInfoNode *info, void *cfg, RK_U32 val)
{
    return mpp_cfg_set(info, cfg, &val);
}

MPP_RET mpp_cfg_get_u32(MppCfgInfoNode *info, void *cfg, RK_U32 *val)
{
    return mpp_cfg_get(info, cfg, val);
}

MPP_RET mpp_cfg_set_s64(MppCfgInfoNode *info, void *cfg, RK_S64 val)
{
    return mpp_cfg_set(info, cfg, &val);
}

MPP_RET mpp_cfg_get_s64(MppCfgInfoNode *info, void *cfg, RK_S64 *val)
{
    return mpp_cfg_get(info, cfg, val);
}

MPP_RET mpp_cfg_set_u64(MppCfgInfoNode *info, void *cfg, RK_U64 val)
{
    return mpp_cfg_set(info, cfg, &val);
}

MPP_RET mpp_cfg_get_u64(MppCfgInfoNode *info, void *cfg, RK_U64 *val)
{
    return mpp_cfg_get(info, cfg, val);
}

MPP_RET mpp_cfg_set_st(MppCfgInfoNode *info, void *cfg, void *val)
{
    return mpp_cfg_set(info, cfg, val);
}

MPP_RET mpp_cfg_get_st(MppCfgInfoNode *info, void *cfg, void *val)
{
    return mpp_cfg_get(info, cfg, val);
}

MPP_RET mpp_cfg_set_ptr(MppCfgInfoNode *info, void *cfg, void *val)
{
    return mpp_cfg_set(info, cfg, &val);
}

MPP_RET mpp_cfg_get_ptr(MppCfgInfoNode *info, void *cfg, void **val)
{
    return mpp_cfg_get(info, cfg, val);
}

MppCfgOps mpp_cfg_ops = {
    mpp_cfg_set_s32,
    mpp_cfg_get_s32,
    mpp_cfg_set_u32,
    mpp_cfg_get_u32,
    mpp_cfg_set_s64,
    mpp_cfg_get_s64,
    mpp_cfg_set_u64,
    mpp_cfg_get_u64,
    mpp_cfg_set_st,
    mpp_cfg_get_st,
    mpp_cfg_set_ptr,
    mpp_cfg_get_ptr,
};

#define MPP_CFG_SET_OPS(type)   ((long)(((void **)&mpp_cfg_ops)[type * 2 + 0]))
#define MPP_CFG_GET_OPS(type)   ((long)(((void **)&mpp_cfg_ops)[type * 2 + 1]))

MPP_RET mpp_cfg_node_fixup_func(MppCfgInfoNode *node)
{
    RK_S32 data_type = node->data_type;

    mpp_assert(data_type < CFG_FUNC_TYPE_BUTT);
    node->set_func = MPP_CFG_SET_OPS(data_type);
    node->get_func = MPP_CFG_GET_OPS(data_type);
    return MPP_OK;
}

MPP_RET check_cfg_info(MppCfgInfoNode *node, const char *name, CfgType type,
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
            show_cfg_info_err(node, type, func);
            ret = MPP_NOK;
        }
        if (cfg_size <= 0) {
            mpp_err("%s: cfg %s found invalid size %d\n", func, node->name, cfg_size);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_Ptr : {
        if (cfg_type != type) {
            show_cfg_info_err(node, type, func);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_S32 :
    case CFG_FUNC_TYPE_U32 : {
        if (cfg_size != sizeof(RK_S32)) {
            show_cfg_info_err(node, type, func);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_S64 :
    case CFG_FUNC_TYPE_U64 : {
        if (cfg_size != sizeof(RK_S64)) {
            show_cfg_info_err(node, type, func);
            ret = MPP_NOK;
        }
    } break;
    default : {
        mpp_err("%s: cfg %s found invalid cfg type %d\n", func, node->name, type);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}
