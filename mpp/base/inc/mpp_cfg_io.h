/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_CFG_IO__
#define __MPP_CFG_IO__

#include "mpp_internal.h"

typedef enum MppCfgType_e {
    MPP_CFG_TYPE_INVALID = 0,

    /* invalid or empty value type */
    MPP_CFG_TYPE_NULL,

    /* leaf type must with name */
    MPP_CFG_TYPE_BOOL,
    MPP_CFG_TYPE_S32,
    MPP_CFG_TYPE_U32,
    MPP_CFG_TYPE_S64,
    MPP_CFG_TYPE_U64,
    MPP_CFG_TYPE_F32,
    MPP_CFG_TYPE_F64,
    MPP_CFG_TYPE_STRING,
    MPP_CFG_TYPE_RAW,

    /* branch type */
    MPP_CFG_TYPE_OBJECT,
    MPP_CFG_TYPE_ARRAY,

    MPP_CFG_TYPE_BUTT,
} MppCfgType;

typedef enum MppCfgStrFmt_e {
    MPP_CFG_STR_FMT_LOG,
    MPP_CFG_STR_FMT_JSON,
    MPP_CFG_STR_FMT_TOML,
    MPP_CFG_STR_FMT_BUTT,
} MppCfgStrFmt;

typedef union MppCfgVal_u {
    rk_bool     b1;
    rk_s32      s32;
    rk_u32      u32;
    rk_s64      s64;
    rk_u64      u64;
    rk_float    f32;
    rk_double   f64;
    void        *str;
    void        *ptr;
} MppCfgVal;

typedef void* MppCfgObj;

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_cfg_get_object(MppCfgObj *obj, const char *name, MppCfgType type, MppCfgVal *val);
rk_s32 mpp_cfg_get_array(MppCfgObj *obj, const char *name, rk_s32 count);
rk_s32 mpp_cfg_put(MppCfgObj obj);
rk_s32 mpp_cfg_put_all(MppCfgObj obj);

/* object tree build */
rk_s32 mpp_cfg_add(MppCfgObj root, MppCfgObj leaf);
/* object tree release */
rk_s32 mpp_cfg_del(MppCfgObj obj);

/* attach MppCfgInfo for access location */
rk_s32 mpp_cfg_set_info(MppCfgObj obj, MppCfgInfo *info);

void mpp_cfg_dump(MppCfgObj obj, const char *func);
#define mpp_cfg_dump_f(obj) mpp_cfg_dump(obj, __FUNCTION__)

/* mark all MppCfgObject ready and build trie for string access */
MppTrie mpp_cfg_to_trie(MppCfgObj obj);

/* mpp_cfg output to string and input from string */
rk_s32 mpp_cfg_to_string(MppCfgObj obj, MppCfgStrFmt fmt, char **buf);
rk_s32 mpp_cfg_from_string(MppCfgObj *obj, MppCfgStrFmt fmt, const char *buf);

/*
 * obj  - read from file or string and get an object as source
 * type - struct type object root for location table indexing and access
 * st   - struct body to write obj values to
 */
rk_s32 mpp_cfg_to_struct(MppCfgObj obj, MppCfgObj type, void *st);
/*
 * obj  - output object root for the struct values
 * type - struct type object root for location table access
 * st   - struct body to write obj values
 */
rk_s32 mpp_cfg_from_struct(MppCfgObj *obj, MppCfgObj type, void *st);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_CFG_IO__ */
