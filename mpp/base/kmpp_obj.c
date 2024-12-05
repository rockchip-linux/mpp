/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp_obj"

#include <string.h>
#include <sys/ioctl.h>

#include "mpp_list.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_trie.h"
#include "kmpp_obj_impl.h"

#define KMPP_SHM_IOC_MAGIC              'm'
#define KMPP_SHM_IOC_QUERY_INFO         _IOW(KMPP_SHM_IOC_MAGIC, 1, unsigned int)
#define KMPP_SHM_IOC_GET_SHM            _IOW(KMPP_SHM_IOC_MAGIC, 2, unsigned int)
#define KMPP_SHM_IOC_PUT_SHM            _IOW(KMPP_SHM_IOC_MAGIC, 3, unsigned int)
#define KMPP_SHM_IOC_DUMP               _IOW(KMPP_SHM_IOC_MAGIC, 4, unsigned int)

#define MPP_OBJ_DBG_SET                 (0x00000001)
#define MPP_OBJ_DBG_GET                 (0x00000002)

#define kmpp_obj_dbg(flag, fmt, ...)    _mpp_dbg(kmpp_obj_debug, flag, fmt, ## __VA_ARGS__)

#define kmpp_obj_dbg_set(fmt, ...)      kmpp_obj_dbg(MPP_OBJ_DBG_SET, fmt, ## __VA_ARGS__)
#define kmpp_obj_dbg_get(fmt, ...)      kmpp_obj_dbg(MPP_OBJ_DBG_GET, fmt, ## __VA_ARGS__)

#define U64_TO_PTR(ptr)                 ((void *)(intptr_t)(ptr))

#define ENTRY_TO_PTR(tbl, entry)        ((char *)entry + tbl->data_offset)
#define ENTRY_TO_s32_PTR(tbl, entry)    ((rk_s32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u32_PTR(tbl, entry)    ((rk_u32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_s64_PTR(tbl, entry)    ((rk_s64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u64_PTR(tbl, entry)    ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_ptr_PTR(tbl, entry)    ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_fp_PTR(tbl, entry)     ((void **)ENTRY_TO_PTR(tbl, entry))

#define ENTRY_TO_FLAG_PTR(tbl, entry)   ((rk_u16 *)((char *)entry + tbl->flag_offset))

