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
    /* size for the whole node including name */
    RK_S32          node_size;
    RK_U32          name_len;
    /* CfgType */
    RK_S32          data_type;
    /* update flag info 32bit */
    RK_S32          flag_offset;
    RK_U32          flag_value;
    /* data access info */
    RK_S32          data_offset;
    RK_S32          data_size;
    /* linked next node offset for pointer type */
    RK_S32          node_next;
    /* function pointer for get / put accessor (user filled) */
    RK_U64          set_func;
    RK_U64          get_func;
    /* reserve for extension */
    RK_U64          stuff[2];
    char            name[];
} MppCfgInfoNode;

typedef MPP_RET (*CfgSetS32)(MppCfgInfoNode *info, void *cfg, RK_S32 val);
typedef MPP_RET (*CfgGetS32)(MppCfgInfoNode *info, void *cfg, RK_S32 *val);
typedef MPP_RET (*CfgSetU32)(MppCfgInfoNode *info, void *cfg, RK_U32 val);
typedef MPP_RET (*CfgGetU32)(MppCfgInfoNode *info, void *cfg, RK_U32 *val);
typedef MPP_RET (*CfgSetS64)(MppCfgInfoNode *info, void *cfg, RK_S64 val);
typedef MPP_RET (*CfgGetS64)(MppCfgInfoNode *info, void *cfg, RK_S64 *val);
typedef MPP_RET (*CfgSetU64)(MppCfgInfoNode *info, void *cfg, RK_U64 val);
typedef MPP_RET (*CfgGetU64)(MppCfgInfoNode *info, void *cfg, RK_U64 *val);
typedef MPP_RET (*CfgSetSt) (MppCfgInfoNode *info, void *cfg, void *val);
typedef MPP_RET (*CfgGetSt) (MppCfgInfoNode *info, void *cfg, void *val);
typedef MPP_RET (*CfgSetPtr)(MppCfgInfoNode *info, void *cfg, void *val);
typedef MPP_RET (*CfgGetPtr)(MppCfgInfoNode *info, void *cfg, void **val);

#define MPP_CFG_SET_S32(info, cfg, val) (mpp_cfg_ops.fp_SetS32)(info, cfg, val)
#define MPP_CFG_GET_S32(info, cfg, val) (mpp_cfg_ops.fp_GetS32)(info, cfg, (RK_S32 *)(val))
#define MPP_CFG_SET_U32(info, cfg, val) (mpp_cfg_ops.fp_SetU32)(info, cfg, val)
#define MPP_CFG_GET_U32(info, cfg, val) (mpp_cfg_ops.fp_GetU32)(info, cfg, (RK_U32 *)(val))
#define MPP_CFG_SET_S64(info, cfg, val) (mpp_cfg_ops.fp_SetS64)(info, cfg, val)
#define MPP_CFG_GET_S64(info, cfg, val) (mpp_cfg_ops.fp_GetS64)(info, cfg, (RK_S64 *)(val))
#define MPP_CFG_SET_U64(info, cfg, val) (mpp_cfg_ops.fp_SetU64)(info, cfg, val)
#define MPP_CFG_GET_U64(info, cfg, val) (mpp_cfg_ops.fp_GetU64)(info, cfg, (RK_U64 *)(val))
#define MPP_CFG_SET_St(info, cfg, val)  (mpp_cfg_ops.fp_SetSt )(info, cfg, val)
#define MPP_CFG_GET_St(info, cfg, val)  (mpp_cfg_ops.fp_GetSt )(info, cfg, (void *)(val))
#define MPP_CFG_SET_Ptr(info, cfg, val) (mpp_cfg_ops.fp_SetPtr)(info, cfg, val)
#define MPP_CFG_GET_Ptr(info, cfg, val) (mpp_cfg_ops.fp_GetPtr)(info, cfg, (void **)(val))

/* header size 128 byte */
typedef struct MppCfgInfoHead_t {
    char            version[112];
    RK_S32          info_size;
    RK_S32          info_count;
    RK_S32          node_count;
    RK_S32          cfg_size;
} MppCfgInfoHead;

typedef struct MppCfgOps_t {
    CfgSetS32       fp_SetS32;
    CfgGetS32       fp_GetS32;
    CfgSetU32       fp_SetU32;
    CfgGetU32       fp_GetU32;
    CfgSetS64       fp_SetS64;
    CfgGetS64       fp_GetS64;
    CfgSetU64       fp_SetU64;
    CfgGetU64       fp_GetU64;
    CfgSetSt        fp_SetSt;
    CfgGetSt        fp_GetSt;
    CfgSetPtr       fp_SetPtr;
    CfgGetPtr       fp_GetPtr;
} MppCfgOps;

#ifdef  __cplusplus
extern "C" {
#endif

extern const char *cfg_type_names[];

extern MppCfgOps mpp_cfg_ops;
MPP_RET mpp_cfg_node_fixup_func(MppCfgInfoNode *node);

#define CHECK_CFG_INFO(node, name, type) \
    check_cfg_info(node, name, type, __FUNCTION__)

MPP_RET check_cfg_info(MppCfgInfoNode *node, const char *name, CfgType type,
                       const char *func);

#ifdef  __cplusplus
}
#endif

#endif /*__MPP_CFG_H__*/
