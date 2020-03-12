/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_COMMON_H__
#define __MPP_COMMON_H__

#include "rk_type.h"

#define MPP_TAG_SIZE            32

#define MPP_ABS(x)              ((x) < (0) ? -(x) : (x))

#define MPP_MAX(a, b)           ((a) > (b) ? (a) : (b))
#define MPP_MAX3(a, b, c)       MPP_MAX(MPP_MAX(a,b),c)
#define MPP_MAX4(a, b, c, d)    MPP_MAX((a), MPP_MAX3((b), (c), (d)))

#define MPP_MIN(a,b)            ((a) > (b) ? (b) : (a))
#define MPP_MIN3(a,b,c)         MPP_MIN(MPP_MIN(a,b),c)
#define MPP_MIN4(a, b, c, d)    MPP_MIN((a), MPP_MIN3((b), (c), (d)))

#define MPP_DIV(a, b)           ((b) ? (a) / (b) : (a))

#define MPP_CLIP3(l, h, v)      ((v) < (l) ? (l) : ((v) > (h) ? (h) : (v)))
#define MPP_SIGN(a)             ((a) < (0) ? (-1) : (1))
#define MPP_DIV_SIGN(a, b)      (((a) + (MPP_SIGN(a) * (b)) / 2) / (b))

#define MPP_SWAP(type, a, b)    do {type SWAP_tmp = b; b = a; a = SWAP_tmp;} while(0)
#define MPP_ARRAY_ELEMS(a)      (sizeof(a) / sizeof((a)[0]))
#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))
#define MPP_VSWAP(a, b)         { a ^= b; b ^= a; a ^= b; }

#define MPP_RB16(x)  ((((const RK_U8*)(x))[0] << 8) | ((const RK_U8*)(x))[1])
#define MPP_WB16(p, d) do { \
        ((RK_U8*)(p))[1] = (d); \
        ((RK_U8*)(p))[0] = (d)>>8; } while(0)

#define MPP_RL16(x)  ((((const RK_U8*)(x))[1] << 8) | \
                     ((const RK_U8*)(x))[0])
#define MPP_WL16(p, d) do { \
        ((RK_U8*)(p))[0] = (d); \
        ((RK_U8*)(p))[1] = (d)>>8; } while(0)

#define MPP_RB32(x)  ((((const RK_U8*)(x))[0] << 24) | \
                     (((const RK_U8*)(x))[1] << 16) | \
                     (((const RK_U8*)(x))[2] <<  8) | \
                     ((const RK_U8*)(x))[3])
#define MPP_WB32(p, d) do { \
        ((RK_U8*)(p))[3] = (d); \
        ((RK_U8*)(p))[2] = (d)>>8; \
        ((RK_U8*)(p))[1] = (d)>>16; \
        ((RK_U8*)(p))[0] = (d)>>24; } while(0)

#define MPP_RL32(x) ((((const RK_U8*)(x))[3] << 24) | \
                    (((const RK_U8*)(x))[2] << 16) | \
                    (((const RK_U8*)(x))[1] <<  8) | \
                    ((const RK_U8*)(x))[0])
#define MPP_WL32(p, d) do { \
        ((RK_U8*)(p))[0] = (d); \
        ((RK_U8*)(p))[1] = (d)>>8; \
        ((RK_U8*)(p))[2] = (d)>>16; \
        ((RK_U8*)(p))[3] = (d)>>24; } while(0)

#define MPP_RB64(x)  (((RK_U64)((const RK_U8*)(x))[0] << 56) | \
                     ((RK_U64)((const RK_U8*)(x))[1] << 48) | \
                     ((RK_U64)((const RK_U8*)(x))[2] << 40) | \
                     ((RK_U64)((const RK_U8*)(x))[3] << 32) | \
                     ((RK_U64)((const RK_U8*)(x))[4] << 24) | \
                     ((RK_U64)((const RK_U8*)(x))[5] << 16) | \
                     ((RK_U64)((const RK_U8*)(x))[6] <<  8) | \
                     (RK_U64)((const RK_U8*)(x))[7])
#define MPP_WB64(p, d) do { \
        ((RK_U8*)(p))[7] = (d);     \
        ((RK_U8*)(p))[6] = (d)>>8;  \
        ((RK_U8*)(p))[5] = (d)>>16; \
        ((RK_U8*)(p))[4] = (d)>>24; \
        ((RK_U8*)(p))[3] = (d)>>32; \
        ((RK_U8*)(p))[2] = (d)>>40; \
        ((RK_U8*)(p))[1] = (d)>>48; \
        ((RK_U8*)(p))[0] = (d)>>56; } while(0)

