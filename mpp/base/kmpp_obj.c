/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp_obj"

#include <string.h>
#include <sys/ioctl.h>

#include "mpp_list.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_trie.h"
#include "kmpp_obj_impl.h"

#define KMPP_SHM_IOC_MAGIC              'm'
#define KMPP_SHM_IOC_QUERY_INFO         _IOW(KMPP_SHM_IOC_MAGIC, 1, unsigned int)
#define KMPP_SHM_IOC_RELEASE_INFO       _IOW(KMPP_SHM_IOC_MAGIC, 2, unsigned int)
#define KMPP_SHM_IOC_GET_SHM            _IOW(KMPP_SHM_IOC_MAGIC, 3, unsigned int)
#define KMPP_SHM_IOC_PUT_SHM            _IOW(KMPP_SHM_IOC_MAGIC, 4, unsigned int)
#define KMPP_SHM_IOC_DUMP               _IOW(KMPP_SHM_IOC_MAGIC, 5, unsigned int)

#define MPP_OBJ_DBG_FLOW                (0x00000001)
#define MPP_OBJ_DBG_TRIE                (0x00000002)
#define MPP_OBJ_DBG_SET                 (0x00000010)
#define MPP_OBJ_DBG_GET                 (0x00000020)

#define kmpp_obj_dbg(flag, fmt, ...)    _mpp_dbg(kmpp_obj_debug, flag, fmt, ## __VA_ARGS__)
#define kmpp_obj_dbg_f(flag, fmt, ...)  _mpp_dbg_f(kmpp_obj_debug, flag, fmt, ## __VA_ARGS__)

#define kmpp_obj_dbg_flow(fmt, ...)     kmpp_obj_dbg(MPP_OBJ_DBG_FLOW, fmt, ## __VA_ARGS__)
#define kmpp_obj_dbg_trie(fmt, ...)     kmpp_obj_dbg(MPP_OBJ_DBG_TRIE, fmt, ## __VA_ARGS__)
#define kmpp_obj_dbg_set(fmt, ...)      kmpp_obj_dbg(MPP_OBJ_DBG_SET, fmt, ## __VA_ARGS__)
#define kmpp_obj_dbg_get(fmt, ...)      kmpp_obj_dbg(MPP_OBJ_DBG_GET, fmt, ## __VA_ARGS__)

#define U64_TO_PTR(ptr)                 ((void *)(intptr_t)(ptr))

#define ENTRY_TO_PTR(tbl, entry)        (((char *)entry) + tbl->data_offset)
#define ENTRY_TO_s32_PTR(tbl, entry)    ((rk_s32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u32_PTR(tbl, entry)    ((rk_u32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_s64_PTR(tbl, entry)    ((rk_s64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u64_PTR(tbl, entry)    ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_obj_PTR(tbl, entry)    ((KmppObj *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_ptr_PTR(tbl, entry)    ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_fp_PTR(tbl, entry)     ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_st_PTR(tbl, entry)     ((void *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_shm_PTR(tbl, entry)    ((void *)ENTRY_TO_PTR(tbl, entry))

#define ENTRY_TO_FLAG_PTR(tbl, entry)   ((rk_u16 *)((char *)entry + tbl->flag_offset))

/* kernel object trie share info */
/*
 * kernel object trie share info
 * used in KMPP_SHM_IOC_QUERY_INFO
 *
 * input  : object name userspace address
 * output : trie_root userspace address (read only)
 */
typedef union KmppObjTrie_u {
    __u64       name_uaddr;
    __u64       trie_root;
} KmppObjTrie;

/* kernel object share memory userspace visable header data */
typedef struct KmppObjShm_t {
    /* kobj_uaddr   - the userspace base address for kernel object */
    __u64       kobj_uaddr;
    /* kobj_kaddr   - the kernel base address for kernel object */
    __u64       kobj_kaddr;
    /* DO NOT access reserved data only used by kernel */
} KmppObjShm;

/* kernel object share memory get / put ioctl data */
typedef struct KmppObjIocArg_t {
    /* address array element count */
    __u32       count;

    /* flag for batch operation */
    __u32       flag;

    /*
     * at KMPP_SHM_IOC_GET_SHM
     * name_uaddr   - kernel object name in userspace address
     * kobj_uaddr   - kernel object userspace address for KmppObjShm
     *
     * at KMPP_SHM_IOC_PUT_SHM
     * kobj_uaddr   - kernel object userspace address for KmppObjShm
     */
    union {
        __u64   name_uaddr[0];
        __u64   kobj_uaddr[0];
    };
} KmppObjIocArg;

