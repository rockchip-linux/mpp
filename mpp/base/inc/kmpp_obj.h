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

/* kernel object trie share info */
typedef struct KmppObjTrie_t {
    /* share object trie root userspace address (read only) */
    __u64               trie_root;
} KmppObjTrie;

/* kernel object share memory */
typedef struct KmppObjShm_t {
    __u64               kobj_uaddr;
    __u64               kobj_kaddr;
    /* DO NOT access reserved data only used by kernel */
} KmppObjShm;

/* KmppObjDef - mpp object name size and access table trie definition */
typedef void* KmppObjDef;
/* KmppObj - mpp object for string name access and function access */
typedef void* KmppObj;

typedef void (*KmppObjPreset)(void *obj);
typedef rk_s32 (*KmppObjDump)(void *obj);

rk_s32 kmpp_objdef_init(KmppObjDef *def, const char *name);
rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const char *name, KmppLocTbl **tbl);
rk_s32 kmpp_objdef_add_trie_root(KmppObjDef def, void *root);
rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump);
rk_s32 kmpp_objdef_deinit(KmppObjDef def);
rk_u32 kmpp_objdef_lookup(KmppObjDef *def, const char *name);

/* mpp objcet internal element set / get function */
const char *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
MppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* import kernel object ref */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def);
rk_s32 kmpp_obj_put(KmppObj obj);
rk_s32 kmpp_obj_check(KmppObj obj, const char *caller);

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
rk_s32 kmpp_obj_dump(KmppObj obj, const char *caller);

#define kmpp_obj_dump_f(obj) kmpp_obj_dump(obj, __FUNCTION__)

#endif /* __KMPP_OBJ_H__ */