/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include "kmpp_obj_macro.h"

#ifdef OBJECT_HERLPER_H
#error “MUST NOT include obj_helper.h within obj_helper.h“
#endif

/* define object helper for loop include detection */
#define OBJECT_HERLPER_H

#if !defined(KMPP_OBJ_NAME) || \
    !defined(KMPP_OBJ_INTF_TYPE)

#warning "When using kmpp_obj_helper.h The following macro must be defined:"
#warning "KMPP_OBJ_NAME                 - object name"
#warning "KMPP_OBJ_INTF_TYPE            - object interface type"
#warning "option macro:"
#warning "KMPP_OBJ_IMPL_TYPE            - object implement type"
#warning "KMPP_OBJ_EXTRA_SIZE           - object extra size in bytes"
#warning "KMPP_OBJ_ENTRY_TABLE          - object element value / pointer entry table"
#warning "KMPP_OBJ_FUNC_IOCTL           - object element ioctl cmd and function table"
#warning "KMPP_OBJ_FUNC_INIT            - add object init function"
#warning "KMPP_OBJ_FUNC_DEINIT          - add object deinit function"
#warning "KMPP_OBJ_FUNC_DUMP            - add object dump function"
#warning "KMPP_OBJ_FUNC_RESIZE          - add object resize callback"
#warning "KMPP_OBJ_FUNC_CACHE_INIT      - add object cache init callback (called on cache hit)"
#warning "KMPP_OBJ_FUNC_CACHE_DEINIT    - add object cache deinit callback (called before return to pool)"
#warning "KMPP_OBJ_SGLN                 - singleton macro (default MPP_SINGLETON)"
#warning "KMPP_OBJ_SGLN_ID              - add object singleton id for singleton macro"
#warning "KMPP_OBJ_FUNC_EXPORT_DISABLE  - disable function exprot by EXPORT_SYMBOL"
#warning "KMPP_OBJ_ACCESS_DISABLE       - disable access function creation"
#warning "KMPP_OBJ_SHARE_DISABLE        - disable object sharing by /dev/kmpp_objs to userspace"
#warning "KMPP_OBJ_CACHE_ENABLE         - enable object cache (kmpp_obj_put returns to pool)"
#warning "KMPP_OBJ_HIERARCHY_ENABLE     - enable hierarchy name creation"
#warning "KMPP_OBJ_FLEX_ENTRY_ENABLE    - enable flexible entry allocation (VLA support)"
#warning "KMPP_OBJ_MISMATCH_LOG_DISABLE - disable entry query mismatch log"

#ifndef KMPP_OBJ_NAME
#error "KMPP_OBJ_NAME not defined"
#endif
#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE not defined"
#endif

#else /* all input macro defined */

#include <linux/stddef.h>

#ifndef KMPP_OBJ_PRIV_SIZE
#define KMPP_OBJ_PRIV_SIZE      0
#endif

#ifndef KMPP_OBJ_EXTRA_SIZE
#define KMPP_OBJ_EXTRA_SIZE     0
#endif

#ifndef KMPP_OBJ_ENTRY_TABLE
#define KMPP_OBJ_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS)
#endif

#ifndef KMPP_OBJ_SGLN
#define KMPP_OBJ_SGLN           MPP_SINGLETON
#endif

#ifdef KMPP_OBJ_IMPL_TYPE
#ifdef KMPP_OBJ_HIERARCHY_ENABLE

#define MPP_CFG_TYPE_ptr    MPP_CFG_TYPE_OBJECT
#define MPP_CFG_TYPE_st     MPP_CFG_TYPE_OBJECT
#define MPP_CFG_TYPE_arr    MPP_CFG_TYPE_ARRAY
#define ELEM_TYPE_arr       ELEM_TYPE_arr