typedef struct KmppObjDefImpl_t {
    MppTrie trie;
    rk_s32 fd;
    rk_s32 index;
    rk_s32 ref_cnt;
    rk_s32 priv_offset;
    rk_s32 entry_offset;
    rk_s32 entry_size;
    const char *name_check;            /* for object name address check */
    const char *name;
} KmppObjDefImpl;

typedef struct KmppObjImpl_t {
    const char *name_check;
    /* class infomation link */
    KmppObjDefImpl *def;
    /* trie for fast access */
    MppTrie trie;
    /* malloc flag */
    rk_u32 need_free;
    KmppObjShm *shm;
    void *entry;
} KmppObjImpl;

typedef struct KmppObjs_t {
    rk_s32              fd;
    rk_s32              count;
    rk_s32              entry_offset;
    rk_s32              priv_offset;
    MppTrie             trie;
    KmppObjDefImpl      defs[0];
} KmppObjs;

//static rk_u32 kmpp_obj_debug = MPP_OBJ_DBG_SET | MPP_OBJ_DBG_GET;
static rk_u32 kmpp_obj_debug = 0;
static LIST_HEAD(kmpp_obj_list);
static KmppObjs *objs = NULL;

const char *strof_entry_type(EntryType type)
{
    static const char *entry_type_names[] = {
        "rk_s32",
        "rk_u32",
        "rk_s64",
        "rk_u64",
        "void *",
        "struct",
    };

    return entry_type_names[type];
}

