/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_vdec_cfg"

#include <string.h>
#include <pthread.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "kmpp_obj.h"
#include "rk_vdec_kcfg.h"

#define VDEC_KCFG_DBG_FUNC              (0x00000001)
#define VDEC_KCFG_DBG_INFO              (0x00000002)
#define VDEC_KCFG_DBG_SET               (0x00000004)
#define VDEC_KCFG_DBG_GET               (0x00000008)

#define vdec_kcfg_dbg(flag, fmt, ...)   mpp_dbg_f(vdec_kcfg_debug, flag, fmt, ## __VA_ARGS__)

#define vdec_kcfg_dbg_func(fmt, ...)    vdec_kcfg_dbg(VDEC_KCFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define vdec_kcfg_dbg_info(fmt, ...)    vdec_kcfg_dbg(VDEC_KCFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define vdec_kcfg_dbg_set(fmt, ...)     vdec_kcfg_dbg(VDEC_KCFG_DBG_SET, fmt, ## __VA_ARGS__)
#define vdec_kcfg_dbg_get(fmt, ...)     vdec_kcfg_dbg(VDEC_KCFG_DBG_GET, fmt, ## __VA_ARGS__)

static RK_U32 vdec_kcfg_debug = 0;

static char *kcfg_names[] = {
    [MPP_VDEC_KCFG_TYPE_INIT]   = "KmppVdecInitCfg",
    [MPP_VDEC_KCFG_TYPE_DEINIT] = "KmppVdecDeinitCfg",
    [MPP_VDEC_KCFG_TYPE_RESET]  = "KmppVdecResetCfg",
    [MPP_VDEC_KCFG_TYPE_START]  = "KmppVdecStartCfg",
    [MPP_VDEC_KCFG_TYPE_STOP]   = "KmppVdecStopCfg",
};
static KmppObjDef kcfg_defs[MPP_VDEC_KCFG_TYPE_BUTT] = {NULL};

static void mpp_vdec_kcfg_def_init(void)
{
    RK_U32 i;

    for (i = 0; i < MPP_VDEC_KCFG_TYPE_BUTT; i++) {
        kmpp_objdef_get(&kcfg_defs[i], 0, kcfg_names[i]);
    }
}

static void mpp_vdec_kcfg_def_deinit(void)
{
    RK_U32 i;

    for (i = 0; i < MPP_VDEC_KCFG_TYPE_BUTT; i++) {
        if (kcfg_defs[i]) {
            kmpp_objdef_put(kcfg_defs[i]);
            kcfg_defs[i] = NULL;
        }
    }
}

MPP_SINGLETON(MPP_SGLN_KMPP_VDEC_CFG, kmpp_vdec_cfg, mpp_vdec_kcfg_def_init, mpp_vdec_kcfg_def_deinit)

MPP_RET mpp_vdec_kcfg_init(MppVdecKcfg *cfg, MppVdecKcfgType type)
{
    KmppObj obj = NULL;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    if (type >= MPP_VDEC_KCFG_TYPE_BUTT) {
        mpp_err_f("invalid config type %d\n", type);
        return MPP_ERR_VALUE;
    }

    mpp_env_get_u32("vdec_kcfg_debug", &vdec_kcfg_debug, 0);

    if (kcfg_defs[type])
        kmpp_obj_get_f(&obj, kcfg_defs[type]);

    *cfg = obj;

    return obj ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_vdec_kcfg_init_by_name(MppVdecKcfg *cfg, const char *name)
{
    KmppObj obj = NULL;
    MppVdecKcfgType type = MPP_VDEC_KCFG_TYPE_BUTT;
    RK_U32 i;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    for (i = 0; i < MPP_VDEC_KCFG_TYPE_BUTT; i++) {
        if (!strncmp(name, kcfg_names[i], strlen(kcfg_names[i]))) {
            type = i;
            break;
        }
    }

    if (type >= MPP_VDEC_KCFG_TYPE_BUTT) {
        mpp_err_f("invalid config name %s\n", name);
        return MPP_ERR_VALUE;
    }

    mpp_env_get_u32("vdec_kcfg_debug", &vdec_kcfg_debug, 0);

    kmpp_obj_get_f(&obj, kcfg_defs[type]);

    *cfg = obj;

    return obj ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_vdec_kcfg_deinit(MppVdecKcfg cfg)
{
    KmppObj obj = cfg;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    return kmpp_obj_put_f(obj);
}

#define MPP_VDEC_KCFG_ACCESS(set_type, get_type, cfg_type) \
    MPP_RET mpp_vdec_kcfg_set_##cfg_type(MppVdecKcfg cfg, const char *name, set_type val) \
    { \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        KmppObj obj = (KmppObj)cfg; \
        MPP_RET ret = (MPP_RET)kmpp_obj_set_##cfg_type(obj, name, val); \
        return ret; \
    } \
    MPP_RET mpp_vdec_kcfg_get_##cfg_type(MppVdecKcfg cfg, const char *name, get_type val) \
    { \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        KmppObj obj = (KmppObj)cfg; \
        MPP_RET ret = (MPP_RET)kmpp_obj_get_##cfg_type(obj, name, val); \
        return ret; \
    }

MPP_VDEC_KCFG_ACCESS(RK_S32, RK_S32*, s32);
MPP_VDEC_KCFG_ACCESS(RK_U32, RK_U32*, u32);
MPP_VDEC_KCFG_ACCESS(RK_S64, RK_S64*, s64);
MPP_VDEC_KCFG_ACCESS(RK_U64, RK_U64*, u64);
MPP_VDEC_KCFG_ACCESS(void *, void **, ptr);
MPP_VDEC_KCFG_ACCESS(void *, void  *, st);

void mpp_vdec_kcfg_show(MppVdecKcfg cfg)
{
    KmppObj obj = cfg;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return;
    }

    kmpp_obj_udump(obj);
}