#define KMPP_TYPE_TO_ELEM_TYPE(type) ({ \
    ElemType _ret = ELEM_TYPE_BUTT; \
    if (__builtin_types_compatible_p(type, rk_s8)) \
        _ret = ELEM_TYPE_s8; \
    else if (__builtin_types_compatible_p(type, rk_u8)) \
        _ret = ELEM_TYPE_u8; \
    else if (__builtin_types_compatible_p(type, rk_s16)) \
        _ret = ELEM_TYPE_s16; \
    else if (__builtin_types_compatible_p(type, rk_u16)) \
        _ret = ELEM_TYPE_u16; \
    else if (__builtin_types_compatible_p(type, rk_s32)) \
        _ret = ELEM_TYPE_s32; \
    else if (__builtin_types_compatible_p(type, rk_u32)) \
        _ret = ELEM_TYPE_u32; \
    else if (__builtin_types_compatible_p(type, rk_s64)) \
        _ret = ELEM_TYPE_s64; \
    else if (__builtin_types_compatible_p(type, rk_u64)) \
        _ret = ELEM_TYPE_u64; \
    _ret; \
})

#define ENTRY_TO_TRIE(prefix, ftype, _etype, name, flag, ...) \
    do { \
            KmppEntry tbl = { .val = 0 }; \
            tbl.tbl.type = ENTRY_TYPE_LOC_TBL; \
            tbl.tbl.elem_offset = ((size_t)&(((KMPP_OBJ_IMPL_TYPE *)0)->CONCAT_DOT(__VA_ARGS__))); \
            tbl.tbl.elem_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->CONCAT_DOT(__VA_ARGS__)); \
            tbl.tbl.elem_type = ELEM_TYPE_##ftype; \
            tbl.tbl.flag_offset = FLAG_TYPE_TO_OFFSET(name, flag, #flag); \
            MppCfgObj CONCAT_US(obj, name) = NULL; \
            kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), 0, ENTRY_TO_NAME_START(name), &tbl); \
            if (tbl.tbl.elem_type == ELEM_TYPE_arr) { \
                mpp_cfg_get_array(&CONCAT_US(obj, name), TO_STR(name)); \
                KmppEntry _vla = { .val = 0 }; \
                _vla.vla.type = ENTRY_TYPE_VLA_INFO; \
                _vla.vla.elem_size = sizeof(_etype); \
                _vla.vla.elem_count = (rk_s32)(tbl.tbl.elem_size / sizeof(_etype)); \
                _vla.vla.base_off = (rk_u16)tbl.tbl.elem_offset; \
                mpp_cfg_set_vla(CONCAT_US(obj, name), &_vla, \
                    mpp_cfg_type_from_elem_type(KMPP_TYPE_TO_ELEM_TYPE(_etype))); \
                mpp_cfg_set_entry(CONCAT_US(obj, name), &tbl); \
                mpp_cfg_add_detail(__parent, CONCAT_US(obj, name)); \
            } else { \
                mpp_cfg_get_object(&CONCAT_US(obj, name), TO_STR(name), MPP_CFG_TYPE_##ftype, NULL); \
                mpp_cfg_set_entry(CONCAT_US(obj, name), &tbl); \
                mpp_cfg_add_detail(__parent, CONCAT_US(obj, name)); \
            } \
            ENTRY_TO_NAME_END(name); \
    } while (0);
#else
#define ENTRY_TO_TRIE(prefix, ftype, _type, name, flag, ...) \
    do { \
        KmppEntry tbl = { .val = 0 }; \
        tbl.tbl.type = ENTRY_TYPE_LOC_TBL; \
        tbl.tbl.elem_offset = ((size_t)&(((KMPP_OBJ_IMPL_TYPE *)0)->CONCAT_DOT(__VA_ARGS__))); \
        tbl.tbl.elem_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->CONCAT_DOT(__VA_ARGS__)); \
        tbl.tbl.elem_type = ELEM_TYPE_##ftype; \
        tbl.tbl.flag_offset = FLAG_TYPE_TO_OFFSET(name, flag, #flag); \
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), 0, ENTRY_TO_NAME_START(name), &tbl); \
        ENTRY_TO_NAME_END(name); \
    } while (0);

#endif /* KMPP_OBJ_HIERARCHY_ENABLE */

