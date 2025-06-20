/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_cfg_io"

#include <errno.h>
#include <float.h>
#include <string.h>
#include <limits.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_trie.h"
#include "mpp_cfg.h"
#include "mpp_cfg_io.h"
#include "rk_venc_cfg.h"

#define MAX_CFG_DEPTH                   (64)

#define CFG_IO_DBG_FLOW                 (0x00000001)
#define CFG_IO_DBG_BYTE                 (0x00000002)
#define CFG_IO_DBG_TO                   (0x00000004)
#define CFG_IO_DBG_FROM                 (0x00000008)
#define CFG_IO_DBG_FREE                 (0x00000010)
#define CFG_IO_DBG_NAME                 (0x00000020)
#define CFG_IO_DBG_SHOW                 (0x00000040)
#define CFG_IO_DBG_INFO                 (0x00000080)

#define cfg_io_dbg(flag, fmt, ...)      _mpp_dbg(mpp_cfg_io_debug, flag, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_f(flag, fmt, ...)    _mpp_dbg_f(mpp_cfg_io_debug, flag, fmt, ## __VA_ARGS__)

#define cfg_io_dbg_flow(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_FLOW, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_byte(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_BYTE, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_to(fmt, ...)         cfg_io_dbg(CFG_IO_DBG_TO, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_from(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_FROM, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_free(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_FREE, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_name(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_NAME, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_show(fmt, ...)       cfg_io_dbg_f(CFG_IO_DBG_SHOW, fmt, ## __VA_ARGS__)
#define cfg_io_dbg_info(fmt, ...)       cfg_io_dbg(CFG_IO_DBG_INFO, fmt, ## __VA_ARGS__)

typedef enum MppCfgParserType_e {
    MPP_CFG_PARSER_TYPE_KEY = 0,
    MPP_CFG_PARSER_TYPE_VALUE,
    MPP_CFG_PARSER_TYPE_TABLE,
    MPP_CFG_PARSER_TYPE_ARRAY,
    MPP_CFG_PARSER_TYPE_BUTT,
} MppCfgParserType;

typedef struct MppCfgIoImpl_t MppCfgIoImpl;
typedef void (*MppCfgIoFunc)(MppCfgIoImpl *obj, void *data);

struct MppCfgIoImpl_t {
    /* list for bothers */
    struct list_head        list;
    /* list for children */
    struct list_head        child;
    /* parent of current object */
    MppCfgIoImpl            *parent;
    /* valid condition callback for the current object */
    MppCfgObjCond           cond;

    MppCfgType              type;
    MppCfgVal               val;

    rk_s32                  buf_size;
    /* depth in tree */
    rk_s32                  depth;

    /* internal name storage */
    char                    *name;
    rk_s32                  name_len;
    rk_s32                  name_buf_len;

    /* location info for structure access */
    MppTrie                 trie;
    MppCfgInfo              info;

    union {
        /* MPP_CFG_TYPE_STRING */
        struct {
            char            *string;
            rk_s32          str_len;
        };
        /* MPP_CFG_TYPE_ARRAY */
        struct {
            MppCfgIoImpl    **elems;
            rk_s32          array_size;
        };
    };
};

typedef struct MppCfgStrBuf_t {
    char *buf;
    rk_s32 buf_size;
    rk_s32 offset;
    rk_s32 depth;
} MppCfgStrBuf;

static rk_u32 mpp_cfg_io_debug = 0;

