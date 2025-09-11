/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_meta"

#include <string.h>
#include <endian.h>

#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_lock.h"
#include "mpp_debug.h"
#include "mpp_thread.h"
#include "mpp_singleton.h"

#include "kmpp_obj.h"
#include "kmpp_meta_impl.h"

#define KMETA_DBG_FUNC              (0x00000001)
#define KMETA_DBG_SIZE              (0x00000002)
#define KMETA_DBG_SET               (0x00000004)
#define KMETA_DBG_GET               (0x00000008)

#define kmeta_dbg(flag, fmt, ...)   _mpp_dbg_f(kmpp_meta_debug, flag, fmt, ## __VA_ARGS__)

#define kmeta_dbg_func(fmt, ...)    kmeta_dbg(KMETA_DBG_FUNC, fmt, ## __VA_ARGS__)
#define kmeta_dbg_size(fmt, ...)    kmeta_dbg(KMETA_DBG_SIZE, fmt, ## __VA_ARGS__)
#define kmeta_dbg_set(fmt, ...)     kmeta_dbg(KMETA_DBG_SET, fmt, ## __VA_ARGS__)
#define kmeta_dbg_get(fmt, ...)     kmeta_dbg(KMETA_DBG_GET, fmt, ## __VA_ARGS__)

#define META_ON_OPS                 (0x00010000)
#define META_VAL_INVALID            (0x00000000)
#define META_VAL_VALID              (0x00000001)
#define META_VAL_READY              (0x00000002)
#define META_READY_MASK             (META_VAL_VALID | META_VAL_READY)
/* property mask */
#define META_VAL_IS_OBJ             (0x00000010)
#define META_VAL_IS_SHM             (0x00000020)
#define META_PROP_MASK              (META_VAL_IS_OBJ | META_VAL_IS_SHM)
#define META_UNMASK_PROP(x)         MPP_FETCH_AND(x, (~META_PROP_MASK))

#define META_KEY_TO_U64(key, type)  ((rk_u64)((rk_u32)htobe32(key)) | ((rk_u64)type << 32))

typedef enum KmppMetaDataType_e {
    /* kmpp meta data of normal data type */
    TYPE_VAL_32         = '3',
    TYPE_VAL_64         = '6',
    TYPE_KPTR           = 'k',  /* kernel pointer */
    TYPE_UPTR           = 'u',  /* userspace pointer */
    TYPE_SPTR           = 's',  /* share memory pointer */
} KmppMetaType;

typedef struct KmppMetaSrv_s {
    pthread_mutex_t     lock;
    struct list_head    list;
    KmppObjDef          def;

    rk_s32              offset_size;
    rk_u32              meta_id;
    rk_s32              meta_count;
} KmppMetaSrv;

typedef struct KmppMetaPriv_s {
    struct list_head    list;

    KmppObj             meta;
    rk_u32              meta_id;
} KmppMetaPriv;

static KmppMetaSrv *srv_meta = NULL;
static rk_u32 kmpp_meta_debug = 0;