#else
#define ENTRY_TO_TRIE(prefix, ftype, _type, name, flag, ...)
#endif /* KMPP_OBJ_IMPL_TYPE */

#if !defined(KMPP_OBJ_ACCESS_DISABLE)
#define VAL_ENTRY_TBL(prefix, ftype, etype, name, flag, ...) \
    static KmppEntry *CONCAT_US(tbl, prefix, __VA_ARGS__) = NULL;

#define VAL_HOOK_IDX(prefix, ftype, etype, name, flag, ...) \
    static rk_s32 CONCAT_US(hook, prefix, get, __VA_ARGS__) = -1; \
    static rk_s32 CONCAT_US(hook, prefix, set, __VA_ARGS__) = -1;

#define ENTRY_QUERY(prefix, ftype, etype, name, flag, ...) \
    do { \
        kmpp_objdef_get_entry(KMPP_OBJ_DEF(prefix), ENTRY_TO_NAME_START(name), &CONCAT_US(tbl, prefix, __VA_ARGS__)); \
    } while (0);

#define HOOK_QUERY(prefix, ftype, etype, name, flag, ...) \
    do { \
        CONCAT_US(hook, prefix, set, __VA_ARGS__) = \
        kmpp_objdef_get_hook(KMPP_OBJ_DEF(prefix), CONCAT_STR(set, __VA_ARGS__)); \
        CONCAT_US(hook, prefix, get, __VA_ARGS__) = \
        kmpp_objdef_get_hook(KMPP_OBJ_DEF(prefix), CONCAT_STR(get, __VA_ARGS__)); \
    } while (0);

#ifdef KMPP_OBJ_IMPL_TYPE
#define ENTRY_TO_FUNC(prefix, ftype, etype, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_get_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->CONCAT_DOT(__VA_ARGS__); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype v) \
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

#define STRUCT_TO_FUNC(prefix, ftype, etype, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
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
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
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
#else
#define ENTRY_TO_FUNC(prefix, ftype, etype, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_get_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_set_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, test, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, CONCAT_US(tbl, prefix, __VA_ARGS__)); \
    }

#define STRUCT_TO_FUNC(prefix, ftype, etype, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_get_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(tbl, prefix, __VA_ARGS__)) \
            ret = kmpp_obj_tbl_set_##ftype(s, CONCAT_US(tbl, prefix, __VA_ARGS__), v); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, test, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, CONCAT_US(tbl, prefix, __VA_ARGS__)); \
    }
#endif

#define EHOOK_TO_FUNC(prefix, ftype, etype, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(hook, prefix, get, __VA_ARGS__) >= 0) \
            ret = kmpp_obj_idx_run(s, CONCAT_US(hook, prefix, get, __VA_ARGS__), (void *)v, __FUNCTION__); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(hook, prefix, set, __VA_ARGS__) >= 0) \
            ret = kmpp_obj_idx_run(s, CONCAT_US(hook, prefix, set, __VA_ARGS__), (void *)&v, __FUNCTION__); \
        return ret; \
    }

#define SHOOK_TO_FUNC(prefix, ftype, etype, name, flag, ...) \ \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (CONCAT_US(hook, prefix, get, __VA_ARGS__) >= 0) \
            ret = kmpp_obj_idx_run(s, CONCAT_US(hook, prefix, get, __VA_ARGS__), (void *)v, __FUNCTION__); \
        return ret; \
    } \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE s, etype *v) \
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

#define ARRAY_START(...)
#define ARRAY_START_FLEX_CNT_OFF(...)
#define ARRAY_ENTRY(...)
#define ARRAY_END(...)

/* object definition common functions */
static KmppObjDef KMPP_OBJ_DEF(KMPP_OBJ_NAME) = NULL;
static rk_u32 KMPP_OBJ_DEF_DEUBG(KMPP_OBJ_NAME) = 0;

/* globla variable definitions */
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, VAL_ENTRY_TBL, VAL_ENTRY_TBL,
                     VAL_HOOK_IDX, VAL_HOOK_IDX, ENTRY_NOTHING)

