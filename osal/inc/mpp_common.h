/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define MPP_MAX(a, b)           ((a) > (b) ? (a) : (b))
#define MPP_MAX3(a, b, c)       MPP_MAX(MPP_MAX(a,b),c)
#define MPP_MAX4(a, b, c, d)    MPP_MAX((a), MPP_MAX3((b), (c), (d)))

#define MPP_MIN(a,b)            ((a) > (b) ? (b) : (a))
#define MPP_MIN3(a,b,c)         MPP_MIN(MPP_MIN(a,b),c)
#define MPP_MIN4(a, b, c, d)    MPP_MIN((a), MPP_MIN3((b), (c), (d)))

#define MPP_SWAP(type, a, b)    do {type SWAP_tmp = b; b = a; a = SWAP_tmp;} while(0)
#define MPP_ARRAY_ELEMS(a)      (sizeof(a) / sizeof((a)[0]))
#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))
#define MPP_VSWAP(a, b)         { a ^= b; b ^= a; a ^= b; }


#include <stdio.h>
#if defined(_WIN32) && !defined(__MINGW32CE__)
#define snprintf    _snprintf
#define fseeko      _fseeki64

#include <direct.h>
#include <io.h>
#define chdir       _chdir
#define mkdir       _mkdir
#define access      _access
#elif defined(__MINGW32CE__)
#define fseeko      fseeko64
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#define mkdir(x)    mkdir(x, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#endif

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

#ifdef __cplusplus
}
#endif

#endif /*__MPP_COMMON_H__*/

