/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_MACRO_H__
#define __KMPP_OBJ_MACRO_H__

#include <string.h>

#include "mpp_env.h"
#include "mpp_debug.h"
#include "mpp_singleton.h"

#include "kmpp_obj.h"

#define _TO_STR(x)                  #x
#define TO_STR(x)                   _TO_STR(x)

/* concat two args */
#define CONCAT_1(a)                 a
#define CONCAT_2(a,b)               a##b
#define CONCAT_3(a,b,c)             a##b##c
#define CONCAT_4(a,b,c,d)           a##b##c##d
#define CONCAT_5(a,b,c,d,e)         a##b##c##d##e
#define CONCAT_6(a,b,c,d,e,f)       a##b##c##d##e##f

#define CONCAT_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CONCAT(...)     CONCAT_HELPER(__VA_ARGS__, CONCAT_6, CONCAT_5, CONCAT_4, CONCAT_3, CONCAT_2, CONCAT_1)(__VA_ARGS__)

/* concat by underscore */
#define CONCAT_US1(a)               a
#define CONCAT_US2(a,b)             a##_##b
#define CONCAT_US3(a,b,c)           a##_##b##_##c
#define CONCAT_US4(a,b,c,d)         a##_##b##_##c##_##d
#define CONCAT_US5(a,b,c,d,e)       a##_##b##_##c##_##d##_##e
#define CONCAT_US6(a,b,c,d,e,f)     a##_##b##_##c##_##d##_##e##_##f

#define CONCAT_US_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CONCAT_US(...)  CONCAT_US_HELPER(__VA_ARGS__, CONCAT_US6, CONCAT_US5, CONCAT_US4, CONCAT_US3, CONCAT_US2, CONCAT_US1)(__VA_ARGS__)

/* concat by dot */
#define CONCAT_DOT1(a)              a
#define CONCAT_DOT2(a,b)            a.b
#define CONCAT_DOT3(a,b,c)          a.b.c
#define CONCAT_DOT4(a,b,c,d)        a.b.c.d
#define CONCAT_DOT5(a,b,c,d,e)      a.b.c.d.e
#define CONCAT_DOT6(a,b,c,d,e,f)    a.b.c.d.e.f

#define CONCAT_DOT_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CONCAT_DOT(...) CONCAT_DOT_HELPER(__VA_ARGS__, CONCAT_DOT6, CONCAT_DOT5, CONCAT_DOT4, CONCAT_DOT3, CONCAT_DOT2, CONCAT_DOT1)(__VA_ARGS__)

/* concat to string connect with colon */
#define CONCAT_STR1(a)              TO_STR(a)
#define CONCAT_STR2(a,b)            TO_STR(a:b)
#define CONCAT_STR3(a,b,c)          TO_STR(a:b:c)
#define CONCAT_STR4(a,b,c,d)        TO_STR(a:b:c:d)
#define CONCAT_STR5(a,b,c,d,e)      TO_STR(a:b:c:d:e)
#define CONCAT_STR6(a,b,c,d,e,f)    TO_STR(a:b:c:d:e:f)

#define CONCAT_STR_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CONCAT_STR(...) CONCAT_STR_HELPER(__VA_ARGS__, CONCAT_STR6, CONCAT_STR5, CONCAT_STR4, CONCAT_STR3, CONCAT_STR2, CONCAT_STR1)(__VA_ARGS__)

/* objdef struct name */
#define KMPP_OBJ_DEF(x)             CONCAT_US(x, def)
#define KMPP_OBJ_DEF_DEUBG(x)       CONCAT_US(x, debug)
#define KMPP_OBJ_DBG_LOG(...)       mpp_logi_c(KMPP_OBJ_DEF_DEUBG(KMPP_OBJ_NAME), __VA_ARGS__)

/*
 * element update flag bits usage:
 * bit 0  - 7   record / replay operation index bit
 * bit 8  - 9   record / replay operation bit
 * bit 10 - 11  update flag update operation invalid / start / update / hold
 */
typedef union ElemFlagDef_u {
    rk_u32 val;
    struct {
        rk_u32 idx      : 8;
        rk_u32 slot     : 4;
        rk_u32 op       : 4;
        rk_u32 record   : 1;
        rk_u32 replay   : 1;
        rk_u32 reserved : 10;
    };
} ElemFlagDef;

#define ELEM_FLAG_OP_SHIFT  8
#define ELEM_FLAG_IDX_MASK  ((1 << ELEM_FLAG_OP_SHIFT) - 1)

