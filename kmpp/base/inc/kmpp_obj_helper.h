/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include "kmpp_obj_macro.h"

#ifdef __OBJECT_HERLPER_H__
#error “MUST NOT include obj_helper.h within obj_helper.h“
#endif

/* define object helper for loop include detection */
#define __OBJECT_HERLPER_H__

#if !defined(KMPP_OBJ_NAME) || \
    !defined(KMPP_OBJ_INTF_TYPE) || \
    !defined(KMPP_OBJ_IMPL_TYPE)

#warning "When using kmpp_obj_helper.h The following macro must be defined:"
#warning "KMPP_OBJ_NAME                 - object name"
#warning "KMPP_OBJ_INTF_TYPE            - object interface type"
#warning "KMPP_OBJ_IMPL_TYPE            - object implement type"
#warning "option macro:"
#warning "KMPP_OBJ_EXTRA_SIZE           - object extra size in bytes"
#warning "KMPP_OBJ_ENTRY_TABLE          - object element value / pointer entry table"
#warning "KMPP_OBJ_FUNC_IOCTL           - object element ioctl cmd and function table"
#warning "KMPP_OBJ_FUNC_INIT            - add object init function"
#warning "KMPP_OBJ_FUNC_DEINIT          - add object deinit function"
#warning "KMPP_OBJ_FUNC_DUMP            - add object dump function"
#warning "KMPP_OBJ_SGLN_ID              - add object singleton id for singleton macro"
#warning "KMPP_OBJ_FUNC_EXPORT_DISABLE  - disable function exprot by EXPORT_SYMBOL"
#warning "KMPP_OBJ_ACCESS_DISABLE       - disable access function creation"
#warning "KMPP_OBJ_SHARE_DISABLE        - disable object sharing by /dev/kmpp_objs to userspace"
#warning "KMPP_OBJ_HIERARCHY_ENABLE     - enable hierarchy name creation"

#ifndef KMPP_OBJ_NAME
#error "KMPP_OBJ_NAME not defined"
#endif
#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE not defined"
#endif
#ifndef KMPP_OBJ_IMPL_TYPE
#error "KMPP_OBJ_IMPL_TYPE not defined"
#endif

#else /* all input macro defined */

#include <linux/stddef.h>

#ifndef KMPP_OBJ_PRIV_SIZE
#define KMPP_OBJ_PRIV_SIZE     0
#endif

#ifndef KMPP_OBJ_EXTRA_SIZE
#define KMPP_OBJ_EXTRA_SIZE     0
#endif

#ifndef KMPP_OBJ_ENTRY_TABLE
#define KMPP_OBJ_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS)
#endif

#define ENTRY_TO_TRIE(prefix, ftype, type, name, flag, ...) \
    do { \
        KmppEntry tbl = { \
            .tbl.elem_offset = ((size_t)&(((KMPP_OBJ_IMPL_TYPE *)0)->CONCAT_DOT(__VA_ARGS__))), \
            .tbl.elem_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->CONCAT_DOT(__VA_ARGS__)), \
            .tbl.elem_type = ELEM_TYPE_##ftype, \
            .tbl.flag_offset = FLAG_TYPE_TO_OFFSET(name, flag, #flag), \
        }; \
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), ENTRY_TO_NAME_START(name), &tbl); \
        ENTRY_TO_NAME_END(name); \
    } while (0);

#if !defined(KMPP_OBJ_ACCESS_DISABLE)
#define VAL_ENTRY_TBL(prefix, ftype, type, name, flag, ...) \
    static KmppEntry *CONCAT_US(tbl, prefix, __VA_ARGS__) = NULL;

#define VAL_HOOK_IDX(prefix, ftype, type, name, flag, ...) \
    static rk_s32 CONCAT_US(hook, prefix, get, __VA_ARGS__) = -1; \
    static rk_s32 CONCAT_US(hook, prefix, set, __VA_ARGS__) = -1;

#define ENTRY_QUERY(prefix, ftype, type, name, flag, ...) \
    do { \
        kmpp_objdef_get_entry(KMPP_OBJ_DEF(prefix), ENTRY_TO_NAME_START(name), &CONCAT_US(tbl, prefix, __VA_ARGS__)); \
    } while (0);