/* enable structure layout macro for objdef registration */
#ifdef KMPP_OBJ_HIERARCHY_ENABLE

#include "mpp_cfg_io.h"

#undef CFG_DEF_START
#undef CFG_DEF_END
#undef STRUCT_START
#undef STRUCT_END
#undef ENTRY_TO_NAME_START
#undef ENTRY_TO_NAME_END
#undef ARRAY_START
#undef ARRAY_START_FLEX_CNT_OFF
#undef ARRAY_ENTRY
#undef ARRAY_END

#define CFG_DEF_START(...) \
    { \
        char str_buf[256] = {0}; \
        rk_s32 str_pos __maybe_unused = 0; \
        rk_s32 str_size __maybe_unused = sizeof(str_buf) - 1; \
        rk_s32 __subroot __maybe_unused = 0; \
        MppCfgObj __parent __maybe_unused = NULL; \
        MppCfgObj root = NULL; \
        typedef KMPP_OBJ_IMPL_TYPE _array_elem_t __maybe_unused; \
        if (once) { \
            mpp_cfg_get_object(&root, TO_STR(KMPP_OBJ_NAME), MPP_CFG_TYPE_OBJECT, NULL); \
            __parent = root; \
        }

#define CFG_DEF_END(...) \
        { \
            if (once) { \
                kmpp_objdef_add_cfg_root(KMPP_OBJ_DEF(KMPP_OBJ_NAME), root); \
            } \
        } \
    }

#define STRUCT_START(...) \
    { \
        rk_s32 CONCAT_US(pos, __VA_ARGS__, root) = str_pos; \
        MppCfgObj CONCAT_US(obj, __VA_ARGS__) = NULL; \
        MppCfgObj CONCAT_US(__parent, __VA_ARGS__) = __parent; \
        if (once) { \
            str_pos += snprintf(str_buf + str_pos, str_size - str_pos, \
                                str_pos ? ":%s" : "%s", CONCAT_STR(__VA_ARGS__)); \
            mpp_cfg_get_object(&CONCAT_US(obj, __VA_ARGS__), CONCAT_STR(__VA_ARGS__), MPP_CFG_TYPE_OBJECT, NULL); \
            mpp_cfg_add_detail(CONCAT_US(__parent, __VA_ARGS__), CONCAT_US(obj, __VA_ARGS__)); \
            __parent = CONCAT_US(obj, __VA_ARGS__); \
        }

#define STRUCT_END(...) \
        str_pos = CONCAT_US(pos, __VA_ARGS__, root); \
        str_buf[str_pos] = '\0'; \
        if (__parent) \
            __parent = CONCAT_US(__parent, __VA_ARGS__); \
    }

#define ENTRY_TO_NAME_START(name, ...) \
    ({ \
        snprintf(str_buf + str_pos, str_size - str_pos, str_pos ? ":%s" : "%s", TO_STR(name)); \
        str_buf; \
    })

#define ENTRY_TO_NAME_END(...) \
    str_buf[str_pos] = '\0';

