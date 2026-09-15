/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_cfg_io"

#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "mpp_bit.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_trie.h"
#include "mpp_cfg.h"
#include "mpp_cfg_io.h"
#include "rk_venc_cfg.h"
#include "kmpp_obj.h"

#define MAX_CFG_DEPTH                   (64)
#define CFG_IO_ARRAY_ELEM_COUNT         (8)
#define VLA_INIT_CNT                    (8)

#define CFG_IO_DBG_FLOW                 (0x00000001)
#define CFG_IO_DBG_BYTE                 (0x00000002)
#define CFG_IO_DBG_TO                   (0x00000004)
#define CFG_IO_DBG_FROM                 (0x00000008)
#define CFG_IO_DBG_FREE                 (0x00000010)
#define CFG_IO_DBG_NAME                 (0x00000020)
#define CFG_IO_DBG_SHOW                 (0x00000040)
#define CFG_IO_DBG_INFO                 (0x00000080)

#define cfg_io_dbg(flag, fmt, ...)      mpp_dbg(mpp_cfg_io_debug, flag, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_f(flag, fmt, ...)    mpp_dbg_f(mpp_cfg_io_debug, flag, fmt, ## __VA_ARGS__)

#define cfg_io_dbg_flow(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_FLOW, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_byte(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_BYTE, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_to(fmt, ...)         cfg_io_dbg(CFG_IO_DBG_TO, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_from(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_FROM, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_free(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_FREE, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_name(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_NAME, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_show(fmt, ...)       cfg_io_dbg_f(CFG_IO_DBG_SHOW, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_info(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_INFO, fmt, ## __VA_ARGS__)

#define VLA_SIMPLE_TYPE                 (MPP_BIT32_OR(MPP_CFG_TYPE_BOOL) | \
                                        MPP_BIT32_OR(MPP_CFG_TYPE_s8, MPP_CFG_TYPE_u8)  | \
                                        MPP_BIT32_OR(MPP_CFG_TYPE_s16, MPP_CFG_TYPE_u16) | \
                                        MPP_BIT32_OR(MPP_CFG_TYPE_s32, MPP_CFG_TYPE_u32) | \
                                        MPP_BIT32_OR(MPP_CFG_TYPE_s64, MPP_CFG_TYPE_u64) | \
                                        MPP_BIT32_OR(MPP_CFG_TYPE_f32, MPP_CFG_TYPE_f64))
#define VLA_COMPLEX_TYPE                (MPP_BIT32_OR(MPP_CFG_TYPE_STRING, MPP_CFG_TYPE_RAW) | \
                                        MPP_BIT32_OR(MPP_CFG_TYPE_OBJECT, MPP_CFG_TYPE_ARRAY) | \
                                        MPP_BIT32_OR(MPP_CFG_TYPE_NULL, MPP_CFG_TYPE_BUTT))

#define IS_VLA_SIMPLE_TYPE(type)        (type > 0 && (MPP_BIT(type) & VLA_SIMPLE_TYPE) > 0)
#define IS_VLA_COMPLEX_TYPE(type)       (type > 0 && (MPP_BIT(type) & VLA_COMPLEX_TYPE) > 0)

typedef enum MppCfgParserType_e {
    MPP_CFG_PARSER_TYPE_KEY = 0,
    MPP_CFG_PARSER_TYPE_VALUE,
    MPP_CFG_PARSER_TYPE_TABLE,
    MPP_CFG_PARSER_TYPE_ARRAY_TABLE,
    MPP_CFG_PARSER_TYPE_BUTT,
} MppCfgParserType;

typedef struct MppCfgIoImpl_t MppCfgIoImpl;
typedef void (*MppCfgIoFunc)(MppCfgIoImpl *obj, void *data);

/*
 * cfg node tree roles: data trees keep fields in child, template trees keep
 * them in detail.
 *
 *   data tree (child)                      template tree (detail)
 *   +----------------------------+         +-----------------------------+
 *   | mpp_cfg_from_string parse  |         | objdef macro-built cfg_root |
 *   | hand-built via mpp_cfg_add |         | mpp_cfg_from_trie output    |
 *   |                            |         |                             |
 *   | used by: mpp_cfg_to_string |         | used by: apply / extract    |
 *   | (serialize)                |         | read_struct walks detail    |
 *   |                            |         | type_find_child walks       |
 *   +----------------------------+         |   child then detail         |
 *                                          +-----------------------------+
 */
struct MppCfgIoImpl_t {
    /* list for bothers */
    struct list_head        list;
    /* list for children */
    struct list_head        child;
    /* list for VLA complex element sub-struct detail description info */
    struct list_head        detail;
    /* parent of current object */
    MppCfgIoImpl            *parent;

    MppCfgType              type;
    MppCfgVal               val;

    rk_s32                  buf_size;
    /* depth in tree */
    rk_s32                  depth;

    /* internal name storage */
    char                    *name;
    rk_s32                  name_len;
    rk_s32                  name_buf_len;

    rk_s32                  array_count;

    /* location entry for structure access */
    MppTrie                 trie;
    KmppEntry               entry;

    /* varialble length array info */
    KmppEntry               vla;
    /* array_type only valid on add_raw function */
    MppCfgType              array_type;

    union {
        void               *ptr;
        /* MPP_CFG_TYPE_STRING */
        struct {
            char            *string;
            rk_s32          str_len;
        };
        /* MPP_CFG_TYPE_ARRAY - complex object array */
        struct {
            MppCfgIoImpl    **elems;
            rk_s32          array_size;
        };
        /* MPP_CFG_TYPE_ARRAY - simple array (s8/u8/s16/u16/s32/u32/s64/u64/f32/f64) */
        struct {
            void            *raw;
            rk_u32          raw_size    : 16;
            rk_u32          raw_count   : 16;
        };
    };
};

typedef struct MppCfgStrBuf_t {
    char *buf;
    rk_s32 buf_size;
    rk_s32 offset;
    rk_s32 depth;
    MppCfgStrFmt type;
} MppCfgStrBuf;

static const char *strof_type(MppCfgType type)
{
    static const char *str[MPP_CFG_TYPE_BUTT + 1] = {
        [MPP_CFG_TYPE_INVALID] = "invalid",
        [MPP_CFG_TYPE_NULL] = "null",
        [MPP_CFG_TYPE_BOOL] = "bool",
        [MPP_CFG_TYPE_s8] = "s8",
        [MPP_CFG_TYPE_u8] = "u8",
        [MPP_CFG_TYPE_s16] = "s16",
        [MPP_CFG_TYPE_u16] = "u16",
        [MPP_CFG_TYPE_s32] = "s32",
        [MPP_CFG_TYPE_u32] = "u32",
        [MPP_CFG_TYPE_s64] = "s64",
        [MPP_CFG_TYPE_u64] = "u64",
        [MPP_CFG_TYPE_f32] = "f32",
        [MPP_CFG_TYPE_f64] = "f64",
        [MPP_CFG_TYPE_STRING] = "string",
        [MPP_CFG_TYPE_RAW] = "raw",
        [MPP_CFG_TYPE_OBJECT] = "object",
        [MPP_CFG_TYPE_ARRAY] = "array",
        [MPP_CFG_TYPE_BUTT] = "unknown",
    };

    if (type < 0 || type > MPP_CFG_TYPE_BUTT)
        type = MPP_CFG_TYPE_BUTT;

    return str[type];
}

static rk_u32 sizeof_type(MppCfgType type)
{
    static rk_u32 sizes[MPP_CFG_TYPE_BUTT + 1] = {
        [MPP_CFG_TYPE_INVALID] = 0,
        [MPP_CFG_TYPE_NULL] = 0,
        [MPP_CFG_TYPE_BOOL] = sizeof(rk_bool),
        [MPP_CFG_TYPE_s8] = sizeof(rk_s8),
        [MPP_CFG_TYPE_u8] = sizeof(rk_u8),
        [MPP_CFG_TYPE_s16] = sizeof(rk_s16),
        [MPP_CFG_TYPE_u16] = sizeof(rk_u16),
        [MPP_CFG_TYPE_s32] = sizeof(rk_s32),
        [MPP_CFG_TYPE_u32] = sizeof(rk_u32),
        [MPP_CFG_TYPE_s64] = sizeof(rk_s64),
        [MPP_CFG_TYPE_u64] = sizeof(rk_u64),
        [MPP_CFG_TYPE_f32] = sizeof(rk_float),
        [MPP_CFG_TYPE_f64] = sizeof(rk_double),
        [MPP_CFG_TYPE_STRING] = 0,
        [MPP_CFG_TYPE_RAW] = 0,
        [MPP_CFG_TYPE_OBJECT] = 0,
        [MPP_CFG_TYPE_ARRAY] = 0,
        [MPP_CFG_TYPE_BUTT] = 0,
    };

    if (type < 0 || type > MPP_CFG_TYPE_BUTT)
        type = MPP_CFG_TYPE_BUTT;

    return sizes[type];
}

static ElemType mpp_cfg_type_to_elem_type(MppCfgType type)
{
    static const ElemType elem_types[MPP_CFG_TYPE_BUTT + 1] = {
        [MPP_CFG_TYPE_INVALID] = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_NULL]    = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_BOOL]    = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_s8]      = ELEM_TYPE_s8,
        [MPP_CFG_TYPE_u8]      = ELEM_TYPE_u8,
        [MPP_CFG_TYPE_s16]     = ELEM_TYPE_s16,
        [MPP_CFG_TYPE_u16]     = ELEM_TYPE_u16,
        [MPP_CFG_TYPE_s32]     = ELEM_TYPE_s32,
        [MPP_CFG_TYPE_u32]     = ELEM_TYPE_u32,
        [MPP_CFG_TYPE_s64]     = ELEM_TYPE_s64,
        [MPP_CFG_TYPE_u64]     = ELEM_TYPE_u64,
        [MPP_CFG_TYPE_f32]     = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_f64]     = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_STRING]  = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_RAW]     = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_OBJECT]  = ELEM_TYPE_BUTT,
        [MPP_CFG_TYPE_ARRAY]   = ELEM_TYPE_arr,
        [MPP_CFG_TYPE_BUTT]    = ELEM_TYPE_BUTT,
    };

    if (type < 0 || type > MPP_CFG_TYPE_BUTT)
        type = MPP_CFG_TYPE_BUTT;

    return elem_types[type];
}

MppCfgType mpp_cfg_type_from_elem_type(ElemType type)
{
    static const MppCfgType cfg_types[ELEM_TYPE_BUTT] = {
        [ELEM_TYPE_s32] = MPP_CFG_TYPE_s32,
        [ELEM_TYPE_u32] = MPP_CFG_TYPE_u32,
        [ELEM_TYPE_s64] = MPP_CFG_TYPE_s64,
        [ELEM_TYPE_u64] = MPP_CFG_TYPE_u64,
        [ELEM_TYPE_arr] = MPP_CFG_TYPE_ARRAY,
        [ELEM_TYPE_s8]  = MPP_CFG_TYPE_s8,
        [ELEM_TYPE_u8]  = MPP_CFG_TYPE_u8,
        [ELEM_TYPE_s16] = MPP_CFG_TYPE_s16,
        [ELEM_TYPE_u16] = MPP_CFG_TYPE_u16,
    };

    if (type < 0 || type >= ELEM_TYPE_BUTT)
        return MPP_CFG_TYPE_BUTT;

    /* unmapped entries default to MPP_CFG_TYPE_INVALID (0), which behaves the
     * same as BUTT for callers (both fail the valid-type range check) */
    return cfg_types[type];
}

static void mpp_cfg_set_flag(void *st, rk_u32 flag_offset)
{
    rk_u32 *flag_ptr = (rk_u32 *)((rk_u8 *)st + (((rk_u32)flag_offset & ~31U) / 8));

    *flag_ptr |= 1U << (flag_offset & 31);
}

static char *dup_str(const char *str, rk_s32 len)
{
    char *ret = NULL;

    if (str && len > 0) {
        ret = mpp_calloc_size(char, len + 1);
        if (ret) {
            memcpy(ret, str, len);
            ret[len] = '\0';
        }
    }

    return ret;
}

static rk_s32 get_full_name(MppCfgIoImpl *obj, char *buf, rk_s32 buf_size)
{
    MppCfgIoImpl *curr = obj;
    char *name[MAX_CFG_DEPTH];
    char *delmiter = ":";
    rk_s32 depth = 0;
    rk_s32 len = 0;
    rk_s32 i = 0;

    while (curr && curr->parent) {
        /* skip the root */
        if (curr->name) {
            /* Add delimiter on object / array only when it has a named
             * descendant already in the stack, so leaf arrays (simple arrays
             * with unnamed elements) do not end up with a trailing ':' */
            if (i > 0 && curr->type >= MPP_CFG_TYPE_OBJECT)
                name[i++] = delmiter;

            name[i++] = curr->name;
        }

        curr = curr->parent;
        depth++;

        if (i >= MAX_CFG_DEPTH) {
            mpp_loge_f("too deep depth %2d\n", depth);
            return 0;
        }
    }

    if (!i) {
        buf[0] = '\0';
        return 0;
    }

    depth = i;
    for (i = depth - 1; i >= 0; i--) {
        len += snprintf(buf + len, buf_size - len, "%s", name[i]);

        if (len >= buf_size) {
            mpp_loge_f("buffer overflow len %d buf_size %d\n", len, buf_size);
            break;
        }
    }

    cfg_io_dbg_name("depth %2d obj %-16s -> %s\n", obj->depth, obj->name, buf);

    return len;
}

void loop_all_children(MppCfgIoImpl *impl, MppCfgIoFunc func, void *data)
{
    MppCfgIoImpl *pos, *n;

    func(impl, data);

    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        loop_all_children(pos, func, data);
    }
}

void loop_all_detail(MppCfgIoImpl *impl, MppCfgIoFunc func, void *data)
{
    MppCfgIoImpl *pos, *n;

    func(impl, data);

    list_for_each_entry_safe(pos, n, &impl->detail, MppCfgIoImpl, list) {
        loop_all_detail(pos, func, data);
    }
}

rk_s32 mpp_cfg_get_object(MppCfgObj *obj, const char *name, MppCfgType type, MppCfgVal *val)
{
    MppCfgIoImpl *impl = NULL;
    rk_s32 name_buf_len = 0;
    rk_s32 name_len = 0;
    rk_s32 buf_size = 0;
    rk_s32 str_len = 0;

    if (!obj || type <= MPP_CFG_TYPE_INVALID || type >= MPP_CFG_TYPE_BUTT) {
        mpp_loge_f("invalid param obj %p name %s type %d val %p\n", obj, name, type, val);
        return rk_nok;
    }

    if (*obj)
        mpp_logw_f("obj %p overwrite\n", *obj);

    *obj = NULL;

    if (name) {
        name_len = strlen(name);
        name_buf_len = MPP_ALIGN(name_len + 1, 4);
    }

    if (type == MPP_CFG_TYPE_STRING && val && val->str)
        str_len = MPP_ALIGN(strlen(val->str) + 1, 4);

    buf_size = sizeof(MppCfgIoImpl) + name_buf_len + str_len;
    impl = mpp_calloc_size(MppCfgIoImpl, buf_size);

    if (!impl) {
        mpp_loge_f("failed to alloc impl size %d\n", buf_size);
        return rk_nok;
    }

    INIT_LIST_HEAD(&impl->list);
    INIT_LIST_HEAD(&impl->child);
    INIT_LIST_HEAD(&impl->detail);

    if (name_buf_len) {
        impl->name = (char *)(impl + 1);
        memcpy(impl->name, name, name_len);
        impl->name[name_len] = '\0';
        impl->name_len = name_len;
        impl->name_buf_len = name_buf_len;
    }

    if (str_len) {
        impl->string = (char *)(impl + 1) + name_buf_len;
        strncpy(impl->string, val->str, str_len);
        impl->str_len = str_len;
    }

    impl->type = type;
    if (val)
        impl->val = *val;
    impl->buf_size = buf_size;
    /* set invalid data type by default */
    impl->entry.tbl.elem_type = ELEM_TYPE_BUTT;

    if (type == MPP_CFG_TYPE_STRING)
        impl->val.str = impl->string;

    *obj = impl;

    return rk_ok;
}

rk_s32 mpp_cfg_get_array(MppCfgObj *obj, const char *name)
{
    MppCfgIoImpl *impl = NULL;
    rk_s32 name_buf_len = 0;
    rk_s32 name_len = 0;
    rk_s32 buf_size = 0;

    if (!obj) {
        mpp_loge_f("invalid param obj %p name %s\n", obj, name);
        return rk_nok;
    } else if (*obj) {
        mpp_logw_f("obj %p overwrite\n", *obj);
        *obj = NULL;
    }

    if (name) {
        name_len = strlen(name);
        name_buf_len = MPP_ALIGN(name_len + 1, 4);
    }

    buf_size = sizeof(MppCfgIoImpl) + name_buf_len;
    impl = mpp_calloc_size(MppCfgIoImpl, buf_size);

    if (!impl) {
        mpp_loge_f("failed to alloc impl size %d\n", buf_size);
        return rk_nok;
    }

    INIT_LIST_HEAD(&impl->list);
    INIT_LIST_HEAD(&impl->child);
    INIT_LIST_HEAD(&impl->detail);

    if (name_len) {
        impl->name = (char *)(impl + 1);
        memcpy(impl->name, name, name_len);
        impl->name[name_len] = '\0';
        impl->name_len = name_len;
        impl->name_buf_len = name_buf_len;
    }

    impl->type = MPP_CFG_TYPE_ARRAY;
    impl->buf_size = buf_size;
    impl->array_count = 0;
    /* set invalid data type by default */
    impl->entry.tbl.elem_type = ELEM_TYPE_BUTT;

    *obj = impl;

    return rk_ok;
}

static void mpp_cfg_put_all_child(MppCfgIoImpl *impl)
{
    MppCfgIoImpl *pos, *n;

    cfg_io_dbg_free("depth %2d - %p free start type %d name %s\n",
                    impl->depth, impl, impl->type, impl->name);

    /* free children first */
    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        list_del_init(&pos->list);

        cfg_io_dbg_free("depth %2d - %p free child %p type %d name %s\n",
                        impl->depth, impl, pos, pos->type, pos->name);

        mpp_cfg_put_all_child(pos);
    }

    /* free VLA storage */
    if (IS_VLA_COMPLEX_TYPE(impl->array_type)) {
        rk_s32 i;

        for (i = 0; i < impl->array_size; i++) {
            if (!impl->elems[i])
                continue;

            mpp_cfg_put_all_child(impl->elems[i]);
        }

        MPP_FREE(impl->elems);
    } else if (IS_VLA_SIMPLE_TYPE(impl->array_type)) {
        MPP_FREE(impl->raw);
    }

    /* free VLA complex detail info storage */
    list_for_each_entry_safe(pos, n, &impl->detail, MppCfgIoImpl, list) {
        list_del_init(&pos->list);

        cfg_io_dbg_free("depth %2d - %p free detail %p type %d name %s\n",
                        impl->depth, impl, pos, pos->type, pos->name);

        mpp_cfg_put_all_child(pos);
    }

    cfg_io_dbg_free("depth %2d - %p free done type %d name %s\n",
                    impl->depth, impl, impl->type, impl->name);

    mpp_free(impl);
}