#define HOOK_QUERY(prefix, ftype, type, name, flag, ...) \
    do { \
        CONCAT_US(hook, prefix, set, __VA_ARGS__) = \
        kmpp_objdef_get_hook(KMPP_OBJ_DEF(prefix), CONCAT_STR(set, __VA_ARGS__)); \
        CONCAT_US(hook, prefix, get, __VA_ARGS__) = \
        kmpp_objdef_get_hook(KMPP_OBJ_DEF(prefix), CONCAT_STR(get, __VA_ARGS__)); \
    } while (0);

#define ENTRY_TO_FUNC(prefix, ftype, type, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_get_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->CONCAT_DOT(__VA_ARGS__); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_set_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        else \
            ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->CONCAT_DOT(__VA_ARGS__) = v; \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, test, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, CONCAT_US(tbl, prefix, __VA_ARGS__)); \
    }

#define STRUCT_TO_FUNC(prefix, ftype, type, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_get_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        else \
            memcpy(v, &((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->CONCAT_DOT(__VA_ARGS__), \
                   sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->CONCAT_DOT(__VA_ARGS__))); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_set_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        else \
            memcpy(&((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->CONCAT_DOT(__VA_ARGS__), v, \
                   sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->CONCAT_DOT(__VA_ARGS__))); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, test, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, CONCAT_US(tbl, prefix, __VA_ARGS__)); \
    }

#define EHOOK_TO_FUNC(prefix, ftype, type, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(hook, prefix, get, __VA_ARGS__) >= 0) \
            ret = kmpp_obj_idx_run(s, CONCAT_US(hook, prefix, get, __VA_ARGS__), (void *)v, __FUNCTION__); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(hook, prefix, set, __VA_ARGS__) >= 0) \
            ret = kmpp_obj_idx_run(s, CONCAT_US(hook, prefix, set, __VA_ARGS__), (void *)&v, __FUNCTION__); \
        return ret; \
    }

#define SHOOK_TO_FUNC(prefix, ftype, type, name, flag, ...) \ \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(hook, prefix, get, __VA_ARGS__) >= 0) \
            ret = kmpp_obj_idx_run(s, CONCAT_US(hook, prefix, get, __VA_ARGS__), (void *)v, __FUNCTION__); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(hook, prefix, set, __VA_ARGS__) >= 0) \
            ret = kmpp_obj_idx_run(s, CONCAT_US(prefix, set, __VA_ARGS__), (void *)v, __FUNCTION__); \
        return ret; \
    }

#else
#define VAL_ENTRY_TBL   ENTRY_NOTHING
#define VAL_HOOK_IDX    ENTRY_NOTHING
#define ENTRY_QUERY     ENTRY_NOTHING
#define HOOK_QUERY      ENTRY_NOTHING
#define ENTRY_TO_FUNC   ENTRY_NOTHING
#define STRUCT_TO_FUNC  ENTRY_NOTHING
#define EHOOK_TO_FUNC   ENTRY_NOTHING
#define SHOOK_TO_FUNC   ENTRY_NOTHING
#endif

/* disable structure layout macro for global variable definition */
#undef CFG_DEF_START
#undef CFG_DEF_END
#undef STRUCT_START
#undef STRUCT_END
#undef ENTRY_TO_NAME_START
#undef ENTRY_TO_NAME_END

#define CFG_DEF_START(...)
#define CFG_DEF_END(...)
#define STRUCT_START(...)
#define STRUCT_END(...)
#define ENTRY_TO_NAME_START(name, ...) TO_STR(name)
#define ENTRY_TO_NAME_END(...)

/* object definition common functions */
static KmppObjDef KMPP_OBJ_DEF(KMPP_OBJ_NAME) = NULL;
static rk_u32 KMPP_OBJ_DEF_DEUBG(KMPP_OBJ_NAME) = 0;

/* globla variable definitions */
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, VAL_ENTRY_TBL, VAL_ENTRY_TBL,
                     VAL_HOOK_IDX, VAL_HOOK_IDX, ENTRY_NOTHING)

/* enable structure layout macro for objdef registration */
#ifdef KMPP_OBJ_HIERARCHY_ENABLE
#undef CFG_DEF_START
#undef CFG_DEF_END
#undef STRUCT_START
#undef STRUCT_END
#undef ENTRY_TO_NAME_START
#undef ENTRY_TO_NAME_END

#define CFG_DEF_START(...) \
    { \
        char str_buf[256] = {0}; \
        rk_s32 str_pos = 0; \
        rk_s32 str_size = sizeof(str_buf) - 1;

#define CFG_DEF_END(...) \
    }