typedef struct KmppObjDefImpl_t {
    struct list_head list;
    rk_s32 ref_cnt;
    rk_s32 entry_size;
    MppTrie trie;
    rk_s32 obj_fd;
    KmppObjDump dump;
    const char *name_check;            /* for object name address check */
    char *name;
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

//static rk_u32 kmpp_obj_debug = MPP_OBJ_DBG_SET | MPP_OBJ_DBG_GET;
static rk_u32 kmpp_obj_debug = 0;
static LIST_HEAD(kmpp_obj_list);

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
        if (!tbl->flag_type) { \
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
MPP_OBJ_ACCESS_IMPL(s64, rk_s64, % llx)
MPP_OBJ_ACCESS_IMPL(u64, rk_u64, % llx)
MPP_OBJ_ACCESS_IMPL(ptr, void *, % p)
MPP_OBJ_ACCESS_IMPL(fp, void *, % p)

rk_s32 kmpp_obj_impl_set_st(KmppLocTbl *tbl, void *entry, void *val)
{
    char *dst = ENTRY_TO_PTR(tbl, entry);

    if (!tbl->flag_type) {
        /* simple copy */
        kmpp_obj_dbg_set("%p + %x set struct change %p -> %p\n", entry, tbl->data_offset, dst, val);
        memcpy(dst, val, tbl->data_size);
        return MPP_OK;
    }

    /* copy with flag check and updata */
    if (memcmp(dst, val, tbl->data_size)) {
        kmpp_obj_dbg_set("%p + %x set struct update %p -> %p flag %d|%x\n",
                         entry, tbl->data_offset, dst, val, tbl->flag_offset, tbl->flag_value);
        memcpy(dst, val, tbl->data_size);
        ENTRY_TO_FLAG_PTR(tbl, entry)[0] |= tbl->flag_value;
    } else {
        kmpp_obj_dbg_set("%p + %x set struct keep   %p\n", entry, tbl->data_offset, dst);
    }

    return MPP_OK;
}

rk_s32 kmpp_obj_impl_get_st(KmppLocTbl *tbl, void *entry, void *val)
{
    if (tbl && tbl->data_size) {
        void *src = (void *)ENTRY_TO_PTR(tbl, entry);

        kmpp_obj_dbg_get("%p + %d get st from %p\n", entry, tbl->data_offset, src);
        memcpy(val, src, tbl->data_size);
        return MPP_OK;
    }

    return MPP_NOK;
}

static void show_entry_tbl_err(KmppLocTbl *tbl, EntryType type, const char *func, const char *name)
{
    mpp_loge("%s entry %s expect %s input NOT %s\n", func, name,
             strof_entry_type(tbl->data_type), strof_entry_type(type));
}

rk_s32 check_entry_tbl(KmppLocTbl *tbl, const char *name, EntryType type,
                       const char *func)
{
    EntryType entry_type;
    rk_s32 entry_size;
    rk_s32 ret;

    if (!tbl) {
        mpp_loge("%s: entry %s is invalid\n", func, name);
        return MPP_NOK;
    }

    entry_type = tbl->data_type;
    entry_size = tbl->data_size;
    ret = MPP_OK;

    switch (type) {
    case ENTRY_TYPE_st : {
        if (entry_type != type) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = MPP_NOK;
        }
        if (entry_size <= 0) {
            mpp_loge("%s: entry %s found invalid size %d\n", func, name, entry_size);
            ret = MPP_NOK;
        }
    } break;
    case ENTRY_TYPE_ptr :
    case ENTRY_TYPE_fp : {
        if (entry_type != type) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    case ENTRY_TYPE_s32 :
    case ENTRY_TYPE_u32 : {
        if (entry_size != sizeof(rk_s32)) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    case ENTRY_TYPE_s64 :
    case ENTRY_TYPE_u64 : {
        if (entry_size != sizeof(rk_s64)) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = MPP_NOK;
        }
    } break;
    default : {
        mpp_loge("%s: entry %s found invalid type %d\n", func, name, type);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}

rk_s32 kmpp_objdef_init(KmppObjDef *def, const char *name)
{
    KmppObjDefImpl *impl = NULL;
    char dev_name[64];
    rk_s32 len;
    rk_s32 ret = MPP_NOK;

    if (!def || !name) {
        mpp_loge_f("invalid param def %p name %p\n", def, name);
        goto DONE;
    }

    *def = NULL;
    len = MPP_ALIGN(strlen(name) + 1, sizeof(rk_s32));
    impl = mpp_calloc_size(KmppObjDefImpl, sizeof(KmppObjDefImpl) + len);
    if (!impl) {
        mpp_loge_f("malloc contex %d failed\n", sizeof(KmppObjDefImpl) + len);
        goto DONE;
    }

    impl->name_check = name;
    impl->name = (char *)(impl + 1);
    strncpy(impl->name, name, len);
    snprintf(dev_name, sizeof(dev_name) - 1, "/dev/%s", name);
    impl->obj_fd = open(dev_name, O_RDWR);

    if (impl->obj_fd < 0) {
        mpp_loge_f("open %s failed\n", dev_name);
        goto DONE;
    } else {
        KmppObjTrie s;
        MppTrieInfo *info = NULL;
        void *root = NULL;
        RK_S32 obj_size = 0;

        ret = ioctl(impl->obj_fd, KMPP_SHM_IOC_QUERY_INFO, &s);
        if (ret) {
            mpp_err_f("%d ioctl KMPP_SHM_IOC_QUERY_INFO failed\n", impl->obj_fd);
            goto DONE;
        }

        root = U64_TO_PTR(s.trie_root);
        info = mpp_trie_get_info_from_root(root, "__size");
        obj_size = info ? *(RK_S32 *)mpp_trie_info_ctx(info) : 0;

        ret = mpp_trie_init(&impl->trie, name);
        if (ret) {
            mpp_err_f("class %s init trie failed\n", name);
            goto DONE;
        }

        ret = mpp_trie_import(impl->trie, root);
        if (ret) {
            mpp_err_f("class %s import trie failed\n", name);
            goto DONE;
        }

        impl->entry_size = obj_size;
    }

    *def = impl;

    INIT_LIST_HEAD(&impl->list);
    list_add_tail(&impl->list, &kmpp_obj_list);
    impl->ref_cnt++;

    ret = MPP_OK;
DONE:
    if (ret && impl) {
        kmpp_objdef_deinit(impl);
        impl = NULL;
    }
    return ret;
}

rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const char *name, KmppLocTbl **tbl)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = MPP_NOK;

    if (impl->trie) {
        MppTrieInfo *info = mpp_trie_get_info(impl->trie, name);

        if (info) {
            *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);
            ret = MPP_OK;
        }
    }

    if (ret)
        mpp_loge_f("class %s get entry %s failed ret %d\n",
                   impl ? impl->name : NULL, name, ret);

    return ret;
}

rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

        impl->dump = dump;
        return MPP_OK;
    }

    return MPP_NOK;
}

rk_s32 kmpp_objdef_deinit(KmppObjDef def)
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

            list_del_init(&impl->list);

            if (impl->obj_fd > 0) {
                close(impl->obj_fd);
                impl->obj_fd = -1;
            }

            mpp_free(impl);
        }

        ret = MPP_OK;
    }

    return ret;
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

