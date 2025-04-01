/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef KMPP_OBJ_NAME
#error "KMPP_OBJ_NAME must be defined on using kmpp_obj_func.h"
#endif

#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE must be defined on using kmpp_obj_func.h"
#endif

#ifndef KMPP_OBJ_CONCAT2
#define KMPP_OBJ_CONCAT2(a, b)          a##_##b
#endif
#ifndef KMPP_OBJ_CONCAT3
#define KMPP_OBJ_CONCAT3(a, b, c)       a##_##b##_##c
#endif
#ifndef KMPP_OBJ_CONCAT4
#define KMPP_OBJ_CONCAT4(a, b, c, d)    a##_##b##_##c##_##d
#endif

#define KMPP_OBJ_FUNC_DEFINE(prefix) \
rk_s32 KMPP_OBJ_CONCAT2(prefix, size)(void); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, get)(KMPP_OBJ_INTF_TYPE *p); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, put)(KMPP_OBJ_INTF_TYPE p); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, dump)(KMPP_OBJ_INTF_TYPE p, const char *caller);

KMPP_OBJ_FUNC_DEFINE(KMPP_OBJ_NAME)

/* simple entry access funcitons */
#ifdef KMPP_OBJ_ENTRY_TABLE
#define ENTRY_TO_DECLARE(prefix, ftype, type, f1, ...) \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, set, f1)(KMPP_OBJ_INTF_TYPE p, type  val); \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, get, f1)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, test, f1)(KMPP_OBJ_INTF_TYPE p);

KMPP_OBJ_ENTRY_TABLE(ENTRY_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_ENTRY_TABLE
#undef ENTRY_TO_DECLARE
#endif

/* simple entry read-only access funcitons */
#ifdef KMPP_OBJ_ENTRY_RO_TABLE
#define ENTRY_TO_DECLARE(prefix, ftype, type, f1, ...) \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, get, f1)(KMPP_OBJ_INTF_TYPE p, type* val);

KMPP_OBJ_ENTRY_RO_TABLE(ENTRY_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_ENTRY_RO_TABLE
#undef ENTRY_TO_DECLARE
#endif

/* structure entry access funcitons */
#ifdef KMPP_OBJ_STRUCT_TABLE
#define STRUCT_TO_DECLARE(prefix, ftype, type, f1, ...) \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, set, f1)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, get, f1)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, test, f1)(KMPP_OBJ_INTF_TYPE p);

KMPP_OBJ_STRUCT_TABLE(STRUCT_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_STRUCT_TABLE
#undef STRUCT_TO_DECLARE
#endif

/* simple entry hook access funcitons */
#ifdef KMPP_OBJ_ENTRY_HOOK
#define STRUCT_TO_DECLARE(prefix, ftype, type, f1, ...) \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, set, f1)(KMPP_OBJ_INTF_TYPE p, type val); \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, get, f1)(KMPP_OBJ_INTF_TYPE p, type* val);

KMPP_OBJ_ENTRY_HOOK(STRUCT_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_ENTRY_HOOK
#undef STRUCT_TO_DECLARE
#endif

/* structure entry hook access funcitons */
#ifdef KMPP_OBJ_STRUCT_HOOK
#define STRUCT_TO_DECLARE(prefix, ftype, type, f1, ...) \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, set, f1)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, get, f1)(KMPP_OBJ_INTF_TYPE p, type* val);

KMPP_OBJ_STRUCT_HOOK(STRUCT_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_STRUCT_HOOK
#undef STRUCT_TO_DECLARE
#endif

/* internal structure simple entry access funcitons */
#ifdef KMPP_OBJ_ENTRY_TABLE2
#define ENTRY_TO_DECLARE(prefix, ftype, type, f1, f2, ...) \
    rk_s32 KMPP_OBJ_CONCAT4(prefix, set, f1, f2)(KMPP_OBJ_INTF_TYPE p, type  val); \
    rk_s32 KMPP_OBJ_CONCAT4(prefix, get, f1, f2)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 KMPP_OBJ_CONCAT4(prefix, test, f1, f2)(KMPP_OBJ_INTF_TYPE p);

KMPP_OBJ_ENTRY_TABLE2(ENTRY_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_ENTRY_TABLE2
#undef ENTRY_TO_DECLARE
#endif

/* internal structure entry access funcitons */
#ifdef KMPP_OBJ_STRUCT_TABLE2
#define ENTRY_TO_DECLARE(prefix, ftype, type, f1, f2, ...) \
    rk_s32 KMPP_OBJ_CONCAT4(prefix, set, f1, f2)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 KMPP_OBJ_CONCAT4(prefix, get, f1, f2)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 KMPP_OBJ_CONCAT4(prefix, test, f1, f2)(KMPP_OBJ_INTF_TYPE p);

KMPP_OBJ_STRUCT_TABLE2(ENTRY_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_STRUCT_TABLE2
#undef ENTRY_TO_DECLARE
#endif

#undef KMPP_OBJ_NAME
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_FUNC_DEFINE