rk_s32 mpp_cfg_put_all(MppCfgObj obj)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;
    MppCfgIoImpl *root;

    if (!obj) {
        mpp_loge_f("invalid param obj %p\n", obj);
        return rk_nok;
    }

    if (impl->trie) {
        mpp_trie_deinit(impl->trie);
        impl->trie = NULL;
    }

    root = impl->parent;
    do {
        mpp_cfg_put_all_child(impl);

        if (!root)
            break;

        impl = root;
        root = impl->parent;
    } while (impl);

    return rk_ok;
}

static void update_depth(MppCfgIoImpl *impl, void *data)
{
    (void)data;

    if (impl->parent)
        impl->depth = impl->parent->depth + 1;
}

rk_s32 mpp_cfg_add(MppCfgObj root, MppCfgObj leaf)
{
    MppCfgIoImpl *root_impl = (MppCfgIoImpl *)root;
    MppCfgIoImpl *leaf_impl = (MppCfgIoImpl *)leaf;

    if (!root || !leaf) {
        mpp_loge_f("invalid param root %p leaf %p\n", root, leaf);
        return rk_nok;
    }

    if (root_impl->type <= MPP_CFG_TYPE_INVALID || root_impl->type >= MPP_CFG_TYPE_BUTT) {
        mpp_loge_f("obj %-16s invalid root type %d\n",
                   root_impl->name, root_impl->type);
        return rk_nok;
    }

    list_add_tail(&leaf_impl->list, &root_impl->child);
    leaf_impl->parent = root_impl;

    loop_all_children(root, update_depth, NULL);

    return rk_ok;
}

rk_s32 mpp_cfg_vla_add_raw(MppCfgObj array, rk_s32 idx, MppCfgVal *val)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)array;
    rk_s32 elem_size;

    if (!array || !val) {
        mpp_loge_f("invalid param array %p val %p\n", array, val);
        return rk_nok;
    }

    if (impl->type != MPP_CFG_TYPE_ARRAY || impl->raw == NULL) {
        mpp_loge_f("vla %-16s invalid array type %d raw buf %p\n",
                   impl->name, impl->type, impl->raw);
        return rk_nok;
    }

    if (!IS_VLA_SIMPLE_TYPE(impl->array_type) || idx < 0) {
        mpp_loge_f("vla %-16s invalid elem_type %d idx %d\n",
                   impl->name, impl->array_type, idx);
        return rk_nok;
    }

    if (impl->vla.vla.flex_count) {
        /* flexible count */
        rk_s32 count = impl->raw_count;

        if (idx >= (count * 2)) {
            /* check flexible count in double range and update */
            mpp_loge_f("vla %-16s invalid index %d flex_count %d (%d)\n",
                       impl->name, idx, count, count * 2);
            return rk_nok;
        } else if (idx >= count) {
            /* enlarge raw data buffer */
            rk_s32 old_size = impl->raw_size;
            rk_s32 new_size = old_size * 2;
            void *ptr = mpp_realloc_size(impl->raw, void, new_size);

            if (!ptr) {
                mpp_loge_f("vla %-16s realloc failed old_size %d new_size %d\n",
                           impl->name, old_size, new_size);
                return rk_nok;
            }

            memset((char *)ptr + old_size, 0, old_size);
            impl->raw = ptr;
            impl->raw_count *= 2;
            impl->raw_size = new_size;
        }
    } else if (idx >= impl->raw_count) {
        /* fix count */
        mpp_loge_f("vla %-16s invalid index %d raw data count %d size %d\n",
                   impl->name, idx, impl->raw_count, impl->raw_size);
        return rk_nok;
    }

    elem_size = sizeof_type(impl->array_type);

    {
        void *ptr = (char *)impl->raw + elem_size * idx;

        memcpy(ptr, val, elem_size);
    }

    return rk_ok;
}

rk_s32 mpp_cfg_vla_add_elem(MppCfgObj array, rk_s32 idx, MppCfgObj elem)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)array;
    KmppEntry *entry;
    rk_u32 flag;

    if (!array || !elem) {
        mpp_loge_f("invalid param array %p elem %p\n", array, elem);
        return rk_nok;
    }

    if (impl->type != MPP_CFG_TYPE_ARRAY || impl->elems == NULL) {
        mpp_loge_f("vla %-16s invalid array type %d elem buf %p\n",
                   impl->name, impl->type, impl->elems);
        return rk_nok;
    }

    if (impl->array_type != MPP_CFG_TYPE_BUTT || idx < 0) {
        mpp_loge_f("vla %-16s invalid elem_type %d idx %d\n",
                   impl->name, impl->array_type, idx);
        return rk_nok;
    }

    entry = &impl->vla;
    flag = entry->vla.flex_count;
    if (flag) {
        /* flexible count */
        rk_s32 count = entry->vla.elem_count;

        if (idx >= (count * 2)) {
            /* check flexible count in double range and update */
            mpp_loge_f("vla %-16s invalid index %d flex_count %d (%d)\n",
                       impl->name, idx, count, count * 2);
            return rk_nok;
        } else if (idx >= count) {
            /* enlarge element buffer */
            rk_s32 old_cnt = count;
            rk_s32 new_cnt = old_cnt * 2;
            rk_s32 elem_size = sizeof(MppCfgIoImpl *);
            void *ptr = mpp_realloc_size(impl->elems, void, elem_size * new_cnt);

            if (!ptr) {
                mpp_loge_f("vla %-16s realloc failed old_cnt %d new_cnt %d\n",
                           impl->name, old_cnt, new_cnt);
                return rk_nok;
            }

            memset((char *)ptr + elem_size * old_cnt, 0, elem_size * old_cnt);
            impl->elems = ptr;
            impl->array_size = new_cnt;
            entry->vla.elem_count = new_cnt;
        }
    } else if (idx >= impl->array_size) {
        /* fix count */
        mpp_loge_f("vla %-16s invalid index %d array_size %d\n",
                   impl->name, idx, impl->array_size);
        return rk_nok;
    }

    if (impl->elems[idx]) {
        mpp_loge_f("vla %-16s overwrite element %d from %p -> %p\n",
                   impl->name, idx, impl->elems[idx], elem);
    }

    impl->elems[idx] = (MppCfgIoImpl *)elem;

    return rk_ok;
}

rk_s32 mpp_cfg_add_detail(MppCfgObj root, MppCfgObj detail)
{
    MppCfgIoImpl *root_impl = (MppCfgIoImpl *)root;
    MppCfgIoImpl *detail_impl = (MppCfgIoImpl *)detail;

    if (!root || !detail) {
        mpp_loge_f("invalid param root %p detail %p\n", root, detail);
        return rk_nok;
    }

    if (root_impl->type <= MPP_CFG_TYPE_INVALID || root_impl->type >= MPP_CFG_TYPE_BUTT) {
        mpp_loge_f("obj %-16s invalid detail root type %d\n",
                   root_impl->name, root_impl->type);
        return rk_nok;
    }

    list_add_tail(&detail_impl->list, &root_impl->detail);
    detail_impl->parent = root_impl;

    loop_all_detail(root, update_depth, NULL);

    return rk_ok;
}

rk_s32 mpp_cfg_find(MppCfgObj *obj, MppCfgObj root, char *name, rk_s32 type)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)root;
    rk_s32 str_start = 0;
    rk_s32 str_len = 0;
    rk_s32 i;
    char delimiter;

    if (!obj || !root || !name) {
        mpp_loge_f("invalid param obj %p root %p name %s\n", obj, root, name);
        return rk_nok;
    }

    delimiter = (type == MPP_CFG_STR_FMT_TOML) ? '.' : ':';
    str_len = strlen(name);

    for (i = 0; i <= str_len; i++) {
        if (name[i] == delimiter || name[i] == '\0') {
            MppCfgIoImpl *pos, *n;
            MppCfgIoImpl *last_array = NULL;
            char bak = name[i];
            rk_s32 found = 0;

            name[i] = '\0';
            mpp_logi("try match %s\n", name + str_start);
            list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
                if (pos->name && !strcmp(pos->name, name + str_start)) {
                    impl = pos;
                    found = 1;
                    break;
                }

                /* if impl is array, find impl->chil is object and has no name, to match its child */
                if (impl->type == MPP_CFG_TYPE_ARRAY && pos->type == MPP_CFG_TYPE_OBJECT && !pos->name)
                    last_array = pos;
            }

            if (last_array) {
                MppCfgIoImpl *array_pos, *array_n;

                list_for_each_entry_safe(array_pos, array_n, &last_array->child, MppCfgIoImpl, list) {
                    if (array_pos->name && !strcmp(array_pos->name, name + str_start)) {
                        impl = array_pos;
                        found = 1;
                        break;
                    }
                }
            }

            name[i] = bak;

            if (!found) {
                *obj = NULL;
                return rk_nok;
            }

            str_start = i + 1;
        }
    }

    *obj = impl;
    return rk_ok;
}

static rk_s32 mpp_cfg_lookup(MppCfgObj *obj, MppCfgObj root, const char *name)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)root;
    MppCfgIoImpl *pos, *n;

    if (!obj || !root || !name) {
        mpp_loge_f("invalid param obj %p root %p name %s\n", obj, root, name);
        return rk_nok;
    }

    *obj = NULL;

    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        if (pos->name && !strcmp(pos->name, name)) {
            *obj = pos;
            return rk_ok;
        }
    }

    list_for_each_entry_safe(pos, n, &impl->detail, MppCfgIoImpl, list) {
        if (pos->name && !strcmp(pos->name, name)) {
            *obj = pos;
            return rk_ok;
        }
    }

    return rk_nok;
}

#define CFG_ARRAY_MIN     8

typedef struct CfgArraySlot_t {
    rk_s32          subroot;
    MppCfgObj       node;
} CfgArraySlot;

typedef struct CfgFromTrieCtx_t {
    MppCfgObj       root;

    rk_s32          path_len;
    char            *path;

    rk_s32          array_cnt;
    rk_s32          array_cap;
    CfgArraySlot    *arrays;
} CfgFromTrieCtx;

static MppCfgObj cfg_get_path_node(MppCfgObj root, char *path)
{
    MppCfgObj cur = root;
    char *seg = path;
    char *next;

    while (seg && *seg) {
        MppCfgObj found = NULL;

        next = strchr(seg, ':');
        if (next)
            *next = '\0';

        if (mpp_cfg_lookup(&found, cur, seg) || !found) {
            if (mpp_cfg_get_object(&found, seg, MPP_CFG_TYPE_OBJECT, NULL))
                return NULL;

            mpp_cfg_add_detail(cur, found);
        }

        cur = found;
        if (next)
            *next = ':';

        seg = next ? next + 1 : NULL;
    }

    return cur;
}

static rk_s32 cfg_prepare_path(CfgFromTrieCtx *c, rk_s32 len)
{
    char *path = c->path;
    rk_s32 path_len = c->path_len;
    rk_s32 new_len;

    if (path && len < path_len)
        return rk_ok;

    new_len = path_len ? path_len * 2 : 128;
    while (new_len <= len)
        new_len *= 2;

    path = mpp_realloc(c->path, char, new_len);

    if (!path)
        return rk_nok;

    c->path = path;
    c->path_len = new_len;

    return rk_ok;
}

/* split name into the parent path and leaf segment, creating missing parents */
static rk_s32 cfg_split_path(CfgFromTrieCtx *c, const char *name,
                             MppCfgObj *parent, const char **leaf)
{
    char *path;
    char *seg;

    if (cfg_prepare_path(c, strlen(name))) {
        mpp_loge_f("name %s too long to buffer\n", name);
        return rk_nok;
    }

    /* path is stable after cfg_prepare_path */
    path = c->path;
    strcpy(path, name);

    *parent = c->root;
    seg = strrchr(path, ':');
    if (seg) {
        *seg = '\0';
        *parent = cfg_get_path_node(c->root, path);
        if (!*parent)
            return rk_nok;
        *leaf = seg + 1;
    } else {
        *leaf = path;
    }

    return rk_ok;
}

/* find-or-create the array node for a subroot index */
static MppCfgObj cfg_get_array(CfgFromTrieCtx *c, rk_s32 subroot,
                               const char *name, KmppEntry *entry)
{
    CfgArraySlot *array = c->arrays;
    MppCfgObj parent = NULL;
    MppCfgObj obj = NULL;
    const char *leaf = NULL;
    rk_s32 cnt = c->array_cnt;
    rk_s32 i;

    for (i = 0; i < cnt; i++) {
        if (array[i].subroot == subroot)
            return array[i].node;
    }

    if (cnt >= c->array_cap) {
        rk_s32 new_cap = c->array_cap ? c->array_cap * 2 : CFG_ARRAY_MIN;

        array = mpp_realloc(c->arrays, CfgArraySlot, new_cap);
        if (!array)
            return NULL;

        c->arrays = array;
        c->array_cap = new_cap;
    }

    if (cfg_split_path(c, name, &parent, &leaf))
        return NULL;

    if (mpp_cfg_get_array(&obj, leaf))
        return NULL;

    /* VLA element type is not in the trie; conversion only reads entry/vla */
    mpp_cfg_set_vla(obj, entry, MPP_CFG_TYPE_OBJECT);
    mpp_cfg_add_detail(parent, obj);

    array[cnt].subroot = subroot;
    array[cnt].node = obj;
    c->array_cnt = cnt + 1;

    return obj;
}

/* create a leaf cfg node with the given entry, attached under parent */
static rk_s32 cfg_add_leaf(MppCfgObj parent, const char *leaf, KmppEntry *entry)
{
    MppCfgObj obj = NULL;

    if (mpp_cfg_get_object(&obj, leaf, MPP_CFG_TYPE_OBJECT, NULL))
        return rk_nok;

    mpp_cfg_set_entry(obj, entry);
    mpp_cfg_add_detail(parent, obj);

    return rk_ok;
}

static rk_s32 cfg_build_cb(const char *name, void *info, rk_s32 subroot, void *arg)
{
    CfgFromTrieCtx *c = (CfgFromTrieCtx *)arg;
    KmppEntry *entry = (KmppEntry *)info;
    MppCfgObj parent = NULL;
    const char *leaf = NULL;

    /* subroot entries belong to a vla: the vla node or its element fields */
    if (subroot) {
        parent = cfg_get_array(c, subroot, name, entry);
        if (!parent)
            return rk_nok;

        if (entry->type == ENTRY_TYPE_VLA_INFO)
            return rk_ok;   /* vla node itself, element fields attach later */

        leaf = name;
    } else if (cfg_split_path(c, name, &parent, &leaf)) {
        return rk_nok;
    }

    return cfg_add_leaf(parent, leaf, entry);
}

MppCfgObj mpp_cfg_from_trie(MppTrie trie)
{
    const char *name = mpp_trie_get_name(trie);
    CfgFromTrieCtx c;
    rk_s32 ret;

    if (!name)
        return NULL;

    memset(&c, 0, sizeof(c));

    if (mpp_cfg_get_object(&c.root, name, MPP_CFG_TYPE_OBJECT, NULL))
        return NULL;

    ret = mpp_trie_for_each_entry(trie, cfg_build_cb, &c);

    MPP_FREE(c.arrays);
    MPP_FREE(c.path);

    if (ret) {
        mpp_loge_f("build cfg from trie %s failed\n", name);
        mpp_cfg_put_all(c.root);
        return NULL;
    }

    return c.root;
}

rk_s32 mpp_cfg_set_entry(MppCfgObj obj, KmppEntry *entry)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;

    if (impl && entry) {
        cfg_io_dbg_info("obj %-16s set entry type %s offset %d size %d\n",
                        impl->name, strof_elem_type(entry->tbl.elem_type),
                        entry->tbl.elem_offset, entry->tbl.elem_size);

        if (entry->tbl.elem_type < ELEM_TYPE_BUTT) {
            memcpy(&impl->entry, entry, sizeof(impl->entry));
            impl->type = mpp_cfg_type_from_elem_type(entry->tbl.elem_type);
        } else {
            impl->entry.tbl.elem_type = ELEM_TYPE_BUTT;
        }

        return rk_ok;
    }

    return rk_nok;
}

rk_s32 mpp_cfg_set_vla(MppCfgObj obj, KmppEntry *entry, MppCfgType type)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;

    if (impl && entry && type) {
        rk_s32 is_simple = IS_VLA_SIMPLE_TYPE(type);
        rk_u32 elem_count = entry->vla.elem_count;
        rk_u32 elem_size = entry->vla.elem_size;
        rk_u32 flag = entry->vla.flex_count;
        rk_s32 size;
        void *ptr;

        cfg_io_dbg_info("vla %-16s set flag %x elem size %d count %d offset count %x base %x\n",
                        impl->name, entry->vla.flex_count, entry->vla.elem_size,
                        entry->vla.elem_count, entry->vla.count_off, entry->vla.base_off);

        impl->vla.val = entry->val;

        if (!flag) {
            /* fix size array */
            if (elem_count == 0) {
                mpp_loge_f("vla %-16s fix count invalid zero elem count\n",
                           impl->name);
                return rk_nok;
            }
        } else {
            /* flex count array default 16 elements */
            if (elem_count == 0)
                elem_count = 16;
        }

        if (is_simple) {
            /* simple array  - update by real elem_type */
            if (elem_size != sizeof_type(type)) {
                mpp_logw_f("vla %-16s elem size %d not match type %s\n",
                           impl->name, elem_size, strof_type(type));
            }
            elem_size = sizeof_type(type);
        } else {
            /* complex array - use pointer array */
            elem_size = sizeof(void *);
        }

        size = elem_count * elem_size;
        if ((size & ~0xffff) || (elem_count & ~0xffff)) {
            mpp_loge_f("vla %-16s size %d or count %d exceeds 16bit limit\n",
                       impl->name, size, elem_count);
            return rk_nok;
        }
        ptr = mpp_calloc_size(void, size);
        if (!ptr) {
            mpp_loge_f("vla %-16s failed to alloc buffer %dx%d %d\n",
                       impl->name, elem_size, elem_count, size);
            return rk_nok;
        }

        if (is_simple) {
            /* simple type set to elem_type */
            impl->array_type = type;
            impl->raw = ptr;
            impl->raw_size = size;
            impl->raw_count = elem_count;
        } else {
            /* complex type set elem_type to MPP_CFG_TYPE_BUTT */
            impl->array_type = MPP_CFG_TYPE_BUTT;
            impl->elems = ptr;
            impl->array_size = elem_count;
        }

        return rk_ok;
    }

    return rk_nok;
}

typedef struct CfgToTrieCtx_t {
    MppTrie trie;
    char *buf;
    rk_s32 buf_size;
} CfgToTrieCtx;

/* register a cfg node into the trie, mirroring the objdef macro layout */
static void add_obj_info_loop(MppCfgIoImpl *impl, CfgToTrieCtx *ctx, rk_s32 vla_root)
{
    MppCfgIoImpl *pos, *n;

    if (impl->type > MPP_CFG_TYPE_INVALID && impl->type < MPP_CFG_TYPE_BUTT &&
        impl->parent && impl->name) {
        KmppEntry *entry;
        KmppEntry e = { .val = 0 };

        if (impl->vla.vla.type == ENTRY_TYPE_VLA_INFO) {
            entry = &impl->vla;
        } else if (impl->entry.tbl.elem_type < ELEM_TYPE_BUTT) {
            entry = &impl->entry;
        } else {
            /* hand-built / parsed nodes carry no entry: synthesize one */
            e.tbl.type = ENTRY_TYPE_LOC_TBL;
            e.tbl.elem_type = mpp_cfg_type_to_elem_type(impl->type);
            e.tbl.elem_size = (rk_u16)sizeof_type(impl->type);
            entry = &e;
        }

        if (vla_root) {
            MppTrieStatus st = { .root_idx = vla_root };

            mpp_trie_add_entry(ctx->trie, &st, impl->name, entry);
            if (entry->type == ENTRY_TYPE_VLA_INFO)
                vla_root = st.node_idx;
        } else {
            get_full_name(impl, ctx->buf, ctx->buf_size);

            if (entry->type == ENTRY_TYPE_VLA_INFO) {
                MppTrieStatus st = { .root_idx = 0 };

                mpp_trie_add_entry(ctx->trie, &st, ctx->buf, entry);
                vla_root = st.node_idx;
            } else {
                mpp_trie_add_info(ctx->trie, ctx->buf, entry, sizeof(KmppEntry));
            }
        }
    }

    /* detail first (objdef macro trees), then child (hand-built trees) */
    list_for_each_entry_safe(pos, n, &impl->detail, MppCfgIoImpl, list) {
        add_obj_info_loop(pos, ctx, vla_root);
    }

    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        add_obj_info_loop(pos, ctx, vla_root);
    }
}