#define get_meta_srv(caller) \
    ({ \
        KmppMetaSrv *__tmp; \
        if (srv_meta) { \
            __tmp = srv_meta; \
        } else { \
            mpp_loge_f("kmpp meta srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

static rk_s32 kmpp_meta_impl_init(void *entry, KmppObj obj, const char *caller)
{
    KmppMetaPriv *priv = (KmppMetaPriv *)kmpp_obj_to_priv(obj);
    KmppMetaSrv *srv = get_meta_srv(caller);
    (void)entry;

    if (srv) {
        priv->meta = obj;
        INIT_LIST_HEAD(&priv->list);

        pthread_mutex_lock(&srv->lock);
        list_add_tail(&priv->list, &srv->list);
        priv->meta_id = srv->meta_id++;
        srv->meta_count++;
        pthread_mutex_unlock(&srv->lock);
    }

    return rk_ok;
}

static rk_s32 kmpp_meta_impl_deinit(void *entry, KmppObj obj, const char *caller)
{
    KmppMetaPriv *priv = (KmppMetaPriv *)kmpp_obj_to_priv(obj);
    KmppMetaSrv *srv = get_meta_srv(caller);
    (void)entry;

    if (srv) {
        pthread_mutex_lock(&srv->lock);
        list_del_init(&priv->list);
        srv->meta_count--;
        pthread_mutex_unlock(&srv->lock);
    }

    return rk_ok;
}

static void kmpp_meta_deinit(void)
{
    KmppMetaSrv *srv = srv_meta;

    if (!srv) {
        kmeta_dbg_func("kmpp meta already deinit\n");
        return;
    }

    if (srv->def) {
        kmpp_objdef_put(srv->def);
        srv->def = NULL;
    }

    pthread_mutex_destroy(&srv->lock);

    MPP_FREE(srv);
    srv_meta = NULL;
}

static void kmpp_meta_init(void)
{
    KmppMetaSrv *srv = srv_meta;
    pthread_mutexattr_t attr;

    if (srv) {
        kmeta_dbg_func("kmpp meta %p already init\n", srv);
        kmpp_meta_deinit();
    }

    srv = mpp_calloc(KmppMetaSrv, 1);
    if (!srv) {
        mpp_loge_f("kmpp meta malloc failed\n");
        return;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&srv->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    INIT_LIST_HEAD(&srv->list);
    kmpp_objdef_get(&srv->def, sizeof(KmppMetaPriv), "KmppMeta");
    if (!srv->def) {
        kmeta_dbg_func("kmpp meta get objdef failed\n");
        MPP_FREE(srv);
        return;
    }

    kmpp_objdef_add_init(srv->def, kmpp_meta_impl_init);
    kmpp_objdef_add_deinit(srv->def, kmpp_meta_impl_deinit);

    {
        KmppEntry *tbl = NULL;

        kmpp_objdef_get_entry(srv->def, "size", &tbl);
        srv->offset_size = tbl ? tbl->tbl.elem_offset : 0;
    }

    srv_meta = srv;
}

MPP_SINGLETON(MPP_SGLN_KMPP_META, kmpp_meta, kmpp_meta_init, kmpp_meta_deinit);

static void *meta_key_to_addr(KmppObj meta, KmppMetaKey key, KmppMetaType type)
{
    if (meta) {
        KmppMetaSrv *srv = srv_meta;
        rk_u64 val = META_KEY_TO_U64(key, type);
        KmppEntry *tbl = NULL;

        kmpp_objdef_get_entry(srv->def, (const char *)&val, &tbl);
        if (tbl)
            return ((rk_u8 *)kmpp_obj_to_entry(meta)) + tbl->tbl.elem_offset;
    }

    return NULL;
}

static rk_s32 meta_inc_size(KmppObj meta, rk_s32 val, const char *caller)
{
    rk_s32 ret = 0;

    if (meta && srv_meta) {
        void *entry = kmpp_obj_to_entry(meta);
        rk_s32 offset = srv_meta->offset_size;

        if (entry && offset) {
            rk_s32 *p = (rk_s32 *)((rk_u8 *)entry + offset);

            ret = MPP_FETCH_ADD(p, val);
            kmeta_dbg_size("meta %p size %d -> %d at %s\n",
                           meta, p[0], ret, caller);
        }
    }

    return ret;
}

static rk_s32 meta_dec_size(KmppObj meta, rk_s32 val, const char *caller)
{
    rk_s32 ret = 0;

    if (meta && srv_meta) {
        void *entry = kmpp_obj_to_entry(meta);
        rk_s32 offset = srv_meta->offset_size;

        if (entry && offset) {
            rk_s32 *p = (rk_s32 *)((rk_u8 *)entry + offset);

            ret = MPP_FETCH_SUB(p, val);
            kmeta_dbg_size("meta %p size %d -> %d at %s\n",
                           meta, p[0], ret, caller);
        }
    }

    return ret;
}

rk_s32 kmpp_meta_get(KmppMeta *meta, const char *caller)
{
    KmppMetaSrv *srv = get_meta_srv(caller);

    if (!srv)
        return rk_nok;

    return kmpp_obj_get(meta, srv->def, caller);
}

rk_s32 kmpp_meta_put(KmppMeta meta, const char *caller)
{
    KmppMetaSrv *srv = get_meta_srv(caller);

    if (!srv)
        return rk_nok;

    return kmpp_obj_put(meta, caller);
}

rk_s32 kmpp_meta_size(KmppMeta meta, const char *caller)
{
    return meta_inc_size(meta, 0, caller);
}

rk_s32 kmpp_meta_dump(KmppMeta meta, const char *caller)
{
    return kmpp_obj_udump_f(meta, caller);
}

rk_s32 kmpp_meta_dump_all(const char *caller)
{
    KmppMetaSrv *srv = get_meta_srv(caller);

    if (srv) {
        KmppMeta meta = NULL;
        KmppMetaPriv *pos, *n;

        pthread_mutex_lock(&srv->lock);
        list_for_each_entry_safe(pos, n, &srv->list, KmppMetaPriv, list) {
            meta = pos->meta;
            mpp_logi("meta %p:%d size %d\n", meta, pos->meta_id,
                     kmpp_meta_size(meta, caller));
            kmpp_meta_dump(meta, caller);
        }
    }

    return rk_ok;
}

#define KMPP_META_ACCESSOR(func_type, arg_type, key_type, key_field)  \
    rk_s32 kmpp_meta_set_##func_type(KmppMeta meta, KmppMetaKey key, arg_type val) \
    { \
        KmppMetaVal *meta_val = meta_key_to_addr(meta, key, key_type); \
        if (!meta_val) \
            return rk_nok; \
        if (MPP_BOOL_CAS(&meta_val->state, META_VAL_INVALID, META_VAL_VALID)) \
            meta_inc_size(meta, 1, __FUNCTION__); \
        meta_val->key_field = val; \
        MPP_FETCH_OR(&meta_val->state, META_VAL_READY); \
        return rk_ok; \
    } \
    rk_s32 kmpp_meta_get_##func_type(KmppMeta meta, KmppMetaKey key, arg_type *val) \
    { \
        KmppMetaVal *meta_val = meta_key_to_addr(meta, key, key_type); \
        if (!meta_val) \
            return rk_nok; \
        if (MPP_BOOL_CAS(&meta_val->state, META_READY_MASK, META_VAL_INVALID)) { \
            if (val) *val = meta_val->key_field; \
            meta_dec_size(meta, 1, __FUNCTION__); \
            return rk_ok; \
        } \
        return rk_nok; \
    } \
    rk_s32 kmpp_meta_get_##func_type##_d(KmppMeta meta, KmppMetaKey key, arg_type *val, arg_type def) \
    { \
        KmppMetaVal *meta_val = meta_key_to_addr(meta, key, key_type); \
        if (!meta_val) \
            return rk_nok; \
        if (MPP_BOOL_CAS(&meta_val->state, META_READY_MASK, META_VAL_INVALID)) { \
            if (val) *val = meta_val->key_field; \
            meta_dec_size(meta, 1, __FUNCTION__); \
        } else { \
            if (val) *val = def; \
        } \
        return rk_ok; \
    }

KMPP_META_ACCESSOR(s32, rk_s32, TYPE_VAL_32, val_s32)
KMPP_META_ACCESSOR(s64, rk_s64, TYPE_VAL_64, val_s64)
KMPP_META_ACCESSOR(ptr, void *, TYPE_UPTR, val_ptr)

rk_s32 kmpp_meta_set_obj(KmppMeta meta, KmppMetaKey key, KmppObj val)
{
    KmppMetaObj *meta_obj = meta_key_to_addr(meta, key, TYPE_SPTR);

    if (!meta_obj)
        return rk_nok;

    if (MPP_BOOL_CAS(&meta_obj->state, META_VAL_INVALID, META_VAL_VALID))
        meta_inc_size(meta, 1, __FUNCTION__);

    {
        KmppShmPtr *ptr = kmpp_obj_to_shm(val);

        if (ptr) {
            meta_obj->val_shm.uaddr = ptr->uaddr;
            meta_obj->val_shm.kaddr = ptr->kaddr;;
            MPP_FETCH_OR(&meta_obj->state, META_VAL_IS_SHM);
        } else {
            meta_obj->val_shm.uaddr = 0;
            meta_obj->val_shm.kptr = val;
            MPP_FETCH_AND(&meta_obj->state, ~META_VAL_IS_SHM);
        }
    }
    MPP_FETCH_OR(&meta_obj->state, META_VAL_READY);
    return rk_ok;
}

rk_s32 kmpp_meta_get_obj(KmppMeta meta, KmppMetaKey key, KmppObj *val)
{
    KmppMetaObj *meta_obj = meta_key_to_addr(meta, key, TYPE_SPTR);

    if (!meta_obj)
        return rk_nok;

    META_UNMASK_PROP(&meta_obj->state);
    if (MPP_BOOL_CAS(&meta_obj->state, META_READY_MASK, META_VAL_INVALID)) {
        if (val)
            *val = meta_obj->val_shm.kptr;
        meta_dec_size(meta, 1, __FUNCTION__);
        return rk_ok;
    }

    return rk_nok;
}

rk_s32 kmpp_meta_get_obj_d(KmppMeta meta, KmppMetaKey key, KmppObj *val, KmppObj def)
{
    KmppMetaObj *meta_obj = meta_key_to_addr(meta, key, TYPE_SPTR);

    if (!meta_obj)
        return rk_nok;

    META_UNMASK_PROP(&meta_obj->state);
    if (MPP_BOOL_CAS(&meta_obj->state, META_READY_MASK, META_VAL_INVALID)) {
        if (val)
            *val = meta_obj->val_shm.kptr;
        meta_dec_size(meta, 1, __FUNCTION__);
    } else {
        if (val)
            *val = def ? def : NULL;
    }

    return rk_ok;
}

rk_s32 kmpp_meta_set_shm(KmppMeta meta, KmppMetaKey key, KmppShmPtr *sptr)
{
    KmppMetaObj *meta_obj = (KmppMetaObj *)meta_key_to_addr(meta, key, TYPE_SPTR);

    if (!meta_obj)
        return rk_nok;

    if (MPP_BOOL_CAS(&meta_obj->state, META_VAL_INVALID, META_VAL_VALID))
        meta_inc_size(meta, 1, __FUNCTION__);

    if (sptr) {
        meta_obj->val_shm.uaddr = sptr->uaddr;
        meta_obj->val_shm.kaddr = sptr->kaddr;
    } else {
        meta_obj->val_shm.uaddr = 0;
        meta_obj->val_shm.kptr = 0;
    }

    if (sptr && sptr->uaddr)
        MPP_FETCH_OR(&meta_obj->state, META_VAL_IS_SHM);
    else
        MPP_FETCH_AND(&meta_obj->state, ~META_VAL_IS_SHM);

    MPP_FETCH_OR(&meta_obj->state, META_VAL_READY);

    return rk_ok;
}

rk_s32 kmpp_meta_get_shm(KmppMeta meta, KmppMetaKey key, KmppShmPtr *sptr)
{
    KmppMetaObj *meta_obj = meta_key_to_addr(meta, key, TYPE_SPTR);

    if (!meta_obj)
        return rk_nok;

    META_UNMASK_PROP(&meta_obj->state);
    if (MPP_BOOL_CAS(&meta_obj->state, META_READY_MASK, META_VAL_INVALID)) {
        if (sptr) {
            sptr->uaddr = meta_obj->val_shm.uaddr;
            sptr->kaddr = meta_obj->val_shm.kaddr;
        }
        meta_dec_size(meta, 1, __FUNCTION__);
        return rk_ok;
    }
    return rk_nok;
}

rk_s32 kmpp_meta_get_shm_d(KmppMeta meta, KmppMetaKey key, KmppShmPtr *sptr, KmppShmPtr *def)
{
    KmppMetaObj *meta_obj = meta_key_to_addr(meta, key, TYPE_SPTR);

    if (!meta_obj)
        return rk_nok;

    META_UNMASK_PROP(&meta_obj->state);
    if (MPP_BOOL_CAS(&meta_obj->state, META_READY_MASK, META_VAL_INVALID)) {
        if (sptr) {
            sptr->uaddr = meta_obj->val_shm.uaddr;
            sptr->kaddr = meta_obj->val_shm.kaddr;
        }
        meta_dec_size(meta, 1, __FUNCTION__);
    } else {
        if (sptr) {
            if (def) {
                sptr->uaddr = def->uaddr;
                sptr->kaddr = def->kaddr;
            } else {
                sptr->uaddr = 0;
                sptr->kaddr = 0;
            }
        }
    }

    return rk_ok;
}
