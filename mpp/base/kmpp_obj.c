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
#include "mpp_lock.h"

#include "mpp_trie.h"
#include "kmpp_obj_impl.h"

#define KMPP_SHM_IOC_MAGIC              'm'
#define KMPP_SHM_IOC_QUERY_INFO         _IOW(KMPP_SHM_IOC_MAGIC, 1, unsigned int)
#define KMPP_SHM_IOC_RELEASE_INFO       _IOW(KMPP_SHM_IOC_MAGIC, 2, unsigned int)
#define KMPP_SHM_IOC_GET_SHM            _IOW(KMPP_SHM_IOC_MAGIC, 3, unsigned int)
#define KMPP_SHM_IOC_PUT_SHM            _IOW(KMPP_SHM_IOC_MAGIC, 4, unsigned int)
#define KMPP_SHM_IOC_DUMP               _IOW(KMPP_SHM_IOC_MAGIC, 5, unsigned int)

#define OBJ_DBG_FLOW                    (0x00000001)
#define OBJ_DBG_SHARE                   (0x00000002)
#define OBJ_DBG_ENTRY                   (0x00000004)
#define OBJ_DBG_HOOK                    (0x00000008)
#define OBJ_DBG_IOCTL                   (0x00000010)
#define OBJ_DBG_SET                     (0x00000040)
#define OBJ_DBG_GET                     (0x00000080)

#define obj_dbg(flag, fmt, ...)         _mpp_dbg(kmpp_obj_debug, flag, fmt, ## __VA_ARGS__)

#define obj_dbg_flow(fmt, ...)          obj_dbg(OBJ_DBG_FLOW, fmt, ## __VA_ARGS__)
#define obj_dbg_share(fmt, ...)         obj_dbg(OBJ_DBG_SHARE, fmt, ## __VA_ARGS__)
#define obj_dbg_entry(fmt, ...)         obj_dbg(OBJ_DBG_ENTRY, fmt, ## __VA_ARGS__)
#define obj_dbg_hook(fmt, ...)          obj_dbg(OBJ_DBG_HOOK, fmt, ## __VA_ARGS__)
#define obj_dbg_ioctl(fmt, ...)         obj_dbg(OBJ_DBG_IOCTL, fmt, ## __VA_ARGS__)
#define obj_dbg_set(fmt, ...)           obj_dbg(OBJ_DBG_SET, fmt, ## __VA_ARGS__)
#define obj_dbg_get(fmt, ...)           obj_dbg(OBJ_DBG_GET, fmt, ## __VA_ARGS__)

#define U64_TO_PTR(ptr)                 ((void *)(intptr_t)(ptr))

#define ENTRY_TO_PTR(tbl, entry)        (((char *)entry) + tbl->tbl.elem_offset)
#define ENTRY_TO_s32_PTR(tbl, entry)    ((rk_s32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u32_PTR(tbl, entry)    ((rk_u32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_s64_PTR(tbl, entry)    ((rk_s64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u64_PTR(tbl, entry)    ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_obj_PTR(tbl, entry)    ((KmppObj *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_ptr_PTR(tbl, entry)    ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_fp_PTR(tbl, entry)     ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_st_PTR(tbl, entry)     ((void *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_shm_PTR(tbl, entry)    ((void *)ENTRY_TO_PTR(tbl, entry))

/* 32bit unsigned long pointer */
#define ELEM_FLAG_U32_POS(offset)      (((offset) & (~31)) / 8)
#define ELEM_FLAG_BIT_POS(offset)      ((offset) & 31)
#define ENTRY_TO_FLAG_PTR(tbl, entry)   ((rk_ul *)((rk_u8 *)entry + ELEM_FLAG_U32_POS(tbl->tbl.flag_offset)))

#define ENTRY_SET_FLAG(tbl, entry) \
    *ENTRY_TO_FLAG_PTR(tbl, entry) |= 1ul << (ELEM_FLAG_BIT_POS(tbl->tbl.flag_offset))