static const char *strof_type(MppCfgType type)
{
    static const char *str[MPP_CFG_TYPE_BUTT + 1] = {
        [MPP_CFG_TYPE_INVALID] = "invalid",
        [MPP_CFG_TYPE_NULL] = "null",
        [MPP_CFG_TYPE_BOOL] = "bool",
        [MPP_CFG_TYPE_S32] = "s32",
        [MPP_CFG_TYPE_U32] = "u32",
        [MPP_CFG_TYPE_S64] = "s64",
        [MPP_CFG_TYPE_U64] = "u64",
        [MPP_CFG_TYPE_F32] = "f32",
        [MPP_CFG_TYPE_F64] = "f64",
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
            /* Add delimiter on object */
            if (curr->type >= MPP_CFG_TYPE_OBJECT)
                name[i++] = delmiter;

            name[i++] = curr->name;
        }

        curr = curr->parent;
        depth++;

        if (i >= MAX_CFG_DEPTH) {
            mpp_loge_f("too deep depth %d\n", depth);
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

    cfg_io_dbg_name("depth %d obj %-16s -> %s\n", obj->depth, obj->name, buf);

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

    mpp_env_get_u32("mpp_cfg_io_debug", &mpp_cfg_io_debug, mpp_cfg_io_debug);

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
    impl->info.data_type = CFG_FUNC_TYPE_BUTT;

    if (type == MPP_CFG_TYPE_STRING)
        impl->val.str = impl->string;

    *obj = impl;

    return rk_ok;
}

rk_s32 mpp_cfg_get_array(MppCfgObj *obj, const char *name, rk_s32 count)
{
    MppCfgIoImpl *impl = NULL;
    rk_s32 name_buf_len = 0;
    rk_s32 name_len = 0;
    rk_s32 buf_size = 0;

    if (!obj) {
        mpp_loge_f("invalid param obj %p name %s count %d\n", obj, name, count);
        return rk_nok;
    }

    if (*obj)
        mpp_logw_f("obj %p overwrite\n", *obj);

    *obj = NULL;

    if (name) {
        name_len = strlen(name);
        name_buf_len = MPP_ALIGN(name_len + 1, 4);
    }

    buf_size = sizeof(MppCfgIoImpl) + name_buf_len + count * sizeof(MppCfgObj);
    impl = mpp_calloc_size(MppCfgIoImpl, buf_size);

    if (!impl) {
        mpp_loge_f("failed to alloc impl size %d\n", buf_size);
        return rk_nok;
    }

    INIT_LIST_HEAD(&impl->list);
    INIT_LIST_HEAD(&impl->child);

    if (name_len) {
        impl->name = (char *)(impl + 1);
        memcpy(impl->name, name, name_len);
        impl->name[name_len] = '\0';
        impl->name_len = name_len;
        impl->name_buf_len = name_buf_len;
    }

    impl->type = MPP_CFG_TYPE_ARRAY;
    impl->buf_size = buf_size;
    /* set invalid data type by default */
    impl->info.data_type = CFG_FUNC_TYPE_BUTT;

    if (count) {
        impl->elems = (MppCfgIoImpl **)((char *)(impl + 1) + name_buf_len);
        impl->array_size = count;
    }

    *obj = impl;

    return rk_ok;
}

rk_s32 mpp_cfg_put(MppCfgObj obj)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;

    if (!obj) {
        mpp_loge_f("invalid param obj %p\n", obj);
        return rk_nok;
    }

    list_del_init(&impl->list);

    {
        MppCfgIoImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
            list_del_init(&pos->list);
        }
    }

    impl->parent = NULL;

    mpp_free(impl);

    return rk_ok;
}

static void mpp_cfg_put_all_child(MppCfgIoImpl *impl)
{
    MppCfgIoImpl *pos, *n;

    cfg_io_dbg_free("depth %d - %p free start type %d name %s\n",
                    impl->depth, impl, impl->type, impl->name);

    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        list_del_init(&pos->list);

        cfg_io_dbg_free("depth %d - %p free child %p type %d name %s\n",
                        impl->depth, impl, pos, pos->type, pos->name);

        mpp_cfg_put_all_child(pos);
    }

    cfg_io_dbg_free("depth %d - %p free done type %d name %s\n",
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
        mpp_loge_f("invalid root type %d\n", root_impl->type);
        return rk_nok;
    }

    list_add_tail(&leaf_impl->list, &root_impl->child);
    leaf_impl->parent = root_impl;

    loop_all_children(root, update_depth, NULL);

    if (root_impl->type == MPP_CFG_TYPE_ARRAY && root_impl->elems) {
        rk_s32 i;

        for (i = 0; i < root_impl->array_size; i++) {
            if (!root_impl->elems[i]) {
                root_impl->elems[i] = leaf_impl;
                break;
            }
        }
    }

    return rk_ok;
}

rk_s32 mpp_cfg_del(MppCfgObj obj)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;
    MppCfgIoImpl *parent;

    if (!obj) {
        mpp_loge_f("invalid param obj %p\n", obj);
        return rk_nok;
    }

    parent = impl->parent;
    if (parent) {
        list_del_init(&impl->list);

        if (parent->type == MPP_CFG_TYPE_ARRAY && parent->elems) {
            rk_s32 i;

            for (i = 0; i < parent->array_size; i++) {
                if (parent->elems[i] == impl) {
                    parent->elems[i] = NULL;
                    break;
                }
            }
        }

        impl->parent = NULL;
        impl->depth = 0;
        loop_all_children(impl, update_depth, NULL);
    }

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