MppTrie mpp_cfg_to_trie(MppCfgObj obj)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;
    MppTrie p = NULL;

    do {
        CfgToTrieCtx ctx;
        rk_s32 ret = rk_nok;
        char name[256];

        if (!impl) {
            mpp_loge_f("invalid param obj\n", impl);
            break;
        }

        if (impl->parent) {
            mpp_loge_f("obj %-16s invalid param obj %p not root\n",
                       impl->name, impl);
            break;
        }

        if (impl->trie) {
            p = impl->trie;
            break;
        }

        ret = mpp_trie_init(&p, impl->name ? impl->name : "cfg_io");
        if (ret || !p) {
            mpp_loge_f("failed to init obj %s trie\n", impl->name ? impl->name : "cfg_io");
            break;
        }

        ctx.trie = p;
        ctx.buf = name;
        ctx.buf_size = sizeof(name) - 1;

        /* data trees keep fields in child, template trees (objdef cfg_roots)
         * keep them in detail - walk both so to_trie works on either */
        add_obj_info_loop(impl, &ctx, 0);
        mpp_trie_add_info(p, NULL, NULL, 0);
        impl->trie = p;
    } while (0);

    return p;
}

/* read byte functions */
/* check valid len, get offset position */
#define test_byte_f(str, len)           test_byte(str, len, __FUNCTION__)
/* check valid pos, get offset + pos position */
#define show_byte_f(str, pos)           show_byte(str, pos, __FUNCTION__)
/* check valid len, get offset + len position and increase offset by len */
#define skip_byte_f(str, len)           skip_byte(str, len, __FUNCTION__)
#define skip_ws_f(str)                  skip_ws(str, __FUNCTION__)

/* write byte functions */
#define write_byte_f(str, buf, size)    write_byte(str, (void *)buf, size, __FUNCTION__)
#define write_indent_f(str)             write_indent(str, __FUNCTION__)
/* revert comma for json */
#define revert_comma_f(str)             revert_comma(str, __FUNCTION__)

static char *test_byte(MppCfgStrBuf *str, rk_s32 len, const char *caller)
{
    char *ret = NULL;

    if (str->offset + len >= str->buf_size) {
        cfg_io_dbg_byte("str %p-[%p:%d] offset %d test %d get the end at %s\n",
                        str, str->buf, str->buf_size, str->offset, len, caller);
        return ret;
    }

    ret = str->buf + str->offset;

    cfg_io_dbg_byte("str %p-[%p:%d] offset %d test %d ret %p at %s\n",
                    str, str->buf, str->buf_size, str->offset, len, ret, caller);

    return ret;
}

static char *show_byte(MppCfgStrBuf *str, rk_s32 pos, const char *caller)
{
    char *ret = NULL;

    if (str->offset + pos >= str->buf_size) {
        cfg_io_dbg_byte("str %p-[%p:%d] offset %d show pos %d get the end at %s\n",
                        str, str->buf, str->buf_size, str->offset, pos, caller);
        return ret;
    }

    ret = str->buf + str->offset + pos;

    cfg_io_dbg_byte("str %p-[%p:%d] offset %d show %d ret %p at %s\n",
                    str, str->buf, str->buf_size, str->offset, pos, ret, caller);

    return ret;
}

static char *skip_byte(MppCfgStrBuf *str, rk_s32 len, const char *caller)
{
    char *ret = NULL;

    if (str->offset + len >= str->buf_size) {
        cfg_io_dbg_byte("str %p-[%p:%d] offset %d skip %d get the end at %s\n",
                        str, str->buf, str->buf_size, str->offset, len, caller);
        return NULL;
    }

    ret = str->buf + str->offset + len;

    cfg_io_dbg_byte("str %p-[%p:%d] offset %d skip %d ret %p at %s\n",
                    str, str->buf, str->buf_size, str->offset, len, ret, caller);

    str->offset += len;
    return ret;
}

static char *skip_ws(MppCfgStrBuf *str, const char *caller)
{
    rk_s32 old = str->offset;
    char *p;

    cfg_io_dbg_byte("str %p-[%p:%d] offset %d skip ws start at %s\n",
                    str, str->buf, str->buf_size, old, caller);

    while ((p = show_byte(str, 0, caller)) && p[0] <= 32)
        str->offset++;

    if (str->offset >= str->buf_size) {
        cfg_io_dbg_byte("str %p-[%p:%d] offset %d skip ws to the end at %s\n",
                        str, str->buf, str->buf_size, str->offset, caller);
        str->offset--;
        return NULL;
    }

    cfg_io_dbg_byte("str %p-[%p:%d] offset %d skip ws to %d at %s\n",
                    str, str->buf, str->buf_size, old, str->offset, caller);

    return str->buf + str->offset;
}

static rk_s32 write_byte(MppCfgStrBuf *str, void *buf, rk_s32 *size, const char *caller)
{
    rk_s32 len = size[0];

    if (!len)
        return rk_ok;

    if (str->offset + len >= str->buf_size) {
        void *ptr = mpp_realloc_size(str->buf, void, str->buf_size * 2);

        if (!ptr) {
            mpp_loge("failed to realloc buf size %d -> %d at %s\n",
                     str->buf_size, str->buf_size * 2, caller);
            return rk_nok;
        }

        cfg_io_dbg_byte("str %p-[%p:%d] enlarger buffer to [%p:%d] at %s\n",
                        str, str->buf, str->buf_size, ptr, str->buf_size * 2, caller);

        str->buf = ptr;
        str->buf_size *= 2;
    }

    cfg_io_dbg_byte("str %p-[%p:%d] write offset %d from [%p:%d] at %s\n",
                    str, str->buf, str->buf_size, str->offset, buf, len, caller);

    memcpy(str->buf + str->offset, buf, len);
    str->offset += len;
    str->buf[str->offset] = '\0';
    size[0] = 0;

    return rk_ok;
}

static rk_s32 write_indent(MppCfgStrBuf *str, const char *caller)
{
    cfg_io_dbg_byte("str %p-[%p:%d] write indent %d at %s\n",
                    str, str->buf, str->buf_size, str->depth, caller);

    if (str->depth) {
        char space[17] = "                ";
        rk_s32 i;

        for (i = 0; i < str->depth; i++) {
            rk_s32 indent_width = 4;

            if (write_byte_f(str, space, &indent_width))
                return rk_nok;
        }
    }

    return rk_ok;
}

static rk_s32 revert_comma(MppCfgStrBuf *str, const char *caller)
{
    cfg_io_dbg_byte("str %p-[%p:%d] revert_comma %d at %s\n",
                    str, str->buf, str->buf_size, str->depth, caller);

    if (str->offset <= 1) {
        cfg_io_dbg_byte("str %p offset %d skip revert_comma at %s\n",
                        str, str->offset, caller);
        return rk_ok;
    }

    if (str->buf[str->offset - 2] == ',') {
        str->buf[str->offset - 2] = str->buf[str->offset - 1];
        str->buf[str->offset - 1] = str->buf[str->offset];
        str->offset--;
    }

    return rk_ok;
}

static rk_s32 mpp_cfg_format_leaf_value(MppCfgIoImpl *impl, char *buf, rk_s32 total)
{
    rk_s32 len = 0;

    switch (impl->type) {
    case MPP_CFG_TYPE_NULL : {
        len += snprintf(buf + len, total - len, "null");
    } break;
    case MPP_CFG_TYPE_BOOL : {
        len += snprintf(buf + len, total - len, "%s", impl->val.b1 ? "true" : "false");
    } break;
    case MPP_CFG_TYPE_s8 : {
        len += snprintf(buf + len, total - len, "%d", impl->val.s8);
    } break;
    case MPP_CFG_TYPE_u8 : {
        len += snprintf(buf + len, total - len, "%u", impl->val.u8);
    } break;
    case MPP_CFG_TYPE_s16 : {
        len += snprintf(buf + len, total - len, "%d", impl->val.s16);
    } break;
    case MPP_CFG_TYPE_u16 : {
        len += snprintf(buf + len, total - len, "%u", impl->val.u16);
    } break;
    case MPP_CFG_TYPE_s32 : {
        len += snprintf(buf + len, total - len, "%d", impl->val.s32);
    } break;
    case MPP_CFG_TYPE_u32 : {
        len += snprintf(buf + len, total - len, "%u", impl->val.u32);
    } break;
    case MPP_CFG_TYPE_s64 : {
        len += snprintf(buf + len, total - len, "%lld", impl->val.s64);
    } break;
    case MPP_CFG_TYPE_u64 : {
        len += snprintf(buf + len, total - len, "%llu", impl->val.u64);
    } break;
    case MPP_CFG_TYPE_f32 : {
        len += snprintf(buf + len, total - len, "%f", impl->val.f32);
    } break;
    case MPP_CFG_TYPE_f64 : {
        len += snprintf(buf + len, total - len, "%lf", impl->val.f64);
    } break;
    case MPP_CFG_TYPE_STRING :
    case MPP_CFG_TYPE_RAW : {
        len += snprintf(buf + len, total - len, "\"%s\"", (char *)impl->val.str);
    } break;
    default : {
        mpp_loge("invalid type %d name %s\n", impl->type, impl->name);
    } break;
    }

    return len;
}

static rk_s32 mpp_cfg_format_vla_elem(MppCfgIoImpl *impl, rk_s32 idx,
                                      char *buf, rk_s32 total)
{
    rk_s32 len = 0;
    rk_s32 elem_size;
    void *ptr;

    if (!impl->raw || idx < 0)
        return 0;

    elem_size = sizeof_type(impl->array_type);
    if (elem_size <= 0)
        return 0;

    if (idx * elem_size + elem_size > (rk_s32)impl->raw_size)
        return 0;

    ptr = (char *)impl->raw + elem_size * idx;

    switch (impl->array_type) {
    case MPP_CFG_TYPE_BOOL : {
        rk_bool v = *(rk_bool *)ptr;
        len += snprintf(buf + len, total - len, "%s", v ? "true" : "false");
    } break;
    case MPP_CFG_TYPE_s8 : {
        len += snprintf(buf + len, total - len, "%d", *(rk_s8 *)ptr);
    } break;
    case MPP_CFG_TYPE_u8 : {
        len += snprintf(buf + len, total - len, "%u", *(rk_u8 *)ptr);
    } break;
    case MPP_CFG_TYPE_s16 : {
        len += snprintf(buf + len, total - len, "%d", *(rk_s16 *)ptr);
    } break;
    case MPP_CFG_TYPE_u16 : {
        len += snprintf(buf + len, total - len, "%u", *(rk_u16 *)ptr);
    } break;
    case MPP_CFG_TYPE_s32 : {
        len += snprintf(buf + len, total - len, "%d", *(rk_s32 *)ptr);
    } break;
    case MPP_CFG_TYPE_u32 : {
        len += snprintf(buf + len, total - len, "%u", *(rk_u32 *)ptr);
    } break;
    case MPP_CFG_TYPE_s64 : {
        len += snprintf(buf + len, total - len, "%lld", *(rk_s64 *)ptr);
    } break;
    case MPP_CFG_TYPE_u64 : {
        len += snprintf(buf + len, total - len, "%llu", *(rk_u64 *)ptr);
    } break;
    case MPP_CFG_TYPE_f32 : {
        len += snprintf(buf + len, total - len, "%f", *(rk_float *)ptr);
    } break;
    case MPP_CFG_TYPE_f64 : {
        len += snprintf(buf + len, total - len, "%lf", *(rk_double *)ptr);
    } break;
    default : {
        len += snprintf(buf + len, total - len, "?");
    } break;
    }

    return len;
}

static rk_s32 mpp_cfg_to_log(MppCfgIoImpl *impl, MppCfgStrBuf *str)
{
    MppCfgIoImpl *pos, *n;
    char buf[256];
    rk_s32 len = 0;
    rk_s32 total = sizeof(buf) - 1;
    rk_s32 ret = rk_ok;
    rk_s32 is_array_elem = impl->parent && impl->parent->type == MPP_CFG_TYPE_ARRAY;
    rk_s32 is_array = impl->type == MPP_CFG_TYPE_ARRAY;
    rk_s32 skip_indent = 0;  /* Flag to skip indent for array elements */

    if (impl->type == MPP_CFG_TYPE_INVALID)
        return rk_ok;

    /* For simple array elements, skip indent - parent will handle it */
    if (is_array_elem && impl->type < MPP_CFG_TYPE_OBJECT)
        skip_indent = 1;

    if (!skip_indent)
        write_indent_f(str);

    /* leaf node write once and finish */
    if (impl->type < MPP_CFG_TYPE_OBJECT) {
        cfg_io_dbg_to("depth %2d leaf write name %s type %d\n", str->depth, impl->name, impl->type);

        if (impl->name && impl->parent)
            len += snprintf(buf + len, total - len, "%s : ", impl->name);

        len += mpp_cfg_format_leaf_value(impl, buf + len, total - len);

        /* Add separator: " " for array elements (except last), "\n" for others */
        if (is_array_elem) {
            rk_s32 is_last = list_is_last(&impl->list, &impl->parent->child);

            if (!is_last)
                len += snprintf(buf + len, total - len, " ");
        } else {
            len += snprintf(buf + len, total - len, "\n");
        }

        return write_byte_f(str, buf, &len);
    }

    cfg_io_dbg_to("depth %2d branch write name %s type %d\n", str->depth, impl->name, impl->type);

    if (impl->name && impl->parent)
        len += snprintf(buf + len, total - len, "%s : ", impl->name);

    if (list_empty(&impl->child)) {
        if (IS_VLA_COMPLEX_TYPE(impl->array_type)) {
            /* vla mode with element array (object/complex) */
            rk_s32 i;

            len += snprintf(buf + len, total - len, "[\n");
            ret = write_byte_f(str, buf, &len);
            if (ret)
                return ret;

            str->depth++;
            for (i = 0; i < impl->array_size; i++) {
                if (!impl->elems[i])
                    continue;

                if (impl->elems[i]->type < MPP_CFG_TYPE_OBJECT) {
                    if (i == 0)
                        write_indent_f(str);
                } else if (i > 0 && (str->offset == 0 || str->buf[str->offset - 1] != '\n')) {
                    write_byte_f(str, "\n", &(rk_s32) {1});
                }

                ret = mpp_cfg_to_log(impl->elems[i], str);
                if (ret)
                    return ret;
            }
            str->depth--;

            /* Add newline before closing bracket */
            if (str->offset == 0 || str->buf[str->offset - 1] != '\n')
                write_byte_f(str, "\n", &(rk_s32) {1});

            write_indent_f(str);
            len = snprintf(buf, total, "]");
        } else if (IS_VLA_SIMPLE_TYPE(impl->array_type)) {
            /* vla mode with simple type and raw data value */
            rk_s32 elem_size = sizeof_type(impl->array_type);
            rk_s32 elem_count = impl->raw_size / elem_size;
            rk_s32 i;

            if (elem_size <= 0 || !impl->raw) {
                mpp_loge("invalid elem_size %d or invalid raw %p\n", elem_size, impl->raw);
                return -1;
            }

            if (elem_count == 0) {
                len += snprintf(buf + len, total - len, "[]");
            } else {
                len += snprintf(buf + len, total - len, "[\n");
                ret = write_byte_f(str, buf, &len);
                if (ret)
                    return ret;

                str->depth++;
                for (i = 0; i < elem_count; i++) {
                    if ((i & 0xf) == 0)
                        write_indent_f(str);

                    len = mpp_cfg_format_vla_elem(impl, i, buf, total);

                    /* new line for every 16 elems and last elems */
                    if (i == elem_count - 1 || (i & 0xf) == 0xf)
                        len += snprintf(buf + len, total - len, "\n");
                    else
                        len += snprintf(buf + len, total - len, " ");

                    ret = write_byte_f(str, buf, &len);
                    if (ret)
                        return ret;
                }
                str->depth--;
                write_indent_f(str);
                len = snprintf(buf, total, "]");
            }
        } else {
            len += snprintf(buf + len, total - len, "%s",
                            impl->type == MPP_CFG_TYPE_OBJECT ? "{}" : "[]");
        }

        len += snprintf(buf + len, total - len, "\n");

        return write_byte_f(str, buf, &len);
    }

    len += snprintf(buf + len, total - len, "%c\n",
                    impl->type == MPP_CFG_TYPE_OBJECT ? '{' : '[');

    ret = write_byte_f(str, buf, &len);
    if (ret)
        return ret;

    str->depth++;

    /* For arrays, track element count to implement line break */
    if (is_array) {
        rk_s32 elem_count = 0;
        list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
            cfg_io_dbg_to("depth %2d child write name %s type %d\n", str->depth, pos->name, pos->type);

            /* Add indent for first element, newline + indent every define elements */
            if (pos->type < MPP_CFG_TYPE_OBJECT) {
                if (elem_count == 0) {
                    write_indent_f(str);
                } else if (elem_count % CFG_IO_ARRAY_ELEM_COUNT == 0) {
                    write_byte_f(str, "\n", &(rk_s32) {1});
                    write_indent_f(str);
                }
            }

            ret = mpp_cfg_to_log(pos, str);
            if (ret)
                break;
            elem_count++;
        }
    } else {
        list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
            cfg_io_dbg_to("depth %2d child write name %s type %d\n", str->depth, pos->name, pos->type);
            ret = mpp_cfg_to_log(pos, str);
            if (ret)
                break;
        }
    }

    str->depth--;

    /* Add newline before closing bracket for semi-compact format */
    if (str->offset == 0 || str->buf[str->offset - 1] != '\n')
        write_byte_f(str, "\n", &(rk_s32) {1});
    write_indent_f(str);

    len += snprintf(buf + len, total - len, "%c\n",
                    impl->type == MPP_CFG_TYPE_OBJECT ? '}' : ']');

    return write_byte_f(str, buf, &len);
}