#define STRUCT_START(...) \
    { \
        rk_s32 CONCAT_US(pos, __VA_ARGS__, root) = str_pos; \
        str_pos += snprintf(str_buf + str_pos, str_size - str_pos, str_pos ? ":%s" : "%s", CONCAT_STR(__VA_ARGS__));

#define STRUCT_END(...) \
        str_pos = CONCAT_US(pos, __VA_ARGS__, root); \
        str_buf[str_pos] = '\0'; \
    }

#define ENTRY_TO_NAME_START(name, ...) \
    ({ \
        snprintf(str_buf + str_pos, str_size - str_pos, str_pos ? ":%s" : "%s", TO_STR(name)); \
        str_buf; \
    })

#define ENTRY_TO_NAME_END(...) \
    str_buf[str_pos] = '\0';

#endif

static void CONCAT_US(KMPP_OBJ_NAME, register)(void)
{
    mpp_env_get_u32(TO_STR(CONCAT_US(KMPP_OBJ_NAME, debug)), &KMPP_OBJ_DEF_DEUBG(KMPP_OBJ_NAME), 0);

    KMPP_OBJ_DBG_LOG("register enter\n");

    kmpp_objdef_get(&KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_PRIV_SIZE, TO_STR(KMPP_OBJ_INTF_TYPE));
    if (KMPP_OBJ_DEF(KMPP_OBJ_NAME)) {
        KMPP_OBJ_DBG_LOG(TO_STR(KMPP_OBJ_NAME) " found at kernel\n");
    } else {
        rk_s32 impl_size = (sizeof(KMPP_OBJ_IMPL_TYPE) + KMPP_OBJ_EXTRA_SIZE + 3) & ~3;
        rk_s32 __flag_base = impl_size << 3;
        rk_s32 __flag_step = 0;
        rk_s32 __flag_prev = 0;
        rk_s32 __flag_record[ELEM_FLAG_RECORD_MAX];
        (void) __flag_base;
        (void) __flag_step;
        (void) __flag_prev;
        (void) __flag_record;

        kmpp_objdef_register(&KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_PRIV_SIZE,
                             impl_size, TO_STR(KMPP_OBJ_INTF_TYPE));

        if (!KMPP_OBJ_DEF(KMPP_OBJ_NAME)) {
            mpp_loge_f(TO_STR(KMPP_OBJ_NAME) " init failed\n");
            return;
        }

        KMPP_OBJ_DBG_LOG(TO_STR(KMPP_OBJ_NAME) " registered at userspace\n");

        KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, ENTRY_TO_TRIE, ENTRY_TO_TRIE,
                             ENTRY_TO_TRIE, ENTRY_TO_TRIE, ENTRY_TO_TRIE)
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(KMPP_OBJ_NAME), NULL, NULL);
    }

    KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, ENTRY_QUERY, ENTRY_QUERY,
                         HOOK_QUERY, HOOK_QUERY, ENTRY_NOTHING);

#if defined(KMPP_OBJ_FUNC_INIT)
    kmpp_objdef_add_init(KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_FUNC_INIT);
#endif
#if defined(KMPP_OBJ_FUNC_DEINIT)
    kmpp_objdef_add_deinit(KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_FUNC_DEINIT);
#endif
#if defined(KMPP_OBJ_FUNC_DUMP)
    kmpp_objdef_add_dump(KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_FUNC_DUMP);
#endif
#if !defined(KMPP_OBJ_SHARE_DISABLE) && defined(__KERNEL__)
    kmpp_objdef_share(KMPP_OBJ_DEF(KMPP_OBJ_NAME));
#endif
    KMPP_OBJ_DBG_LOG("register leave\n");
}

static void CONCAT_US(KMPP_OBJ_NAME, unregister)(void)
{
    KmppObjDef def = __sync_fetch_and_and(&KMPP_OBJ_DEF(KMPP_OBJ_NAME), NULL);

    KMPP_OBJ_DBG_LOG("unregister enter\n");
    kmpp_objdef_put(def);
    KMPP_OBJ_DBG_LOG("unregister leave\n");
}

MPP_SINGLETON(KMPP_OBJ_SGLN_ID, KMPP_OBJ_NAME, CONCAT_US(KMPP_OBJ_NAME, register), CONCAT_US(KMPP_OBJ_NAME, unregister));

rk_s32 CONCAT_US(KMPP_OBJ_NAME, size)(void)
{
    return kmpp_objdef_get_entry_size(KMPP_OBJ_DEF(KMPP_OBJ_NAME));
}