rk_s32 mpp_cfg_set_info(MppCfgObj obj, MppCfgInfo *info)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;

    if (impl && info) {
        cfg_io_dbg_info("obj %-16s set info type %s offset %d size %d\n",
                        impl->name, strof_cfg_type(info->data_type),
                        info->data_offset, info->data_size);

        if (info->data_type < CFG_FUNC_TYPE_BUTT) {
            memcpy(&impl->info, info, sizeof(impl->info));

            switch (info->data_type) {
            case CFG_FUNC_TYPE_S32 : {
                impl->type = MPP_CFG_TYPE_S32;
            } break;
            case CFG_FUNC_TYPE_U32 : {
                impl->type = MPP_CFG_TYPE_U32;
            } break;
            case CFG_FUNC_TYPE_S64 : {
                impl->type = MPP_CFG_TYPE_S64;
            } break;
            case CFG_FUNC_TYPE_U64 : {
                impl->type = MPP_CFG_TYPE_U64;
            } break;
            default : {
            } break;
            }
        } else {
            impl->info.data_type = CFG_FUNC_TYPE_BUTT;
        }

        return rk_ok;
    }

    return rk_nok;
}

rk_s32 mpp_cfg_set_cond(MppCfgObj obj, MppCfgObjCond cond)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;

    if (impl)
        impl->cond = cond;

    return rk_ok;
}
typedef struct MppCfgFullNameCtx_t {
    MppTrie trie;
    char *buf;
    rk_s32 buf_size;
} MppCfgFullNameCtx;

static void add_obj_info(MppCfgIoImpl *impl, void *data)
{
    /* NOTE: skip the root object and the invalid object */
    if (impl->info.data_type < CFG_FUNC_TYPE_BUTT && impl->parent) {
        MppCfgFullNameCtx *ctx = (MppCfgFullNameCtx *)data;

        get_full_name(impl, ctx->buf, ctx->buf_size);
        mpp_trie_add_info(ctx->trie, ctx->buf, &impl->info, sizeof(impl->info));
    }
}

