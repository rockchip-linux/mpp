/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include <linux/types.h>
#include "mpp_trie.h"


/*
 * kernel - userspace transaction trie node ctx info (64 bit) definition
 *
 * +------+--------------------+---------------------------+
 * | 8bit |       24 bit       |          32 bit           |
 * +------+--------------------+---------------------------+
 *
 * bit 0~3 - 4-bit entry type (EntryType)
 * 0 - invalid entry
 * 1 - trie self info node
 * 2 - access location table node
 * 3 - ioctl cmd node
 *
 * bit 4~7 - 4-bit entry flag (EntryFlag) for different entry type
 */
typedef enum EntryType_e {
    ENTRY_TYPE_NONE     = 0x0,  /* invalid entry type */
    ENTRY_TYPE_VAL      = 0x1,  /* 32-bit value  */
    ENTRY_TYPE_STR      = 0x2,  /* string info property */
    ENTRY_TYPE_LOC_TBL  = 0x3,  /* entry location table */
    ENTRY_TYPE_BUTT,
} EntryType;

/*
 * 4-bit extention flag for different entry property
 * EntryValFlag     - for ENTRY_TYPE_VAL
 * EntryValFlag     - for ENTRY_TYPE_STR
 * EntryLocTblFlag  - for ENTRY_TYPE_LOC_TBL
 */
typedef enum EntryValFlag_e {
    /*
     * 0 - value is unsigned value
     * 1 - value is signed value
     */
    VALUE_SIGNED        = 0x1,
} EntryValFlag;

typedef enum EntryValUsage_e {
    VALUE_NORMAL        = 0x0,

    VALUE_TRIE          = 0x10,
    /* trie info value */
    VALUE_TRIE_INFO     = (VALUE_TRIE + 1),
    /* trie offset from the trie root */
    VALUE_TRIE_OFFSET   = (VALUE_TRIE + 2),

    /* ioctl cmd */
    VALUE_IOCTL_CMD     = 0x20,
} EntryValUsage;

typedef enum EntryStrFlag_e {
    STRING_NORMAL       = 0x0,
    /* string is trie self info */
    STRING_TRIE         = 0x1,
} EntryStrFlag;

typedef enum EntryLocTblFlag_e {
    /*
     * bit 4    - element can be accessed by kernel
     * bit 5    - element can be accessed by userspace
     * bit 6    - element is read-only
     */
    LOCTBL_KERNEL       = 0x1,
    LOCTBL_USERSPACE    = 0x2,
    LOCTBL_READONLY     = 0x4,
} EntryLocTblFlag;

typedef enum ElemType_e {
    /* commaon fix size value */
    ELEM_TYPE_FIX       = 0x0,
    ELEM_TYPE_s32       = (ELEM_TYPE_FIX + 0),
    ELEM_TYPE_u32       = (ELEM_TYPE_FIX + 1),
    ELEM_TYPE_s64       = (ELEM_TYPE_FIX + 2),
    ELEM_TYPE_u64       = (ELEM_TYPE_FIX + 3),
    /* pointer type stored by 64-bit */
    ELEM_TYPE_ptr       = (ELEM_TYPE_FIX + 4),
    /* value only structure */
    ELEM_TYPE_st        = (ELEM_TYPE_FIX + 5),

    /* kernel and userspace share data */
    ELEM_TYPE_SHARE     = 0x6,
    /* share memory between kernel and userspace */
    ELEM_TYPE_shm       = (ELEM_TYPE_SHARE + 0),

    /* kernel access only data */
    ELEM_TYPE_KERNEL    = 0x8,
    /* kenrel object poineter */
    ELEM_TYPE_kobj      = (ELEM_TYPE_KERNEL + 0),
    /* kenrel normal data poineter */
    ELEM_TYPE_kptr      = (ELEM_TYPE_KERNEL + 1),
    /* kernel function poineter */
    ELEM_TYPE_kfp       = (ELEM_TYPE_KERNEL + 2),

    /* userspace access only data */
    ELEM_TYPE_USER      = 0xc,
    /* userspace object poineter */
    ELEM_TYPE_uobj      = (ELEM_TYPE_USER + 0),
    /* userspace normal data poineter */
    ELEM_TYPE_uptr      = (ELEM_TYPE_USER + 1),
    /* userspace function poineter */
    ELEM_TYPE_ufp       = (ELEM_TYPE_USER + 2),

    ELEM_TYPE_BUTT      = 0xf,
} ElemType;