static rk_s32 mpp_cfg_to_json(MppCfgIoImpl *impl, MppCfgStrBuf *str)
{
    MppCfgIoImpl *pos, *n;
    char buf[256];
    rk_s32 len = 0;
    rk_s32 total = sizeof(buf) - 1;
    rk_s32 ret = rk_ok;
    rk_s32 is_array_elem = impl->parent && impl->parent->type == MPP_CFG_TYPE_ARRAY;
    rk_s32 is_array = impl->type == MPP_CFG_TYPE_ARRAY;
    rk_s32 skip_indent = 0;  /* Flag to skip indent for array elements */

    if (impl->type == MPP_CFG_TYPE_INVALID)
        return rk_ok;

    /* For simple array elements, skip indent - parent will handle it */
    if (is_array_elem && impl->type < MPP_CFG_TYPE_OBJECT)
        skip_indent = 1;

    if (!skip_indent)
        write_indent_f(str);

    /* leaf node write once and finish */
    if (impl->type < MPP_CFG_TYPE_OBJECT) {
        cfg_io_dbg_to("depth %2d leaf write name %s type %d\n", str->depth, impl->name, impl->type);

        if (impl->name && impl->parent)
            len += snprintf(buf + len, total - len, "\"%s\" : ", impl->name);

        len += mpp_cfg_format_leaf_value(impl, buf + len, total - len);

        /* Add separator: ",\n" for non-array elements, ", " for array elements (except last) */
        if (is_array_elem) {
            rk_s32 is_last = list_is_last(&impl->list, &impl->parent->child);

            if (!is_last)
                len += snprintf(buf + len, total - len, ", ");
        } else {
            len += snprintf(buf + len, total - len, ",\n");
        }

        return write_byte_f(str, buf, &len);
    }

    cfg_io_dbg_to("depth %2d branch write name %s type %d\n", str->depth, impl->name, impl->type);

    if (impl->name && impl->parent)
        len += snprintf(buf + len, total - len, "\"%s\" : ", impl->name);

    if (list_empty(&impl->child)) {
        if (IS_VLA_COMPLEX_TYPE(impl->array_type) && impl->elems) {
            /* vla mode with element array (object/complex) */
            rk_s32 i;

            len += snprintf(buf + len, total - len, "[\n");
            ret = write_byte_f(str, buf, &len);
            if (ret)
                return ret;

            str->depth++;
            for (i = 0; i < impl->array_size; i++) {
                if (!impl->elems[i])
                    continue;

                if (impl->elems[i]->type < MPP_CFG_TYPE_OBJECT) {
                    if (i == 0)
                        write_indent_f(str);
                } else if (i > 0 && (str->offset == 0 || str->buf[str->offset - 1] != '\n')) {
                    write_byte_f(str, "\n", &(rk_s32) {1});
                }

                ret = mpp_cfg_to_json(impl->elems[i], str);
                if (ret)
                    return ret;
            }
            revert_comma_f(str);
            str->depth--;

            /* Add newline before closing bracket */
            if (str->offset == 0 || str->buf[str->offset - 1] != '\n')
                write_byte_f(str, "\n", &(rk_s32) {1});

            write_indent_f(str);
            len = snprintf(buf, total, "]");
        } else if (IS_VLA_SIMPLE_TYPE(impl->array_type)) {
            /* vla mode with simple type and raw data value */
            rk_s32 elem_size = sizeof_type(impl->array_type);
            rk_s32 elem_count;
            rk_s32 i;

            if (elem_size <= 0 || !impl->raw) {
                mpp_loge("invalid elem_size %d or invalid raw %p\n", elem_size, impl->raw);
                return -1;
            }

            elem_count = impl->raw_count;

            if (elem_count == 0) {
                len += snprintf(buf + len, total - len, "[]");
            } else {
                len += snprintf(buf + len, total - len, "[\n");
                ret = write_byte_f(str, buf, &len);
                if (ret)
                    return ret;

                str->depth++;
                for (i = 0; i < elem_count; i++) {
                    if ((i & 0xf) == 0)
                        write_indent_f(str);

                    len = mpp_cfg_format_vla_elem(impl, i, buf, total);

                    if (i == elem_count - 1)
                        len += snprintf(buf + len, total - len, "\n");
                    else if ((i & 0xf) == 0xf)
                        len += snprintf(buf + len, total - len, ",\n");
                    else
                        len += snprintf(buf + len, total - len, ", ");

                    ret = write_byte_f(str, buf, &len);
                    if (ret)
                        return ret;
                }
                str->depth--;

                write_indent_f(str);
                len = snprintf(buf, total, "]");
            }
        } else {
            len += snprintf(buf + len, total - len, "%s",
                            impl->type == MPP_CFG_TYPE_OBJECT ? "{}" : "[]");
        }

        if (is_array_elem)
            len += snprintf(buf + len, total - len, ", ");
        else
            len += snprintf(buf + len, total - len, ",\n");

        return write_byte_f(str, buf, &len);
    }

    len += snprintf(buf + len, total - len, "%c\n",
                    impl->type == MPP_CFG_TYPE_OBJECT ? '{' : '[');

    ret = write_byte_f(str, buf, &len);
    if (ret)
        return ret;

    str->depth++;

    /* For arrays, track element count to implement line break */
    if (is_array) {
        rk_s32 elem_count = 0;
        list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
            cfg_io_dbg_to("depth %2d child write name %s type %d\n", str->depth, pos->name, pos->type);

            /* Add indent for first element, newline + indent every define elements */
            if (pos->type < MPP_CFG_TYPE_OBJECT) {
                if (elem_count == 0) {
                    write_indent_f(str);
                } else if (elem_count % CFG_IO_ARRAY_ELEM_COUNT == 0) {
                    write_byte_f(str, "\n", &(rk_s32) {1});
                    write_indent_f(str);
                }
            }

            ret = mpp_cfg_to_json(pos, str);
            if (ret)
                break;
            elem_count++;
        }
    } else {
        list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
            cfg_io_dbg_to("depth %2d child write name %s type %d\n", str->depth, pos->name, pos->type);
            ret = mpp_cfg_to_json(pos, str);
            if (ret)
                break;
        }
    }

    revert_comma_f(str);

    str->depth--;

    /* Add newline before closing bracket for semi-compact format */
    if (str->offset == 0 || str->buf[str->offset - 1] != '\n')
        write_byte_f(str, "\n", &(rk_s32) {1});
    write_indent_f(str);

    if (str->depth)
        len += snprintf(buf + len, total - len, "%c,\n",
                        impl->type == MPP_CFG_TYPE_OBJECT ? '}' : ']');
    else
        len += snprintf(buf + len, total - len, "%c\n",
                        impl->type == MPP_CFG_TYPE_OBJECT ? '}' : ']');

    return write_byte_f(str, buf, &len);
}

static rk_s32 mpp_toml_parent_is_array_table(MppCfgIoImpl *impl)
{
    return impl->type == MPP_CFG_TYPE_OBJECT &&
           !impl->name && impl->parent && impl->parent->type == MPP_CFG_TYPE_ARRAY;
}

static rk_s32 is_array_of_tables(MppCfgIoImpl *impl)
{
    if (impl->type != MPP_CFG_TYPE_ARRAY)
        return 0;

    if (!list_empty(&impl->child)) {
        MppCfgIoImpl *first = list_first_entry(&impl->child,
                                               MppCfgIoImpl, list);

        return first && first->type == MPP_CFG_TYPE_OBJECT;
    }

    if (IS_VLA_COMPLEX_TYPE(impl->array_type) &&
        impl->elems && impl->array_size > 0) {
        MppCfgIoImpl *first = impl->elems[0];

        return first && first->type == MPP_CFG_TYPE_OBJECT;
    }

    return 0;
}

static rk_s32 mpp_toml_header(MppCfgIoImpl *impl, MppCfgStrBuf *str)
{
    char buf[256];
    rk_s32 len = 0;
    rk_s32 total = sizeof(buf) - 1;
    MppCfgIoImpl *p = NULL;

    /* array-of-tables element — anonymous OBJECT child of ARRAY */
    if (mpp_toml_parent_is_array_table(impl)) {
        len += snprintf(buf + len, total - len, "\n[[%s]]\n",
                        impl->parent->name);
        return write_byte_f(str, buf, &len);
    }

    /* nested table — named OBJECT inside another OBJECT */
    if (impl->type == MPP_CFG_TYPE_OBJECT && impl->parent &&
        impl->parent->type == MPP_CFG_TYPE_OBJECT) {
        rk_s32 depth_count = 0;
        rk_s32 i;
        MppCfgIoImpl *path[8];

        p = impl;
        while (p && p->type == MPP_CFG_TYPE_OBJECT && depth_count < 8) {
            if (p->name)
                path[depth_count++] = p;
            p = p->parent;
        }
        /* Build dotted key from outermost named to this */
        len += snprintf(buf + len, total - len, "\n[");
        for (i = depth_count - 1; i >= 0; i--) {
            if (i < depth_count - 1)
                len += snprintf(buf + len, total - len, ".");
            len += snprintf(buf + len, total - len, "%s", path[i]->name);
        }
        len += snprintf(buf + len, total - len, "]\n");
        return write_byte_f(str, buf, &len);
    }

    if (impl->name && !is_array_of_tables(impl))
        len += snprintf(buf + len, total - len, "%s = ", impl->name);

    return write_byte_f(str, buf, &len);
}

static rk_s32 mpp_cfg_to_toml(MppCfgIoImpl *impl, MppCfgStrBuf *str)
{
    MppCfgIoImpl *pos, *n;
    char buf[256];
    rk_s32 len = 0;
    rk_s32 total = sizeof(buf) - 1;
    rk_s32 ret = rk_ok;

    if (impl->type == MPP_CFG_TYPE_INVALID)
        return rk_ok;

    /* leaf node write once and finish */
    if (impl->type < MPP_CFG_TYPE_OBJECT) {
        cfg_io_dbg_to("depth %2d leaf write name %s type %d\n", str->depth, impl->name, impl->type);

        if (impl->name)
            len += snprintf(buf + len, total - len, "%s = ", impl->name);

        switch (impl->type) {
        case MPP_CFG_TYPE_NULL : {
            len += snprintf(buf + len, total - len, "null");
        } break;
        case MPP_CFG_TYPE_BOOL : {
            len += snprintf(buf + len, total - len, "%s", impl->val.b1 ? "true" : "false");
        } break;
        case MPP_CFG_TYPE_s8 : {
            len += snprintf(buf + len, total - len, "%d", impl->val.s8);
        } break;
        case MPP_CFG_TYPE_u8 : {
            len += snprintf(buf + len, total - len, "%u", impl->val.u8);
        } break;
        case MPP_CFG_TYPE_s16 : {
            len += snprintf(buf + len, total - len, "%d", impl->val.s16);
        } break;
        case MPP_CFG_TYPE_u16 : {
            len += snprintf(buf + len, total - len, "%u", impl->val.u16);
        } break;
        case MPP_CFG_TYPE_s32 : {
            len += snprintf(buf + len, total - len, "%d", impl->val.s32);
        } break;
        case MPP_CFG_TYPE_u32 : {
            len += snprintf(buf + len, total - len, "%u", impl->val.u32);
        } break;
        case MPP_CFG_TYPE_s64 : {
            len += snprintf(buf + len, total - len, "%lld", impl->val.s64);
        } break;
        case MPP_CFG_TYPE_u64 : {
            len += snprintf(buf + len, total - len, "%llu", impl->val.u64);
        } break;
        case MPP_CFG_TYPE_f32 : {
            len += snprintf(buf + len, total - len, "%f", impl->val.f32);
        } break;
        case MPP_CFG_TYPE_f64 : {
            len += snprintf(buf + len, total - len, "%lf", impl->val.f64);
        } break;
        case MPP_CFG_TYPE_STRING :
        case MPP_CFG_TYPE_RAW : {
            len += snprintf(buf + len, total - len, "\"%s\"", (char *)impl->val.str);
        } break;
        default : {
            mpp_loge("invalid type %d name %s\n", impl->type, impl->name);
        } break;
        }

        len += snprintf(buf + len, total - len, "\n");

        return write_byte_f(str, buf, &len);
    }

    cfg_io_dbg_to("depth %2d branch write name %s type %d\n", str->depth, impl->name, impl->type);

    ret = mpp_toml_header(impl, str);
    if (ret)
        return ret;

    if (list_empty(&impl->child) && !impl->ptr) {
        if (impl->type == MPP_CFG_TYPE_OBJECT) {
            if (impl->name)
                len += snprintf(buf + len, total - len, "\n[%s]\n", impl->name);
        } else {
            len += snprintf(buf + len, total - len, "[]\n");
        }
        return write_byte_f(str, buf, &len);
    }

    if (impl->type == MPP_CFG_TYPE_ARRAY) {
        rk_s32 is_inline = 0;

        if (IS_VLA_SIMPLE_TYPE(impl->array_type) && impl->raw) {
            rk_s32 elem_size = sizeof_type(impl->array_type);
            rk_s32 elem_count = impl->raw_count;
            rk_s32 i;

            if (elem_size <= 0) {
                mpp_loge("invalid elem_size %d\n", elem_size);
                return -1;
            }

            len += snprintf(buf + len, total - len, "[");
            for (i = 0; i < elem_count; i++) {
                if (i > 0)
                    len += snprintf(buf + len, total - len, ", ");
                len += mpp_cfg_format_vla_elem(impl, i, buf + len, total - len);
            }
            len += snprintf(buf + len, total - len, "]\n");
            is_inline = 1;
        } else if (IS_VLA_COMPLEX_TYPE(impl->array_type) && impl->elems &&
                   impl->array_size > 0) {
            MppCfgIoImpl *first = impl->elems[0];

            if (first && first->type < MPP_CFG_TYPE_OBJECT) {
                rk_s32 i;

                len += snprintf(buf + len, total - len, "[");
                for (i = 0; i < impl->array_size; i++) {
                    MppCfgIoImpl *elem = impl->elems[i];

                    if (!elem)
                        continue;
                    if (i > 0)
                        len += snprintf(buf + len, total - len, ", ");
                    len += mpp_cfg_format_leaf_value(elem, buf + len, total - len);
                }
                len += snprintf(buf + len, total - len, "]\n");
                is_inline = 1;
            }
        } else if (!list_empty(&impl->child)) {
            MppCfgIoImpl *first = list_first_entry(&impl->child, MppCfgIoImpl, list);

            if (first && first->type < MPP_CFG_TYPE_OBJECT) {
                rk_s32 i = 0;

                len += snprintf(buf + len, total - len, "[");
                list_for_each_entry(pos, &impl->child, MppCfgIoImpl, list) {
                    if (i > 0)
                        len += snprintf(buf + len, total - len, ", ");
                    len += mpp_cfg_format_leaf_value(pos, buf + len, total - len);
                    i++;
                }
                len += snprintf(buf + len, total - len, "]\n");
                is_inline = 1;
            }
        }

        if (is_inline)
            return write_byte_f(str, buf, &len);
    }

    /* VLA complex with object/array elements: multi-line array */
    if (IS_VLA_COMPLEX_TYPE(impl->array_type) && impl->elems) {
        rk_s32 i;
        rk_s32 saved_depth;

        len += snprintf(buf + len, total - len, "[\n");
        ret = write_byte_f(str, buf, &len);
        if (ret)
            return ret;

        saved_depth = str->depth;
        str->depth++;

        for (i = 0; i < impl->array_size; i++) {
            if (!impl->elems[i])
                continue;

            write_indent_f(str);
            ret = mpp_cfg_to_toml(impl->elems[i], str);
            if (ret)
                return ret;

            if (i < impl->array_size - 1 && str->offset > 0 &&
                str->buf[str->offset - 1] == '\n') {
                str->offset--;
                write_byte_f(str, ",\n", &(rk_s32) {2});
            }
        }

        str->depth = saved_depth;
        write_indent_f(str);
        len += snprintf(buf + len, total - len, "]\n");
        return write_byte_f(str, buf, &len);
    }

    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        cfg_io_dbg_to("depth %2d child write name %s type %d\n", str->depth, pos->name, pos->type);
        ret = mpp_cfg_to_toml(pos, str);
        if (ret)
            break;

        if (impl->type == MPP_CFG_TYPE_ARRAY &&
            pos->type >= MPP_CFG_TYPE_OBJECT &&
            !mpp_toml_parent_is_array_table(pos) &&
            !list_is_last(&pos->list, &impl->child))
            write_byte_f(str, ",\n", &(rk_s32) {2});
    }

    return write_byte_f(str, buf, &len);
}

static rk_s32 parse_number(MppCfgStrBuf *str, MppCfgType *type, MppCfgVal *val, rk_s32 peek)
{
    char *buf = NULL;
    char tmp[64];
    long double value;
    rk_u32 i;
    rk_u32 str_offset = 0;

    if (peek)
        str_offset = str->offset;

    for (i = 0; i < sizeof(tmp) - 1; i++) {
        buf = show_byte_f(str, 0);
        if (!buf)
            break;

        switch (buf[0]) {
        case '0' ... '9' :
        case '.' :
        case 'e' :
        case 'E' :
        case '+' :
        case '-' : {
            tmp[i] = buf[0];
        } break;
        default : {
            tmp[i] = '\0';
            goto done;
        } break;
        }
        skip_byte_f(str, 1);
    }

done:
    if (peek)
        str->offset = str_offset;

    if (!i)
        return rk_nok;

    errno = 0;
    value = strtold(tmp, NULL);
    if (errno) {
        mpp_loge_f("failed to parse number %s errno %s\n", tmp, strerror(errno));
        return rk_nok;
    }

    if (strstr(tmp, ".")) {
        if (value >= FLT_MIN && value <= FLT_MAX) {
            *type = MPP_CFG_TYPE_f32;
            val->f32 = (float)value;
        } else {
            *type = MPP_CFG_TYPE_f64;
            val->f64 = (double)value;
        }
    } else {
        if (value >= INT_MIN && value <= INT_MAX) {
            *type = MPP_CFG_TYPE_s32;
            val->s32 = (int)value;
        } else if (value >= 0 && value <= UINT_MAX) {
            *type = MPP_CFG_TYPE_u32;
            val->u32 = (unsigned int)value;
        } else if (value >= (long double)LLONG_MIN && value <= (long double)LLONG_MAX) {
            *type = MPP_CFG_TYPE_u64;
            val->u64 = (unsigned long long)value;
        } else if (value >= 0 && value <= (long double)ULLONG_MAX) {
            *type = MPP_CFG_TYPE_s64;
            val->s64 = (long long)value;
        } else {
            mpp_loge_f("invalid number %s\n", tmp);
            return rk_nok;
        }
    }

    return rk_ok;
}

static rk_s32 parse_log_string(MppCfgStrBuf *str, char **name, rk_s32 *len, rk_u32 type)
{
    char *buf = NULL;
    char *start = NULL;
    rk_s32 name_len = 0;
    char terminator = (type != 0) ? '\"' : ' ';

    *name = NULL;
    *len = 0;

    /* skip whitespace and find first double quotes */
    buf = skip_ws_f(str);
    if (!buf)
        return -101;

    if (type) {
        if (buf[0] != '\"')
            return -101;

        buf = skip_byte_f(str, 1);
        if (!buf)
            return -102;
    }

    start = buf;

    /* find the terminator */
    while ((buf = show_byte_f(str, name_len)) && buf[0] != terminator) {
        name_len++;
    }

    if (!buf || buf[0] != terminator)
        return -103;

    /* find complete string skip the string and terminator */
    buf = skip_byte_f(str, name_len + 1);
    if (!buf)
        return -104;

    *name = start;
    *len = name_len;

    return rk_ok;
}

