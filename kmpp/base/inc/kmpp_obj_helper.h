/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_HERLPER_H__
#define __KMPP_OBJ_HERLPER_H__

#if !defined(KMPP_OBJ_NAME) || \
    !defined(KMPP_OBJ_INTF_TYPE) || \
    !defined(KMPP_OBJ_IMPL_TYPE)

#warning "When using kmpp_obj_helper.h The following macro must be defined:"
#warning "KMPP_OBJ_NAME                 - object name"
#warning "KMPP_OBJ_INTF_TYPE            - object interface type"
#warning "KMPP_OBJ_IMPL_TYPE            - object implement type"
#warning "option macro:"
#warning "KMPP_OBJ_ENTRY_TABLE          - object element value / pointer entry table"
#warning "KMPP_OBJ_STRUCT_TABLE         - object element structure / array / share memory table"
#warning "KMPP_OBJ_ENTRY_TABLE2         - object element value / pointer entry 2 level table"
#warning "KMPP_OBJ_STRUCT_TABLE2        - object element structure / array / share memory 2 level table"
#warning "KMPP_OBJ_FUNC_IOCTL           - object element ioctl cmd and function table"
#warning "KMPP_OBJ_FUNC_STUB_ENABLE     - enable function stub mode by define empty function"

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

#include <string.h>
#include <linux/stddef.h>

#include "mpp_debug.h"
#include "kmpp_obj.h"

#define KMPP_OBJ_TO_STR(x)      #x
#define KMPP_OBJ_DEF(x)         x##_def
#define KMPP_OBJ_DEF_NAME(x)    KMPP_OBJ_TO_STR(x)

#ifndef KMPP_OBJ_ENTRY_TABLE
#define KMPP_OBJ_ENTRY_TABLE(ENTRY, prefix)
#endif
#ifndef KMPP_OBJ_ENTRY_RO_TABLE
#define KMPP_OBJ_ENTRY_RO_TABLE(ENTRY, prefix)
#endif
#ifndef KMPP_OBJ_STRUCT_TABLE
#define KMPP_OBJ_STRUCT_TABLE(ENTRY, prefix)
#endif
#ifndef KMPP_OBJ_ENTRY_TABLE2
#define KMPP_OBJ_ENTRY_TABLE2(ENTRY, prefix)
#endif
#ifndef KMPP_OBJ_STRUCT_TABLE2
#define KMPP_OBJ_STRUCT_TABLE2(ENTRY, prefix)
#endif

#define ENTRY_QUERY(prefix, ftype, type, f1, ...) \
    do { \
        kmpp_objdef_get_entry(KMPP_OBJ_DEF(prefix), #f1, &tbl_##prefix##_##f1); \
    } while (0);

#define ENTRY_QUERY2(prefix, ftype, type, f1, f2, ...) \
    do { \
        kmpp_objdef_get_entry(KMPP_OBJ_DEF(prefix), #f1":"#f2, &tbl_##prefix##_##f1##_##f2); \
    } while (0);

#define KMPP_OBJ_FUNC2(a, b)        a##_##b
#define KMPP_OBJ_FUNC3(a, b, c)     a##_##b##_##c
#define KMPP_OBJ_FUNC4(a, b, c, d)  a##_##b##_##c##_##d

#if defined(KMPP_OBJ_FUNC_IOCTL)
#define KMPP_OBJ_ADD_IOCTL(x)       kmpp_objdef_add_ioctl(KMPP_OBJ_DEF(x), &KMPP_OBJ_FUNC_IOCTL)
#else
#define KMPP_OBJ_ADD_IOCTL(x)
#endif

#define KMPP_OBJ_ENTRY_FUNC(prefix, ftype, type, field, ...) \
    static KmppEntry *tbl_##prefix##_##field = NULL; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field; \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field = v; \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, test, field)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, tbl_##prefix##_##field); \
    }

#define KMPP_OBJ_ENTRY_RO_FUNC(prefix, ftype, type, field, ...) \
    static KmppEntry *tbl_##prefix##_##field = NULL; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field; \
        return ret; \
    }

#define KMPP_OBJ_STRUCT_FUNC(prefix, ftype, type, field, ...) \
    static KmppEntry *tbl_##prefix##_##field = NULL; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            memcpy(v, &((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field, \
                   sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->field)); \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            memcpy(&((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field, v, \
                   sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->field)); \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, test, field)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, tbl_##prefix##_##field); \
    }

#define KMPP_OBJ_ENTRY_FUNC2(prefix, ftype, type, f1, f2, ...) \
    static KmppEntry *tbl_##prefix##_##f1##_##f2 = NULL; \
    rk_s32 KMPP_OBJ_FUNC4(prefix, get, f1, f2)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##f1##_##f2) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##prefix##_##f1##_##f2, v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->f1.f2; \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC4(prefix, set, f1, f2)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##f1##_##f2) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##prefix##_##f1##_##f2, v); \
        else \
            ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->f1.f2 = v; \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC4(prefix, test, f1, f2)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, tbl_##prefix##_##f1##_##f2); \
    }