MppTrie mpp_cfg_to_trie(MppCfgObj obj)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;
    MppTrie p = NULL;

    do {
        MppCfgFullNameCtx ctx;
        rk_s32 ret = rk_nok;
        char name[256];

        if (!impl) {
            mpp_loge_f("invalid param obj\n", impl);
            break;
        }

        if (impl->parent) {
            mpp_loge_f("invalid param obj %p not root\n", impl);
            break;
        }

        if (impl->trie) {
            p = impl->trie;
            break;
        }

        ret = mpp_trie_init(&p, impl->name);
        if (ret || !p) {
            mpp_loge_f("failed to init obj %s trie\n", impl->name);
            break;
        }

        ctx.trie = p;
        ctx.buf = name;
        ctx.buf_size = sizeof(name) - 1;

        loop_all_children(impl, add_obj_info, &ctx);
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

static rk_s32 mpp_cfg_to_log(MppCfgIoImpl *impl, MppCfgStrBuf *str)
{
    MppCfgIoImpl *pos, *n;
    char buf[256];
    rk_s32 len = 0;
    rk_s32 total = sizeof(buf) - 1;
    rk_s32 ret = rk_ok;

    write_indent_f(str);

    /* leaf node write once and finish */
    if (impl->type < MPP_CFG_TYPE_OBJECT) {
        cfg_io_dbg_to("depth %d leaf write name %s type %d\n", str->depth, impl->name, impl->type);

        if (impl->name)
            len += snprintf(buf + len, total - len, "%s : ", impl->name);

        switch (impl->type) {
        case MPP_CFG_TYPE_NULL : {
            len += snprintf(buf + len, total - len, "null\n");
        } break;
        case MPP_CFG_TYPE_BOOL : {
            len += snprintf(buf + len, total - len, "%s\n", impl->val.b1 ? "true" : "false");
        } break;
        case MPP_CFG_TYPE_S32 : {
            len += snprintf(buf + len, total - len, "%d\n", impl->val.s32);
        } break;
        case MPP_CFG_TYPE_U32 : {
            len += snprintf(buf + len, total - len, "%u\n", impl->val.u32);
        } break;
        case MPP_CFG_TYPE_S64 : {
            len += snprintf(buf + len, total - len, "%lld\n", impl->val.s64);
        } break;
        case MPP_CFG_TYPE_U64 : {
            len += snprintf(buf + len, total - len, "%llu\n", impl->val.u64);
        } break;
        case MPP_CFG_TYPE_F32 : {
            len += snprintf(buf + len, total - len, "%f\n", impl->val.f32);
        } break;
        case MPP_CFG_TYPE_F64 : {
            len += snprintf(buf + len, total - len, "%lf\n", impl->val.f64);
        } break;
        case MPP_CFG_TYPE_STRING :
        case MPP_CFG_TYPE_RAW : {
            len += snprintf(buf + len, total - len, "\"%s\"\n", (char *)impl->val.str);
        } break;
        default : {
            mpp_loge("invalid type %d\n", impl->type);
        } break;
        }

        return write_byte_f(str, buf, &len);
    }

    cfg_io_dbg_to("depth %d branch write name %s type %d\n", str->depth, impl->name, impl->type);

    if (impl->name)
        len += snprintf(buf + len, total - len, "%s : ", impl->name);

    if (list_empty(&impl->child)) {
        len += snprintf(buf + len, total - len, "%s\n",
                        impl->type == MPP_CFG_TYPE_OBJECT ? "{}" : "[]");
        return write_byte_f(str, buf, &len);
    }

    len += snprintf(buf + len, total - len, "%c\n",
                    impl->type == MPP_CFG_TYPE_OBJECT ? '{' : '[');

    ret = write_byte_f(str, buf, &len);
    if (ret)
        return ret;

    str->depth++;

    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        cfg_io_dbg_to("depth %d child write name %s type %d\n", str->depth, pos->name, pos->type);
        ret = mpp_cfg_to_log(pos, str);
        if (ret)
            break;
    }

    str->depth--;

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

    write_indent_f(str);

    /* leaf node write once and finish */
    if (impl->type < MPP_CFG_TYPE_OBJECT) {
        cfg_io_dbg_to("depth %d leaf write name %s type %d\n", str->depth, impl->name, impl->type);

        if (impl->name)
            len += snprintf(buf + len, total - len, "\"%s\" : ", impl->name);

        switch (impl->type) {
        case MPP_CFG_TYPE_NULL : {
            len += snprintf(buf + len, total - len, "null,\n");
        } break;
        case MPP_CFG_TYPE_BOOL : {
            len += snprintf(buf + len, total - len, "%s,\n", impl->val.b1 ? "true" : "false");
        } break;
        case MPP_CFG_TYPE_S32 : {
            len += snprintf(buf + len, total - len, "%d,\n", impl->val.s32);
        } break;
        case MPP_CFG_TYPE_U32 : {
            len += snprintf(buf + len, total - len, "%u,\n", impl->val.u32);
        } break;
        case MPP_CFG_TYPE_S64 : {
            len += snprintf(buf + len, total - len, "%lld,\n", impl->val.s64);
        } break;
        case MPP_CFG_TYPE_U64 : {
            len += snprintf(buf + len, total - len, "%llu,\n", impl->val.u64);
        } break;
        case MPP_CFG_TYPE_F32 : {
            len += snprintf(buf + len, total - len, "%f,\n", impl->val.f32);
        } break;
        case MPP_CFG_TYPE_F64 : {
            len += snprintf(buf + len, total - len, "%lf,\n", impl->val.f64);
        } break;
        case MPP_CFG_TYPE_STRING :
        case MPP_CFG_TYPE_RAW : {
            len += snprintf(buf + len, total - len, "\"%s\",\n", (char *)impl->val.str);
        } break;
        default : {
            mpp_loge("invalid type %d\n", impl->type);
        } break;
        }

        return write_byte_f(str, buf, &len);
    }

    cfg_io_dbg_to("depth %d branch write name %s type %d\n", str->depth, impl->name, impl->type);

    if (impl->name)
        len += snprintf(buf + len, total - len, "\"%s\" : ", impl->name);

    if (list_empty(&impl->child)) {
        len += snprintf(buf + len, total - len, "%s,\n",
                        impl->type == MPP_CFG_TYPE_OBJECT ? "{}" : "[]");
        return write_byte_f(str, buf, &len);
    }

    len += snprintf(buf + len, total - len, "%c\n",
                    impl->type == MPP_CFG_TYPE_OBJECT ? '{' : '[');

    ret = write_byte_f(str, buf, &len);
    if (ret)
        return ret;

    str->depth++;

    list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
        cfg_io_dbg_to("depth %d child write name %s type %d\n", str->depth, pos->name, pos->type);
        ret = mpp_cfg_to_json(pos, str);
        if (ret)
            break;
    }

    revert_comma_f(str);

    str->depth--;

    write_indent_f(str);

    if (str->depth)
        len += snprintf(buf + len, total - len, "%c,\n",
                        impl->type == MPP_CFG_TYPE_OBJECT ? '}' : ']');
    else
        len += snprintf(buf + len, total - len, "%c\n",
                        impl->type == MPP_CFG_TYPE_OBJECT ? '}' : ']');

    return write_byte_f(str, buf, &len);
}