static rk_s32 store_vla_simple(MppCfgIoImpl *parent, rk_s32 idx, void *val)
{
    rk_s32 esz = sizeof_type(parent->array_type);

    if (!parent->raw) {
        rk_s32 init_cnt = VLA_INIT_CNT;
        void *raw_buf = NULL;

        raw_buf = mpp_calloc_size(void, init_cnt * esz);
        if (!raw_buf) {
            mpp_loge_f("vla %-16s calloc raw_buf failed\n", parent->name);
            return rk_nok;
        }

        parent->raw = raw_buf;
        parent->raw_size = init_cnt * esz;
        parent->raw_count = init_cnt;
        parent->vla.vla.type = ENTRY_TYPE_VLA_INFO;
        parent->vla.vla.elem_size = esz;
        parent->vla.vla.elem_count = init_cnt;
        parent->vla.vla.flex_count = 1;
        memcpy(raw_buf, val, esz);
    } else {
        char *ptr = parent->raw;

        if (idx >= parent->raw_count) {
            rk_s32 new_cnt = parent->raw_count * 2;
            rk_s32 new_size = new_cnt * esz;

            if ((new_cnt & ~0xffff) || (new_size & ~0xffff)) {
                mpp_loge_f("vla %-16s raw_count %d size %d exceeds 16bit limit\n",
                           parent->name, new_cnt, new_size);
                return rk_nok;
            }

            ptr = mpp_realloc_size(ptr, char, new_size);
            if (!ptr) {
                mpp_loge_f("vla %-16s realloc raw_buf to %d bytes failed\n",
                           parent->name, new_size);
                return rk_nok;
            }

            memset(ptr + parent->raw_size, 0, new_size - parent->raw_size);
            parent->raw = (void *)ptr;
            parent->raw_count = new_cnt;
            parent->raw_size = new_size;
        }

        memcpy(ptr + idx * esz, val, esz);
    }

    return rk_ok;
}

static rk_s32 store_vla_complex(MppCfgIoImpl *parent, MppCfgIoImpl *elem)
{
    rk_s32 idx = parent->array_count;

    if (!parent->elems) {
        rk_s32 init_cnt = VLA_INIT_CNT;
        void **elems_buf = NULL;

        elems_buf = mpp_calloc(void *, init_cnt);
        if (!elems_buf) {
            mpp_loge_f("vla %-16s calloc elems_buf failed\n", parent->name);
            return rk_nok;
        }

        parent->elems = (MppCfgIoImpl **)elems_buf;
        parent->array_size = init_cnt;
        parent->vla.vla.type = ENTRY_TYPE_VLA_INFO;
        parent->vla.vla.elem_size = sizeof(MppCfgIoImpl *);
        parent->vla.vla.elem_count = init_cnt;
        parent->vla.vla.flex_count = 1;
    } else {
        if (idx >= parent->array_size) {
            MppCfgIoImpl **ptr = parent->elems;
            rk_s32 new_cnt = parent->array_size * 2;
            rk_s32 new_size = new_cnt * sizeof(MppCfgIoImpl *);

            if ((new_cnt & ~0xffff) || (new_size & ~0xffff)) {
                mpp_loge_f("vla %-16s elem_count %d size %d exceeds 16bit limit\n",
                           parent->name, new_cnt, new_size);
                return rk_nok;
            }

            ptr = mpp_realloc_size(ptr, MppCfgIoImpl *, new_size);
            if (!ptr) {
                mpp_loge_f("vla %-16s realloc elems_buf to %d bytes failed\n",
                           parent->name, new_size);
                return rk_nok;
            }

            memset(&ptr[parent->array_size], 0,
                   (new_cnt - parent->array_size) * sizeof(MppCfgIoImpl *));
            parent->elems = ptr;
            parent->array_size = new_cnt;
        }
    }

    parent->elems[idx] = elem;
    list_del_init(&elem->list);

    return rk_ok;
}

static void finish_vla_trim(MppCfgIoImpl *parent)
{
    rk_s32 count = parent->array_count;

    if (IS_VLA_SIMPLE_TYPE(parent->array_type)) {
        parent->raw_count = count;
        parent->raw_size = count * sizeof_type(parent->array_type);
    } else {
        parent->array_size = count;
    }

    parent->vla.vla.elem_count = count;
}

static rk_s32 peek_vla_is_simple(MppCfgStrBuf *str)
{
    char *buf = show_byte_f(str, 0);

    if (!buf)
        return 0;

    switch (buf[0]) {
    case 't':
    case 'f':
    case '-':
    case '0' ... '9': {
        return 1;
    } break;
    default: {
        return 0;
    } break;
    }
}

static rk_s32 parse_vla_number_and_bool(MppCfgStrBuf *str, MppCfgType *type,
                                        MppCfgVal *val, rk_s32 peek)
{
    char *buf = NULL;
    char *b = NULL;
    rk_s32 ret;

    buf = show_byte_f(str, 0);
    if (!buf)
        goto failed;

    if (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')) {
        MppCfgType orig_type;

        ret = parse_number(str, &orig_type, val, peek);
        if (ret)
            goto failed;

        /* unify integers as s64 and floats as f64, avoid element width mismatch within array */
        if (orig_type == MPP_CFG_TYPE_f32 || orig_type == MPP_CFG_TYPE_f64) {
            *type = MPP_CFG_TYPE_f64;
            val->f64 = (double)(orig_type == MPP_CFG_TYPE_f32 ? val->f32 : val->f64);
        } else {
            *type = MPP_CFG_TYPE_s64;
            switch (orig_type) {
            case MPP_CFG_TYPE_s32: {
                val->s64 = val->s32;
            } break;
            case MPP_CFG_TYPE_u32: {
                val->s64 = (rk_s64)val->u32;
            } break;
            case MPP_CFG_TYPE_u64: {
                val->s64 = (rk_s64)val->u64;
            } break;
            case MPP_CFG_TYPE_s64:
            default: {
            } break;
            }
        }
        return ret;
    }


    if (buf[0] == 't') {
        b = test_byte_f(str, 4);
        if (b && !strncmp(b, "true", 4)) {
            val->b1 = 1;
            *type = MPP_CFG_TYPE_BOOL;
            if (!peek)
                skip_byte_f(str, 4);
            return rk_ok;
        }
        goto failed;
    }

    if (buf[0] == 'f') {
        b = test_byte_f(str, 5);
        if (b && !strncmp(b, "false", 5)) {
            val->b1 = 0;
            *type = MPP_CFG_TYPE_BOOL;
            if (!peek)
                skip_byte_f(str, 5);
            return rk_ok;
        }
        goto failed;
    }

failed:
    mpp_loge_f("parse number/bool failed at offset %d char '%c'.\n",
               str->offset, buf ? buf[0] : '\0');

    return rk_nok;
}

typedef rk_s32 (*ParseVlaValueFunc)(MppCfgIoImpl *, const char *, MppCfgStrBuf *);

static rk_s32 parse_vla_type(MppCfgIoImpl *parent, MppCfgStrBuf *str,
                             ParseVlaValueFunc parse_val)
{
    rk_s32 ret;

    if (peek_vla_is_simple(str)) {
        MppCfgVal val;
        MppCfgType type;

        ret = parse_vla_number_and_bool(str, &type, &val, 1);
        if (ret) {
            mpp_loge_f("vla %-16s failed to peek simple type for array\n",
                       parent->name);
            return ret;
        }

        parent->array_type = type;
    } else {
        MppCfgIoImpl *first_child = NULL;

        ret = parse_val(parent, NULL, str);
        if (ret) {
            mpp_loge_f("vla %-16s failed to parse first complex element\n",
                       parent->name);
            return ret;
        }

        first_child = list_last_entry(&parent->child, MppCfgIoImpl, list);

        if (IS_VLA_COMPLEX_TYPE(first_child->type)) {
            parent->array_type = first_child->type;
        } else {
            mpp_loge_f("vla %-16s first element type %s is not a valid VLA element type\n",
                       parent->name, strof_type(first_child->type));
            return rk_nok;
        }
    }

    return rk_ok;
}

static rk_s32 parse_vla_elem(MppCfgIoImpl *parent, MppCfgStrBuf *str,
                             ParseVlaValueFunc parse_val)
{
    rk_s32 idx = parent->array_count;
    rk_s32 ret;

    if (parent->array_type == MPP_CFG_TYPE_INVALID) {
        /* first element: determine mode by peeking type */
        ret = parse_vla_type(parent, str, parse_val);
        if (ret) {
            mpp_loge_f("vla %-16s failed to detect array element type\n",
                       parent->name);
            return -10;
        }
    }

    if (IS_VLA_SIMPLE_TYPE(parent->array_type)) {
        /* simple mode: all elements must be simple values */
        MppCfgVal val;
        MppCfgType num_type;

        ret = parse_vla_number_and_bool(str, &num_type, &val, 0);
        if (ret) {
            mpp_loge_f("vla %-16s element %d: expected simple type %s, got non-simple\n",
                       parent->name, idx, strof_type(parent->array_type));
            return -10;
        }
        if (num_type != parent->array_type) {
            mpp_loge_f("vla %-16s element %d: type mismatch expected %s got %s\n",
                       parent->name, idx, strof_type(parent->array_type), strof_type(num_type));
            return -10;
        }
        ret = store_vla_simple(parent, idx, &val);
        if (ret) {
            mpp_loge_f("vla %-16s element %d: failed to store simple value\n",
                       parent->name, idx);
            return -10;
        }
    } else {
        /* complex mode: all elements must be complex values of same type */
        MppCfgIoImpl *elem = NULL;

        if (parent->elems) {
            ret = parse_val(parent, NULL, str);
            if (ret)
                return ret;
        }

        elem = list_last_entry(&parent->child, MppCfgIoImpl, list);
        if (elem->type != parent->array_type) {
            mpp_loge_f("vla %-16s element %d: type mismatch expected %s got %s\n",
                       parent->name, idx, strof_type(parent->array_type), strof_type(elem->type));
            return -10;
        }
        ret = store_vla_complex(parent, elem);
        if (ret) {
            mpp_loge_f("vla %-16s element %d: failed to store complex value\n",
                       parent->name, idx);
            return -10;
        }
    }

    parent->array_count++;

    return rk_ok;
}

static rk_s32 parse_log_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str);