#define MPP_OBJ_ACCESS_IMPL(type, base_type, log_str) \
    rk_s32 kmpp_obj_impl_set_##type(KmppLocTbl *tbl, void *entry, base_type val) \
    { \
        base_type *dst = ENTRY_TO_##type##_PTR(tbl, entry); \
        base_type old = dst[0]; \
        dst[0] = val; \
        if (!tbl->flag_value) { \
            kmpp_obj_dbg_set("%p + %x set " #type " change " #log_str " -> " #log_str "\n", entry, tbl->data_offset, old, val); \
        } else { \
            if (old != val) { \
                kmpp_obj_dbg_set("%p + %x set " #type " update " #log_str " -> " #log_str " flag %d|%x\n", \
                                entry, tbl->data_offset, old, val, tbl->flag_offset, tbl->flag_value); \
                ENTRY_TO_FLAG_PTR(tbl, entry)[0] |= tbl->flag_value; \
            } else { \
                kmpp_obj_dbg_set("%p + %x set " #type " keep   " #log_str "\n", entry, tbl->data_offset, old); \
            } \
        } \
        return MPP_OK; \
    } \
    rk_s32 kmpp_obj_impl_get_##type(KmppLocTbl *tbl, void *entry, base_type *val) \
    { \
        if (tbl && tbl->data_size) { \
            base_type *src = ENTRY_TO_##type##_PTR(tbl, entry); \
            kmpp_obj_dbg_get("%p + %x get " #type " value  " #log_str "\n", entry, tbl->data_offset, src[0]); \
            val[0] = src[0]; \
            return MPP_OK; \
        } \
        return MPP_NOK; \
    }

MPP_OBJ_ACCESS_IMPL(s32, rk_s32, % d)
MPP_OBJ_ACCESS_IMPL(u32, rk_u32, % u)
MPP_OBJ_ACCESS_IMPL(s64, rk_s64, % #llx)
MPP_OBJ_ACCESS_IMPL(u64, rk_u64, % #llx)
MPP_OBJ_ACCESS_IMPL(obj, KmppObj, % p)
MPP_OBJ_ACCESS_IMPL(ptr, void *, % p)
MPP_OBJ_ACCESS_IMPL(fp, void *, % p)

#define MPP_OBJ_STRUCT_ACCESS_IMPL(type, base_type, log_str) \
    rk_s32 kmpp_obj_impl_set_##type(KmppLocTbl *tbl, void *entry, base_type *val) \
    { \
        void *dst = ENTRY_TO_##type##_PTR(tbl, entry); \
        if (!tbl->flag_value) { \
            /* simple copy */ \
            kmpp_obj_dbg_set("%p + %x set " #type " size %d change %p -> %p\n", entry, tbl->data_offset, tbl->data_size, dst, val); \
            memcpy(dst, val, tbl->data_size); \
            return MPP_OK; \
        } \
        /* copy with flag check and updata */ \
        if (memcmp(dst, val, tbl->data_size)) { \
            kmpp_obj_dbg_set("%p + %x set " #type " size %d update %p -> %p flag %d|%x\n", \
                            entry, tbl->data_offset, tbl->data_size, dst, val, tbl->flag_offset, tbl->flag_value); \
            memcpy(dst, val, tbl->data_size); \
            ENTRY_TO_FLAG_PTR(tbl, entry)[0] |= tbl->flag_value; \
        } else { \
            kmpp_obj_dbg_set("%p + %x set " #type " size %d keep   %p\n", entry, tbl->data_offset, tbl->data_size, dst); \
        } \
        return MPP_OK; \
    } \
    rk_s32 kmpp_obj_impl_get_##type(KmppLocTbl *tbl, void *entry, base_type *val) \
    { \
        if (tbl && tbl->data_size) { \
            void *src = ENTRY_TO_##type##_PTR(tbl, entry); \
            kmpp_obj_dbg_get("%p + %x get " #type " size %d value  " #log_str "\n", entry, tbl->data_offset, tbl->data_size, src); \
            memcpy(val, src, tbl->data_size); \
            return MPP_OK; \
        } \
        return MPP_NOK; \
    }

MPP_OBJ_STRUCT_ACCESS_IMPL(st, void, % p)
MPP_OBJ_STRUCT_ACCESS_IMPL(shm, KmppShmPtr, % p)

__attribute__ ((constructor))
void kmpp_objs_init(void)
{
    static const char *dev = "/dev/kmpp_objs";
    rk_s32 fd = -1;
    rk_u64 ioc = 0;
    void *root = NULL;
    MppTrie trie = NULL;
    MppTrieInfo *info;
    rk_s32 offset;
    rk_s32 count;
    rk_s32 ret;
    rk_s32 i;

    if (objs) {
        mpp_loge_f("objs already inited %p\n", objs);
        return;
    }

    mpp_env_get_u32("kmpp_obj_debug", &kmpp_obj_debug, 0);

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        kmpp_obj_dbg_flow("%s open failed ret fd %d\n", dev, fd);
        goto __failed;
    }

    ret = ioctl(fd, KMPP_SHM_IOC_QUERY_INFO, &ioc);
    if (ret < 0) {
        mpp_loge_f("%s ioctl failed ret %d\n", dev, ret);
        goto __failed;
    }

    root = (void *)ioc;
    kmpp_obj_dbg_trie("query fd %d root %p from kernel\n", fd, root);

    ret = mpp_trie_init_by_root(&trie, root);
    if (ret || !trie) {
        mpp_loge_f("init trie by root failed ret %d\n", ret);
        goto __failed;
    }

    if (kmpp_obj_debug & MPP_OBJ_DBG_TRIE)
        mpp_trie_dump_f(trie);

    info = mpp_trie_get_info(trie, "__count");
    count = info ? *(rk_s32 *)mpp_trie_info_ctx(info) : 0;

    objs = mpp_calloc_size(KmppObjs, sizeof(KmppObjs) + sizeof(KmppObjDefImpl) * count);
    if (!objs) {
        mpp_loge_f("alloc objs failed\n");
        goto __failed;
    }

    objs->fd = fd;
    objs->count = count;
    objs->trie = trie;

    info = mpp_trie_get_info(trie, "__offset");
    offset = info ? *(rk_s32 *)mpp_trie_info_ctx(info) : 0;
    objs->entry_offset = offset;
    info = mpp_trie_get_info(trie, "__priv");
    offset = info ? *(rk_s32 *)mpp_trie_info_ctx(info) : 0;
    objs->priv_offset = offset;

    kmpp_obj_dbg_trie("count %d object entry at %d priv at %d\n", count,
                      objs->entry_offset, objs->priv_offset);

    info = mpp_trie_get_info_first(trie);

    for (i = 0; i < count && info; i++) {
        KmppObjDefImpl *impl = &objs->defs[i];
        const char *name = mpp_trie_info_name(info);
        MppTrie trie_objdef = NULL;
        MppTrieInfo *info_objdef;

        offset = *(rk_s32 *)mpp_trie_info_ctx(info);
        ret = mpp_trie_init_by_root(&trie_objdef, root + offset);
        if (!trie_objdef) {
            mpp_loge_f("init %d:%d obj trie failed\n", count, i);
            break;
        }

        impl->trie = trie_objdef;
        impl->index = i;
        impl->fd = fd;
        info_objdef = mpp_trie_get_info(trie_objdef, "__size");
        impl->entry_size = info_objdef ? *(rk_s32 *)mpp_trie_info_ctx(info_objdef) : 0;
        impl->entry_offset = objs->entry_offset;
        impl->priv_offset = objs->priv_offset;
        impl->name = name;

        info = mpp_trie_get_info_next(trie_objdef, info);
        kmpp_obj_dbg_trie("name %s offset %d entry_size %d\n",
                          name, offset, impl->entry_size);
    }

    return;

__failed:
    if (fd > 0) {
        close(fd);
        fd = -1;
    }
    if (trie) {
        mpp_trie_deinit(trie);
        trie = NULL;
    }
}

__attribute__ ((destructor))
void kmpp_objs_deinit(void)
{
    kmpp_obj_dbg_flow("kmpp_objs_deinit objs %p\n", objs);

    if (objs) {
        rk_s32 i;

        if (objs->fd > 0) {
            close(objs->fd);
            objs->fd = -1;
        }

        if (objs->trie) {
            mpp_trie_deinit(objs->trie);
            objs->trie = NULL;
        }

        for (i = 0; i < objs->count; i++) {
            KmppObjDefImpl *impl = &objs->defs[i];

            if (impl->trie) {
                mpp_trie_deinit(impl->trie);
                impl->trie = NULL;
            }
        }

        mpp_free(objs);
    }
}

rk_s32 kmpp_objdef_put(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = MPP_NOK;

    if (impl) {
        impl->ref_cnt--;

        if (!impl->ref_cnt) {
            if (impl->trie) {
                ret = mpp_trie_deinit(impl->trie);
                impl->trie = NULL;
            }
        }

        ret = MPP_OK;
    }

    return ret;
}

rk_s32 kmpp_objdef_get(KmppObjDef *def, const char *name)
{
    KmppObjs *p = objs;
    MppTrieInfo *info = NULL;

    if (!def || !name || !p) {
        mpp_loge_f("invalid param def %p name %p objs %p\n", def, name, p);
        return MPP_NOK;
    }

    *def = NULL;

    info = mpp_trie_get_info(p->trie, name);
    if (!info) {
        mpp_loge_f("failed to get objdef %s\n", name);
        return MPP_NOK;
    }

    if (p->count > 0 && info->index < (RK_U32)p->count) {
        KmppObjDefImpl *impl = &p->defs[info->index];

        impl->ref_cnt++;
        *def = impl;

        return MPP_OK;
    }

    mpp_loge_f("failed to get objdef %s index %d max %d\n",
               name, info->index, p->count);

    return MPP_NOK;
}

rk_s32 kmpp_objdef_dump(KmppObjDef def)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
        MppTrie trie = impl->trie;
        MppTrieInfo *info = NULL;
        const char *name = impl->name;
        RK_S32 i = 0;

        mpp_logi("dump objdef %-16s entry_size %d element count %d\n",
                 name, impl->entry_size, mpp_trie_get_info_count(trie));

        info = mpp_trie_get_info_first(trie);
        while (info) {
            name = mpp_trie_info_name(info);
            if (!strstr(name, "__")) {
                KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);
                rk_s32 idx = i++;

                mpp_logi("%-2d - %-16s offset %4d size %d\n", idx,
                         name, tbl->data_offset, tbl->data_size);
            }
            info = mpp_trie_get_info_next(trie, info);
        }

        info = mpp_trie_get_info_first(trie);
        while (info) {
            name = mpp_trie_info_name(info);
            if (strstr(name, "__")) {
                void *p = mpp_trie_info_ctx(info);
                rk_s32 idx = i++;

                if (info->ctx_len == sizeof(RK_U32)) {

                    mpp_logi("%-2d - %-16s val %d\n", idx, name, *(RK_U32 *)p);
                } else {
                    mpp_logi("%-2d - %-16s str %s\n", idx, name, p);
                }
            }
            info = mpp_trie_get_info_next(trie, info);
        }

        return MPP_OK;
    }

    return MPP_NOK;
}

const char *kmpp_objdef_get_name(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    return impl ? impl->name : NULL;
}

rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    return impl ? impl->entry_size : 0;
}

MppTrie kmpp_objdef_get_trie(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    return impl ? impl->trie : NULL;
}

rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def)
{
    KmppObjImpl *impl;
    KmppObjDefImpl *def_impl;
    KmppObjIocArg *ioc;
    rk_s32 ret = MPP_NOK;

    if (!obj || !def) {
        mpp_loge_f("invalid param obj %p def %p\n", obj, def);
        return ret;
    }

    *obj = NULL;
    def_impl = (KmppObjDefImpl *)def;
    impl = mpp_calloc(KmppObjImpl, 1);
    if (!impl) {
        mpp_loge_f("malloc obj impl %d failed\n", sizeof(KmppObjImpl));
        return ret;
    }

    ioc = alloca(sizeof(KmppObjIocArg) + sizeof(rk_u64));

    ioc->count = 1;
    ioc->flag = 0;
    ioc->name_uaddr[0] = (__u64)def_impl->name;

    ret = ioctl(def_impl->fd, KMPP_SHM_IOC_GET_SHM, ioc);
    if (ret) {
        mpp_err("%s fd %d ioctl KMPP_SHM_IOC_GET_SHM failed\n",
                def_impl->name, def_impl->fd);
        mpp_free(impl);
        return ret;
    }

    impl->name_check = def_impl->name_check;
    impl->def = def;
    impl->trie = def_impl->trie;
    impl->need_free = 1;
    impl->shm = U64_TO_PTR(ioc->kobj_uaddr[0]);
    impl->entry = U64_TO_PTR(ioc->kobj_uaddr[0] + def_impl->entry_offset);

    /* write userspace object address to share memory userspace private value */
    *(RK_U64 *)U64_TO_PTR(ioc->kobj_uaddr[0] + def_impl->priv_offset) = (RK_U64)impl;

    *obj = impl;

    return MPP_OK;
}

rk_s32 kmpp_obj_get_by_sptr(KmppObj *obj, KmppObjDef def, KmppShmPtr *sptr)
{
    KmppObjImpl *impl;
    KmppObjDefImpl *def_impl;
    rk_s32 ret = MPP_NOK;

    if (!obj || !def) {
        mpp_loge_f("invalid param obj %p def %p\n", obj, def);
        return ret;
    }

    *obj = NULL;
    def_impl = (KmppObjDefImpl *)def;
    impl = mpp_calloc(KmppObjImpl, 1);
    if (!impl) {
        mpp_loge_f("malloc obj impl %d failed\n", sizeof(KmppObjImpl));
        return ret;
    }

    impl->name_check = def_impl->name_check;
    impl->def = def;
    impl->trie = def_impl->trie;
    impl->need_free = 1;
    impl->shm = sptr->uptr;
    impl->entry = sptr->uptr + def_impl->entry_offset;

    /* write userspace object address to share memory userspace private value */
    *(RK_U64 *)U64_TO_PTR(sptr->uaddr + def_impl->priv_offset) = (RK_U64)impl;

    *obj = impl;

    return MPP_OK;
}

rk_s32 kmpp_obj_put(KmppObj obj)
{
    if (obj) {
        KmppObjImpl *impl = (KmppObjImpl *)obj;
        KmppObjDefImpl *def = impl->def;

        if (impl->shm) {
            KmppObjIocArg *ioc = alloca(sizeof(KmppObjIocArg) + sizeof(rk_u64));
            rk_s32 ret;

            ioc->count = 1;
            ioc->flag = 0;
            ioc->kobj_uaddr[0] = (__u64)impl->shm;

            ret = ioctl(def->fd, KMPP_SHM_IOC_PUT_SHM, ioc);
            if (ret)
                mpp_err("ioctl KMPP_SHM_IOC_PUT_SHM failed ret %d\n", ret);

            impl->shm = NULL;
        }

        if (impl->need_free)
            mpp_free(impl);

        return MPP_OK;
    }

    return MPP_NOK;
}

rk_s32 kmpp_obj_check(KmppObj obj, const char *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (!impl) {
        mpp_loge_f("from %s failed for NULL arg\n", caller);
        return MPP_NOK;
    }

    if (!impl->name_check || !impl->def ||
        impl->name_check != impl->def->name_check) {
        mpp_loge_f("from %s failed for name check %s but %s\n", caller,
                   impl->def ? impl->def->name_check : NULL, impl->name_check);
        return MPP_NOK;
    }

    return MPP_OK;
}

void *kmpp_obj_get_hnd(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (!impl) {
        mpp_loge("invalid obj %p\n", obj);
        return NULL;
    }

    return impl->shm;
}

rk_s32 kmpp_obj_get_hnd_size(KmppObj obj)
{
    (void)obj;
    return sizeof(KmppObjShm);
}

void *kmpp_obj_get_entry(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    return impl ? impl->entry : NULL;
}

rk_s32 kmpp_obj_get_offset(KmppObj obj, const char *name)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (!impl || !name) {
        mpp_loge("invalid obj %p name %s\n", obj, name);
        return -1;
    }

    if (impl->trie) {
        MppTrieInfo *info = mpp_trie_get_info(impl->trie, name);

        if (info) {
            KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);

            if (tbl)
                return tbl->data_offset;
        }
    }

    mpp_loge("invalid offset for name %s\n", name);

    return -1;
}