#define MPP_RL64(x)  (((RK_U64)((const RK_U8*)(x))[7] << 56) | \
                     ((RK_U64)((const RK_U8*)(x))[6] << 48) | \
                     ((RK_U64)((const RK_U8*)(x))[5] << 40) | \
                     ((RK_U64)((const RK_U8*)(x))[4] << 32) | \
                     ((RK_U64)((const RK_U8*)(x))[3] << 24) | \
                     ((RK_U64)((const RK_U8*)(x))[2] << 16) | \
                     ((RK_U64)((const RK_U8*)(x))[1] <<  8) | \
                     (RK_U64)((const RK_U8*)(x))[0])
#define MPP_WL64(p, d) do { \
        ((RK_U8*)(p))[0] = (d);     \
        ((RK_U8*)(p))[1] = (d)>>8;  \
        ((RK_U8*)(p))[2] = (d)>>16; \
        ((RK_U8*)(p))[3] = (d)>>24; \
        ((RK_U8*)(p))[4] = (d)>>32; \
        ((RK_U8*)(p))[5] = (d)>>40; \
        ((RK_U8*)(p))[6] = (d)>>48; \
        ((RK_U8*)(p))[7] = (d)>>56; } while(0)

#define MPP_RB24(x)  ((((const RK_U8*)(x))[0] << 16) | \
                     (((const RK_U8*)(x))[1] <<  8) | \
                     ((const RK_U8*)(x))[2])
#define MPP_WB24(p, d) do { \
        ((RK_U8*)(p))[2] = (d); \
        ((RK_U8*)(p))[1] = (d)>>8; \
        ((RK_U8*)(p))[0] = (d)>>16; } while(0)

#define MPP_RL24(x)  ((((const RK_U8*)(x))[2] << 16) | \
                     (((const RK_U8*)(x))[1] <<  8) | \
                     ((const RK_U8*)(x))[0])

#define MPP_WL24(p, d) do { \
        ((RK_U8*)(p))[0] = (d); \
        ((RK_U8*)(p))[1] = (d)>>8; \
        ((RK_U8*)(p))[2] = (d)>>16; } while(0)

#include <stdio.h>
#if defined(_WIN32) && !defined(__MINGW32CE__)
#define snprintf                _snprintf
#define fseeko                  _fseeki64

#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#define chdir                   _chdir
#define mkdir                   _mkdir
#define access                  _access
#define off_t                   _off_t

#define R_OK                    4 /* Test for read permission. */
#define W_OK                    2 /* Test for write permission. */
#define X_OK                    1 /* Test for execute permission. */
#define F_OK                    0 /* Test for existence. */

#elif defined(__MINGW32CE__)
#define fseeko                  fseeko64
#else
#include <unistd.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#define mkdir(x)                mkdir(x, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#endif

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define __RETURN                __Return
#define __FAILED                __failed

#define ARG_T(t)                t
#define ARG_N(a,b,c,d,N,...)    N
#define ARG_N_HELPER(...)       ARG_T(ARG_N(__VA_ARGS__))
#define COUNT_ARG(...)          ARG_N_HELPER(__VA_ARGS__,4,3,2,1,0)

#define SZ_1K                   (1024)
#define SZ_2K                   (SZ_1K*2)
#define SZ_4K                   (SZ_1K*4)
#define SZ_8K                   (SZ_1K*8)
#define SZ_16K                  (SZ_1K*16)
#define SZ_32K                  (SZ_1K*32)
#define SZ_64K                  (SZ_1K*64)
#define SZ_128K                 (SZ_1K*128)
#define SZ_256K                 (SZ_1K*256)
#define SZ_512K                 (SZ_1K*512)
#define SZ_1M                   (SZ_1K*SZ_1K)
#define SZ_2M                   (SZ_1M*2)
#define SZ_4M                   (SZ_1M*4)
#define SZ_8M                   (SZ_1M*8)
#define SZ_16M                  (SZ_1M*16)
#define SZ_32M                  (SZ_1M*32)
#define SZ_64M                  (SZ_1M*64)
#define SZ_80M                  (SZ_1M*80)
#define SZ_128M                 (SZ_1M*128)

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpp_log2(RK_U32 v);
RK_S32 mpp_log2_16bit(RK_U32 v);

static __inline RK_S32 mpp_ceil_log2(RK_S32 x)
{
    return mpp_log2((x - 1) << 1);
}

static __inline RK_S32 mpp_clip(RK_S32 a, RK_S32 amin, RK_S32 amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}

static __inline RK_U32 mpp_is_32bit()
{
    return ((sizeof(void *) == 4) ? (1) : (0));
}

RK_S32 axb_div_c(RK_S32 a, RK_S32 b, RK_S32 c);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_COMMON_H__*/