rk_u32 kmpp_objdef_lookup(KmppObjDef *def, const char *name)
{
    KmppObjDefImpl *impl = NULL;
    KmppObjDefImpl *n = NULL;

    list_for_each_entry_safe(impl, n, &kmpp_obj_list, KmppObjDefImpl, list) {
        if (strcmp(impl->name, name) == 0) {
            impl->ref_cnt++;
            *def = impl;
            return MPP_OK;
        }
    }

    *def = NULL;

    return MPP_NOK;
}

rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def)
{
    KmppObjImpl *impl;
    KmppObjDefImpl *def_impl;
    KmppObjShm *shm = NULL;
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

    ret = ioctl(def_impl->obj_fd, KMPP_SHM_IOC_GET_SHM, &shm);
    if (ret) {
        mpp_err("%s fd %d ioctl KMPP_SHM_IOC_GET_SHM failed\n",
                def_impl->name, def_impl->obj_fd);
        mpp_free(impl);
        return ret;
    }

    impl->name_check = def_impl->name_check;
    impl->def = def;
    impl->trie = def_impl->trie;
    impl->need_free = 1;
    impl->shm = shm;
    impl->entry = U64_TO_PTR(shm->kobj_uaddr);

    *obj = impl;

    return MPP_OK;
}

rk_s32 kmpp_obj_put(KmppObj obj)
{
    if (obj) {
        KmppObjImpl *impl = (KmppObjImpl *)obj;
        KmppObjDefImpl *def = impl->def;

        if (impl->shm) {
            rk_s32 ret = ioctl(def->obj_fd, KMPP_SHM_IOC_PUT_SHM, impl->shm);
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

void *kmpp_obj_get_entry(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    return impl ? impl->entry : NULL;
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
        return MPP_NOK; \
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
        return MPP_NOK; \
    }

MPP_OBJ_ACCESS(s32, rk_s32)
MPP_OBJ_ACCESS(u32, rk_u32)
MPP_OBJ_ACCESS(s64, rk_s64)
MPP_OBJ_ACCESS(u64, rk_u64)
MPP_OBJ_ACCESS(ptr, void *)
MPP_OBJ_ACCESS(fp, void *)

rk_s32 kmpp_obj_set_st(KmppObj obj, const char *name, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = MPP_NOK;

    if (impl->trie) {
        MppTrieInfo *info = mpp_trie_get_info(impl->trie, name);

        if (info) {
            KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);
            ret = kmpp_obj_impl_set_st(tbl, impl->entry, val);
        }
    }

    if (ret)
        mpp_loge("obj %s set %s st failed ret %d\n",
             impl ? impl->def ? impl->def->name : NULL : NULL, name, ret);

    return MPP_NOK;
}

rk_s32 kmpp_obj_get_st(KmppObj obj, const char *name, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = MPP_NOK;

    if (impl->trie) {
        MppTrieInfo *info = mpp_trie_get_info(impl->trie, name);

        if (info) {
            KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);
            ret = kmpp_obj_impl_get_st(tbl, impl->entry, val);
        }
    }

    if (ret)
        mpp_loge("obj %s get %s st failed ret %d\n",
             impl ? impl->def ? impl->def->name : NULL : NULL, name, ret);

    return MPP_NOK;
}

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
        return MPP_NOK; \
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
        return MPP_NOK; \
    }

MPP_OBJ_TBL_ACCESS(s32, rk_s32)
MPP_OBJ_TBL_ACCESS(u32, rk_u32)
MPP_OBJ_TBL_ACCESS(s64, rk_s64)
MPP_OBJ_TBL_ACCESS(u64, rk_u64)
MPP_OBJ_TBL_ACCESS(ptr, void *)
MPP_OBJ_TBL_ACCESS(fp, void *)

rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppLocTbl *tbl, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = MPP_NOK;

    if (impl)
        ret = kmpp_obj_impl_set_st(tbl, impl->entry, val);

    if (ret)
        mpp_loge("obj %s tbl %08x set %s st failed ret %d\n",
             impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret);

    return MPP_NOK;
}

rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppLocTbl *tbl, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = MPP_NOK;

    if (impl)
        ret = kmpp_obj_impl_get_st(tbl, impl->entry, val);

    if (ret)
        mpp_loge("obj %s tbl %08x get %s st failed ret %d\n",
             impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret);

    return MPP_NOK;
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
            KmppLocTbl *tbl = (KmppLocTbl *)mpp_trie_info_ctx(info);

            ret = kmpp_obj_impl_get_fp(tbl, impl->entry, &val);
        }

        if (val)
            ret = kmpp_obj_impl_run(val, impl->entry);
    }

    return ret;
}

rk_s32 kmpp_obj_dump(KmppObj obj, const char *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (impl && impl->def && impl->def->dump) {
        mpp_logi_f("%s obj %p entry %p from %s\n",
                   impl->def->name, impl, impl->entry, caller);
        return impl->def->dump(impl);
    }

    return MPP_NOK;
}