rk_s32 CONCAT_US(KMPP_OBJ_NAME, get)(KMPP_OBJ_INTF_TYPE *obj)
{
    return kmpp_obj_get_f((KmppObj *)obj, KMPP_OBJ_DEF(KMPP_OBJ_NAME));
}

rk_s32 CONCAT_US(KMPP_OBJ_NAME, put)(KMPP_OBJ_INTF_TYPE obj)
{
    return kmpp_obj_put_f(obj);
}

rk_s32 CONCAT_US(KMPP_OBJ_NAME, dump)(KMPP_OBJ_INTF_TYPE obj, const char *caller)
{
    if (!obj)
        return rk_nok;

    return kmpp_obj_is_kobj(obj) ? kmpp_obj_kdump_f(obj, caller) : kmpp_obj_udump_f(obj, caller);
}

KmppObjDef CONCAT_US(KMPP_OBJ_NAME, objdef)(void)
{
    return KMPP_OBJ_DEF(KMPP_OBJ_NAME);
}

#if !defined(KMPP_OBJ_FUNC_EXPORT_DISABLE) && defined(__KERNEL__)
#include <linux/export.h>

EXPORT_SYMBOL(CONCAT_US(KMPP_OBJ_NAME, size));
EXPORT_SYMBOL(CONCAT_US(KMPP_OBJ_NAME, get));
EXPORT_SYMBOL(CONCAT_US(KMPP_OBJ_NAME, put));
EXPORT_SYMBOL(CONCAT_US(KMPP_OBJ_NAME, dump));
#endif

/* disable structure layout macro for access function definition */
#undef CFG_DEF_START
#undef CFG_DEF_END
#undef STRUCT_START
#undef STRUCT_END
#undef ENTRY_TO_NAME_START
#undef ENTRY_TO_NAME_END

#define CFG_DEF_START(...)
#define CFG_DEF_END(...)
#define STRUCT_START(...)
#define STRUCT_END(...)

#if !defined(KMPP_OBJ_ACCESS_DISABLE)
/* object element access functions */
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, ENTRY_TO_FUNC, STRUCT_TO_FUNC,
                     EHOOK_TO_FUNC, SHOOK_TO_FUNC, ENTRY_NOTHING)

#if !defined(KMPP_OBJ_FUNC_EXPORT_DISABLE) && defined(__KERNEL__)
#define KMPP_OBJ_EXPORT(prefix, ftype, type, name, flag, ...) \
    EXPORT_SYMBOL(CONCAT_US(prefix, get, __VA_ARGS__)); \
    EXPORT_SYMBOL(CONCAT_US(prefix, set, __VA_ARGS__));

#define KMPP_OBJ_EXPORT_NONE(prefix, ftype, type, name, flag, ...)

KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, KMPP_OBJ_EXPORT, KMPP_OBJ_EXPORT,
                     KMPP_OBJ_EXPORT, KMPP_OBJ_EXPORT, KMPP_OBJ_EXPORT_NONE)
#undef KMPP_OBJ_EXPORT
#endif
#endif

/* restore layout definition */
#undef CFG_DEF_START
#undef CFG_DEF_END
#undef STRUCT_START
#undef STRUCT_END

/* kmpp_obj ioctl function */
#ifdef KMPP_OBJ_FUNC_IOCTL

#define IOCTL_CTX(prefix, func, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx) \
    { \
        static rk_s32 old_cmd = GET_ARG0(-1, __VA_ARGS__); \
        static rk_s32 cmd = -1; \
        static rk_s32 once = 1; \
        KmppObjDef def = KMPP_OBJ_DEF(KMPP_OBJ_NAME); \
        if (!def) { \
            mpp_loge_f(TO_STR(KMPP_OBJ_NAME) " can not be found for ioctl\n"); \
            return rk_nok; \
        } \
        /* legacy compatible */ \
        if (old_cmd >= 0) \
            return kmpp_obj_ioctl(ctx, old_cmd, ctx, NULL, __FUNCTION__); \
        if (cmd < 0) { \
            cmd = kmpp_objdef_get_cmd(def, TO_STR(func)); \
            if (cmd < 0) { \
                mpp_loge_cf(once, TO_STR(KMPP_OBJ_NAME) " ioctl cmd %s not supported\n", TO_STR(func)); \
                once = 0; \
                return rk_nok; \
            } \
        } \
        return kmpp_obj_ioctl(ctx, cmd, NULL, NULL, __FUNCTION__); \
    }