/* element update flag type */
typedef enum ElemFlagType_e {
    ELEM_FLAG_NONE,     /* element without update flag */
    ELEM_FLAG_START,    /* element update flag will align to new 32bit */
    ELEM_FLAG_UPDATE,   /* element flag increase by one */
    ELEM_FLAG_HOLD,     /* element flag equal to previous one */
} ElemFlagType;

typedef union KmppEntry_u {
    rk_u64                  val;
    union {
        EntryType           type            : 4;
        struct {
            EntryType       prop            : 4;
            EntryValFlag    flag            : 4;
            EntryValUsage   usage           : 8;
            rk_u32          reserve         : 16;
            rk_u32          val;
        } v;
        struct {
            EntryType       prop            : 4;
            EntryValFlag    flag            : 4;
            rk_u32          len             : 24;
            rk_u32          offset;
        } str;
        struct {
            EntryType       type            : 4;
            EntryLocTblFlag flag            : 4;
            ElemType        elem_type       : 8;
            rk_u16          elem_size;
            rk_u16          elem_offset;
            rk_u16          flag_offset;    /* define by ElemFlagType */
        } tbl;
    };
} KmppEntry;

/* MUST be the same to the KmppObjShm in rk-mpp-kobj.h */
typedef struct KmppShmPtr_t {
    /* uaddr - the userspace base address for userspace access */
    union {
        rk_u64 uaddr;
        void *uptr;
    };
    /* kaddr - the kernel base address for kernel access */
    union {
        rk_u64 kaddr;
        void *kptr;
    };
    /* DO NOT access reserved data only used by kernel */
} KmppShmPtr;

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

rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const char *name, KmppEntry **tbl);
rk_s32 kmpp_objdef_dump(KmppObjDef def);

/* mpp objcet internal element set / get function */
const char *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
MppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* import kernel object ref */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def, const char *caller);
rk_s32 kmpp_obj_get_by_name(KmppObj *obj, const char *name, const char *caller);
rk_s32 kmpp_obj_get_by_sptr(KmppObj *obj, KmppShmPtr *sptr, const char *caller);
rk_s32 kmpp_obj_put(KmppObj obj, const char *caller);
rk_s32 kmpp_obj_check(KmppObj obj, const char *caller);
rk_s32 kmpp_obj_ioctl(KmppObj obj, rk_s32 cmd, KmppObj in, KmppObj out, const char *caller);

#define kmpp_obj_get_f(obj, def)                kmpp_obj_get(obj, def, __FUNCTION__)
#define kmpp_obj_get_by_name_f(obj, name)       kmpp_obj_get_by_name(obj, name, __FUNCTION__)
#define kmpp_obj_get_by_sptr_f(obj, sptr)       kmpp_obj_get_by_sptr(obj, sptr, __FUNCTION__)
#define kmpp_obj_put_f(obj)                     kmpp_obj_put(obj, __FUNCTION__)
#define kmpp_obj_check_f(obj)                   kmpp_obj_check(obj, __FUNCTION__)
#define kmpp_obj_ioctl_f(obj, cmd, in, out)     kmpp_obj_ioctl(obj, cmd, in, out, __FUNCTION__)

/* KmppShmPtr is the kernel share object userspace base address for kernel ioctl */
KmppShmPtr *kmpp_obj_to_shm(KmppObj obj);
/* KmppShmPtr size defined the copy size for kernel ioctl */
rk_s32 kmpp_obj_to_shm_size(KmppObj obj);
const char *kmpp_obj_get_name(KmppObj obj);
/*
 * entry is the userspace address for kernel share object body
 * entry = KmppShmPtr->uaddr + entry_offset
 */