#define ENTRY_CLR_FLAG(tbl, entry) \
    *ENTRY_TO_FLAG_PTR(tbl, entry) &= ~(1ul << (ELEM_FLAG_BIT_POS(tbl->tbl.flag_offset)))

#define ENTRY_TEST_FLAG(tbl, entry) \
    (*ENTRY_TO_FLAG_PTR(tbl, entry) & 1ul << (ELEM_FLAG_BIT_POS(tbl->tbl.flag_offset))) ? 1 : 0

/* kernel object share memory get / put ioctl data */
typedef struct KmppObjIocArg_t {
    /* address array element count */
    __u32       count;

    /* flag for batch operation */
    __u32       flag;

    /*
     * at KMPP_SHM_IOC_GET_SHM
     * name_uaddr   - kernel object name in userspace address
     * obj_sptr     - kernel object userspace / kernel address of KmppShmPtr
     *
     * at KMPP_SHM_IOC_PUT_SHM
     * obj_sptr     - kernel object userspace / kernel address of KmppShmPtr
     */
    union {
        __u64       name_uaddr[0];
        /* ioctl object userspace / kernel address */
        KmppShmPtr  obj_sptr[0];
    };
} KmppObjIocArg;

typedef struct KmppObjDefImpl_t {
    MppTrie trie;
    rk_s32 index;
    rk_s32 ref_cnt;
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
    KmppShmPtr *shm;
    void *entry;
} KmppObjImpl;

typedef struct KmppObjs_t {
    rk_s32              fd;
    rk_s32              count;
    rk_s32              entry_offset;
    rk_s32              priv_offset;
    rk_s32              name_offset;
    MppTrie             trie;
    void                *root;
    KmppObjDefImpl      defs[0];
} KmppObjs;

static rk_u32 kmpp_obj_debug = 0;
static KmppObjs *objs = NULL;

const char *strof_entry_type(EntryType type)
{
    static const char *ELEM_TYPE_names[] = {
        [ELEM_TYPE_s32]    = "s32",
        [ELEM_TYPE_u32]    = "u32",
        [ELEM_TYPE_s64]    = "s64",
        [ELEM_TYPE_u64]    = "u64",
        [ELEM_TYPE_st]     = "struct",
        [ELEM_TYPE_shm]    = "shm_ptr",
        [ELEM_TYPE_kobj]   = "kobj",
        [ELEM_TYPE_kptr]   = "kptr",
        [ELEM_TYPE_kfp]    = "kfunc_ptr",
        [ELEM_TYPE_uobj]   = "uobj",
        [ELEM_TYPE_uptr]   = "uptr",
        [ELEM_TYPE_ufp]    = "ufunc_ptr",
    };
    static const char *invalid_type_str = "invalid";

    if (type & (~ELEM_TYPE_BUTT))
        return invalid_type_str;

    return ELEM_TYPE_names[type] ? ELEM_TYPE_names[type] : invalid_type_str;
}