#define IOCTL_IN_(prefix, func, in_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, in_type in) \
    { \
        KmppObjDef def = KMPP_OBJ_DEF(KMPP_OBJ_NAME); \
        static rk_s32 cmd = GET_ARG0(-1, __VA_ARGS__); \
        static rk_s32 once = 1; \
        if (!def) { \
            mpp_loge_f(TO_STR(KMPP_OBJ_NAME) " can not be found for ioctl\n"); \
            return rk_nok; \
        } \
        if (cmd < 0) { \
            cmd = kmpp_objdef_get_cmd(def, TO_STR(func)); \
            if (cmd < 0) { \
                mpp_loge_cf(once, TO_STR(KMPP_OBJ_NAME) " ioctl cmd %s not supported\n", TO_STR(func)); \
                once = 0; \
                return rk_nok; \
            } \
        } \
        return kmpp_obj_ioctl(ctx, cmd, in, NULL, __FUNCTION__); \
    }

#define IOCTL_OUT(prefix, name, out_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, out_type out) \
    { \
        KmppObjDef def = KMPP_OBJ_DEF(KMPP_OBJ_NAME); \
        static rk_s32 cmd = GET_ARG0(-1, __VA_ARGS__); \
        static rk_s32 once = 1; \
        if (!def) { \
            mpp_loge_f(TO_STR(KMPP_OBJ_NAME) " can not be found for ioctl\n"); \
            return rk_nok; \
        } \
        if (cmd < 0) { \
            cmd = kmpp_objdef_get_cmd(def, TO_STR(func)); \
            if (cmd < 0) { \
                mpp_loge_cf(once, TO_STR(KMPP_OBJ_NAME) " ioctl cmd %s not supported\n", TO_STR(func)); \
                once = 0; \
                return rk_nok; \
            } \
        } \
        return kmpp_obj_ioctl(ctx, cmd, NULL, out, __FUNCTION__); \
    }

#define IOCTL_IO_(prefix, name, in_type, out_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, in_type in, out_type out) \
    { \
        KmppObjDef def = KMPP_OBJ_DEF(KMPP_OBJ_NAME); \
        static rk_s32 cmd = GET_ARG0(-1, __VA_ARGS__); \
        static rk_s32 once = 1; \
        if (!def) { \
            mpp_loge_f(TO_STR(KMPP_OBJ_NAME) " can not be found for ioctl\n"); \
            return rk_nok; \
        } \
        if (cmd < 0) { \
            cmd = kmpp_objdef_get_cmd(def, TO_STR(func)); \
            if (cmd < 0) { \
                mpp_loge_cf(once, TO_STR(KMPP_OBJ_NAME) " ioctl cmd %s not supported\n", TO_STR(func)); \
                once = 0; \
                return rk_nok; \
            } \
        } \
        return kmpp_obj_ioctl(ctx, cmd, in, out, __FUNCTION__); \
    }

#ifdef __cplusplus
extern "C" {
#endif

KMPP_OBJ_FUNC_IOCTL(KMPP_OBJ_NAME, IOCTL_CTX, IOCTL_IN_, IOCTL_OUT, IOCTL_IO_)

#ifdef __cplusplus
}
#endif

#undef IOCTL_CTX
#undef IOCTL_IN_
#undef IOCTL_OUT
#undef IOCTL_IO_

#endif /* KMPP_OBJ_FUNC_IOCTL */

#undef KMPP_OBJ_NAME
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_IMPL_TYPE
#undef KMPP_OBJ_EXTRA_SIZE
#undef KMPP_OBJ_ENTRY_TABLE
#undef KMPP_OBJ_PRIV_SIZE
#undef KMPP_OBJ_FUNC_INIT
#undef KMPP_OBJ_FUNC_DEINIT
#undef KMPP_OBJ_FUNC_IOCTL
#undef KMPP_OBJ_FUNC_DUMP
#undef KMPP_OBJ_SGLN_ID
#undef KMPP_OBJ_FUNC_EXPORT_DISABLE
#undef KMPP_OBJ_ACCESS_DISABLE
#undef KMPP_OBJ_SHARE_DISABLE
#undef KMPP_OBJ_HIERARCHY_ENABLE

/* undef tmp macro */
#undef ENTRY_TO_TRIE
#undef ENTRY_TO_FUNC
#undef STRUCT_TO_FUNC
#undef EHOOK_TO_FUNC
#undef SHOOK_TO_FUNC
#undef VAL_ENTRY_TBL
#undef VAL_HOOK_IDX
#undef ENTRY_QUERY
#undef HOOK_QUERY

#undef __OBJECT_HERLPER_H__

#endif
