/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_INTERNAL_H__
#define __MPP_INTERNAL_H__

#include "rk_type.h"
#include "mpp_err.h"

/*
 * All mpp / kmpp internal interface object list here.
 */
typedef void* MppTrie;
typedef void* MppCfgObj;

/* MppObjDef - mpp object name size and access table trie definition */
typedef void* MppObjDef;
/* MppObj    - mpp object for string name access and function access */
typedef void* MppObj;
typedef void* MppIoc;

/* KmppObjDef - mpp kernel object name size and access table trie definition */
typedef void* KmppObjDef;
/* KmppObj    - mpp kernel object for string name access and function access */
typedef void* KmppObj;

typedef enum CfgType_e {
    CFG_FUNC_TYPE_S32,
    CFG_FUNC_TYPE_U32,
    CFG_FUNC_TYPE_S64,
    CFG_FUNC_TYPE_U64,
    CFG_FUNC_TYPE_St,
    CFG_FUNC_TYPE_Ptr,
    CFG_FUNC_TYPE_BUTT,
} CfgType;

typedef struct MppCfgInfo_t {
    CfgType             data_type;
    /* update flag info 32bit */
    RK_U32              flag_offset;
    RK_U32              flag_value;
    /* data access info */
    RK_U32              data_offset;
    RK_S32              data_size;
} MppCfgInfo;

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

#define ELEM_FLAG_OP_SHIFT  8
#define ELEM_FLAG_IDX_MASK  ((1 << ELEM_FLAG_OP_SHIFT) - 1)
/*
 * element update flag bits usage:
 * bit 0  - 7   record / replay operation index bit
 * bit 8  - 9   record / replay operation bit
 * bit 10 - 11  update flag update operation invalid / start / update / hold
 */
typedef enum ElemFlagType_e {
    /* element without update flag (not available) */
    ELEM_FLAG_NONE          = (1 << ELEM_FLAG_OP_SHIFT),
    /* element update flag will align to new 32bit */
    ELEM_FLAG_START         = (2 << ELEM_FLAG_OP_SHIFT),
    /* element flag increase by one */
    ELEM_FLAG_UPDATE        = (3 << ELEM_FLAG_OP_SHIFT),
    /* element flag equal to previous one */
    ELEM_FLAG_HOLD          = (4 << ELEM_FLAG_OP_SHIFT),
    ELEM_FLAG_OP_MASK       = (7 << ELEM_FLAG_OP_SHIFT),

    /* index for record element update flag */
    ELEM_FLAG_RECORD        = (8 << ELEM_FLAG_OP_SHIFT),
    ELEM_FLAG_RECORD_0      = (ELEM_FLAG_RECORD + 0),
    ELEM_FLAG_RECORD_1      = (ELEM_FLAG_RECORD + 1),
    ELEM_FLAG_RECORD_2      = (ELEM_FLAG_RECORD + 2),
    ELEM_FLAG_RECORD_3      = (ELEM_FLAG_RECORD + 3),
    ELEM_FLAG_RECORD_4      = (ELEM_FLAG_RECORD + 4),
    ELEM_FLAG_RECORD_5      = (ELEM_FLAG_RECORD + 5),
    ELEM_FLAG_RECORD_6      = (ELEM_FLAG_RECORD + 6),
    ELEM_FLAG_RECORD_7      = (ELEM_FLAG_RECORD + 7),
    ELEM_FLAG_RECORD_8      = (ELEM_FLAG_RECORD + 8),
    ELEM_FLAG_RECORD_9      = (ELEM_FLAG_RECORD + 9),
    ELEM_FLAG_RECORD_BUT,
    ELEM_FLAG_RECORD_MAX    = (ELEM_FLAG_RECORD_BUT - ELEM_FLAG_RECORD),

    /* index for replay element update flag */
    ELEM_FLAG_REPLAY        = (16 << ELEM_FLAG_OP_SHIFT),
    ELEM_FLAG_REPLAY_0      = (ELEM_FLAG_REPLAY + 0),
    ELEM_FLAG_REPLAY_1      = (ELEM_FLAG_REPLAY + 1),
    ELEM_FLAG_REPLAY_2      = (ELEM_FLAG_REPLAY + 2),
    ELEM_FLAG_REPLAY_3      = (ELEM_FLAG_REPLAY + 3),
    ELEM_FLAG_REPLAY_4      = (ELEM_FLAG_REPLAY + 4),
    ELEM_FLAG_REPLAY_5      = (ELEM_FLAG_REPLAY + 5),
    ELEM_FLAG_REPLAY_6      = (ELEM_FLAG_REPLAY + 6),
    ELEM_FLAG_REPLAY_7      = (ELEM_FLAG_REPLAY + 7),
    ELEM_FLAG_REPLAY_8      = (ELEM_FLAG_REPLAY + 8),
    ELEM_FLAG_REPLAY_9      = (ELEM_FLAG_REPLAY + 9),
    ELEM_FLAG_REPLAY_BUT,
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

#endif /*__MPP_INTERNAL_H__*/