#define MPP_OBJ_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_set_##type(KmppObj obj, const char *name, base_type val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl->trie) { \
            MppTrieInfo *info = mpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            mpp_loge("obj %s set %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    } \
    rk_s32 kmpp_obj_get_##type(KmppObj obj, const char *name, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl->trie) { \
            MppTrieInfo *info = mpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            mpp_loge("obj %s get %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    }

MPP_OBJ_ACCESS(s32, rk_s32)
MPP_OBJ_ACCESS(u32, rk_u32)
MPP_OBJ_ACCESS(s64, rk_s64)
MPP_OBJ_ACCESS(u64, rk_u64)
MPP_OBJ_ACCESS(obj, KmppObj)
MPP_OBJ_ACCESS(ptr, void *)
MPP_OBJ_ACCESS(fp, void *)

#define MPP_OBJ_STRUCT_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_set_##type(KmppObj obj, const char *name, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl->trie) { \
            MppTrieInfo *info = mpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            mpp_loge("obj %s set %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    } \
    rk_s32 kmpp_obj_get_##type(KmppObj obj, const char *name, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl->trie) { \
            MppTrieInfo *info = mpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            mpp_loge("obj %s get %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    }

MPP_OBJ_STRUCT_ACCESS(st, void)
MPP_OBJ_STRUCT_ACCESS(shm, KmppShmPtr)