typedef enum ElemFlagType_e {
    /* element without update flag (not available) */
    ELEM_FLAG_NONE,
    /* element update flag will align to new 32bit */
    ELEM_FLAG_START,
    /* element flag align up to 64bit */
    ELEM_FLAG_START64,
    /* element flag increase by one */
    ELEM_FLAG_OFFSET,
    /* element flag increase by one */
    ELEM_FLAG_INCR,
    /* element flag equal to previous one */
    ELEM_FLAG_PREV,

    ELEM_FLAG_RECORD_MAX    = 16,
} ElemFlagType;

#define FLAG_NONE       ((0 & 0xff) | ((0 & 0xf) << 8) | (ELEM_FLAG_NONE   << 12) | (0 << 16) | (0 << 17))
#define FLAG_BASE(x)    ((0 & 0xff) | ((0 & 0xf) << 8) | (ELEM_FLAG_START  << 12) | (0 << 16) | (0 << 17))
#define FLAG_AT(x)      ((x & 0xff) | ((0 & 0xf) << 8) | (ELEM_FLAG_OFFSET << 12) | (0 << 16) | (0 << 17))
#define FLAG_INCR       ((0 & 0xff) | ((0 & 0xf) << 8) | (ELEM_FLAG_INCR   << 12) | (0 << 16) | (0 << 17))
#define FLAG_PREV       ((0 & 0xff) | ((0 & 0xf) << 8) | (ELEM_FLAG_PREV   << 12) | (0 << 16) | (0 << 17))
#define FLAG_REC(s, x)  ((x & 0xff) | ((s & 0xf) << 8) | (ELEM_FLAG_OFFSET << 12) | (1 << 16) | (0 << 17))
#define FLAG_REC_INC(s) ((0 & 0xff) | ((s & 0xf) << 8) | (ELEM_FLAG_INCR   << 12) | (1 << 16) | (0 << 17))
#define FLAG_REPLAY(s)  ((0 & 0xff) | ((s & 0xf) << 8) | (ELEM_FLAG_OFFSET << 12) | (0 << 16) | (1 << 17))

/* macro for register structure update flag type to offset */
#define FLAG_TYPE_TO_OFFSET(name, flag, flag_str) \
    ({ \
        ElemFlagDef __flag = { .val = flag, }; \
        rk_u16 __offset; \
        switch (__flag.op) { \
        case ELEM_FLAG_START : { \
            /* NOTE: increase to next 32bit */ \
            if (__flag_prev > __flag_base) \
                __flag_base = (__flag_prev + 31) & (~31); \
            else if (__flag_prev == __flag_base) \
                __flag_base = ((__flag_base + 32) & (~31)); \
            __flag_step = 0; \
            __offset = __flag_prev = __flag_base; \
        } break; \
        case ELEM_FLAG_START64 : { \
            /* NOTE: increase to next 64bit */ \
            if (__flag_prev > __flag_base) \
                __flag_base = (__flag_prev + 63) & (~63); \
            else if (__flag_prev == __flag_base) \
                __flag_base = ((__flag_base + 64) & (~63)); \
            __flag_step = 0; \
            __offset = __flag_prev = __flag_base; \
        } break; \
        case ELEM_FLAG_OFFSET : { \
            /* define offset to the base */ \
            __offset = __flag_prev = __flag_base + __flag.idx; \
            if (__flag.idx > __flag_step) \
                __flag_step = __flag.idx; \
        } break; \
        case ELEM_FLAG_INCR : { \
            /* increase from the max step */ \
            __flag_step++; \
            __offset = __flag_prev = __flag_base + __flag_step; \
         } break; \
        case ELEM_FLAG_PREV : { \
            __offset = __flag_prev; \
        } break; \
        default : { \
            __offset = 0; \
        } break; \
        }; \
        if (__flag.record) { \
            __flag_record[__flag.slot] = __offset; \
        } \
        if (__flag.replay) { \
            __offset = __flag_record[__flag.slot]; \
        } \
        KMPP_OBJ_DBG_LOG("%-20s - (%x:%x:%02x) -> %#4x (%2d) - %s\n", \
                         TO_STR(name), __flag_base, __flag_prev, __flag_step, \
                         __offset, __offset ? __offset - __flag_base : 0, flag_str); \
        __offset; \
    })

#define ENTRY_NOTHING(prefix, ftype, type, name, flag, ...)

#define GET_ARG0(val, ...)      GET_ARG0_CHOOSER(dummy, ##__VA_ARGS__)(__VA_ARGS__)
#define GET_ARG0_1(_1)          (-1)
#define GET_ARG0_2(_1, _2, ...) _1
#define GET_ARG0_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, N, ...) GET_ARG0_##N
#define GET_ARG0_CHOOSER(...)  GET_ARG0_N(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#endif /* __KMPP_OBJ_MACRO_H__ */