static rk_s32 parse_log_array(MppCfgIoImpl *obj, MppCfgStrBuf *str)
{
    MppCfgIoImpl *parent = obj;
    char *buf = NULL;
    rk_s32 old = str->offset;
    rk_s32 ret = rk_nok;

    if (str->depth >= MAX_CFG_DEPTH) {
        mpp_loge_f("depth %2d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    parent->array_count = 0;
    str->depth++;

    cfg_io_dbg_from("depth %2d offset %d array parse start\n", str->depth, str->offset);

    buf = test_byte_f(str, 0);
    if (!buf || buf[0] != '[') {
        ret = -2;
        goto failed;
    }

    buf = skip_byte_f(str, 1);
    if (!buf) {
        ret = -3;
        goto failed;
    }

    /* skip whitespace and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf) {
        ret = -4;
        goto failed;
    }

    /* check empty object */
    if (buf[0] == ']') {
        skip_byte_f(str, 1);
        cfg_io_dbg_from("depth %2d found empty array\n", str->depth);
        str->depth--;
        return rk_ok;
    }

    do {
        /* find colon for separater */
        buf = skip_ws_f(str);
        if (!buf) {
            ret = -5;
            goto failed;
        }

        ret = parse_vla_elem(parent, str, parse_log_value);
        if (ret < 0)
            goto failed;

        buf = skip_ws_f(str);
        if (!buf) {
            ret = -7;
            goto failed;
        }

        if (buf[0] == ']')
            break;
    } while (1);

    buf = skip_ws_f(str);
    if (!buf || buf[0] != ']') {
        ret = -9;
        goto failed;
    }

    skip_byte_f(str, 1);
    finish_vla_trim(parent);

    cfg_io_dbg_from("depth %2d offset %d -> %d array parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %2d offset %d -> %d array parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 parse_log_object(MppCfgIoImpl *obj, MppCfgStrBuf *str);

static rk_s32 parse_log_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str)
{
    MppCfgObj obj = NULL;
    char *buf = NULL;

    cfg_io_dbg_from("depth %2d offset %d: parse value\n", str->depth, str->offset);

    buf = test_byte_f(str, 4);
    if (buf && !strncmp(buf, "null", 4)) {
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_NULL, NULL);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value null\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    if (buf && !strncmp(buf, "true", 4)) {
        MppCfgVal val;

        val.b1 = 1;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value true\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    buf = test_byte_f(str, 5);
    if (buf && !strncmp(buf, "false", 5)) {
        MppCfgVal val;

        val.b1 = 0;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value false\n", str->depth, str->offset);
        skip_byte_f(str, 5);
        return rk_ok;
    }

    buf = test_byte_f(str, 0);
    if (buf && buf[0] == '\"') {
        MppCfgVal val;
        char *string = NULL;
        rk_s32 len = 0;

        cfg_io_dbg_from("depth %2d offset %d: get value string start\n", str->depth, str->offset);

        parse_log_string(str, &string, &len, MPP_CFG_PARSER_TYPE_VALUE);
        if (!string)
            return rk_nok;

        val.str = dup_str(string, len);
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_STRING, &val);
        mpp_cfg_add(parent, obj);
        MPP_FREE(val.str);

        cfg_io_dbg_from("depth %2d offset %d: get value string success\n", str->depth, str->offset);
        return rk_ok;
    }

    if (buf && (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9'))) {
        MppCfgType type;
        MppCfgVal val;
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value number start\n",
                        str->depth, str->offset);

        ret = parse_number(str, &type, &val, 0);
        if (ret)
            return ret;

        mpp_cfg_get_object(&obj, name, type, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value number success\n",
                        str->depth, str->offset);
        return ret;
    }

    if (buf && buf[0] == '{') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value object start\n",
                        str->depth, str->offset);

        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_OBJECT, NULL);
        mpp_cfg_add(parent, obj);

        ret = parse_log_object(obj, str);

        cfg_io_dbg_from("depth %2d offset %d: get value object ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    if (buf && buf[0] == '[') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value array start\n",
                        str->depth, str->offset);

        mpp_cfg_get_array(&obj, name);
        mpp_cfg_add(parent, obj);

        ret = parse_log_array(obj, str);

        cfg_io_dbg_from("depth %2d offset %d: get value array ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    return rk_nok;
}

static rk_s32 parse_log_object(MppCfgIoImpl *obj, MppCfgStrBuf *str)
{
    MppCfgIoImpl *parent = obj;
    char *buf = NULL;
    rk_s32 old = str->offset;
    rk_s32 ret = rk_nok;

    if (str->depth >= MAX_CFG_DEPTH) {
        mpp_loge_f("depth %2d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    str->depth++;

    cfg_io_dbg_from("depth %2d offset %d object parse start\n", str->depth, str->offset);

    buf = test_byte_f(str, 0);
    if (!buf || buf[0] != '{') {
        ret = -2;
        goto failed;
    }

    buf = skip_byte_f(str, 1);
    if (!buf) {
        ret = -3;
        goto failed;
    }

    /* skip whitespace and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf) {
        ret = -4;
        goto failed;
    }

    /* check empty object */
    if (buf[0] == '}') {
        skip_byte_f(str, 1);
        cfg_io_dbg_from("depth %2d found empty object\n", str->depth);
        str->depth--;
        return rk_ok;
    }

    do {
        rk_s32 name_len = 0;
        char *name = NULL;
        char *tmp = NULL;

        /* support array without name */
        if (buf[0] == '[') {
            MppCfgObj object = NULL;

            cfg_io_dbg_from("depth %2d offset %d: get value array start\n",
                            str->depth, str->offset);

            mpp_cfg_get_array(&object, NULL);
            mpp_cfg_add(parent, object);

            ret = parse_log_array(object, str);

            cfg_io_dbg_from("depth %2d offset %d: get value array ret %d\n",
                            str->depth, str->offset, ret);

            if (ret) {
                mpp_cfg_put_all_child(object);
                goto failed;
            }

            goto __next;
        }

        ret = parse_log_string(str, &name, &name_len, MPP_CFG_PARSER_TYPE_KEY);
        if (ret) {
            goto failed;
        }

        tmp = dup_str(name, name_len);
        cfg_io_dbg_from("depth %2d offset %d found object key %s len %d\n",
                        str->depth, str->offset, tmp, name_len);
        MPP_FREE(tmp);

        /* find colon for separater */
        buf = skip_ws_f(str);
        if (!buf || buf[0] != ':') {
            ret = -5;
            goto failed;
        }

        /* skip colon */
        buf = skip_byte_f(str, 1);
        if (!buf) {
            ret = -6;
            goto failed;
        }

        buf = skip_ws_f(str);
        if (!buf) {
            ret = -7;
            goto failed;
        }

        tmp = dup_str(name, name_len);
        if (!tmp) {
            mpp_loge_f("failed to dup name\n");
            ret = -8;
            goto failed;
        }

        /* parse value */
        ret = parse_log_value(parent, tmp, str);
        MPP_FREE(tmp);
        if (ret) {
            ret = -9;
            goto failed;
        }
    __next:
        buf = skip_ws_f(str);
        if (!buf) {
            ret = -10;
            goto failed;
        }

        if (buf[0] == '}')
            break;

        cfg_io_dbg_from("depth %2d offset %d: get next object\n", str->depth, str->offset);
    } while (1);

    skip_byte_f(str, 1);

    cfg_io_dbg_from("depth %2d offset %d -> %d object parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %2d offset %d -> %d object parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 mpp_cfg_from_log(MppCfgObj *obj, MppCfgStrBuf *str)
{
    MppCfgObj object = NULL;
    char *buf = NULL;
    rk_s32 ret = rk_ok;

    /* skip white space and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf)
        return rk_nok;

    if (buf[0] == '{') {
        ret = mpp_cfg_get_object(&object, NULL, MPP_CFG_TYPE_OBJECT, NULL);
        if (ret || !object) {
            mpp_loge_f("failed to create top object\n");
            return rk_nok;
        }

        ret = parse_log_object(object, str);
    } else if (buf[0] == '[') {
        ret = mpp_cfg_get_array(&object, NULL);
        if (ret || !object) {
            mpp_loge_f("failed to create top object\n");
            return rk_nok;
        }

        ret = parse_log_array(object, str);
    } else {
        mpp_loge_f("invalid top element '%c' on offset %d\n", buf[0], str->offset);
    }

    *obj = object;

    return ret;
}

static rk_s32 parse_json_string(MppCfgStrBuf *str, char **name, rk_s32 *len)
{
    char *buf = NULL;
    char *start = NULL;
    rk_s32 name_len = 0;

    *name = NULL;
    *len = 0;

    /* skip whitespace and find first double quotes */
    buf = skip_ws_f(str);
    if (!buf || buf[0] != '\"')
        return -101;

    buf = skip_byte_f(str, 1);
    if (!buf)
        return -102;

    start = buf;

    /* find the last double quotes */
    while ((buf = show_byte_f(str, name_len)) && buf[0] != '\"') {
        name_len++;
    }

    if (!buf || buf[0] != '\"')
        return -103;

    /* find complete string skip the string and double quotes */
    buf = skip_byte_f(str, name_len + 1);
    if (!buf)
        return -104;

    *name = start;
    *len = name_len;

    return rk_ok;
}

static rk_s32 parse_json_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str);
static rk_s32 parse_json_array(MppCfgIoImpl *obj, MppCfgStrBuf *str);

static rk_s32 parse_json_object(MppCfgIoImpl *obj, MppCfgStrBuf *str)
{
    MppCfgIoImpl *parent = obj;
    char *buf = NULL;
    rk_s32 old = str->offset;
    rk_s32 ret = rk_nok;

    if (str->depth >= MAX_CFG_DEPTH) {
        mpp_loge_f("depth %2d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    str->depth++;

    cfg_io_dbg_from("depth %2d offset %d object parse start\n", str->depth, str->offset);

    buf = test_byte_f(str, 0);
    if (!buf || buf[0] != '{') {
        ret = -2;
        goto failed;
    }

    buf = skip_byte_f(str, 1);
    if (!buf) {
        ret = -3;
        goto failed;
    }

    /* skip whitespace and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf) {
        ret = -4;
        goto failed;
    }

    /* check empty object */
    if (buf[0] == '}') {
        skip_byte_f(str, 1);
        cfg_io_dbg_from("depth %2d found empty object\n", str->depth);
        str->depth--;
        return rk_ok;
    }

    do {
        rk_s32 name_len = 0;
        char *name = NULL;
        char *tmp = NULL;

        if (buf[0] == '[') {
            MppCfgObj object = NULL;

            cfg_io_dbg_from("depth %2d offset %d: get value array start\n",
                            str->depth, str->offset);

            mpp_cfg_get_array(&object, NULL);
            mpp_cfg_add(parent, object);

            ret = parse_json_array(object, str);

            cfg_io_dbg_from("depth %2d offset %d: get value array ret %d\n",
                            str->depth, str->offset, ret);

            if (ret) {
                mpp_cfg_put_all_child(object);
                goto failed;
            }

            goto __next;
        }

        ret = parse_json_string(str, &name, &name_len);
        if (ret) {
            goto failed;
        }

        /* find colon for separater */
        buf = skip_ws_f(str);
        if (!buf || buf[0] != ':') {
            ret = -5;
            goto failed;
        }

        /* skip colon */
        buf = skip_byte_f(str, 1);
        if (!buf) {
            ret = -6;
            goto failed;
        }

        buf = skip_ws_f(str);
        if (!buf) {
            ret = -7;
            goto failed;
        }

        tmp = dup_str(name, name_len);
        if (!tmp) {
            mpp_loge_f("failed to dup name\n");
            ret = -8;
            goto failed;
        }

        /* parse value */
        ret = parse_json_value(parent, tmp, str);
        MPP_FREE(tmp);
        if (ret) {
            ret = -9;
            goto failed;
        }
    __next:
        buf = skip_ws_f(str);
        if (!buf) {
            ret = -10;
            goto failed;
        }

        if (buf[0] == ',') {
            buf = skip_byte_f(str, 1);
            if (!buf) {
                ret = -11;
                goto failed;
            }

            buf = skip_ws_f(str);
            if (buf[0] == '}')
                break;

            cfg_io_dbg_from("depth %2d offset %d: get next object\n", str->depth, str->offset);
            continue;
        }

        break;
    } while (1);

    buf = skip_ws_f(str);
    if (!buf || buf[0] != '}') {
        ret = -12;
        goto failed;
    }

    skip_byte_f(str, 1);

    cfg_io_dbg_from("depth %2d offset %d -> %d object parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %2d offset %d -> %d object parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 parse_json_array(MppCfgIoImpl *obj, MppCfgStrBuf *str)
{
    MppCfgIoImpl *parent = obj;
    char *buf = NULL;
    rk_s32 old = str->offset;
    rk_s32 ret = rk_nok;

    if (str->depth >= MAX_CFG_DEPTH) {
        mpp_loge_f("depth %2d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    parent->array_count = 0;
    str->depth++;

    cfg_io_dbg_from("depth %2d offset %d array parse start\n", str->depth, str->offset);

    buf = test_byte_f(str, 0);
    if (!buf || buf[0] != '[') {
        ret = -2;
        goto failed;
    }

    buf = skip_byte_f(str, 1);
    if (!buf) {
        ret = -3;
        goto failed;
    }

    /* skip whitespace and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf) {
        ret = -4;
        goto failed;
    }

    /* check empty object */
    if (buf[0] == ']') {
        skip_byte_f(str, 1);
        cfg_io_dbg_from("depth %2d found empty array\n", str->depth);
        str->depth--;
        return rk_ok;
    }

    do {
        /* find colon for separater */
        buf = skip_ws_f(str);
        if (!buf) {
            ret = -5;
            goto failed;
        }

        ret = parse_vla_elem(parent, str, parse_json_value);
        if (ret < 0)
            goto failed;

        buf = skip_ws_f(str);
        if (!buf) {
            ret = -7;
            goto failed;
        }

        if (buf[0] == ',') {
            buf = skip_byte_f(str, 1);
            if (!buf) {
                ret = -8;
                goto failed;
            }

            buf = skip_ws_f(str);
            if (buf[0] == ']')
                break;

            cfg_io_dbg_from("depth %2d offset %d: get next array\n", str->depth, str->offset);
            continue;
        }
        break;
    } while (1);

    buf = skip_ws_f(str);
    if (!buf || buf[0] != ']') {
        ret = -9;
        goto failed;
    }

    skip_byte_f(str, 1);
    finish_vla_trim(parent);

    cfg_io_dbg_from("depth %2d offset %d -> %d array parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %2d offset %d -> %d array parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 parse_json_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str)
{
    MppCfgObj obj = NULL;
    char *buf = NULL;

    cfg_io_dbg_from("depth %2d offset %d: parse value\n", str->depth, str->offset);

    buf = test_byte_f(str, 4);
    if (buf && !strncmp(buf, "null", 4)) {
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_NULL, NULL);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value null\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    if (buf && !strncmp(buf, "true", 4)) {
        MppCfgVal val;

        val.b1 = 1;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value true\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    buf = test_byte_f(str, 5);
    if (buf && !strncmp(buf, "false", 5)) {
        MppCfgVal val;

        val.b1 = 0;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value false\n", str->depth, str->offset);
        skip_byte_f(str, 5);
        return rk_ok;
    }

    buf = test_byte_f(str, 0);
    if (buf && buf[0] == '\"') {
        MppCfgVal val;
        char *string = NULL;
        rk_s32 len = 0;

        cfg_io_dbg_from("depth %2d offset %d: get value string start\n", str->depth, str->offset);

        parse_json_string(str, &string, &len);
        if (!string)
            return rk_nok;

        val.str = dup_str(string, len);
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_STRING, &val);
        mpp_cfg_add(parent, obj);
        MPP_FREE(val.str);

        cfg_io_dbg_from("depth %2d offset %d: get value string success\n", str->depth, str->offset);
        return rk_ok;
    }

    if (buf && (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9'))) {
        MppCfgType type;
        MppCfgVal val;
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value number start\n",
                        str->depth, str->offset);

        ret = parse_number(str, &type, &val, 0);
        if (ret)
            return ret;

        mpp_cfg_get_object(&obj, name, type, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value number success\n",
                        str->depth, str->offset);
        return ret;
    }

    if (buf && buf[0] == '{') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value object start\n",
                        str->depth, str->offset);

        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_OBJECT, NULL);
        mpp_cfg_add(parent, obj);

        ret = parse_json_object(obj, str);

        cfg_io_dbg_from("depth %2d offset %d: get value object ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    if (buf && buf[0] == '[') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value array start\n",
                        str->depth, str->offset);

        mpp_cfg_get_array(&obj, name);
        mpp_cfg_add(parent, obj);

        ret = parse_json_array(obj, str);

        cfg_io_dbg_from("depth %2d offset %d: get value array ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    return rk_nok;
}

static rk_s32 mpp_cfg_from_json(MppCfgObj *obj, MppCfgStrBuf *str)
{
    MppCfgObj object = NULL;
    char *buf = NULL;
    rk_s32 ret = rk_ok;

    /* skip UTF-8 */
    buf = test_byte_f(str, 4);
    if (buf && !strncmp(buf, "\xEF\xBB\xBF", 3))
        skip_byte_f(str, 3);

    /* skip white space and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf)
        return rk_nok;

    if (buf[0] == '{') {
        ret = mpp_cfg_get_object(&object, NULL, MPP_CFG_TYPE_OBJECT, NULL);
        if (ret || !object) {
            mpp_loge_f("failed to create top object\n");
            return rk_nok;
        }

        ret = parse_json_object(object, str);
    } else if (buf[0] == '[') {
        ret = mpp_cfg_get_array(&object, NULL);
        if (ret || !object) {
            mpp_loge_f("failed to create top object\n");
            return rk_nok;
        }

        ret = parse_json_array(object, str);
    } else {
        mpp_loge_f("invalid top element '%c' on offset %d\n", buf[0], str->offset);
    }

    *obj = object;

    return ret;
}

static rk_s32 parse_toml_nested_table(MppCfgIoImpl *root, MppCfgObj *object, char *name,
                                      rk_s32 name_len)
{
    MppCfgObj obj = NULL;
    MppCfgIoImpl *parent = root;
    rk_s32 i = 0;
    char sub_name_offset = 0;
    char sub_name_len = 0;
    char sub_name[256] = {0};
    rk_s32 ret = rk_ok;

    for (i = 0; i <= name_len; i++) {
        if (name[i] == '.' || name[i] == '\0') {
            sub_name_len = i;
            memcpy(sub_name, name, sub_name_len);
            sub_name[i] = '\0';
            obj = NULL;
            mpp_cfg_find(&obj, root, sub_name, MPP_CFG_STR_FMT_TOML);
            if (!obj) {
                memcpy(sub_name, name + sub_name_offset, sub_name_len - sub_name_offset);
                sub_name[sub_name_len - sub_name_offset] = '\0';
                ret = mpp_cfg_get_object(&obj, sub_name, MPP_CFG_TYPE_OBJECT, NULL);
                if (ret || !obj) {
                    mpp_loge_f("failed to create object %s\n", name);
                    ret = -101;
                    return ret;
                }
                mpp_cfg_add(parent, obj);
            }

            parent = obj;
            sub_name_offset = i + 1;
        }
    }

    *object = obj;

    return ret;
}

static rk_s32 parse_toml_nested_array_table(MppCfgIoImpl *root, MppCfgObj *object, char *name,
                                            rk_s32 name_len)
{
    MppCfgObj obj = NULL;
    MppCfgIoImpl *parent = root;
    rk_s32 i = 0;
    char sub_name_offset = 0;
    char sub_name_len = 0;
    char sub_name[256] = {0};
    rk_s32 ret = rk_ok;

    for (i = 0; i <= name_len; i++) {
        if (name[i] == '.' || name[i] == '\0') {
            sub_name_len = i;
            memcpy(sub_name, name, sub_name_len);
            sub_name[i] = '\0';
            obj = NULL;
            mpp_cfg_find(&obj, root, sub_name, MPP_CFG_STR_FMT_TOML);
            if (!obj) {
                memcpy(sub_name, name + sub_name_offset, sub_name_len - sub_name_offset);
                sub_name[sub_name_len - sub_name_offset] = '\0';

                /* if parent type is array, need get its last child as new parent */
                if (parent->type == MPP_CFG_TYPE_ARRAY) {
                    MppCfgIoImpl *child_pos, *child_n;
                    MppCfgIoImpl *last_child = NULL;
                    list_for_each_entry_safe(child_pos, child_n, &parent->child, MppCfgIoImpl, list) {
                        if (!child_pos->name && child_pos->type == MPP_CFG_TYPE_OBJECT) {
                            last_child = child_pos;
                        }
                    }
                    if (!last_child) {
                        mpp_loge_f("failed to find last child\n");
                        ret = -111;
                        return ret;
                    }
                    parent = last_child;
                }
                if (name[i] == '\0') {
                    ret = mpp_cfg_get_array(&obj, sub_name);
                    if (ret || !obj) {
                        mpp_loge_f("failed to create object %s\n", name);
                        ret = -112;
                        return ret;
                    }
                    mpp_cfg_add(parent, obj);
                } else {
                    ret = mpp_cfg_get_object(&obj, sub_name, MPP_CFG_TYPE_OBJECT, NULL);
                    if (ret || !obj) {
                        mpp_loge_f("failed to create nested object %s\n", name);
                        ret = -113;
                        return ret;
                    }
                    mpp_cfg_add(parent, obj);
                }
            }

            parent = obj;
            sub_name_offset = i + 1;
        }
    }

    *object = obj;

    return ret;
}

static rk_s32 parse_toml_string(MppCfgStrBuf *str, char **name, rk_s32 *len, rk_u32 type)
{
    char *buf = NULL;
    char *start = NULL;
    rk_s32 name_len = 0;
    char terminator;

    *name = NULL;
    *len = 0;

    /* skip whitespace and find first double quotes */
    buf = skip_ws_f(str);
    if (!buf)
        return -201;

    if (type == MPP_CFG_PARSER_TYPE_VALUE) {
        terminator = '\"';
        if (buf[0] != '\"')
            return -202;

        buf = skip_byte_f(str, 1);
        if (!buf)
            return -203;
    } else if (type == MPP_CFG_PARSER_TYPE_KEY) {
        terminator = ' ';
    } else if (type == MPP_CFG_PARSER_TYPE_TABLE || type == MPP_CFG_PARSER_TYPE_ARRAY_TABLE) {
        terminator = ']';
    } else {
        return -204;
    }

    start = buf;

    /* find the terminator */
    while ((buf = show_byte_f(str, name_len)) && buf[0] != terminator) {
        name_len++;
    }

    if (!buf || buf[0] != terminator)
        return -205;

    /* find complete string skip the string */
    if (type == MPP_CFG_PARSER_TYPE_VALUE)
        buf = skip_byte_f(str, name_len + 1);
    else
        buf = skip_byte_f(str, name_len);
    if (!buf)
        return -206;

    *name = start;
    *len = name_len;

    return rk_ok;
}

static rk_s32 parse_toml_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str);
static rk_s32 parse_toml_object(MppCfgIoImpl *parent, MppCfgStrBuf *str, rk_s32 is_brace);

static rk_s32 parse_toml_array(MppCfgIoImpl *obj, MppCfgStrBuf *str)
{
    MppCfgIoImpl *parent = obj;
    char *buf = NULL;
    rk_s32 old = str->offset;
    rk_s32 ret = rk_nok;

    if (str->depth >= MAX_CFG_DEPTH) {
        mpp_loge_f("depth %2d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    parent->array_count = 0;
    str->depth++;

    cfg_io_dbg_from("depth %2d offset %d array parse start\n", str->depth, str->offset);

    buf = test_byte_f(str, 0);
    if (!buf || buf[0] != '[') {
        ret = -61;
        goto failed;
    }

    buf = skip_byte_f(str, 1);
    if (!buf) {
        ret = -62;
        goto failed;
    }

    /* skip whitespace and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf) {
        ret = -63;
        goto failed;
    }

    /* check empty object */
    if (buf[0] == ']') {
        skip_byte_f(str, 1);
        cfg_io_dbg_from("depth %2d found empty array\n", str->depth);
        str->depth--;
        return rk_ok;
    }

    do {
        buf = skip_ws_f(str);
        if (!buf) {
            ret = -64;
            goto failed;
        }

        ret = parse_vla_elem(parent, str, parse_toml_value);
        if (ret < 0)
            goto failed;

        buf = skip_ws_f(str);
        if (!buf) {
            ret = -66;
            goto failed;
        }

        if (buf[0] == ',') {
            buf = skip_byte_f(str, 1);
            if (!buf) {
                ret = -67;
                goto failed;
            }

            buf = skip_ws_f(str);
            if (buf[0] == '}')
                break;

            cfg_io_dbg_from("depth %2d offset %d: get next array\n", str->depth, str->offset);
            continue;
        }
        break;
    } while (1);

    buf = skip_ws_f(str);
    if (!buf || buf[0] != ']') {
        ret = -68;
        goto failed;
    }

    skip_byte_f(str, 1);
    finish_vla_trim(parent);

    cfg_io_dbg_from("depth %2d offset %d -> %d array parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %2d offset %d -> %d array parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 parse_toml_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str)
{
    MppCfgObj obj = NULL;
    char *buf = NULL;

    cfg_io_dbg_from("depth %2d offset %d: parse value\n", str->depth, str->offset);

    buf = test_byte_f(str, 4);
    if (buf && !strncmp(buf, "null", 4)) {
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_NULL, NULL);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value null\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    if (buf && !strncmp(buf, "true", 4)) {
        MppCfgVal val;

        val.b1 = 1;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value true\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    buf = test_byte_f(str, 5);
    if (buf && !strncmp(buf, "false", 5)) {
        MppCfgVal val;

        val.b1 = 0;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value false\n", str->depth, str->offset);
        skip_byte_f(str, 5);
        return rk_ok;
    }

    buf = test_byte_f(str, 3);
    if (buf && !strncmp(buf, "\"\"\"", 3)) {
        MppCfgVal val;
        char *string = NULL;
        rk_s32 len = 0;

        skip_byte_f(str, 2);
        cfg_io_dbg_from("depth %2d offset %d: get value multi line string start\n", str->depth, str->offset);

        parse_toml_string(str, &string, &len, MPP_CFG_PARSER_TYPE_VALUE);
        if (!string)
            return rk_nok;
        buf = test_byte_f(str, 1);
        if (!buf || strncmp(buf, "\"\"", 2)) {
            return rk_nok;
        }
        skip_byte_f(str, 2);

        val.str = dup_str(string, len);
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_STRING, &val);
        mpp_cfg_add(parent, obj);
        MPP_FREE(val.str);

        cfg_io_dbg_from("depth %2d offset %d: get value multi line string success\n", str->depth, str->offset);
        return rk_ok;
    }

    buf = test_byte_f(str, 0);
    if (buf && buf[0] == '\"') {
        MppCfgVal val;
        char *string = NULL;
        rk_s32 len = 0;

        cfg_io_dbg_from("depth %2d offset %d: get value string start\n", str->depth, str->offset);

        parse_toml_string(str, &string, &len, MPP_CFG_PARSER_TYPE_VALUE);
        if (!string)
            return rk_nok;

        val.str = dup_str(string, len);
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_STRING, &val);
        mpp_cfg_add(parent, obj);
        MPP_FREE(val.str);

        cfg_io_dbg_from("depth %2d offset %d: get value string success\n", str->depth, str->offset);
        return rk_ok;
    }

    if (buf && (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9'))) {
        MppCfgType type;
        MppCfgVal val;
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value number start\n",
                        str->depth, str->offset);

        ret = parse_number(str, &type, &val, 0);
        if (ret)
            return ret;

        mpp_cfg_get_object(&obj, name, type, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %2d offset %d: get value number success\n",
                        str->depth, str->offset);
        return ret;
    }

    if (buf && buf[0] == '{') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value object start\n",
                        str->depth, str->offset);

        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_OBJECT, NULL);
        mpp_cfg_add(parent, obj);

        ret = parse_toml_object(obj, str, 1);

        cfg_io_dbg_from("depth %2d offset %d: get value object ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    if (buf && buf[0] == '[') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %2d offset %d: get value array start\n",
                        str->depth, str->offset);

        mpp_cfg_get_array(&obj, name);
        mpp_cfg_add(parent, obj);

        ret = parse_toml_array(obj, str);

        cfg_io_dbg_from("depth %2d offset %d: get value array ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    return rk_nok;
}

static rk_s32 parse_toml_object(MppCfgIoImpl *parent, MppCfgStrBuf *str, rk_s32 is_brace)
{
    char *buf = NULL;
    rk_s32 ret = rk_nok;
    rk_s32 old = str->offset;

    if (str->depth >= MAX_CFG_DEPTH) {
        mpp_loge_f("depth %2d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    str->depth++;
    /* skip whitespace and check the end of buffer */
    if (is_brace) {
        buf = test_byte_f(str, 0);
        if (!buf || buf[0] != '{') {
            ret = -31;
            goto failed;
        }

        buf = skip_byte_f(str, 1);
        if (!buf) {
            ret = -32;
            goto failed;
        }

        /* skip whitespace and check the end of buffer */
        buf = skip_ws_f(str);
        if (!buf) {
            ret = -33;
            goto failed;
        }

        /* check empty object */
        if (buf[0] == '}') {
            skip_byte_f(str, 1);
            cfg_io_dbg_from("depth %2d found empty object\n", str->depth);
            str->depth--;
            return rk_ok;
        }
    } else {
        buf = skip_ws_f(str);
        if (!buf) {
            ret = -34;
            goto failed;
        }
    }

    do {
        rk_s32 name_len = 0;
        char *name = NULL;
        char *tmp = NULL;

        if (buf[0] == '[') {
            MppCfgObj object = NULL;

            cfg_io_dbg_from("depth %2d offset %d: get value array start\n",
                            str->depth, str->offset);

            mpp_cfg_get_array(&object, NULL);
            mpp_cfg_add(parent, object);

            ret = parse_toml_array(object, str);

            cfg_io_dbg_from("depth %2d offset %d: get value array ret %d\n",
                            str->depth, str->offset, ret);

            if (ret) {
                mpp_cfg_put_all_child(object);
                goto failed;
            }

            goto __next;
        }

        ret = parse_toml_string(str, &name, &name_len, MPP_CFG_PARSER_TYPE_KEY);
        if (ret) {
            ret = -35;
            goto failed;
        }

        /* find equal for separater */
        buf = skip_ws_f(str);
        if (!buf || buf[0] != '=') {
            ret = -36;
            goto failed;
        }

        /* skip equal */
        buf = skip_byte_f(str, 1);
        if (!buf) {
            ret = -37;
            goto failed;
        }

        buf = skip_ws_f(str);
        if (!buf) {
            ret = -38;
            goto failed;
        }

        tmp = dup_str(name, name_len);
        if (!tmp) {
            mpp_loge_f("failed to dup name\n");
            ret = -39;
            goto failed;
        }

        /* parse value */
        ret = parse_toml_value(parent, tmp, str);
        MPP_FREE(tmp);
        if (ret) {
            ret = -40;
            goto failed;
        }
    __next:
        buf = skip_ws_f(str);
        if (!buf || buf[0] == '[' || buf[0] == '}')
            break;

        if (buf[0] == ',') {
            buf = skip_byte_f(str, 1);
            if (!buf) {
                ret = -41;
                goto failed;
            }

            buf = skip_ws_f(str);
            if (buf[0] == '[' || buf[0] == '}')
                break;

            cfg_io_dbg_from("depth %2d offset %d: get next object\n", str->depth, str->offset);
        }
    } while (1);

    if (is_brace) {
        if (buf && buf[0] == '}')
            skip_byte_f(str, 1);
        else {
            ret = -42;
            goto failed;
        }
    }

    cfg_io_dbg_from("depth %2d offset %d -> %d object parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %2d offset %d -> %d object parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 parse_toml_table(MppCfgIoImpl *parent, MppCfgStrBuf *str)
{
    MppCfgObj obj = NULL;
    char *buf = NULL;
    rk_s32 ret = rk_nok;
    rk_s32 name_len = 0;
    char *name = NULL;
    char *tmp = NULL;

    ret = parse_toml_string(str, &name, &name_len, MPP_CFG_PARSER_TYPE_TABLE);
    if (ret) {
        ret = -11;
        goto failed;
    }

    tmp = dup_str(name, name_len);
    if (!tmp) {
        mpp_loge_f("failed to dup tmp\n");
        ret = -12;
        goto failed;
    }

    if (strchr(tmp, '.')) {
        ret = parse_toml_nested_table(parent, &obj, tmp, name_len);
        MPP_FREE(tmp);
        if (ret || !obj) {
            return ret;
        }
    } else {
        ret = mpp_cfg_get_object(&obj, tmp, MPP_CFG_TYPE_OBJECT, NULL);
        MPP_FREE(tmp);
        if (ret || !obj) {
            mpp_loge_f("failed to create object %s\n", tmp);
            ret = -13;
            goto failed;
        }
        mpp_cfg_add(parent, obj);
    }

    buf = test_byte_f(str, 0);
    if (!buf || buf[0] != ']') {
        ret = -14;
        goto failed;
    }

    buf = skip_byte_f(str, 1);
    if (!buf) {
        ret = -15;
        goto failed;
    }

    buf = skip_ws_f(str);
    if (!buf)
        return rk_nok;

    if (buf[0] == '[')
        ret = rk_ok;
    else
        ret = parse_toml_object(obj, str, 0);

failed:
    if (ret)
        cfg_io_dbg_from("table parse failed ret %d\n", ret);

    return ret;
}

static rk_s32 parse_toml_array_table(MppCfgIoImpl *parent, MppCfgStrBuf *str)
{
    MppCfgObj obj = NULL;
    char *buf = NULL;
    rk_s32 ret = rk_nok;
    rk_s32 name_len = 0;
    char *name = NULL;
    char *tmp = NULL;

    ret = parse_toml_string(str, &name, &name_len, MPP_CFG_PARSER_TYPE_ARRAY_TABLE);
    if (ret) {
        ret = -22;
        goto failed;
    }

    tmp = dup_str(name, name_len);
    if (!tmp) {
        mpp_loge_f("failed to dup tmp\n");
        ret = -23;
        goto failed;
    }

    if (strchr(tmp, '.')) {
        ret = parse_toml_nested_array_table(parent, &obj, tmp, name_len);
        MPP_FREE(tmp);
        if (ret || !obj) {
            return ret;
        }
    } else {
        mpp_cfg_find(&obj, parent, tmp, MPP_CFG_STR_FMT_TOML);
        if (!obj) {
            ret = mpp_cfg_get_array(&obj, tmp);
            MPP_FREE(tmp);
            if (ret || !obj) {
                mpp_loge_f("failed to create object %s\n", tmp);
                ret = -24;
                goto failed;
            }
            mpp_cfg_add(parent, obj);
        } else {
            MPP_FREE(tmp);
        }
    }

    /* array object need create object as child */
    parent = obj;
    obj = NULL;
    mpp_cfg_get_object(&obj, NULL, MPP_CFG_TYPE_OBJECT, NULL);
    mpp_cfg_add(parent, obj);

    buf = test_byte_f(str, 1);
    if (!buf || strncmp(buf, "]]", 2)) {
        ret = -25;
        goto failed;
    }

    buf = skip_byte_f(str, 2);
    if (!buf) {
        ret = -26;
        goto failed;
    }

    buf = skip_ws_f(str);
    if (!buf)
        return rk_nok;

    if (buf[0] == '[')
        ret = rk_ok;
    else
        ret = parse_toml_object(obj, str, 0);

failed:
    if (ret)
        cfg_io_dbg_from("array table parse failed ret %d\n", ret);
    return ret;
}

static rk_s32 parse_toml_section(MppCfgIoImpl *parent, MppCfgStrBuf *str)
{
    char *buf = NULL;
    rk_s32 ret = rk_nok;
    rk_s32 old = str->offset;

    if (str->depth >= MAX_CFG_DEPTH) {
        mpp_loge_f("depth %2d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    buf = test_byte_f(str, 0);
    if (!buf) {
        ret = -2;
        goto failed;
    }

    if (buf[0] == '[') {
        str->depth++;

        buf = skip_byte_f(str, 1);
        if (!buf) {
            ret = -3;
            goto failed;
        }
        if (buf[0] != '[') {
            ret = parse_toml_table(parent, str);
            if (ret)
                goto failed;
        } else {
            buf = skip_byte_f(str, 1);
            if (!buf) {
                ret = -4;
                goto failed;
            }

            ret = parse_toml_array_table(parent, str);
            if (ret)
                goto failed;
        }
        str->depth--;
    } else {
        ret = parse_toml_object(parent, str, 0);
        if (ret)
            goto failed;
    }
    cfg_io_dbg_from("depth %2d offset %d -> %d section parse success\n",
                    str->depth, old, str->offset);

    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %2d offset %d -> %d section parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 mpp_cfg_from_toml(MppCfgObj *obj, MppCfgStrBuf *str)
{
    MppCfgObj object = NULL;
    char *buf = NULL;
    rk_s32 ret = rk_ok;

    /* skip white space and check the end of buffer */
    buf = skip_ws_f(str);
    if (!buf)
        return rk_nok;

    ret = mpp_cfg_get_object(&object, NULL, MPP_CFG_TYPE_OBJECT, NULL);
    if (ret || !object) {
        mpp_loge_f("failed to create top object\n");
        return rk_nok;
    }

    do {
        /* parse section */
        ret = parse_toml_section(object, str);
        if (ret) {
            mpp_loge_f("failed to parse section, ret : %d.\n", ret);
            return rk_nok;
        }

        buf = skip_ws_f(str);
        if (!buf)
            break;
    } while (1);

    *obj = object;

    return ret;
}

static rk_s32 dump_find_max_name(MppCfgIoImpl *impl)
{
    MppCfgIoImpl *pos;
    rk_s32 max_len = 0;

    list_for_each_entry(pos, &impl->detail, MppCfgIoImpl, list) {
        rk_s32 len = pos->name ? (rk_s32)strlen(pos->name) : 2;

        if (len > max_len)
            max_len = len;

        if (pos->type == MPP_CFG_TYPE_OBJECT || pos->type == MPP_CFG_TYPE_ARRAY) {
            rk_s32 child_max = dump_find_max_name(pos);

            if (child_max > max_len)
                max_len = child_max;
        }
    }

    return max_len;
}

static void dump_detail(MppCfgIoImpl *impl, rk_s32 depth, rk_s32 name_w)
{
    MppCfgIoImpl *pos;
    char line[256];
    rk_s32 indent;
    rk_s32 n;

    if (list_empty(&impl->detail))
        return;

    indent = depth * 2;

    list_for_each_entry(pos, &impl->detail, MppCfgIoImpl, list) {
        const char *name = pos->name ? pos->name : "n/a";

        if (pos->type == MPP_CFG_TYPE_ARRAY) {
            struct KmppEntryVLAInfo *vla = &pos->vla.vla;

            n = snprintf(line, sizeof(line), "%*s%-*s %-8s esz %-3d",
                         indent + 2, "", name_w, name,
                         "array", vla->elem_size);
            if (vla->flex_count)
                n += snprintf(line + n, sizeof(line) - n, " cnt@%-3d", vla->count_off);
            else
                n += snprintf(line + n, sizeof(line) - n, " cnt %-3d", vla->elem_count);
            if (vla->flex_base)
                snprintf(line + n, sizeof(line) - n, " base@%-3d", vla->base_off);

            mpp_logi("%s\n", line);
            dump_detail(pos, depth + 1, name_w);
        } else if (pos->type == MPP_CFG_TYPE_OBJECT) {
            mpp_logi("%*s%-*s %-8s\n",
                     indent + 2, "", name_w, name,
                     "object");
            dump_detail(pos, depth + 1, name_w);
        } else {
            mpp_logi("%*s%-*s %-8s off %-3d sz %-3d\n",
                     indent + 2, "", name_w, name,
                     strof_type(pos->type),
                     pos->entry.tbl.elem_offset,
                     pos->entry.tbl.elem_size);
        }
    }
}

void mpp_cfg_dump(MppCfgObj obj, const char *func)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;
    rk_s32 name_w;

    if (!obj) {
        mpp_loge_f("invalid param obj %p at %s\n", obj, func);
        return;
    }

    name_w = dump_find_max_name(impl);

    mpp_logi_f("%s at %s\n", impl->name ? impl->name : "n/a", func);
    dump_detail(impl, 0, name_w);
}

rk_s32 mpp_cfg_to_string(MppCfgObj obj, MppCfgStrFmt fmt, char **buf)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;
    MppCfgStrBuf str;
    rk_s32 ret = rk_nok;

    if (!obj || !buf || fmt >= MPP_CFG_STR_FMT_BUTT) {
        mpp_loge_f("invalid param obj %p fmt %d buf %p\n", obj, fmt, buf);
        return ret;
    }

    mpp_env_get_u32("mpp_cfg_io_debug", &mpp_cfg_io_debug, mpp_cfg_io_debug);

    str.buf_size = 4096;
    str.buf = mpp_malloc_size(void, str.buf_size);
    str.offset = 0;
    str.depth = 0;
    str.type = fmt;

    switch (fmt) {
    case MPP_CFG_STR_FMT_LOG : {
        ret = mpp_cfg_to_log(impl, &str);
    } break;
    case MPP_CFG_STR_FMT_JSON : {
        ret = mpp_cfg_to_json(impl, &str);
    } break;
    case MPP_CFG_STR_FMT_TOML : {
        ret = mpp_cfg_to_toml(impl, &str);
    } break;
    default : {
        mpp_loge_f("obj %-16s invalid format %d\n", impl->name, fmt);
    } break;
    }

    if (ret) {
        mpp_loge_f("obj %-16s %p failed to get string buffer\n",
                   impl->name, impl);
        MPP_FREE(str.buf);
    }

    *buf = str.buf;
    return ret;
}

rk_s32 mpp_cfg_from_string(MppCfgObj *obj, MppCfgStrFmt fmt, const char *buf)
{
    MppCfgObj object = NULL;
    rk_s32 size;
    rk_s32 ret = rk_nok;

    if (!obj || fmt >= MPP_CFG_STR_FMT_BUTT || !buf) {
        mpp_loge_f("invalid param obj %p fmt %d buf %p\n", obj, fmt, buf);
        return ret;
    }

    mpp_env_get_u32("mpp_cfg_io_debug", &mpp_cfg_io_debug, mpp_cfg_io_debug);

    size = strlen(buf);
    if (size) {
        MppCfgStrBuf str;

        size++;

        str.buf = (char *)buf;
        str.buf_size = size;
        str.offset = 0;
        str.depth = 0;
        str.type = fmt;

        cfg_io_dbg_from("buf %p size %d\n", buf, size);
        cfg_io_dbg_from("%s", buf);

        switch (fmt) {
        case MPP_CFG_STR_FMT_LOG : {
            ret = mpp_cfg_from_log(&object, &str);
        } break;
        case MPP_CFG_STR_FMT_JSON : {
            ret = mpp_cfg_from_json(&object, &str);
        } break;
        case MPP_CFG_STR_FMT_TOML : {
            ret = mpp_cfg_from_toml(&object, &str);
        } break;
        default : {
            mpp_loge_f("invalid formoffset %d\n", fmt);
        } break;
        }
    }

    if (ret)
        mpp_loge_f("buf %p size %d failed to get object\n", buf, size);

    *obj = object;
    return ret;
}

rk_s32 mpp_cfg_from_file(MppCfgObj *obj, MppCfgStrFmt fmt, const char *path)
{
    char *buf = NULL;
    rk_s32 size = 0;
    rk_s32 len = 0;
    rk_s32 fd;
    rk_s32 ret = rk_nok;

    if (!obj || fmt >= MPP_CFG_STR_FMT_BUTT || !path) {
        mpp_loge_f("invalid param obj %p fmt %d path %p\n", obj, fmt, path);

        return ret;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        mpp_loge_f("open file %s failed for %s\n", path, strerror(errno));

        return ret;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        mpp_loge_f("lseek file %s failed for %s\n", path, strerror(errno));
        goto done;
    }

    lseek(fd, 0, SEEK_SET);

    buf = mpp_malloc_size(char, size + 1);
    if (!buf) {
        mpp_loge_f("malloc buffer size %d for file %s failed\n", size + 1, path);
        goto done;
    }

    while (len < size) {
        rk_s32 read_len = read(fd, buf + len, size - len);

        if (read_len < 0) {
            if (errno == EINTR)
                continue;

            mpp_loge_f("read file %s failed for %s\n", path, strerror(errno));
            goto done;
        }

        if (!read_len)
            break;

        len += read_len;
    }

    buf[len] = '\0';
    ret = mpp_cfg_from_string(obj, fmt, buf);

done:
    MPP_FREE(buf);
    if (fd >= 0)
        close(fd);

    return ret;
}

static MppCfgIoImpl *type_find_child(MppCfgIoImpl *type, const char *name)
{
    MppCfgIoImpl *pos;

    if (!type || !name)
        return NULL;

    list_for_each_entry(pos, &type->child, MppCfgIoImpl, list) {
        if (pos->name && !strcmp(pos->name, name))
            return pos;
    }

    list_for_each_entry(pos, &type->detail, MppCfgIoImpl, list) {
        if (pos->name && !strcmp(pos->name, name))
            return pos;
    }

    return NULL;
}

/* Safe read of VLA flex offset with alignment check */
static rk_s32 vla_read_count(void *st, struct KmppEntryVLAInfo *vla)
{
    if (vla->flex_count) {
        if (vla->count_off & 0x3) {
            mpp_loge_f("vla count_off %d not aligned\n", vla->count_off);
            return 0;
        }
        return *(rk_s32 *)((rk_u8 *)st + vla->count_off);
    }
    return (rk_s32)vla->elem_count;
}

static rk_u32 vla_read_base(void *st, MppCfgIoImpl *type, KmppEntry *tbl)
{
    if (type && type->vla.vla.type == ENTRY_TYPE_VLA_INFO) {
        struct KmppEntryVLAInfo *vla = &type->vla.vla;

        if (vla->flex_base) {
            if (vla->base_off & 0x3) {
                mpp_loge_f("vla base_off %d not aligned\n", vla->base_off);
                return 0;
            }
            return *(rk_u32 *)((rk_u8 *)st + vla->base_off);
        }

        return vla->base_off;
    }

    return tbl ? tbl->tbl.elem_offset : 0;
}

static void vla_write_count(void *st, struct KmppEntryVLAInfo *vla, rk_s32 val)
{
    if (vla->flex_count && vla->count_off) {
        if (vla->count_off & 0x3) {
            mpp_loge_f("vla count_off %d not aligned\n", vla->count_off);
            return;
        }
        *(rk_s32 *)((rk_u8 *)st + vla->count_off) = val;
    }
}

/* check if base + idx * elem_size overflows rk_u32 */
static rk_s32 vla_elem_off_overflow(rk_u32 base, rk_s32 idx, rk_u16 elem_size)
{
    return base + (rk_u32)idx * elem_size < base;
}

static void write_struct(MppCfgIoImpl *obj, MppTrie trie, MppCfgStrBuf *str,
                         void *st, MppCfgIoImpl *type)
{
    KmppEntry *tbl = &obj->entry;

    /* prefer type's entry for offset/type info (template from detail) */
    if (type && type->entry.tbl.elem_type < ELEM_TYPE_BUTT)
        tbl = &type->entry;

    cfg_io_dbg_show("depth %2d obj type %s name %s -> info %s offset %d size %d\n",
                    obj->depth, strof_type(obj->type), obj->name ? str->buf : "null",
                    strof_elem_type(tbl->tbl.elem_type), tbl->tbl.elem_offset, tbl->tbl.elem_size);

    if (tbl->tbl.elem_type < ELEM_TYPE_BUTT) {
        switch (tbl->tbl.elem_type) {
        case ELEM_TYPE_s8 : {
            kmpp_obj_impl_set_s8(tbl, st, obj->val.s8);
        } break;
        case ELEM_TYPE_u8 : {
            kmpp_obj_impl_set_u8(tbl, st, obj->val.u8);
        } break;
        case ELEM_TYPE_s16 : {
            kmpp_obj_impl_set_s16(tbl, st, obj->val.s16);
        } break;
        case ELEM_TYPE_u16 : {
            kmpp_obj_impl_set_u16(tbl, st, obj->val.u16);
        } break;
        case ELEM_TYPE_s32 : {
            kmpp_obj_impl_set_s32(tbl, st, obj->val.s32);
        } break;
        case ELEM_TYPE_u32 : {
            kmpp_obj_impl_set_u32(tbl, st, obj->val.u32);
        } break;
        case ELEM_TYPE_s64 : {
            kmpp_obj_impl_set_s64(tbl, st, obj->val.s64);
        } break;
        case ELEM_TYPE_u64 : {
            kmpp_obj_impl_set_u64(tbl, st, obj->val.u64);
        } break;
        default : {
        } break;
        }
    }

    /* VLA array: copy raw data from VLA buffer back to struct */
    if (obj->type == MPP_CFG_TYPE_ARRAY &&
        (IS_VLA_SIMPLE_TYPE(obj->array_type) || tbl->tbl.elem_type == ELEM_TYPE_arr)) {
        rk_u32 base = vla_read_base(st, type, tbl);
        rk_s32 src_esz = sizeof_type(obj->array_type);
        rk_s32 dst_esz = (type && type->vla.vla.type == ENTRY_TYPE_VLA_INFO) ?
                         (rk_s32)type->vla.vla.elem_size : src_esz;
        rk_s32 is_vla = (type && type->vla.vla.type == ENTRY_TYPE_VLA_INFO);

        if (src_esz == dst_esz) {
            rk_s32 dst_total = is_vla ?
                               (rk_s32)type->vla.vla.elem_count * type->vla.vla.elem_size :
                               (rk_s32)tbl->tbl.elem_size;
            rk_s32 cpy_size = MPP_MIN(dst_total, (rk_s32)obj->raw_size);

            memcpy((rk_u8 *)st + base, obj->raw, cpy_size);
            if (!is_vla && tbl->tbl.flag_offset && cpy_size > 0)
                mpp_cfg_set_flag(st, tbl->tbl.flag_offset);
        } else {
            rk_s32 src_cnt = obj->raw_count;
            rk_s32 dst_cnt = (rk_s32)type->vla.vla.elem_count;
            rk_s32 cnt = MPP_MIN(src_cnt, dst_cnt);
            rk_s32 i;

            cfg_io_dbg_show("VLA elem size mismatch: src cnt %d size %d, dst cnt %d size %d.\n",
                            src_cnt, src_esz, dst_cnt, dst_esz);

            for (i = 0; i < cnt; i++) {
                void *src = (rk_u8 *)obj->raw + i * src_esz;
                void *dst = (rk_u8 *)st + base + i * dst_esz;

                if (!src || !dst) {
                    mpp_loge_f("VLA elem index %d : src or dst is NULL\n", i);
                    break;
                }

                switch (type->array_type) {
                case MPP_CFG_TYPE_s8: {
                    rk_s8 v = (rk_s8) * (rk_s64 *)src;
                    *(rk_s8 *)dst = v;
                } break;
                case MPP_CFG_TYPE_u8: {
                    rk_u8 v = (rk_u8) * (rk_s64 *)src;
                    *(rk_u8 *)dst = v;
                } break;
                case MPP_CFG_TYPE_s16: {
                    rk_s16 v = (rk_s16) * (rk_s64 *)src;
                    *(rk_s16 *)dst = v;
                } break;
                case MPP_CFG_TYPE_u16: {
                    rk_u16 v = (rk_u16) * (rk_s64 *)src;
                    *(rk_u16 *)dst = v;
                } break;
                case MPP_CFG_TYPE_s32: {
                    rk_s32 v = (rk_s32) * (rk_s64 *)src;
                    *(rk_s32 *)dst = v;
                } break;
                case MPP_CFG_TYPE_u32: {
                    rk_u32 v = (rk_u32) * (rk_s64 *)src;
                    *(rk_u32 *)dst = v;
                } break;
                case MPP_CFG_TYPE_s64: {
                    *(rk_s64 *)dst = *(rk_s64 *)src;
                } break;
                case MPP_CFG_TYPE_u64: {
                    rk_u64 v = (rk_u64) * (rk_s64 *)src;
                    *(rk_u64 *)dst = v;
                } break;
                case MPP_CFG_TYPE_f32: {
                    float v = (float) * (double *)src;
                    *(float *)dst = v;
                } break;
                case MPP_CFG_TYPE_f64: {
                    *(double *)dst = *(double *)src;
                } break;
                default: {
                    mpp_loge_f("VLA unsupported target type %s for src %s\n",
                               strof_type(type->array_type),
                               strof_type(obj->array_type));
                    return;
                } break;
                }
            }
            /* Set update flag for VLA array field */
            if (tbl->tbl.flag_offset && cnt > 0)
                mpp_cfg_set_flag(st, tbl->tbl.flag_offset);
        }
    }

    /* Non-VLA array from parsed string: write child elements to struct */
    if (obj->type == MPP_CFG_TYPE_ARRAY && !IS_VLA_SIMPLE_TYPE(obj->array_type) &&
        tbl->tbl.elem_type == ELEM_TYPE_arr && !list_empty(&obj->child)) {
        MppCfgIoImpl *first = list_entry(obj->child.next, MppCfgIoImpl, list);
        rk_s32 elem_size = sizeof_type(first->type);
        rk_s32 max_count = (elem_size > 0) ? (rk_s32)tbl->tbl.elem_size / elem_size : 0;
        rk_s32 idx = 0;
        MppCfgIoImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &obj->child, MppCfgIoImpl, list) {
            rk_u8 *dst;
            if (idx >= max_count)
                break;

            dst = (rk_u8 *)st + tbl->tbl.elem_offset + idx * elem_size;
            switch (pos->type) {
            case MPP_CFG_TYPE_BOOL : *(rk_bool  *)dst = pos->val.b1;  break;
            case MPP_CFG_TYPE_s8 :  *(rk_s8    *)dst = pos->val.s8;  break;
            case MPP_CFG_TYPE_u8 :  *(rk_u8    *)dst = pos->val.u8;  break;
            case MPP_CFG_TYPE_s16 : *(rk_s16   *)dst = pos->val.s16; break;
            case MPP_CFG_TYPE_u16 : *(rk_u16   *)dst = pos->val.u16; break;
            case MPP_CFG_TYPE_s32 : *(rk_s32   *)dst = pos->val.s32; break;
            case MPP_CFG_TYPE_u32 : *(rk_u32   *)dst = pos->val.u32; break;
            case MPP_CFG_TYPE_s64 : *(rk_s64   *)dst = pos->val.s64; break;
            case MPP_CFG_TYPE_u64 : *(rk_u64   *)dst = pos->val.u64; break;
            case MPP_CFG_TYPE_f32 : *(rk_float *)dst = pos->val.f32; break;
            case MPP_CFG_TYPE_f64 : *(rk_double*)dst = pos->val.f64; break;
            default : break;
            }
            idx++;
        }
        /* Set update flag for non-VLA array field */
        if (tbl->tbl.flag_offset && idx > 0)
            mpp_cfg_set_flag(st, tbl->tbl.flag_offset);
    }

    /* Complex VLA array: write elements back to struct.
     * Use type's vla info (cfg_root definition) — obj->vla.vla is only set by
     * read_struct (from_struct path); nodes built by from_json leave it zero,
     * causing vla_read_base to read the wrong field as base. */
    if (obj->type == MPP_CFG_TYPE_ARRAY && IS_VLA_COMPLEX_TYPE(obj->array_type) &&
        type && type->vla.vla.type == ENTRY_TYPE_VLA_INFO && obj->elems) {
        struct KmppEntryVLAInfo *vla = &type->vla.vla;
        rk_s32 cnt;
        rk_u32 base;
        rk_s32 idx;

        cnt = obj->array_size;

        for (idx = 0; idx < cnt && idx < obj->array_size; idx++) {
            MppCfgIoImpl *elem = (MppCfgIoImpl *)obj->elems[idx];
            MppCfgIoImpl *pos, *n;

            if (!elem)
                continue;

            base = vla_read_base(st, type, tbl);

            if (vla_elem_off_overflow(base, idx, vla->elem_size)) {
                mpp_loge_f("vla elem offset overflow base %u idx %d esz %d\n",
                           base, idx, vla->elem_size);
                continue;
            }

            {
                void *elem_st = (rk_u8 *)st + base + idx * vla->elem_size;

                list_for_each_entry_safe(pos, n, &elem->child, MppCfgIoImpl, list) {
                    MppCfgIoImpl *fld_type = type ? type_find_child(type, pos->name) : NULL;

                    write_struct(pos, trie, str, elem_st, fld_type);
                }
            }
        }

        /* write count back */
        vla_write_count(st, vla, cnt);
    } else if (obj->type == MPP_CFG_TYPE_ARRAY && obj->name && type &&
               type->vla.vla.type == ENTRY_TYPE_VLA_INFO) {
        /* Array from JSON parsing: use type's VLA info and detail entries */
        struct KmppEntryVLAInfo *vla = &type->vla.vla;
        MppCfgIoImpl *pos, *n;
        rk_u32 base;
        rk_s32 idx = 0;

        base = vla_read_base(st, type, tbl);

        list_for_each_entry_safe(pos, n, &obj->child, MppCfgIoImpl, list) {
            void *elem_st;
            MppCfgIoImpl *fld, *fn;

            if (vla_elem_off_overflow(base, idx, vla->elem_size))
                break;

            elem_st = (rk_u8 *)st + base + idx * vla->elem_size;

            list_for_each_entry_safe(fld, fn, &pos->child, MppCfgIoImpl, list) {
                MppCfgIoImpl *fld_type = type_find_child(type, fld->name);

                if (fld_type) {
                    KmppEntry *tbl = &fld_type->entry;

                    if (tbl->tbl.elem_type < ELEM_TYPE_BUTT) {
                        switch (tbl->tbl.elem_type) {
                        case ELEM_TYPE_s8 : {
                            kmpp_obj_impl_set_s8(tbl, elem_st, fld->val.s8);
                        } break;
                        case ELEM_TYPE_u8 : {
                            kmpp_obj_impl_set_u8(tbl, elem_st, fld->val.u8);
                        } break;
                        case ELEM_TYPE_s16 : {
                            kmpp_obj_impl_set_s16(tbl, elem_st, fld->val.s16);
                        } break;
                        case ELEM_TYPE_u16 : {
                            kmpp_obj_impl_set_u16(tbl, elem_st, fld->val.u16);
                        } break;
                        case ELEM_TYPE_s32 : {
                            kmpp_obj_impl_set_s32(tbl, elem_st, fld->val.s32);
                        } break;
                        case ELEM_TYPE_u32 : {
                            kmpp_obj_impl_set_u32(tbl, elem_st, fld->val.u32);
                        } break;
                        case ELEM_TYPE_s64 : {
                            kmpp_obj_impl_set_s64(tbl, elem_st, fld->val.s64);
                        } break;
                        case ELEM_TYPE_u64 : {
                            kmpp_obj_impl_set_u64(tbl, elem_st, fld->val.u64);
                        } break;
                        default : break;
                        }
                    }
                }
            }
            idx++;
        }

        vla_write_count(st, vla, idx);
    } else {
        MppCfgIoImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &obj->child, MppCfgIoImpl, list) {
            MppCfgIoImpl *pos_type = type ? type_find_child(type, pos->name) : NULL;

            write_struct(pos, trie, str, st, pos_type);
        }
    }
}

rk_s32 mpp_cfg_get_val(MppCfgObj obj, MppCfgType type, MppCfgVal *val)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;

    if (!obj || !val || type >= MPP_CFG_TYPE_OBJECT) {
        mpp_loge_f("invalid param obj %p val %p\n", obj, val);
        return rk_nok;
    }

    if (impl->type != type) {
        mpp_loge_f("obj %-16s type mismatch: expected %d, got %d\n",
                   impl->name, type, impl->type);
        return rk_nok;
    }

    *val = impl->val;

    return rk_ok;
}

rk_s32 mpp_cfg_to_struct(MppCfgObj obj, MppCfgObj type, void *st)
{
    MppCfgIoImpl *orig;
    MppCfgIoImpl *impl;
    MppTrie trie;
    MppCfgStrBuf str;
    char name[256] = { 0 };

    if (!obj || !st) {
        mpp_loge_f("invalid param obj %p st %p\n", obj, st);
        return rk_nok;
    }

    impl = (MppCfgIoImpl *)obj;
    orig = (MppCfgIoImpl *)type;
    trie = mpp_cfg_to_trie(orig);

    str.buf = name;
    str.buf_size = sizeof(name) - 1;
    str.offset = 0;
    str.depth = 0;

    write_struct(impl, trie, &str, st + orig->entry.tbl.elem_offset, orig);

    return rk_ok;
}

static MppCfgObj read_struct(MppCfgIoImpl *impl, MppCfgObj parent, void *st)
{
    KmppEntry *entry = &impl->entry;
    MppCfgIoImpl *ret = NULL;

    /* dup node first */
    ret = mpp_calloc_size(MppCfgIoImpl, impl->buf_size);
    if (!ret) {
        mpp_loge_f("obj %-16s failed to alloc impl size %d\n",
                   impl->name, impl->buf_size);
        return NULL;
    }

    INIT_LIST_HEAD(&ret->list);
    INIT_LIST_HEAD(&ret->child);
    INIT_LIST_HEAD(&ret->detail);

    ret->type = impl->type;
    ret->buf_size = impl->buf_size;
    ret->entry = impl->entry;

    if (impl->name_buf_len) {
        ret->name = (char *)(ret + 1);
        memcpy(ret->name, impl->name, impl->name_buf_len);
        ret->name_len = impl->name_len;
        ret->name_buf_len = impl->name_buf_len;
    }

    /* assign value by different type */
    switch (entry->tbl.elem_type) {
    case ELEM_TYPE_s8 :
    case ELEM_TYPE_u8 :
    case ELEM_TYPE_s16 :
    case ELEM_TYPE_u16 :
    case ELEM_TYPE_s32 :
    case ELEM_TYPE_u32 :
    case ELEM_TYPE_s64 :
    case ELEM_TYPE_u64 : {
        switch (entry->tbl.elem_type) {
        case ELEM_TYPE_s8 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_s8);
            kmpp_obj_impl_get_s8(entry, st, &ret->val.s8);
        } break;
        case ELEM_TYPE_u8 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_u8);
            kmpp_obj_impl_get_u8(entry, st, &ret->val.u8);
        } break;
        case ELEM_TYPE_s16 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_s16);
            kmpp_obj_impl_get_s16(entry, st, &ret->val.s16);
        } break;
        case ELEM_TYPE_u16 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_u16);
            kmpp_obj_impl_get_u16(entry, st, &ret->val.u16);
        } break;
        case ELEM_TYPE_s32 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_s32);
            kmpp_obj_impl_get_s32(entry, st, &ret->val.s32);
        } break;
        case ELEM_TYPE_u32 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_u32);
            kmpp_obj_impl_get_u32(entry, st, &ret->val.u32);
        } break;
        case ELEM_TYPE_s64 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_s64);
            kmpp_obj_impl_get_s64(entry, st, &ret->val.s64);
        } break;
        case ELEM_TYPE_u64 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_u64);
            kmpp_obj_impl_get_u64(entry, st, &ret->val.u64);
        } break;
        default : {
        } break;
        }
    } break;
    case ELEM_TYPE_st :
    case ELEM_TYPE_arr :
    case ELEM_TYPE_ptr : {
        ret->val = impl->val;
    } break;
    default : {
    } break;
    }

    /* VLA array: allocate raw buffer and copy data from struct */
    if (ret->type == MPP_CFG_TYPE_ARRAY && IS_VLA_SIMPLE_TYPE(impl->array_type)) {
        rk_s32 cpy_size = MPP_MIN((rk_s32)entry->tbl.elem_size, (rk_s32)impl->raw_size);

        ret->array_type = impl->array_type;
        ret->raw_count  = impl->raw_count;
        ret->raw_size   = impl->raw_size;
        ret->raw = mpp_calloc_size(void, impl->raw_size);

        if (ret->raw)
            memcpy(ret->raw, (rk_u8 *)st + entry->tbl.elem_offset, cpy_size);
    }

    /* Complex VLA array: iterate elements and read fields per element */
    if (ret->type == MPP_CFG_TYPE_ARRAY && IS_VLA_COMPLEX_TYPE(impl->array_type) &&
        impl->vla.vla.type == ENTRY_TYPE_VLA_INFO) {
        struct KmppEntryVLAInfo *vla = &impl->vla.vla;
        rk_s32 cnt;
        rk_u32 base;
        rk_s32 idx;

        cnt = vla_read_count(st, vla);

        base = vla_read_base(st, impl, entry);

        if (cnt <= 0 || cnt > 16384) {
            mpp_loge_f("vla %-16s invalid count %d\n", impl->name, cnt);
            cnt = 0;
        }

        if (cnt > 0 && vla->flex_base) {
            rk_u32 total = base + (rk_u32)cnt * vla->elem_size;

            if (total < base) {
                mpp_loge_f("vla %-16s overflow base %u cnt %d esz %d\n",
                           impl->name, base, cnt, vla->elem_size);
                cnt = 0;
            }
        }

        /* initialize VLA storage for elements */
        ret->vla = impl->vla;
        ret->array_type = MPP_CFG_TYPE_BUTT;
        ret->vla.vla.elem_count = (rk_u16)cnt;
        ret->elems = mpp_calloc_size(MppCfgIoImpl *, sizeof(MppCfgIoImpl *) * cnt);
        ret->array_size = cnt;

        for (idx = 0; idx < cnt; idx++) {
            MppCfgObj elem = NULL;
            MppCfgIoImpl *pos, *n;
            void *elem_st;

            if (vla_elem_off_overflow(base, idx, vla->elem_size)) {
                mpp_loge_f("vla elem %d offset overflow\n", idx);
                break;
            }

            elem_st = (rk_u8 *)st + base + idx * vla->elem_size;

            mpp_cfg_get_object(&elem, NULL, MPP_CFG_TYPE_OBJECT, NULL);

            list_for_each_entry_safe(pos, n, &impl->detail, MppCfgIoImpl, list) {
                read_struct(pos, elem, elem_st);
            }

            mpp_cfg_vla_add_elem(ret, idx, elem);
        }
    }

    cfg_io_dbg_show("depth %2d obj type %s name %s\n", ret->depth,
                    strof_type(ret->type), ret->name);

    if (parent)
        mpp_cfg_add(parent, ret);

    /* skip general child processing for complex VLA (already handled above) */
    if (!(ret->type == MPP_CFG_TYPE_ARRAY && IS_VLA_COMPLEX_TYPE(impl->array_type) &&
          impl->vla.vla.type == ENTRY_TYPE_VLA_INFO)) {
        MppCfgIoImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &impl->detail, MppCfgIoImpl, list) {
            read_struct(pos, ret, st);
        }
    }

    return ret;
}

rk_s32 mpp_cfg_from_struct(MppCfgObj *obj, MppCfgObj type, void *st)
{
    MppCfgIoImpl *orig = (MppCfgIoImpl *)type;

    if (!obj || !type || !st) {
        mpp_loge_f("invalid param obj %p type %p st %p\n", obj, type, st);
        return rk_nok;
    }

    /* NOTE: update structure pointer by data_offset */
    *obj = read_struct(orig, NULL, st + orig->entry.tbl.elem_offset);

    return *obj ? rk_ok : rk_nok;
}

rk_s32 mpp_cfg_print_string(char *buf)
{
    rk_s32 start = 0;
    rk_s32 pos = 0;
    rk_s32 len = strlen(buf);

    /* it may be a very long string, split by \n to different line and print */
    for (pos = 0; pos < len; pos++) {
        if (buf[pos] == '\n') {
            buf[pos] = '\0';
            mpp_logi("%s\n", &buf[start]);
            buf[pos] = '\n';
            start = pos + 1;
        }
    }

    return rk_ok;
}