#define MPP_OBJ_TBL_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_tbl_set_##type(KmppObj obj, KmppLocTbl *tbl, base_type val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl) \
            ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
        if (ret) \
            mpp_loge("obj %s tbl %08x set " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    } \
    rk_s32 kmpp_obj_tbl_get_##type(KmppObj obj, KmppLocTbl *tbl, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl) \
            ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
        if (ret) \
            mpp_loge("obj %s tbl %08x get " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    }

MPP_OBJ_TBL_ACCESS(s32, rk_s32)
MPP_OBJ_TBL_ACCESS(u32, rk_u32)
MPP_OBJ_TBL_ACCESS(s64, rk_s64)
MPP_OBJ_TBL_ACCESS(u64, rk_u64)
MPP_OBJ_TBL_ACCESS(obj, KmppObj)
MPP_OBJ_TBL_ACCESS(ptr, void *)
MPP_OBJ_TBL_ACCESS(fp, void *)

#define MPP_OBJ_STRUCT_TBL_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_tbl_set_##type(KmppObj obj, KmppLocTbl *tbl, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl) \
            ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
        if (ret) \
            mpp_loge("obj %s tbl %08x set " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    } \
    rk_s32 kmpp_obj_tbl_get_##type(KmppObj obj, KmppLocTbl *tbl, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = MPP_NOK; \
        if (impl) \
            ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
        if (ret) \
            mpp_loge("obj %s tbl %08x get " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    }

