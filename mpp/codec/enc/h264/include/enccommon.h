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

#ifndef __ENC_COMMON_H__
#define __ENC_COMMON_H__

/*------------------------------------------------------------------------------
    2. Include headers
------------------------------------------------------------------------------*/

#include "rk_type.h"
#include "ewl.h"

#define ASSERT(expr)
#define DEBUG_PRINT(args)
#define COMMENT(x)
#define TRACE_BIT_STREAM(v,n)

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

typedef enum {
    ENCHW_NOK = -1,
    ENCHW_OK = 0
} bool_e;

typedef enum {
    ENCHW_NO = 0,
    ENCHW_YES = 1
} true_e;

typedef enum {
    NONIDR = 1, /* Coded slice of a non-IDR picture */
    IDR = 5,    /* Coded slice of an IDR picture */
    SEI = 6,    /* SEI message */
    SPSET = 7,  /* Sequence parameter set */
    PPSET = 8,  /* Picture parameter set */
    ENDOFSEQUENCE = 10, /* End of sequence */
    ENDOFSTREAM = 11,   /* End of stream */
    FILLERDATA = 12 /* Filler data */
} nalUnitType_e;

/* VLC TABLE */
typedef struct {
    RK_S32 value;  /* Value of bits  */
    RK_S32 number; /* Number of bits */
} table_s;

/* used in stream buffer handling */
typedef struct {
    RK_U8 *stream; /* Pointer to next byte of stream */
    RK_U32 size;   /* Byte size of stream buffer */
    RK_U32 byteCnt;    /* Byte counter */
    RK_U32 bitCnt; /* Bit counter */
    RK_U32 byteBuffer; /* Byte buffer */
    RK_U32 bufferedBits;   /* Amount of bits in byte buffer, [0-7] */
    RK_U32 zeroBytes;  /* Amount of consecutive zero bytes */
    RK_S32 overflow;    /* This will signal a buffer overflow */
    RK_U32 emulCnt; /* Counter for emulation_3_byte, needed in SEI */
    RK_S32 *table;         /* Video packet or Gob sizes */
    RK_S32 tableSize;      /* Size of above table */
    RK_S32 tableCnt;       /* Table counter of above table */
} stream_s;

/* General tools */
#define ABS(x)          ((x) < (0) ? -(x) : (x))
#define MAX(a, b)       ((a) > (b) ?  (a) : (b))
#define MIN(a, b)       ((a) < (b) ?  (a) : (b))
#define SIGN(a)         ((a) < (0) ? (-1) : (1))

#endif