void *kmpp_obj_to_entry(KmppObj obj);
/* offset is the entry offset from kernel share object body */
rk_s32 kmpp_obj_to_offset(KmppObj obj, const char *name);

/* value access function */
rk_s32 kmpp_obj_set_s32(KmppObj obj, const char *name, rk_s32 val);
rk_s32 kmpp_obj_get_s32(KmppObj obj, const char *name, rk_s32 *val);
rk_s32 kmpp_obj_set_u32(KmppObj obj, const char *name, rk_u32 val);
rk_s32 kmpp_obj_get_u32(KmppObj obj, const char *name, rk_u32 *val);
rk_s32 kmpp_obj_set_s64(KmppObj obj, const char *name, rk_s64 val);
rk_s32 kmpp_obj_get_s64(KmppObj obj, const char *name, rk_s64 *val);
rk_s32 kmpp_obj_set_u64(KmppObj obj, const char *name, rk_u64 val);
rk_s32 kmpp_obj_get_u64(KmppObj obj, const char *name, rk_u64 *val);
rk_s32 kmpp_obj_set_st(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_st(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_tbl_set_s32(KmppObj obj, KmppEntry *tbl, rk_s32 val);
rk_s32 kmpp_obj_tbl_get_s32(KmppObj obj, KmppEntry *tbl, rk_s32 *val);
rk_s32 kmpp_obj_tbl_set_u32(KmppObj obj, KmppEntry *tbl, rk_u32 val);
rk_s32 kmpp_obj_tbl_get_u32(KmppObj obj, KmppEntry *tbl, rk_u32 *val);
rk_s32 kmpp_obj_tbl_set_s64(KmppObj obj, KmppEntry *tbl, rk_s64 val);
rk_s32 kmpp_obj_tbl_get_s64(KmppObj obj, KmppEntry *tbl, rk_s64 *val);
rk_s32 kmpp_obj_tbl_set_u64(KmppObj obj, KmppEntry *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_u64(KmppObj obj, KmppEntry *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppEntry *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppEntry *tbl, void *val);

/* userspace access only function */
rk_s32 kmpp_obj_set_obj(KmppObj obj, const char *name, KmppObj val);
rk_s32 kmpp_obj_get_obj(KmppObj obj, const char *name, KmppObj *val);
rk_s32 kmpp_obj_set_ptr(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_ptr(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_set_fp(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_fp(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_tbl_set_obj(KmppObj obj, KmppEntry *tbl, KmppObj val);
rk_s32 kmpp_obj_tbl_get_obj(KmppObj obj, KmppEntry *tbl, KmppObj *val);
rk_s32 kmpp_obj_tbl_set_ptr(KmppObj obj, KmppEntry *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_ptr(KmppObj obj, KmppEntry *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_fp(KmppObj obj, KmppEntry *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_fp(KmppObj obj, KmppEntry *tbl, void **val);

/* share access function */
rk_s32 kmpp_obj_set_shm(KmppObj obj, const char *name, KmppShmPtr *val);
rk_s32 kmpp_obj_get_shm(KmppObj obj, const char *name, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_set_shm(KmppObj obj, KmppEntry *tbl, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_get_shm(KmppObj obj, KmppEntry *tbl, KmppShmPtr *val);

/* helper for get share object from a share memory element */
rk_s32 kmpp_obj_set_shm_obj(KmppObj obj, const char *name, KmppObj val);
rk_s32 kmpp_obj_get_shm_obj(KmppObj obj, const char *name, KmppObj *val);

/* update flag check function */
rk_s32 kmpp_obj_test(KmppObj obj, const char *name);
rk_s32 kmpp_obj_tbl_test(KmppObj obj, KmppEntry *tbl);

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