#define ARRAY_START(name, etype, flag, ...) \
    { \
        rk_s32 CONCAT_US(pos, name, root) = str_pos; \
        rk_s32 CONCAT_US(saved, name, sub) = __subroot; \
        rk_s32 __subroot = CONCAT_US(saved, name, sub); \
        MppCfgObj CONCAT_US(obj, name) = NULL; \
        MppCfgObj CONCAT_US(__parent, name) = __parent; \
        if (once) { \
            KmppEntry _vla = { .val = 0 }; \
            _vla.vla.type = ENTRY_TYPE_VLA_INFO; \
            _vla.vla.elem_size = (rk_u16)sizeof(etype); \
            _vla.vla.elem_count = (rk_u16)(sizeof(((_array_elem_t *)0)->CONCAT_DOT(__VA_ARGS__)) / sizeof(etype)); \
            _vla.vla.flex_count = 0; \
            _vla.vla.flex_base = 0; \
            _vla.vla.base_off = (rk_u16)((size_t)&(((_array_elem_t *)0)->CONCAT_DOT(__VA_ARGS__))); \
            str_pos += snprintf(str_buf + str_pos, str_size - str_pos, \
                                str_pos ? ":%s" : "%s", TO_STR(name)); \
            { \
                rk_s32 _ret = kmpp_objdef_add_entry(KMPP_OBJ_DEF(KMPP_OBJ_NAME), __subroot, \
                                                    __subroot ? TO_STR(name) : str_buf, &_vla); \
                if (_ret >= 0) __subroot = _ret; \
            } \
            mpp_cfg_get_array(&CONCAT_US(obj, name), TO_STR(name)); \
            mpp_cfg_set_vla(CONCAT_US(obj, name), &_vla, \
                mpp_cfg_type_from_elem_type(KMPP_TYPE_TO_ELEM_TYPE(etype))); \
            { \
                KmppEntry _tbl = { .val = 0 }; \
                _tbl.tbl.type = ENTRY_TYPE_LOC_TBL; \
                _tbl.tbl.elem_type = ELEM_TYPE_arr; \
                _tbl.tbl.elem_size = (rk_u16)(_vla.vla.elem_count * _vla.vla.elem_size); \
                _tbl.tbl.elem_offset = _vla.vla.base_off; \
                mpp_cfg_set_entry(CONCAT_US(obj, name), &_tbl); \
            } \
            mpp_cfg_add_detail(CONCAT_US(__parent, name), CONCAT_US(obj, name)); \
            __parent = CONCAT_US(obj, name); \
        } \
        typedef etype _array_elem_t __maybe_unused;

#define ARRAY_START_FLEX_CNT_OFF(name, etype, flag, count_fld, base_fld) \
    { \
        typedef etype _array_elem_t; \
        rk_s32 CONCAT_US(pos, name, root) = str_pos; \
        rk_s32 CONCAT_US(saved, name, sub) = __subroot; \
        rk_s32 __subroot = CONCAT_US(saved, name, sub); \
        MppCfgObj CONCAT_US(obj, name) = NULL; \
        MppCfgObj CONCAT_US(__parent, name) = __parent; \
        if (once) { \
            KmppEntry _vla = { .val = 0 }; \
            _vla.vla.type = ENTRY_TYPE_VLA_INFO; \
            _vla.vla.elem_size = (rk_u16)sizeof(etype); \
            _vla.vla.flex_count = 1; \
            _vla.vla.flex_base = 1; \
            _vla.vla.count_off = (rk_u16)((size_t)&(((KMPP_OBJ_IMPL_TYPE *)0)->count_fld)); \
            _vla.vla.base_off = (rk_u16)((size_t)&(((KMPP_OBJ_IMPL_TYPE *)0)->base_fld)); \
            str_pos += snprintf(str_buf + str_pos, str_size - str_pos, \
                                str_pos ? ":%s" : "%s", TO_STR(name)); \
            { \
                rk_s32 _ret = kmpp_objdef_add_entry(KMPP_OBJ_DEF(KMPP_OBJ_NAME), __subroot, \
                                                    __subroot ? TO_STR(name) : str_buf, &_vla); \
                if (_ret >= 0) __subroot = _ret; \
            } \
            mpp_cfg_get_array(&CONCAT_US(obj, name), TO_STR(name)); \
            mpp_cfg_set_vla(CONCAT_US(obj, name), &_vla, MPP_CFG_TYPE_OBJECT); \
            mpp_cfg_add_detail(CONCAT_US(__parent, name), CONCAT_US(obj, name)); \
            __parent = CONCAT_US(obj, name); \
        }

#define ARRAY_ENTRY(ftype, name, flag, ...) \
        do { \
            if (once) { \
                KmppEntry _fld = { .val = 0 }; \
                _fld.tbl.type = ENTRY_TYPE_LOC_TBL; \
                _fld.tbl.elem_type = ELEM_TYPE_##ftype; \
                _fld.tbl.elem_size = (rk_u16)sizeof(((_array_elem_t *)0)->CONCAT_DOT(__VA_ARGS__)); \
                _fld.tbl.elem_offset = (rk_u16)((size_t)&((_array_elem_t *)0)->CONCAT_DOT(__VA_ARGS__)); \
                snprintf(str_buf + str_pos, str_size - str_pos, \
                         str_pos ? ":%s" : "%s", TO_STR(name)); \
                if (kmpp_objdef_add_entry(KMPP_OBJ_DEF(KMPP_OBJ_NAME), \
                                          __subroot, TO_STR(name), &_fld) < 0) \
                    mpp_loge("add array entry %s failed\n", TO_STR(name)); \
                MppCfgObj CONCAT_US(obj, name) = NULL; \
                mpp_cfg_get_object(&CONCAT_US(obj, name), TO_STR(name), MPP_CFG_TYPE_##ftype, NULL); \
                mpp_cfg_set_entry(CONCAT_US(obj, name), &_fld); \
                mpp_cfg_add_detail(__parent, CONCAT_US(obj, name)); \
                str_buf[str_pos] = '\0'; \
            } \
        } while (0);

#define ARRAY_END(name) \
        str_pos = CONCAT_US(pos, name, root); \
        str_buf[str_pos] = '\0'; \
        if (__parent) \
            __parent = CONCAT_US(__parent, name); \
    }

#endif

static void CONCAT_US(KMPP_OBJ_NAME, register)(void)
{
    rk_u32 once __maybe_unused = 1;
    rk_u32 flags = 0;

    mpp_env_get_u32(TO_STR(CONCAT_US(KMPP_OBJ_NAME, debug)), &KMPP_OBJ_DEF_DEUBG(KMPP_OBJ_NAME), 0);

    /* objdef flags for kmpp_objdef_get() / kmpp_objdef_register() */
#ifdef KMPP_OBJ_HIERARCHY_ENABLE
    flags |= KMPP_OBJDEF_HIERARCHY;
#endif
#ifdef KMPP_OBJ_FLEX_ENTRY_ENABLE
    flags |= KMPP_OBJDEF_FLEX_ENTRY;
#endif
#ifdef KMPP_OBJ_CACHE_ENABLE
    flags |= KMPP_OBJDEF_CACHED;
#endif
#ifdef KMPP_OBJ_MISMATCH_LOG_DISABLE
    flags |= KMPP_OBJDEF_MISMATCH_LOG_DISABLE;
#endif

    KMPP_OBJ_DBG_LOG("register enter\n");

    kmpp_objdef_get(&KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_PRIV_SIZE,
                    TO_STR(KMPP_OBJ_INTF_TYPE), flags);
    if (KMPP_OBJ_DEF(KMPP_OBJ_NAME)) {
        KMPP_OBJ_DBG_LOG(TO_STR(KMPP_OBJ_NAME) " found at kernel\n");
    } else {
#ifdef KMPP_OBJ_IMPL_TYPE
        rk_s32 impl_size = (sizeof(KMPP_OBJ_IMPL_TYPE) + KMPP_OBJ_EXTRA_SIZE + 3) & ~3;
        rk_s32 __flag_base __maybe_unused = impl_size << 3;
        rk_s32 __flag_step __maybe_unused = 0;
        rk_s32 __flag_prev __maybe_unused = 0;
        rk_s32 __flag_record[ELEM_FLAG_RECORD_MAX] __maybe_unused;

        kmpp_objdef_register(&KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_PRIV_SIZE,
                             impl_size, TO_STR(KMPP_OBJ_INTF_TYPE), flags);

        if (!KMPP_OBJ_DEF(KMPP_OBJ_NAME)) {
            mpp_loge_f(TO_STR(KMPP_OBJ_NAME) " init failed\n");
            return;
        }

        KMPP_OBJ_DBG_LOG(TO_STR(KMPP_OBJ_NAME) " registered at userspace\n");

        KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, ENTRY_TO_TRIE, ENTRY_TO_TRIE,
                             ENTRY_TO_TRIE, ENTRY_TO_TRIE, ENTRY_TO_TRIE)
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(KMPP_OBJ_NAME), 0, NULL, NULL);
        once = 0;

#else
        KMPP_OBJ_DBG_LOG(TO_STR(KMPP_OBJ_NAME) " has no implementation\n");
        return;
#endif
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
#if defined(KMPP_OBJ_FUNC_RESIZE)
    kmpp_objdef_add_resize(KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_FUNC_RESIZE);
#endif
#if defined(KMPP_OBJ_FUNC_CACHE_INIT)
    kmpp_objdef_add_cache_init(KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_FUNC_CACHE_INIT);
#endif
#if defined(KMPP_OBJ_FUNC_CACHE_DEINIT)
    kmpp_objdef_add_cache_deinit(KMPP_OBJ_DEF(KMPP_OBJ_NAME), KMPP_OBJ_FUNC_CACHE_DEINIT);
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

KMPP_OBJ_SGLN(KMPP_OBJ_SGLN_ID, KMPP_OBJ_NAME, CONCAT_US(KMPP_OBJ_NAME, register), CONCAT_US(KMPP_OBJ_NAME, unregister))

rk_s32 CONCAT_US(KMPP_OBJ_NAME, size)(void)
{
    return kmpp_objdef_get_entry_size(KMPP_OBJ_DEF(KMPP_OBJ_NAME));
}

#ifndef KMPP_OBJ_RET_TYPE
#define KMPP_OBJ_RET_TYPE rk_s32
#endif

KMPP_OBJ_RET_TYPE CONCAT_US(KMPP_OBJ_NAME, get)(KMPP_OBJ_INTF_TYPE *obj)
{
    return kmpp_obj_get_f((KmppObj *)obj, KMPP_OBJ_DEF(KMPP_OBJ_NAME));
}

KMPP_OBJ_RET_TYPE CONCAT_US(KMPP_OBJ_NAME, put)(KMPP_OBJ_INTF_TYPE obj)
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

#undef ARRAY_START
#undef ARRAY_START_FLEX_CNT_OFF
#undef ARRAY_ENTRY
#undef ARRAY_END

#define ARRAY_START(...)
#define ARRAY_START_FLEX_CNT_OFF(...)
#define ARRAY_ENTRY(...)
#define ARRAY_END(...)

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
        static rk_s32 cmd = -1; \
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

#define IOCTL_OUT(prefix, func, out_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, out_type *out) \
    { \
        KmppObjDef def = KMPP_OBJ_DEF(KMPP_OBJ_NAME); \
        static rk_s32 cmd = -1; \
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

#define IOCTL_IO_(prefix, func, in_type, out_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, in_type in, out_type *out) \
    { \
        KmppObjDef def = KMPP_OBJ_DEF(KMPP_OBJ_NAME); \
        static rk_s32 cmd = -1; \
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

#undef KMPP_OBJ_RET_TYPE
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
#undef KMPP_OBJ_FUNC_RESIZE
#undef KMPP_OBJ_FUNC_CACHE_INIT
#undef KMPP_OBJ_FUNC_CACHE_DEINIT
#undef KMPP_OBJ_SGLN_ID
#undef KMPP_OBJ_FUNC_EXPORT_DISABLE
#undef KMPP_OBJ_ACCESS_DISABLE
#undef KMPP_OBJ_SHARE_DISABLE
#undef KMPP_OBJ_HIERARCHY_ENABLE
#undef KMPP_OBJ_MISMATCH_LOG_DISABLE
#undef KMPP_OBJ_FLEX_ENTRY_ENABLE
#undef KMPP_OBJ_CACHE_ENABLE

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
#undef MPP_CFG_TYPE_ptr
#undef MPP_CFG_TYPE_st
#undef MPP_CFG_TYPE_arr
#undef ELEM_TYPE_arr
#undef ARRAY_START
#undef ARRAY_START_FLEX_CNT_OFF
#undef ARRAY_ENTRY
#undef ARRAY_END

#undef OBJECT_HERLPER_H

#endif
