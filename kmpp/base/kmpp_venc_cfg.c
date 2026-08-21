/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_venc_cfg"

#include <string.h>

#include "mpp_env.h"
#include "mpp_debug.h"
#include "mpp_singleton.h"

#include "kmpp_obj.h"
#include "rk_venc_kcfg.h"

#define VENC_KCFG_DBG_FUNC              (0x00000001)
#define VENC_KCFG_DBG_INFO              (0x00000002)
#define VENC_KCFG_DBG_SET               (0x00000004)
#define VENC_KCFG_DBG_GET               (0x00000008)

#define venc_kcfg_dbg(flag, fmt, ...)   mpp_dbg_f(venc_kcfg_debug, flag, fmt, ## __VA_ARGS__)

#define venc_kcfg_dbg_func(fmt, ...)    venc_kcfg_dbg(VENC_KCFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define venc_kcfg_dbg_info(fmt, ...)    venc_kcfg_dbg(VENC_KCFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define venc_kcfg_dbg_set(fmt, ...)     venc_kcfg_dbg(VENC_KCFG_DBG_SET, fmt, ## __VA_ARGS__)
#define venc_kcfg_dbg_get(fmt, ...)     venc_kcfg_dbg(VENC_KCFG_DBG_GET, fmt, ## __VA_ARGS__)

typedef struct KmppVencKcfgInfo_t  {
    const char      *name;
    KmppObjInit     cache_init;
    KmppObjInit     cache_deinit;
    rk_u32          flags;
} KmppVencKcfgInfo;

static RK_U32 venc_kcfg_debug = 0;
static KmppObjDef kcfg_defs[MPP_VENC_KCFG_TYPE_BUTT] = {NULL};
static rk_s32 ref_cfg_check_cmd = -1;

static rk_s32 kcfg_ctrl_cache_init(void *entry, KmppObj obj, const char *caller);

KmppVencKcfgInfo kcfg_info[MPP_VENC_KCFG_TYPE_BUTT] = {
    [MPP_VENC_KCFG_TYPE_INIT]       = { "KmppVencInitCfg",   NULL, NULL, KMPP_OBJDEF_HIERARCHY },
    [MPP_VENC_KCFG_TYPE_DEINIT]     = { "KmppVencDeinitCfg", NULL, NULL, 0 },
    [MPP_VENC_KCFG_TYPE_RESET]      = { "KmppVencResetCfg",  NULL, NULL, 0 },
    [MPP_VENC_KCFG_TYPE_START]      = { "KmppVencStartCfg",  NULL, NULL, 0 },
    [MPP_VENC_KCFG_TYPE_STOP]       = { "KmppVencStopCfg",   NULL, NULL, 0 },
    [MPP_VENC_KCFG_TYPE_ST_CFG]     = { "KmppVencStCfg",     NULL, NULL, KMPP_OBJDEF_HIERARCHY },
    [MPP_VENC_KCFG_TYPE_REF_CFG]    = { "KmppVencRefCfg",    NULL, NULL, KMPP_OBJDEF_HIERARCHY },
    [MPP_VENC_KCFG_TYPE_CTRL_CFG]   = {
        "KmppVencCtrlCfg",   kcfg_ctrl_cache_init, NULL, KMPP_OBJDEF_HIERARCHY | KMPP_OBJDEF_CACHED
    },
};

static rk_s32 kcfg_ctrl_cache_init(void *entry, KmppObj obj, const char *caller)
{
    KmppShmPtr zero = { .uaddr = 0, .kaddr = 0 };
    (void)entry;
    (void)caller;

    kmpp_obj_set_s32(obj, "cmd", 0);
    kmpp_obj_set_s64(obj, "val", 0);
    kmpp_obj_set_shm(obj, "arg", &zero);
    kmpp_obj_set_shm(obj, "ret", &zero);

    return rk_ok;
}

static void mpp_venc_kcfg_def_init(void)
{
    RK_U32 i;

    for (i = 0; i < MPP_VENC_KCFG_TYPE_BUTT; i++) {
        KmppVencKcfgInfo *info = &kcfg_info[i];
        KmppObjDef def = NULL;

        kmpp_objdef_get(&def, 0, info->name, info->flags);
        if (def && (info->flags & KMPP_OBJDEF_CACHED)) {
            if (info->cache_init)
                kmpp_objdef_add_cache_init(def, info->cache_init);
            if (info->cache_deinit)
                kmpp_objdef_add_cache_deinit(def, info->cache_deinit);
        }

        kcfg_defs[i] = def;
    }

    if (kcfg_defs[MPP_VENC_KCFG_TYPE_REF_CFG]) {
        KmppObjDef def = kcfg_defs[MPP_VENC_KCFG_TYPE_REF_CFG];

        ref_cfg_check_cmd = kmpp_objdef_get_cmd(def, "check");
    }
}

static void mpp_venc_kcfg_def_deinit(void)
{
    RK_U32 i;

    for (i = 0; i < MPP_VENC_KCFG_TYPE_BUTT; i++) {
        if (kcfg_defs[i]) {
            kmpp_objdef_put(kcfg_defs[i]);
            kcfg_defs[i] = NULL;
        }
    }
}