#define MPP_OBJ_ACCESS_IMPL(type, base_type, log_str) \
    rk_s32 kmpp_obj_impl_set_##type(KmppEntry *tbl, void *entry, base_type val) \
    { \
        base_type *dst = ENTRY_TO_##type##_PTR(tbl, entry); \
        base_type old = dst[0]; \
        dst[0] = val; \
        if (!tbl->tbl.flag_offset) { \
            obj_dbg_set("%p + %x set " #type " change " #log_str " -> " #log_str "\n", entry, tbl->tbl.elem_offset, old, val); \
        } else { \
            if (old != val) { \
                obj_dbg_set("%p + %x set " #type " update " #log_str " -> " #log_str " flag %d\n", \
                                entry, tbl->tbl.elem_offset, old, val, tbl->tbl.flag_offset); \
                ENTRY_SET_FLAG(tbl, entry); \
            } else { \
                obj_dbg_set("%p + %x set " #type " keep   " #log_str "\n", entry, tbl->tbl.elem_offset, old); \
            } \
        } \
        return MPP_OK; \
    } \
    rk_s32 kmpp_obj_impl_get_##type(KmppEntry *tbl, void *entry, base_type *val) \
    { \
        if (tbl && tbl->tbl.elem_size) { \
            base_type *src = ENTRY_TO_##type##_PTR(tbl, entry); \
            obj_dbg_get("%p + %x get " #type " value  " #log_str "\n", entry, tbl->tbl.elem_offset, src[0]); \
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
    rk_s32 kmpp_obj_impl_set_##type(KmppEntry *tbl, void *entry, base_type *val) \
    { \
        void *dst = ENTRY_TO_##type##_PTR(tbl, entry); \
        if (!tbl->tbl.flag_offset) { \
            /* simple copy */ \
            obj_dbg_set("%p + %x set " #type " size %d change %p -> %p\n", entry, tbl->tbl.elem_offset, tbl->tbl.elem_size, dst, val); \
            memcpy(dst, val, tbl->tbl.elem_size); \
            return MPP_OK; \
        } \
        /* copy with flag check and updata */ \
        if (memcmp(dst, val, tbl->tbl.elem_size)) { \
            obj_dbg_set("%p + %x set " #type " size %d update %p -> %p flag %d\n", \
                            entry, tbl->tbl.elem_offset, tbl->tbl.elem_size, dst, val, tbl->tbl.flag_offset); \
            memcpy(dst, val, tbl->tbl.elem_size); \
            ENTRY_SET_FLAG(tbl, entry); \
        } else { \
            obj_dbg_set("%p + %x set " #type " size %d keep   %p\n", entry, tbl->tbl.elem_offset, tbl->tbl.elem_size, dst); \
        } \
        return MPP_OK; \
    } \
    rk_s32 kmpp_obj_impl_get_##type(KmppEntry *tbl, void *entry, base_type *val) \
    { \
        if (tbl && tbl->tbl.elem_size) { \
            void *src = ENTRY_TO_##type##_PTR(tbl, entry); \
            obj_dbg_get("%p + %x get " #type " size %d value  " #log_str "\n", entry, tbl->tbl.elem_offset, tbl->tbl.elem_size, src); \
            memcpy(val, src, tbl->tbl.elem_size); \
            return MPP_OK; \
        } \
        return MPP_NOK; \
    }

MPP_OBJ_STRUCT_ACCESS_IMPL(st, void, % p)
MPP_OBJ_STRUCT_ACCESS_IMPL(shm, KmppShmPtr, % p)

__attribute__ ((destructor))
void kmpp_objs_deinit(void)
{
    KmppObjs *p = MPP_FETCH_AND(&objs, NULL);

    obj_dbg_flow("kmpp_objs_deinit objs %p\n", p);

    if (p) {
        rk_s32 i;

        for (i = 0; i < p->count; i++) {
            KmppObjDefImpl *impl = &p->defs[i];

            if (impl->trie) {
                mpp_trie_deinit(impl->trie);
                impl->trie = NULL;
            }
        }

        if (p->trie) {
            mpp_trie_deinit(p->trie);
            p->trie = NULL;
        }

        if (p->fd > 0) {
            close(p->fd);
            p->fd = -1;
        }

        mpp_free(p);
    }
}

__attribute__ ((constructor))
void kmpp_objs_init(void)
{
    static const char *dev = "/dev/kmpp_objs";
    KmppObjs *p = objs;
    void *root = NULL;
    MppTrie trie = NULL;
    MppTrieInfo *info;
    rk_s32 fd = -1;
    rk_s32 offset;
    rk_s32 count;
    rk_s32 ret;
    rk_s32 i;

    if (p) {
        obj_dbg_flow("objs already inited %p\n", p);
        kmpp_objs_deinit();
        p = NULL;
    }

    mpp_env_get_u32("kmpp_obj_debug", &kmpp_obj_debug, 0);

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        obj_dbg_flow("%s open failed ret fd %d\n", dev, fd);
        goto __failed;
    }

    {
        rk_u64 uaddr = 0;

        ret = ioctl(fd, KMPP_SHM_IOC_QUERY_INFO, &uaddr);
        if (ret < 0) {
            mpp_loge_f("%s ioctl failed ret %d\n", dev, ret);
            goto __failed;
        }

        root = (void *)(intptr_t)uaddr;
        obj_dbg_share("query fd %d root %p from kernel\n", fd, root);
    }

    ret = mpp_trie_init_by_root(&trie, root);
    if (ret || !trie) {
        mpp_loge_f("init trie by root failed ret %d\n", ret);
        goto __failed;
    }

    if (kmpp_obj_debug & OBJ_DBG_SHARE)
        mpp_trie_dump_f(trie);

    info = mpp_trie_get_info(trie, "__count");
    count = info ? *(rk_s32 *)mpp_trie_info_ctx(info) : 0;

    p = mpp_calloc_size(KmppObjs, sizeof(KmppObjs) + sizeof(KmppObjDefImpl) * count);
    if (!p) {
        mpp_loge_f("alloc objs failed\n");
        goto __failed;
    }

    p->fd = fd;
    p->count = count;
    p->trie = trie;
    p->root = root;

    info = mpp_trie_get_info(trie, "__offset");
    offset = info ? *(rk_s32 *)mpp_trie_info_ctx(info) : 0;
    p->entry_offset = offset;
    info = mpp_trie_get_info(trie, "__priv");
    offset = info ? *(rk_s32 *)mpp_trie_info_ctx(info) : 0;
    p->priv_offset = offset;
    info = mpp_trie_get_info(trie, "__name_offset");
    offset = info ? *(rk_s32 *)mpp_trie_info_ctx(info) : 0;
    p->name_offset = offset;

    obj_dbg_share("count %d object offsets - priv %d name %d entry %d\n", count,
                  p->priv_offset, p->name_offset, p->entry_offset);

    info = mpp_trie_get_info_first(trie);

    for (i = 0; i < count && info; i++) {
        KmppObjDefImpl *impl = &p->defs[i];
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

        info_objdef = mpp_trie_get_info(trie_objdef, "__index");
        impl->index = info_objdef ? *(rk_s32 *)mpp_trie_info_ctx(info_objdef) : -1;
        info_objdef = mpp_trie_get_info(trie_objdef, "__size");
        impl->entry_size = info_objdef ? *(rk_s32 *)mpp_trie_info_ctx(info_objdef) : 0;
        impl->name = name;

        info = mpp_trie_get_info_next(trie, info);
        obj_dbg_share("%2d:%2d - %s offset %d entry_size %d\n",
                      count, i, name, offset, impl->entry_size);
    }

    objs = p;

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
                KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info);
                rk_s32 idx = i++;

                mpp_logi("%-2d - %-16s offset %4d size %d\n", idx,
                         name, tbl->tbl.elem_offset, tbl->tbl.elem_size);
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
    rk_u64 uaddr;
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

    ioc = alloca(sizeof(KmppObjIocArg) + sizeof(KmppShmPtr));

    ioc->count = 1;
    ioc->flag = 0;
    ioc->name_uaddr[0] = (__u64)(intptr_t)def_impl->name;

    ret = ioctl(objs->fd, KMPP_SHM_IOC_GET_SHM, ioc);
    if (ret) {
        mpp_err("%s fd %d ioctl KMPP_SHM_IOC_GET_SHM failed\n",
                def_impl->name, objs->fd);
        mpp_free(impl);
        return ret;
    }

    uaddr = ioc->obj_sptr[0].uaddr;
    impl->name_check = def_impl->name_check;
    impl->def = def;
    impl->trie = def_impl->trie;
    impl->need_free = 1;
    impl->shm = U64_TO_PTR(uaddr);
    impl->entry = U64_TO_PTR(uaddr + objs->entry_offset);

    /* write userspace object address to share memory userspace private value */
    *(RK_U64 *)U64_TO_PTR(uaddr + objs->priv_offset) = (RK_U64)(intptr_t)impl;

    *obj = impl;

    return MPP_OK;
}

rk_s32 kmpp_obj_get_by_name(KmppObj *obj, const char *name)
{
    KmppObjs *p = objs;
    MppTrieInfo *info = NULL;

    if (!obj || !name || !p) {
        mpp_loge_f("invalid param obj %p name %p objs %p\n", obj, name, p);
        return MPP_NOK;
    }

    info = mpp_trie_get_info(p->trie, name);
    if (!info) {
        mpp_loge_f("failed to get objdef %s\n", name);
        return MPP_NOK;
    }

    if (p->count > 0 && info->index < (RK_U32)p->count) {
        KmppObjDefImpl *impl = &p->defs[info->index];

        /* NOTE: do NOT increase ref_cnt here */
        return kmpp_obj_get(obj, impl);
    }

    mpp_loge_f("failed to get objdef %s index %d max %d\n",
               name, info->index, p->count);

    return MPP_NOK;
}

rk_s32 kmpp_obj_get_by_sptr(KmppObj *obj, KmppShmPtr *sptr)
{
    KmppObjImpl *impl;
    KmppObjDefImpl *def;
    rk_u8 *uptr = sptr ? sptr->uptr : NULL;
    rk_s32 ret = MPP_NOK;

    if (!obj || !sptr || !uptr) {
        mpp_loge_f("invalid param obj %p sptr %p uptr %p\n", obj, sptr, uptr);
        return ret;
    }

    *obj = NULL;

    {
        rk_u32 val = *((rk_u32 *)(uptr + objs->name_offset));
        char *p;

        if (!val) {
            mpp_loge_f("invalid obj name offset %d\n", val);
            return ret;
        }

        p = (char *)objs->root + val;
        kmpp_objdef_get((KmppObjDef *)&def, p);
        if (!def) {
            mpp_loge_f("failed to get objdef %p - %s\n", p, p);
            return ret;
        }
    }

    impl = mpp_calloc(KmppObjImpl, 1);
    if (!impl) {
        mpp_loge_f("malloc obj impl %d failed\n", sizeof(KmppObjImpl));
        return ret;
    }

    impl->name_check = def->name_check;
    impl->def = def;
    impl->trie = def->trie;
    impl->need_free = 1;
    impl->shm = (KmppShmPtr *)uptr;
    impl->entry = uptr + objs->entry_offset;

    /* write userspace object address to share memory userspace private value */
    *(RK_U64 *)U64_TO_PTR(sptr->uaddr + objs->priv_offset) = (RK_U64)(intptr_t)impl;

    *obj = impl;

    return MPP_OK;
}

rk_s32 kmpp_obj_put(KmppObj obj)
{
    if (obj) {
        KmppObjImpl *impl = (KmppObjImpl *)obj;

        if (impl->shm) {
            KmppObjIocArg *ioc = alloca(sizeof(KmppObjIocArg) + sizeof(KmppShmPtr));
            rk_s32 ret;

            ioc->count = 1;
            ioc->flag = 0;
            ioc->obj_sptr[0].uaddr = impl->shm->uaddr;
            ioc->obj_sptr[0].kaddr = impl->shm->kaddr;

            ret = ioctl(objs->fd, KMPP_SHM_IOC_PUT_SHM, ioc);
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

rk_s32 kmpp_obj_ioctl(KmppObj obj, rk_s32 cmd, KmppObj in, KmppObj out)
{
    KmppObjIocArg *ioc_arg;
    KmppObjImpl *ioc = NULL;
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret;
    rk_s32 fd;

    ret = kmpp_obj_get_by_name((KmppObj *)&ioc, "KmppIoc");
    if (ret) {
        mpp_loge("failed to get KmppIoc ret %d\n", ret);
        return MPP_NOK;
    }

    fd = open("/dev/kmpp_ioctl", O_RDWR);
    if (fd < 0) {
        mpp_loge("failed to open /dev/kmpp_ioctl ret %d\n", fd);
        return MPP_NOK;
    }

    ioc_arg = alloca(sizeof(KmppObjIocArg) + sizeof(KmppShmPtr));
    ioc_arg->count = 1;
    ioc_arg->flag = 0;
    ioc_arg->obj_sptr[0].uaddr = ioc->shm->uaddr;
    ioc_arg->obj_sptr[0].kaddr = ioc->shm->kaddr;

    obj_dbg_ioctl("ioctl arg %p obj_sptr [u:k] %llx : %llx\n", ioc_arg,
                  ioc_arg->obj_sptr[0].uaddr, ioc_arg->obj_sptr[0].kaddr);

    obj_dbg_ioctl("ioctl def %s - %d cmd %d\n", impl->def->name, impl->def->index, cmd);

    kmpp_obj_set_u32(ioc, "def", impl->def->index);
    kmpp_obj_set_u32(ioc, "cmd", cmd);
    kmpp_obj_set_u32(ioc, "flag", 0);
    kmpp_obj_set_u32(ioc, "id", 0);

    if (in) {
        KmppObjImpl *impl_in = (KmppObjImpl *)in;

        kmpp_obj_set_shm(ioc, "in", impl_in->shm);
        obj_dbg_ioctl("ioctl [u:k] in %#llx : %#llx\n",
                      impl_in->shm->uaddr, impl_in->shm->kaddr);
    }
    if (out) {
        KmppObjImpl *impl_out = (KmppObjImpl *)out;

        kmpp_obj_set_shm(ioc, "out", impl_out->shm);
        obj_dbg_ioctl("ioctl [u:k] in %#llx : %#llx\n",
                      impl_out->shm->uaddr, impl_out->shm->kaddr);
    }

    ret = ioctl(fd, 0, ioc_arg);

    kmpp_obj_put(ioc);

    close(fd);

    return ret;
}

KmppShmPtr *kmpp_obj_to_shm(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (!impl) {
        mpp_loge("invalid obj %p\n", obj);
        return NULL;
    }

    return impl->shm;
}

rk_s32 kmpp_obj_to_shm_size(KmppObj obj)
{
    (void)obj;
    return sizeof(KmppShmPtr);
}

const char *kmpp_obj_get_name(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (impl && impl->def && impl->def->name)
        return impl->def->name;

    return NULL;
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
            KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info);

            if (tbl)
                return tbl->tbl.elem_offset;
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
                KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info); \
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
                KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info); \
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
                KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info); \
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
                KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info); \
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
    rk_s32 kmpp_obj_tbl_set_##type(KmppObj obj, KmppEntry *tbl, base_type val) \
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
    rk_s32 kmpp_obj_tbl_get_##type(KmppObj obj, KmppEntry *tbl, base_type *val) \
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
    rk_s32 kmpp_obj_tbl_set_##type(KmppObj obj, KmppEntry *tbl, base_type *val) \
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
    rk_s32 kmpp_obj_tbl_get_##type(KmppObj obj, KmppEntry *tbl, base_type *val) \
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

rk_s32 kmpp_obj_set_shm_obj(KmppObj obj, const char *name, KmppObj val)
{
    rk_s32 ret = MPP_NOK;

    if (!obj || !name || !val) {
        mpp_loge_f("obj %p set shm obj %s to %p failed invalid param\n",
                   obj, name, val);
    } else {
        KmppShmPtr *sptr = kmpp_obj_to_shm(val);

        if (!sptr) {
            mpp_loge_f("obj %p found invalid shm ptr\n", val);
        } else {
            ret = kmpp_obj_set_shm(obj, name, sptr);
        }
    }

    return ret;
}

rk_s32 kmpp_obj_get_shm_obj(KmppObj obj, const char *name, KmppObj *val)
{
    rk_s32 ret = MPP_NOK;

    if (!obj || !name || !val) {
        mpp_loge_f("obj %p get shm obj %s to %p failed invalid param\n",
                   obj, name, val);
    } else {
        KmppObjImpl *impl = (KmppObjImpl *)obj;
        KmppShmPtr sptr = {0};

        *val = NULL;

        ret = kmpp_obj_get_shm(obj, name, &sptr);
        if (ret || !sptr.uptr) {
            mpp_loge_f("obj %p get shm %s failed ret %d\n", impl, name, ret);
        } else {
            ret = kmpp_obj_get_by_sptr(val, &sptr);
        }
    }

    return ret;
}

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
            KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info);

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
            KmppEntry *tbl = (KmppEntry *)mpp_trie_info_ctx(info);
            rk_s32 idx = i++;

            switch (tbl->tbl.elem_type) {
            case ELEM_TYPE_s32 : {
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
            case ELEM_TYPE_u32 : {
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
            case ELEM_TYPE_s64 : {
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
            case ELEM_TYPE_u64 : {
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
            case ELEM_TYPE_st : {
                void *val_chk = mpp_malloc_size(void, tbl->tbl.elem_size);
                void *val = mpp_malloc_size(void, tbl->tbl.elem_size);
                rk_s32 data_size = tbl->tbl.elem_size;
                char logs[128];

                ret = kmpp_obj_tbl_get_st(obj, tbl, val);
                if (!ret) {
                    rk_s32 pos;

                    mpp_logi("%-2d - %-16s st  %d:%d\n",
                             idx, name, tbl->tbl.elem_offset, data_size);

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
                if (memcmp(val, val_chk, tbl->tbl.elem_size)) {
                    mpp_loge("%-2d - %-16s st  check failed\n", idx, name);
                    mpp_loge("val     %p\n", val);
                    mpp_loge("val_chk %p\n", val_chk);
                }

                MPP_FREE(val);
                MPP_FREE(val_chk);
            } break;
            case ELEM_TYPE_shm : {
                KmppShmPtr *val_chk = mpp_malloc_size(void, tbl->tbl.elem_size);
                KmppShmPtr *val = mpp_malloc_size(void, tbl->tbl.elem_size);

                ret = kmpp_obj_tbl_get_st(obj, tbl, val);
                if (!ret)
                    mpp_logi("%-2d - %-16s shm u%#llx:k%#llx\n",
                             idx, name, val->uaddr, val->kaddr);
                else
                    mpp_loge("%-2d - %-16s shm get failed\n", idx, name);

                kmpp_obj_get_st(obj, name, val_chk);
                if (memcmp(val, val_chk, tbl->tbl.elem_size)) {
                    mpp_loge("%-2d - %-16s shm check failed\n", idx, name);
                    mpp_loge("val     %p - %#llx:%#llx\n", val, val->uaddr, val->kaddr);
                    mpp_loge("val_chk %p - %#llx:%#llx\n", val_chk, val_chk->uaddr, val_chk->kaddr);
                }

                MPP_FREE(val);
                MPP_FREE(val_chk);
            } break;
            case ELEM_TYPE_uptr : {
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
            case ELEM_TYPE_ufp : {
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
                mpp_loge("%-2d - %-16s found invalid type %d\n", idx, name, tbl->tbl.elem_type);
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

    ret = ioctl(objs->fd, KMPP_SHM_IOC_DUMP, impl->shm);
    if (ret)
        mpp_err("ioctl KMPP_SHM_IOC_DUMP failed ret %d\n", ret);

    return ret ? MPP_NOK : MPP_OK;
}