MPP_OBJ_STRUCT_TBL_ACCESS(st, void)
MPP_OBJ_STRUCT_TBL_ACCESS(shm, KmppShmPtr)

static rk_s32 kmpp_obj_impl_run(rk_s32 (*run)(void *ctx), void *ctx)
{
    return run(ctx);
}

rk_s32 kmpp_obj_run(KmppObj obj, const char *name)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = MPP_NOK;

    if (impl->trie) {
        MppTrieInfo *info = mpp_trie_get_info(impl->trie, name);
        void *val = NULL;

        if (info) {
            KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);

            ret = kmpp_obj_impl_get_fp(tbl, impl->entry, &val);
        }

        if (val)
            ret = kmpp_obj_impl_run(val, impl->entry);
    }

    return ret;
}

rk_s32 kmpp_obj_udump_f(KmppObj obj, const char *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    KmppObjDefImpl *def = impl ? impl->def : NULL;
    MppTrie trie = NULL;
    MppTrieInfo *info = NULL;
    const char *name = NULL;
    rk_s32 ret = MPP_NOK;
    RK_S32 i = 0;

    if (!impl || !def) {
        mpp_loge_f("invalid obj %p def %p\n", impl, def);
        return MPP_NOK;
    }

    trie = impl->trie;
    name = def->name;

    mpp_logi("dump obj %-12s - %p at %s:\n", name, impl, caller);

    info = mpp_trie_get_info_first(trie);
    while (info) {
        name = mpp_trie_info_name(info);
        if (!strstr(name, "__")) {
            KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);
            rk_s32 idx = i++;

            switch (tbl->data_type) {
            case ENTRY_TYPE_s32 : {
                rk_s32 val;
                rk_s32 val_chk;

                ret = kmpp_obj_tbl_get_s32(obj, tbl, &val);
                if (!ret)
                    mpp_logi("%-2d - %-16s s32 %#x:%d\n", idx, name, val, val);
                else
                    mpp_loge("%-2d - %-16s s32 get failed\n", idx, name);

                kmpp_obj_get_s32(obj, name, &val_chk);
                if (val != val_chk)
                    mpp_loge("%-2d - %-16s s32 check failed\n", idx, name);
            } break;
            case ENTRY_TYPE_u32 : {
                rk_u32 val;
                rk_u32 val_chk;

                ret = kmpp_obj_tbl_get_u32(obj, tbl, &val);
                if (!ret)
                    mpp_logi("%-2d - %-16s u32 %#x:%u\n", idx, name, val, val);
                else
                    mpp_loge("%-2d - %-16s u32 get failed\n", idx, name);

                kmpp_obj_get_u32(obj, name, &val_chk);
                if (val != val_chk)
                    mpp_loge("%-2d - %-16s u32 check failed\n", idx, name);
            } break;
            case ENTRY_TYPE_s64 : {
                rk_s64 val;
                rk_s64 val_chk;

                ret = kmpp_obj_tbl_get_s64(obj, tbl, &val);
                if (!ret)
                    mpp_logi("%-2d - %-16s s64 %#llx:%lld\n", idx, name, val, val);
                else
                    mpp_loge("%-2d - %-16s s64 get failed\n", idx, name);

                kmpp_obj_get_s64(obj, name, &val_chk);
                if (val != val_chk)
                    mpp_loge("%-2d - %-16s s64 check failed\n", idx, name);
            } break;
            case ENTRY_TYPE_u64 : {
                rk_u64 val;
                rk_u64 val_chk;

                ret = kmpp_obj_tbl_get_u64(obj, tbl, &val);
                if (!ret)
                    mpp_logi("%-2d - %-16s u64 %#llx:%llu\n", idx, name, val, val);
                else
                    mpp_loge("%-2d - %-16s u64 get failed\n", idx, name);

                kmpp_obj_get_u64(obj, name, &val_chk);
                if (val != val_chk)
                    mpp_loge("%-2d - %-16s u64 check failed\n", idx, name);
            } break;
            case ENTRY_TYPE_st : {
                void *val_chk = mpp_malloc_size(void, tbl->data_size);
                void *val = mpp_malloc_size(void, tbl->data_size);
                rk_s32 data_size = tbl->data_size;
                char logs[128];

                ret = kmpp_obj_tbl_get_st(obj, tbl, val);
                if (!ret) {
                    rk_s32 pos;

                    mpp_logi("%-2d - %-16s st  %d:%d\n",
                             idx, name, tbl->data_offset, data_size);

                    i = 0;
                    for (; i < data_size / 4 - 8; i += 8) {
                        snprintf(logs, sizeof(logs) - 1, "%-2x - %#08x %#08x %#08x %#08x %#08x %#08x %#08x %#08x", i,
                                 ((RK_U32 *)val)[i + 0], ((RK_U32 *)val)[i + 1],
                                 ((RK_U32 *)val)[i + 2], ((RK_U32 *)val)[i + 3],
                                 ((RK_U32 *)val)[i + 4], ((RK_U32 *)val)[i + 5],
                                 ((RK_U32 *)val)[i + 6], ((RK_U32 *)val)[i + 7]);

                        mpp_logi("%s\n", logs);
                    }

                    pos = snprintf(logs, sizeof(logs) - 1, "%-2x -", i);
                    for (; i < data_size / 4; i++)
                        pos += snprintf(logs + pos, sizeof(logs) - 1 - pos, " %#08x", ((RK_U32 *)val)[i]);

                    mpp_logi("%s\n", logs);
                } else
                    mpp_loge("%-2d - %-16s st  get failed\n", idx, name);

                kmpp_obj_get_st(obj, name, val_chk);
                if (memcmp(val, val_chk, tbl->data_size)) {
                    mpp_loge("%-2d - %-16s st  check failed\n", idx, name);
                    mpp_loge("val     %p\n", val);
                    mpp_loge("val_chk %p\n", val_chk);
                }

                MPP_FREE(val);
                MPP_FREE(val_chk);
            } break;
            case ENTRY_TYPE_shm : {
                KmppShmPtr *val_chk = mpp_malloc_size(void, tbl->data_size);
                KmppShmPtr *val = mpp_malloc_size(void, tbl->data_size);

                ret = kmpp_obj_tbl_get_st(obj, tbl, val);
                if (!ret)
                    mpp_logi("%-2d - %-16s shm u%#llx:k%#llx\n",
                             idx, name, val->uaddr, val->kaddr);
                else
                    mpp_loge("%-2d - %-16s shm get failed\n", idx, name);

                kmpp_obj_get_st(obj, name, val_chk);
                if (memcmp(val, val_chk, tbl->data_size)) {
                    mpp_loge("%-2d - %-16s shm check failed\n", idx, name);
                    mpp_loge("val     %p - %#llx:%#llx\n", val, val->uaddr, val->kaddr);
                    mpp_loge("val_chk %p - %#llx:%#llx\n", val_chk, val_chk->uaddr, val_chk->kaddr);
                }

                MPP_FREE(val);
                MPP_FREE(val_chk);
            } break;
            case ENTRY_TYPE_uptr : {
                void *val;
                void *val_chk;

                ret = kmpp_obj_tbl_get_ptr(obj, tbl, &val);
                if (!ret)
                    mpp_logi("%-2d - %-16s ptr %p\n", idx, name, val);
                else
                    mpp_loge("%-2d - %-16s ptr get failed\n", idx, name);

                kmpp_obj_get_ptr(obj, name, &val_chk);
                if (val != val_chk)
                    mpp_loge("%-2d - %-16s ptr check failed\n", idx, name);
            } break;
            case ENTRY_TYPE_ufp : {
                void *val;
                void *val_chk;

                ret = kmpp_obj_tbl_get_fp(obj, tbl, &val);
                if (!ret)
                    mpp_logi("%-2d - %-16s fp  %p\n", idx, name, val);
                else
                    mpp_loge("%-2d - %-16s fp  get failed\n", idx, name);

                kmpp_obj_get_fp(obj, name, &val_chk);
                if (val != val_chk)
                    mpp_loge("%-2d - %-16s fp  check failed\n", idx, name);
            } break;
            default : {
                mpp_loge("%-2d - %-16s found invalid type %d\n", idx, name, tbl->data_type);
                ret = MPP_NOK;
            } break;
            }
        }
        info = mpp_trie_get_info_next(trie, info);
    }

    return ret ? MPP_NOK : MPP_OK;
}

rk_s32 kmpp_obj_kdump_f(KmppObj obj, const char *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    KmppObjDefImpl *def = impl ? impl->def : NULL;
    rk_s32 ret = MPP_NOK;

    if (!impl || !def) {
        mpp_loge_f("invalid obj %p def %p\n", impl, def);
        return MPP_NOK;
    }

    mpp_logi("dump obj %-12s - %p at %s by kernel\n", def->name, impl, caller);

    ret = ioctl(def->fd, KMPP_SHM_IOC_DUMP, impl->shm);
    if (ret)
        mpp_err("ioctl KMPP_SHM_IOC_DUMP failed ret %d\n", ret);

    return ret ? MPP_NOK : MPP_OK;
}
