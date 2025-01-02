/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include <linux/types.h>
#include "mpp_trie.h"

typedef enum EntryType_e {
    ENTRY_TYPE_s32,
    ENTRY_TYPE_u32,
    ENTRY_TYPE_s64,
    ENTRY_TYPE_u64,
    ENTRY_TYPE_ptr,
    ENTRY_TYPE_fp,      /* function poineter */
    ENTRY_TYPE_st,
    ENTRY_TYPE_BUTT,
} EntryType;

/* location table */
typedef union MppLocTbl_u {
    rk_u64              val;
    struct {
        rk_u16          data_offset;
        rk_u16          data_size       : 12;
        EntryType       data_type       : 3;
        rk_u16          flag_type       : 1;
        rk_u16          flag_offset;
        rk_u16          flag_value;
    };
} KmppLocTbl;

/* KmppObjDef - mpp object name size and access table trie definition */
typedef void* KmppObjDef;
/* KmppObj - mpp object for string name access and function access */
typedef void* KmppObj;

typedef void (*KmppObjPreset)(void *obj);
typedef rk_s32 (*KmppObjDump)(void *obj);

#ifdef __cplusplus
extern "C" {
#endif

/* query objdef from /dev/kmpp_objs */
rk_s32 kmpp_objdef_get(KmppObjDef *def, const char *name);
rk_s32 kmpp_objdef_put(KmppObjDef def);

rk_u32 kmpp_objdef_lookup(KmppObjDef *def, const char *name);
rk_s32 kmpp_objdef_dump(KmppObjDef def);

/* mpp objcet internal element set / get function */
const char *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
MppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* import kernel object ref */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def);
rk_s32 kmpp_obj_put(KmppObj obj);
rk_s32 kmpp_obj_check(KmppObj obj, const char *caller);

/* handle is the kernel share object userspace base address for kernel ioctl */
void *kmpp_obj_get_hnd(KmppObj obj);
/* handle size defined the copy size for kernel ioctl */
rk_s32 kmpp_obj_get_hnd_size(KmppObj obj);
/*
 * entry is the userspace address for kernel share object body
 * entry = handle + entry_offset
 */
void *kmpp_obj_get_entry(KmppObj obj);

rk_s32 kmpp_obj_set_s32(KmppObj obj, const char *name, rk_s32 val);
rk_s32 kmpp_obj_get_s32(KmppObj obj, const char *name, rk_s32 *val);
rk_s32 kmpp_obj_set_u32(KmppObj obj, const char *name, rk_u32 val);
rk_s32 kmpp_obj_get_u32(KmppObj obj, const char *name, rk_u32 *val);
rk_s32 kmpp_obj_set_s64(KmppObj obj, const char *name, rk_s64 val);
rk_s32 kmpp_obj_get_s64(KmppObj obj, const char *name, rk_s64 *val);
rk_s32 kmpp_obj_set_u64(KmppObj obj, const char *name, rk_u64 val);
rk_s32 kmpp_obj_get_u64(KmppObj obj, const char *name, rk_u64 *val);
rk_s32 kmpp_obj_set_ptr(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_ptr(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_set_fp(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_fp(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_set_st(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_st(KmppObj obj, const char *name, void *val);

rk_s32 kmpp_obj_tbl_set_s32(KmppObj obj, KmppLocTbl *tbl, rk_s32 val);
rk_s32 kmpp_obj_tbl_get_s32(KmppObj obj, KmppLocTbl *tbl, rk_s32 *val);
rk_s32 kmpp_obj_tbl_set_u32(KmppObj obj, KmppLocTbl *tbl, rk_u32 val);
rk_s32 kmpp_obj_tbl_get_u32(KmppObj obj, KmppLocTbl *tbl, rk_u32 *val);
rk_s32 kmpp_obj_tbl_set_s64(KmppObj obj, KmppLocTbl *tbl, rk_s64 val);
rk_s32 kmpp_obj_tbl_get_s64(KmppObj obj, KmppLocTbl *tbl, rk_s64 *val);
rk_s32 kmpp_obj_tbl_set_u64(KmppObj obj, KmppLocTbl *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_u64(KmppObj obj, KmppLocTbl *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_ptr(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_ptr(KmppObj obj, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_fp(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_fp(KmppObj obj, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppLocTbl *tbl, void *val);

/* run a callback function */
rk_s32 kmpp_obj_run(KmppObj obj, const char *name);

/* dump by userspace */
rk_s32 kmpp_obj_udump_f(KmppObj obj, const char *caller);
/* dump by kernel */
rk_s32 kmpp_obj_kdump_f(KmppObj obj, const char *caller);

#define kmpp_obj_udump(obj) kmpp_obj_udump_f(obj, __FUNCTION__)
#define kmpp_obj_kdump(obj) kmpp_obj_kdump_f(obj, __FUNCTION__)

#ifdef __cplusplus
}
#endif

#endif /* __KMPP_OBJ_H__ */