#define KMPP_OBJ_STRUCT_FUNC2(prefix, ftype, type, f1, f2, ...) \
    static KmppEntry *tbl_##prefix##_##f1##_##f2 = NULL; \
    rk_s32 KMPP_OBJ_FUNC4(prefix, get, f1, f2)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##f1##_##f2) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##prefix##_##f1##_##f2, v); \
        else \
            memcpy(v, &((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->f1.f2, \
                   sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->f1.f2)); \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC4(prefix, set, f1, f2)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##f1##_##f2) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##prefix##_##f1##_##f2, v); \
        else \
            memcpy(&((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->f1.f2, v, \
                   sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->f1.f2)); \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC4(prefix, test, f1, f2)(KMPP_OBJ_INTF_TYPE s) \
    { \
        if (kmpp_obj_check(s, __FUNCTION__)) return 0; \
        return kmpp_obj_tbl_test(s, tbl_##prefix##_##f1##_##f2); \
    }

#define KMPP_OBJS_USAGE_SET(prefix) \
static KmppObjDef KMPP_OBJ_DEF(prefix) = NULL; \
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_ENTRY_FUNC, prefix) \
KMPP_OBJ_ENTRY_RO_TABLE(KMPP_OBJ_ENTRY_RO_FUNC, prefix) \
KMPP_OBJ_STRUCT_TABLE(KMPP_OBJ_STRUCT_FUNC, prefix) \
KMPP_OBJ_ENTRY_TABLE2(KMPP_OBJ_ENTRY_FUNC2, prefix) \
KMPP_OBJ_STRUCT_TABLE2(KMPP_OBJ_STRUCT_FUNC2, prefix) \
rk_s32 KMPP_OBJ_FUNC2(prefix, init)(void) __attribute__((constructor)); \
rk_s32 KMPP_OBJ_FUNC2(prefix, init)(void) \
{ \
    kmpp_objdef_get(&KMPP_OBJ_DEF(prefix), KMPP_OBJ_DEF_NAME(KMPP_OBJ_INTF_TYPE)); \
    if (!KMPP_OBJ_DEF(prefix)) \
        return rk_nok; \
    KMPP_OBJ_ADD_IOCTL(prefix); \
    KMPP_OBJ_ENTRY_TABLE(ENTRY_QUERY, prefix) \
    KMPP_OBJ_ENTRY_RO_TABLE(ENTRY_QUERY, prefix) \
    KMPP_OBJ_STRUCT_TABLE(ENTRY_QUERY, prefix) \
    KMPP_OBJ_ENTRY_TABLE2(ENTRY_QUERY2, prefix) \
    KMPP_OBJ_STRUCT_TABLE2(ENTRY_QUERY2, prefix) \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, deinit)(void) __attribute__((destructor)); \
rk_s32 KMPP_OBJ_FUNC2(prefix, deinit)(void) \
{ \
    KmppObjDef def = __sync_fetch_and_and(&KMPP_OBJ_DEF(prefix), NULL); \
    return kmpp_objdef_put(def); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, size)(void) \
{ \
    return kmpp_objdef_get_entry_size(KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, get)(KMPP_OBJ_INTF_TYPE *obj) \
{ \
    return kmpp_obj_get_f((KmppObj *)obj, KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, put)(KMPP_OBJ_INTF_TYPE obj) \
{ \
    return kmpp_obj_put_f(obj); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, dump)(KMPP_OBJ_INTF_TYPE obj, const char *caller) \
{ \
    return kmpp_obj_udump_f(obj, caller); \
}

KMPP_OBJS_USAGE_SET(KMPP_OBJ_NAME);

#undef KMPP_OBJ_TO_STR
#undef KMPP_OBJ_DEF

#undef KMPP_OBJ_NAME
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_IMPL_TYPE
#undef KMPP_OBJ_ENTRY_TABLE
#undef KMPP_OBJ_ENTRY_RO_TABLE
#undef KMPP_OBJ_STRUCT_TABLE
#undef KMPP_OBJ_ENTRY_TABLE2
#undef KMPP_OBJ_STRUCT_TABLE2
#undef KMPP_OBJ_FUNC_IOCTL

#undef KMPP_OBJ_FUNC2
#undef KMPP_OBJ_FUNC3
#undef KMPP_OBJ_FUNC4
#undef KMPP_OBJ_ENTRY_FUNC
#undef KMPP_OBJ_ENTRY_RO_FUNC
#undef KMPP_OBJ_ENTRY_FUNC2
#undef KMPP_OBJ_STRUCT_FUNC
#undef KMPP_OBJ_STRUCT_FUNC2
#undef KMPP_OBJ_ADD_IOCTL

#undef __KMPP_OBJ_HERLPER_H__

#endif

#endif /* __KMPP_OBJ_HERLPER_H__ */
