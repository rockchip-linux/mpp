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

#ifndef __MPP_CFG_H__
#define __MPP_CFG_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef enum CfgType_e {
    CFG_FUNC_TYPE_S32,
    CFG_FUNC_TYPE_U32,
    CFG_FUNC_TYPE_S64,
    CFG_FUNC_TYPE_U64,
    CFG_FUNC_TYPE_St,
    CFG_FUNC_TYPE_Ptr,
    CFG_FUNC_TYPE_BUTT,
} CfgType;

typedef struct MppCfgApi_t {
    const char          *name;
    CfgType             data_type;
    RK_U32              flag_offset;
    RK_U32              flag_value;
    RK_U32              data_offset;
    RK_S32              data_size;
} MppCfgApi;

typedef struct MppCfgInfo_t {
    /* CfgType */
    RK_S32          data_type;
    /* update flag info 32bit */
    RK_S32          flag_offset;
    RK_U32          flag_value;
    /* data access info */
    RK_S32          data_offset;
    RK_S32          data_size;
    RK_U8           *name;
} MppCfgInfoNode;

MPP_RET mpp_cfg_set_s32(MppCfgInfoNode *info, void *cfg, RK_S32 val);
MPP_RET mpp_cfg_get_s32(MppCfgInfoNode *info, void *cfg, RK_S32 *val);
MPP_RET mpp_cfg_set_u32(MppCfgInfoNode *info, void *cfg, RK_U32 val);
MPP_RET mpp_cfg_get_u32(MppCfgInfoNode *info, void *cfg, RK_U32 *val);
MPP_RET mpp_cfg_set_s64(MppCfgInfoNode *info, void *cfg, RK_S64 val);
MPP_RET mpp_cfg_get_s64(MppCfgInfoNode *info, void *cfg, RK_S64 *val);
MPP_RET mpp_cfg_set_u64(MppCfgInfoNode *info, void *cfg, RK_U64 val);
MPP_RET mpp_cfg_get_u64(MppCfgInfoNode *info, void *cfg, RK_U64 *val);
MPP_RET mpp_cfg_set_st(MppCfgInfoNode *info, void *cfg, void *val);
MPP_RET mpp_cfg_get_st(MppCfgInfoNode *info, void *cfg, void *val);
MPP_RET mpp_cfg_set_ptr(MppCfgInfoNode *info, void *cfg, void *val);
MPP_RET mpp_cfg_get_ptr(MppCfgInfoNode *info, void *cfg, void **val);

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

extern const char *cfg_type_names[];

#define CHECK_CFG_INFO(node, name, type) \
    check_cfg_info(node, name, type, __FUNCTION__)

MPP_RET check_cfg_info(MppCfgInfoNode *node, const char *name, CfgType type,
                       const char *func);

#ifdef  __cplusplus
}
#endif

#endif /*__MPP_CFG_H__*/
