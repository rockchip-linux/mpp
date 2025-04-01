/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_venc_cfg"

#include <string.h>
#include <pthread.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "kmpp_obj.h"
#include "rk_venc_kcfg.h"

#define VENC_KCFG_DBG_FUNC              (0x00000001)
#define VENC_KCFG_DBG_INFO              (0x00000002)
#define VENC_KCFG_DBG_SET               (0x00000004)
#define VENC_KCFG_DBG_GET               (0x00000008)

#define venc_kcfg_dbg(flag, fmt, ...)   _mpp_dbg_f(venc_kcfg_debug, flag, fmt, ## __VA_ARGS__)

#define venc_kcfg_dbg_func(fmt, ...)    venc_kcfg_dbg(VENC_KCFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define venc_kcfg_dbg_info(fmt, ...)    venc_kcfg_dbg(VENC_KCFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define venc_kcfg_dbg_set(fmt, ...)     venc_kcfg_dbg(VENC_KCFG_DBG_SET, fmt, ## __VA_ARGS__)
#define venc_kcfg_dbg_get(fmt, ...)     venc_kcfg_dbg(VENC_KCFG_DBG_GET, fmt, ## __VA_ARGS__)

static RK_U32 venc_kcfg_debug = 0;

static char *kcfg_names[] = {
    [KMPP_VENC_CFG_TYPE_INIT]   = "KmppVencInitCfg",
    [KMPP_VENC_CFG_TYPE_DEINIT] = "KmppVencDeinitCfg",
    [KMPP_VENC_CFG_TYPE_RESET]  = "KmppVencResetCfg",
    [KMPP_VENC_CFG_TYPE_START]  = "KmppVencStartCfg",
    [KMPP_VENC_CFG_TYPE_STOP]   = "KmppVencStopCfg",
};
static KmppObjDef kcfg_defs[KMPP_VENC_CFG_TYPE_BUTT] = {NULL};
static pthread_mutex_t lock;

static void kmpp_venc_cfg_def_init() __attribute__((constructor));
static void kmpp_venc_cfg_def_deinit() __attribute__((destructor));

static void kmpp_venc_cfg_def_init(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

static void kmpp_venc_cfg_def_deinit(void)
{
    RK_U32 i;

    pthread_mutex_lock(&lock);
    for (i = 0; i < KMPP_VENC_CFG_TYPE_BUTT; i++) {
        if (kcfg_defs[i]) {
            kmpp_objdef_put(kcfg_defs[i]);
            kcfg_defs[i] = NULL;
        }
    }
    pthread_mutex_unlock(&lock);
    pthread_mutex_destroy(&lock);
}

MPP_RET kmpp_venc_cfg_init(KmppVenccfg *cfg, MppVencKcfgType type)
{
    KmppObj obj = NULL;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    if (type >= KMPP_VENC_CFG_TYPE_BUTT) {
        mpp_err_f("invalid config type %d\n", type);
        return MPP_ERR_VALUE;
    }

    mpp_env_get_u32("venc_kcfg_debug", &venc_kcfg_debug, 0);

    pthread_mutex_lock(&lock);
    if (kcfg_defs[type] == NULL) {
        MPP_RET ret = (MPP_RET)kmpp_objdef_get(&kcfg_defs[type], kcfg_names[type]);

        if (ret) {
            mpp_err_f("failed to get %s obj def %d\n", kcfg_names[type], type);
            pthread_mutex_unlock(&lock);
            return MPP_NOK;
        }
    }
    pthread_mutex_unlock(&lock);

    kmpp_obj_get_f(&obj, kcfg_defs[type]);

    *cfg = obj;

    return obj ? MPP_OK : MPP_NOK;
}

MPP_RET kmpp_venc_cfg_init_by_name(KmppVenccfg *cfg, const char *name)
{
    KmppObj obj = NULL;
    MppVencKcfgType type = KMPP_VENC_CFG_TYPE_BUTT;
    RK_U32 i;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    for (i = 0; i < KMPP_VENC_CFG_TYPE_BUTT; i++) {
        if (!strncmp(name, kcfg_names[i], strlen(kcfg_names[i]))) {
            type = i;
            break;
        }
    }

    if (type >= KMPP_VENC_CFG_TYPE_BUTT) {
        mpp_err_f("invalid config name %s\n", name);
        return MPP_ERR_VALUE;
    }

    mpp_env_get_u32("venc_kcfg_debug", &venc_kcfg_debug, 0);

    kmpp_obj_get_f(&obj, kcfg_defs[type]);

    *cfg = obj;

    return obj ? MPP_OK : MPP_NOK;
}

MPP_RET kmpp_venc_cfg_deinit(KmppVenccfg cfg)
{
    KmppObj obj = cfg;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    return kmpp_obj_put_f(obj);
}

#define KMPP_VENC_CFG_ACCESS(set_type, get_type, cfg_type) \
    MPP_RET kmpp_venc_cfg_set_##cfg_type(KmppVenccfg cfg, const char *name, set_type val) \
    { \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        KmppObj obj = (KmppObj)cfg; \
        MPP_RET ret = (MPP_RET)kmpp_obj_set_##cfg_type(obj, name, val); \
        return ret; \
    } \
    MPP_RET kmpp_venc_cfg_get_##cfg_type(KmppVenccfg cfg, const char *name, get_type val) \
    { \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        KmppObj obj = (KmppObj)cfg; \
        MPP_RET ret = (MPP_RET)kmpp_obj_get_##cfg_type(obj, name, val); \
        return ret; \
    }

KMPP_VENC_CFG_ACCESS(RK_S32, RK_S32*, s32);
KMPP_VENC_CFG_ACCESS(RK_U32, RK_U32*, u32);
KMPP_VENC_CFG_ACCESS(RK_S64, RK_S64*, s64);
KMPP_VENC_CFG_ACCESS(RK_U64, RK_U64*, u64);
KMPP_VENC_CFG_ACCESS(void *, void **, ptr);
KMPP_VENC_CFG_ACCESS(void *, void  *, st);

void kmpp_venc_cfg_show(KmppVenccfg cfg)
{
    KmppObj obj = cfg;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return;
    }

    kmpp_obj_udump(obj);
}