MPP_SINGLETON(MPP_SGLN_KMPP_VENC_CFG, kmpp_venc_cfg, mpp_venc_kcfg_def_init, mpp_venc_kcfg_def_deinit)

MPP_RET mpp_venc_kcfg_init(MppVencKcfg *cfg, MppVencKcfgType type)
{
    KmppObj obj = NULL;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    if (type >= MPP_VENC_KCFG_TYPE_BUTT) {
        mpp_err_f("invalid config type %d\n", type);
        return MPP_ERR_VALUE;
    }

    mpp_env_get_u32("venc_kcfg_debug", &venc_kcfg_debug, 0);

    if (kcfg_defs[type])
        kmpp_obj_get_f(&obj, kcfg_defs[type]);

    *cfg = obj;

    return obj ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_venc_kcfg_init_by_name(MppVencKcfg *cfg, const char *name)
{
    KmppObj obj = NULL;
    MppVencKcfgType type = MPP_VENC_KCFG_TYPE_BUTT;
    RK_U32 i;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    for (i = 0; i < MPP_VENC_KCFG_TYPE_BUTT; i++) {
        if (!strncmp(name, kcfg_info[i].name, strlen(kcfg_info[i].name))) {
            type = i;
            break;
        }
    }

    if (type >= MPP_VENC_KCFG_TYPE_BUTT) {
        mpp_err_f("invalid config name %s\n", name);
        return MPP_ERR_VALUE;
    }

    mpp_env_get_u32("venc_kcfg_debug", &venc_kcfg_debug, 0);

    kmpp_obj_get_f(&obj, kcfg_defs[type]);

    *cfg = obj;

    return obj ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_venc_kcfg_deinit(MppVencKcfg cfg)
{
    KmppObj obj = cfg;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    return kmpp_obj_put_f(obj);
}

#define MPP_VENC_KCFG_ACCESS(set_type, get_type, cfg_type) \
    MPP_RET mpp_venc_kcfg_set_##cfg_type(MppVencKcfg cfg, const char *name, set_type val) \
    { \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        KmppObj obj = (KmppObj)cfg; \
        MPP_RET ret = (MPP_RET)kmpp_obj_set_##cfg_type(obj, name, val); \
        return ret; \
    } \
    MPP_RET mpp_venc_kcfg_get_##cfg_type(MppVencKcfg cfg, const char *name, get_type val) \
    { \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        KmppObj obj = (KmppObj)cfg; \
        MPP_RET ret = (MPP_RET)kmpp_obj_get_##cfg_type(obj, name, val); \
        return ret; \
    }

MPP_VENC_KCFG_ACCESS(RK_S8,  RK_S8*,  s8);
MPP_VENC_KCFG_ACCESS(RK_U8,  RK_U8*,  u8);
MPP_VENC_KCFG_ACCESS(RK_S16, RK_S16*, s16);
MPP_VENC_KCFG_ACCESS(RK_U16, RK_U16*, u16);
MPP_VENC_KCFG_ACCESS(RK_S32, RK_S32*, s32);
MPP_VENC_KCFG_ACCESS(RK_U32, RK_U32*, u32);
MPP_VENC_KCFG_ACCESS(RK_S64, RK_S64*, s64);
MPP_VENC_KCFG_ACCESS(RK_U64, RK_U64*, u64);
MPP_VENC_KCFG_ACCESS(void *, void **, ptr);
MPP_VENC_KCFG_ACCESS(void *, void  *, st);

void mpp_venc_kcfg_show(MppVencKcfg cfg)
{
    KmppObj obj = cfg;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return;
    }

    kmpp_obj_udump(obj);
}

void *mpp_venc_ctrl_flex_base(MppVencKcfg ctrl)
{
    KmppObjDef def = kcfg_defs[MPP_VENC_KCFG_TYPE_CTRL_CFG];

    if (!def) {
        mpp_loge_f("can not found KmppVencCtrlCfg objdef\n");
        return NULL;
    }

    return (rk_u8 *)kmpp_obj_to_entry((KmppObj)ctrl) +
           kmpp_objdef_get_entry_size(def) +
           kmpp_obj_to_flags_size((KmppObj)ctrl);
}

MPP_RET mpp_venc_ctrl_set_flex(MppVencKcfg ctrl, const void *data, RK_U32 size)
{
    void *base;
    rk_s32 ret;

    mpp_venc_kcfg_set_u32(ctrl, "flags", KMPP_CTRL_FLAG_FLEX);
    mpp_venc_kcfg_set_u32(ctrl, "size", size);

    ret = kmpp_obj_resize_f((KmppObj)ctrl, size);
    if (ret) {
        mpp_err("ctrl_set_flex resize to %u failed ret %d\n", size, ret);
        return MPP_NOK;
    }

    base = mpp_venc_ctrl_flex_base(ctrl);
    if (base) {
        memcpy(base, data, size);
        return MPP_OK;
    }

    return MPP_NOK;
}

MPP_RET kmpp_venc_ref_cfg_check(MppVencKcfg ref)
{
    if (ref_cfg_check_cmd < 0 || !ref)
        return rk_nok;

    return kmpp_obj_ioctl_f((KmppObj)ref, ref_cfg_check_cmd, NULL, NULL);
}