static rk_s32 mpp_cfg_to_toml(MppCfgIoImpl *impl, MppCfgStrBuf *str)
{
    (void)impl;
    (void)str;
    return 0;
}

static rk_s32 parse_log_string(MppCfgStrBuf *str, char **name, rk_s32 *len, rk_u32 type)
{
    char *buf = NULL;
    char *start = NULL;
    rk_s32 name_len = 0;
    char terminator = type ? '\"' : ' ';

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

static rk_s32 parse_json_number(MppCfgStrBuf *str, MppCfgType *type, MppCfgVal *val)
{
    char *buf = NULL;
    char tmp[64];
    long double value;
    rk_u32 i;

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
            *type = MPP_CFG_TYPE_F32;
            val->f32 = (float)value;
        } else {
            *type = MPP_CFG_TYPE_F64;
            val->f64 = (double)value;
        }
    } else {
        if (value >= INT_MIN && value <= INT_MAX) {
            *type = MPP_CFG_TYPE_S32;
            val->s32 = (int)value;
        } else if (value >= 0 && value <= UINT_MAX) {
            *type = MPP_CFG_TYPE_U32;
            val->u32 = (unsigned int)value;
        } else if (value >= LLONG_MIN && value <= LLONG_MAX) {
            *type = MPP_CFG_TYPE_U64;
            val->u64 = (unsigned long long)value;
        } else if (value >= 0 && value <= ULLONG_MAX) {
            *type = MPP_CFG_TYPE_S64;
            val->s64 = (long long)value;
        } else {
            mpp_loge_f("invalid number %s\n", tmp);
            return rk_nok;
        }
    }

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
        mpp_loge_f("depth %d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    str->depth++;

    cfg_io_dbg_from("depth %d offset %d array parse start\n", str->depth, str->offset);

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
        cfg_io_dbg_from("depth %d found empty array\n", str->depth);
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

        /* parse value */
        ret = parse_log_value(parent, NULL, str);
        if (ret) {
            ret = -6;
            goto failed;
        }

        buf = skip_ws_f(str);
        if (!buf) {
            ret = -7;
            goto failed;
        }

        if (buf[0] == ']')
            break;
    } while (1);

    if (!buf || buf[0] != ']') {
        ret = -9;
        goto failed;
    }

    skip_byte_f(str, 1);

    cfg_io_dbg_from("depth %d offset %d -> %d array parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %d offset %d -> %d array parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 parse_log_object(MppCfgIoImpl *obj, MppCfgStrBuf *str);

static rk_s32 parse_log_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str)
{
    MppCfgObj obj = NULL;
    char *buf = NULL;

    cfg_io_dbg_from("depth %d offset %d: parse value\n", str->depth, str->offset);

    buf = test_byte_f(str, 4);
    if (buf && !strncmp(buf, "null", 4)) {
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_NULL, NULL);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value null\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    if (buf && !strncmp(buf, "true", 4)) {
        MppCfgVal val;

        val.b1 = 1;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value true\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    buf = test_byte_f(str, 5);
    if (buf && !strncmp(buf, "false", 5)) {
        MppCfgVal val;

        val.b1 = 0;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value false\n", str->depth, str->offset);
        skip_byte_f(str, 5);
        return rk_ok;
    }

    buf = test_byte_f(str, 0);
    if (buf && buf[0] == '\"') {
        MppCfgVal val;
        char *string = NULL;
        rk_s32 len = 0;

        cfg_io_dbg_from("depth %d offset %d: get value string start\n", str->depth, str->offset);

        parse_log_string(str, &string, &len, MPP_CFG_PARSER_TYPE_VALUE);
        if (!string)
            return rk_nok;

        val.str = dup_str(string, len);
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_STRING, &val);
        mpp_cfg_add(parent, obj);
        MPP_FREE(val.str);

        cfg_io_dbg_from("depth %d offset %d: get value string success\n", str->depth, str->offset);
        return rk_ok;
    }

    if (buf && (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9'))) {
        MppCfgType type;
        MppCfgVal val;
        rk_s32 ret;

        cfg_io_dbg_from("depth %d offset %d: get value number start\n",
                        str->depth, str->offset);

        ret = parse_json_number(str, &type, &val);
        if (ret)
            return ret;

        mpp_cfg_get_object(&obj, name, type, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value number success\n",
                        str->depth, str->offset);
        return ret;
    }

    if (buf && buf[0] == '{') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %d offset %d: get value object start\n",
                        str->depth, str->offset);

        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_OBJECT, NULL);
        mpp_cfg_add(parent, obj);

        ret = parse_log_object(obj, str);

        cfg_io_dbg_from("depth %d offset %d: get value object ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    if (buf && buf[0] == '[') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %d offset %d: get value array start\n",
                        str->depth, str->offset);

        mpp_cfg_get_array(&obj, name, 0);
        mpp_cfg_add(parent, obj);

        ret = parse_log_array(obj, str);

        cfg_io_dbg_from("depth %d offset %d: get value array ret %d\n",
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
        mpp_loge_f("depth %d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    str->depth++;

    cfg_io_dbg_from("depth %d offset %d object parse start\n", str->depth, str->offset);

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
        cfg_io_dbg_from("depth %d found empty object\n", str->depth);
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

            cfg_io_dbg_from("depth %d offset %d: get value array start\n",
                            str->depth, str->offset);

            mpp_cfg_get_array(&object, NULL, 0);
            mpp_cfg_add(parent, object);

            ret = parse_log_array(object, str);

            cfg_io_dbg_from("depth %d offset %d: get value array ret %d\n",
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
        cfg_io_dbg_from("depth %d offset %d found object key %s len %d\n",
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

        cfg_io_dbg_from("depth %d offset %d: get next object\n", str->depth, str->offset);
    } while (1);

    skip_byte_f(str, 1);

    cfg_io_dbg_from("depth %d offset %d -> %d object parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %d offset %d -> %d object parse failed ret %d\n",
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
        ret = mpp_cfg_get_array(&object, NULL, 0);
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
        mpp_loge_f("depth %d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    str->depth++;

    cfg_io_dbg_from("depth %d offset %d object parse start\n", str->depth, str->offset);

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
        cfg_io_dbg_from("depth %d found empty object\n", str->depth);
        str->depth--;
        return rk_ok;
    }

    do {
        rk_s32 name_len = 0;
        char *name = NULL;
        char *tmp = NULL;

        if (buf[0] == '[') {
            MppCfgObj object = NULL;

            cfg_io_dbg_from("depth %d offset %d: get value array start\n",
                            str->depth, str->offset);

            mpp_cfg_get_array(&object, NULL, 0);
            mpp_cfg_add(parent, object);

            ret = parse_json_array(object, str);

            cfg_io_dbg_from("depth %d offset %d: get value array ret %d\n",
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

            cfg_io_dbg_from("depth %d offset %d: get next object\n", str->depth, str->offset);
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

    cfg_io_dbg_from("depth %d offset %d -> %d object parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %d offset %d -> %d object parse failed ret %d\n",
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
        mpp_loge_f("depth %d reached max\n", MAX_CFG_DEPTH);
        return rk_nok;
    }

    str->depth++;

    cfg_io_dbg_from("depth %d offset %d array parse start\n", str->depth, str->offset);

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
        cfg_io_dbg_from("depth %d found empty array\n", str->depth);
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

        /* parse value */
        ret = parse_json_value(parent, NULL, str);
        if (ret) {
            ret = -6;
            goto failed;
        }

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
            if (buf[0] == '}')
                break;

            cfg_io_dbg_from("depth %d offset %d: get next array\n", str->depth, str->offset);
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

    cfg_io_dbg_from("depth %d offset %d -> %d array parse success\n",
                    str->depth, old, str->offset);

    str->depth--;
    ret = rk_ok;

failed:
    if (ret)
        cfg_io_dbg_from("depth %d offset %d -> %d array parse failed ret %d\n",
                        str->depth, old, str->offset, ret);

    return ret;
}

static rk_s32 parse_json_value(MppCfgIoImpl *parent, const char *name, MppCfgStrBuf *str)
{
    MppCfgObj obj = NULL;
    char *buf = NULL;

    cfg_io_dbg_from("depth %d offset %d: parse value\n", str->depth, str->offset);

    buf = test_byte_f(str, 4);
    if (buf && !strncmp(buf, "null", 4)) {
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_NULL, NULL);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value null\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    if (buf && !strncmp(buf, "true", 4)) {
        MppCfgVal val;

        val.b1 = 1;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value true\n", str->depth, str->offset);
        skip_byte_f(str, 4);
        return rk_ok;
    }

    buf = test_byte_f(str, 5);
    if (buf && !strncmp(buf, "false", 5)) {
        MppCfgVal val;

        val.b1 = 0;
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_BOOL, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value false\n", str->depth, str->offset);
        skip_byte_f(str, 5);
        return rk_ok;
    }

    buf = test_byte_f(str, 0);
    if (buf && buf[0] == '\"') {
        MppCfgVal val;
        char *string = NULL;
        rk_s32 len = 0;

        cfg_io_dbg_from("depth %d offset %d: get value string start\n", str->depth, str->offset);

        parse_json_string(str, &string, &len);
        if (!string)
            return rk_nok;

        val.str = dup_str(string, len);
        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_STRING, &val);
        mpp_cfg_add(parent, obj);
        MPP_FREE(val.str);

        cfg_io_dbg_from("depth %d offset %d: get value string success\n", str->depth, str->offset);
        return rk_ok;
    }

    if (buf && (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9'))) {
        MppCfgType type;
        MppCfgVal val;
        rk_s32 ret;

        cfg_io_dbg_from("depth %d offset %d: get value number start\n",
                        str->depth, str->offset);

        ret = parse_json_number(str, &type, &val);
        if (ret)
            return ret;

        mpp_cfg_get_object(&obj, name, type, &val);
        mpp_cfg_add(parent, obj);

        cfg_io_dbg_from("depth %d offset %d: get value number success\n",
                        str->depth, str->offset);
        return ret;
    }

    if (buf && buf[0] == '{') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %d offset %d: get value object start\n",
                        str->depth, str->offset);

        mpp_cfg_get_object(&obj, name, MPP_CFG_TYPE_OBJECT, NULL);
        mpp_cfg_add(parent, obj);

        ret = parse_json_object(obj, str);

        cfg_io_dbg_from("depth %d offset %d: get value object ret %d\n",
                        str->depth, str->offset, ret);
        return ret;
    }

    if (buf && buf[0] == '[') {
        rk_s32 ret;

        cfg_io_dbg_from("depth %d offset %d: get value array start\n",
                        str->depth, str->offset);

        mpp_cfg_get_array(&obj, name, 0);
        mpp_cfg_add(parent, obj);

        ret = parse_json_array(obj, str);

        cfg_io_dbg_from("depth %d offset %d: get value array ret %d\n",
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
        ret = mpp_cfg_get_array(&object, NULL, 0);
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

static rk_s32 mpp_cfg_from_toml(MppCfgObj *obj, MppCfgStrBuf *str)
{
    (void)obj;
    (void)str;
    return 0;
}

void mpp_cfg_dump(MppCfgObj obj, const char *func)
{
    MppCfgIoImpl *impl = (MppCfgIoImpl *)obj;
    MppCfgStrBuf str;
    rk_s32 ret;

    if (!obj) {
        mpp_loge_f("invalid param obj %p at %s\n", obj, func);
        return;
    }

    mpp_logi_f("obj %s - %p at %s\n", impl->name ? impl->name : "n/a", impl, func);

    str.buf_size = 4096;
    str.buf = mpp_malloc_size(void, str.buf_size);
    str.offset = 0;
    str.depth = 0;

    ret = mpp_cfg_to_log(impl, &str);
    if (ret)
        mpp_loge_f("failed to get log buffer\n");
    else
        mpp_cfg_print_string(str.buf);

    MPP_FREE(str.buf);
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
        mpp_loge_f("invalid formoffset %d\n", fmt);
    } break;
    }

    if (ret) {
        mpp_loge_f("%p %s failed to get string buffer\n", impl, impl->name);
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

static void write_struct(MppCfgIoImpl *obj, MppTrie trie, MppCfgStrBuf *str, void *st)
{
    MppCfgInfo *tbl = NULL;

    if (obj->name) {
        MppTrieInfo *info = NULL;

        /* use string to index the location table */
        get_full_name(obj, str->buf, str->buf_size);

        info = mpp_trie_get_info(trie, str->buf);
        if (info)
            tbl = mpp_trie_info_ctx(info);
    }

    if (!tbl)
        tbl = &obj->info;

    cfg_io_dbg_show("depth %d obj type %s name %s -> info %s offset %d size %d\n",
                    obj->depth, strof_type(obj->type), obj->name ? str->buf : "null",
                    strof_cfg_type(tbl->data_type), tbl->data_offset, tbl->data_size);

    if (tbl->data_type < CFG_FUNC_TYPE_BUTT) {
        switch (tbl->data_type) {
        case CFG_FUNC_TYPE_S32 : {
            mpp_cfg_set_s32(tbl, st, obj->val.s32);
        } break;
        case CFG_FUNC_TYPE_U32 : {
            mpp_cfg_set_u32(tbl, st, obj->val.u32);
        } break;
        case CFG_FUNC_TYPE_S64 : {
            mpp_cfg_set_s64(tbl, st, obj->val.s64);
        } break;
        case CFG_FUNC_TYPE_U64 : {
            mpp_cfg_set_u64(tbl, st, obj->val.u64);
        } break;
        default : {
        } break;
        }
    }

    {
        MppCfgIoImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &obj->child, MppCfgIoImpl, list) {
            write_struct(pos, trie, str, st);
        }
    }
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

    write_struct(impl, trie, &str, st + orig->info.data_offset);

    return rk_ok;
}

static MppCfgObj read_struct(MppCfgIoImpl *impl, MppCfgObj parent, void *st)
{
    MppCfgInfo *info = &impl->info;
    MppCfgIoImpl *ret = NULL;

    /* dup node first */
    ret = mpp_calloc_size(MppCfgIoImpl, impl->buf_size);
    if (!ret) {
        mpp_loge_f("failed to alloc impl size %d\n", impl->buf_size);
        return NULL;
    }

    INIT_LIST_HEAD(&ret->list);
    INIT_LIST_HEAD(&ret->child);

    ret->type = impl->type;
    ret->buf_size = impl->buf_size;

    if (impl->name_buf_len) {
        ret->name = (char *)(ret + 1);
        memcpy(ret->name, impl->name, impl->name_buf_len);
        ret->name_len = impl->name_len;
        ret->name_buf_len = impl->name_buf_len;
    }

    /* assign value by different type */
    switch (info->data_type) {
    case CFG_FUNC_TYPE_S32 :
    case CFG_FUNC_TYPE_U32 :
    case CFG_FUNC_TYPE_S64 :
    case CFG_FUNC_TYPE_U64 : {
        switch (info->data_type) {
        case CFG_FUNC_TYPE_S32 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_S32);
            mpp_cfg_get_s32(info, st, &ret->val.s32);
        } break;
        case CFG_FUNC_TYPE_U32 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_U32);
            mpp_cfg_get_u32(info, st, &ret->val.u32);
        } break;
        case CFG_FUNC_TYPE_S64 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_S64);
            mpp_cfg_get_s64(info, st, &ret->val.s64);
        } break;
        case CFG_FUNC_TYPE_U64 : {
            mpp_assert(impl->type == MPP_CFG_TYPE_U64);
            mpp_cfg_get_u64(info, st, &ret->val.u64);
        } break;
        default : {
        } break;
        }
    } break;
    case CFG_FUNC_TYPE_St :
    case CFG_FUNC_TYPE_Ptr : {
        ret->val = impl->val;
    } break;
    default : {
    } break;
    }

    cfg_io_dbg_show("depth %d obj type %s name %s\n", ret->depth,
                    strof_type(ret->type), ret->name);

    if (parent)
        mpp_cfg_add(parent, ret);

    {
        MppCfgIoImpl *pos, *n;

        list_for_each_entry_safe(pos, n, &impl->child, MppCfgIoImpl, list) {
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
    *obj = read_struct(orig, NULL, st + orig->info.data_offset);

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
