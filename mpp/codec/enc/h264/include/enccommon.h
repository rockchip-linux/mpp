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
    1. External compiler flags
------------------------------------------------------------------------------*/

/* Encoder global definitions
 *
 * _ASSERT_USED     # Asserts enabled
 * _DEBUG_PRINT     # Prints debug information on stdout
 * TRACE_STREAM     # Creates stream trace file
 * TEST_DATA        # Creates test data files
 * MPEG4_HW_VLC_MODE_ENABLED    # Control: MPEG-4 ASIC supports VLC mode
 * MPEG4_HW_RLC_MODE_ENABLED    # Control: MPEG-4 ASIC supports RLC mode
 * LOEFFLER_DCT                 # System: MPEG-4 DCT using SW algorithm
 * LOEFFLER_ASIC_DCT            # System: MPEG-4 DCT using ASIC algorithm
 * LOEFFLER_IDCT                # System: MPEG-4 IDCT using SW algorithm
 * LOEFFLER_ASIC_IDCT           # System: MPEG-4 IDCT using ASIC algorithm
 * SW_QUANT                     # System: MPEG-4 quantization using SW algorithm
 * FIXED_POINT_QUANT            # System: MPEG-4 quantization using ASIC algorithm
 *
 * Can be defined here or using compiler flags */

#define LOEFFLER_ASIC_DCT
#define LOEFFLER_ASIC_IDCT
#define FIXED_POINT_QUANT

/*------------------------------------------------------------------------------
    2. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "ewl.h"

/* Test data generation requires stream trace */
#ifdef TEST_DATA
#ifndef TRACE_STREAM
#define TRACE_STREAM
#endif
#endif

/* Stream tracing requires encdebug.h */
#ifdef TRACE_STREAM
#ifndef H8270_HAVE_ENCDEBUG_H
#define H8270_HAVE_ENCDEBUG_H
#endif
#endif

#ifdef H8270_HAVE_ENCDEBUG_H
#include "encdebug.h"
#else
#define ASSERT(expr)
#define DEBUG_PRINT(args)
#define COMMENT(x)
#define COMMENTMBTYPE(x,y)
#define TRACE_BIT_STREAM(v,n)
#endif

#ifdef H8270_HAVE_ENCTRACE_H
#include "enctrace.h"
#endif

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
    i32 value;  /* Value of bits  */
    i32 number; /* Number of bits */
} table_s;

/* used in stream buffer handling */
typedef struct {
    u8 *stream; /* Pointer to next byte of stream */
    u32 size;   /* Byte size of stream buffer */
    u32 byteCnt;    /* Byte counter */
    u32 bitCnt; /* Bit counter */
    u32 byteBuffer; /* Byte buffer */
    u32 bufferedBits;   /* Amount of bits in byte buffer, [0-7] */
    u32 zeroBytes;  /* Amount of consecutive zero bytes */
    i32 overflow;    /* This will signal a buffer overflow */
    u32 emulCnt; /* Counter for emulation_3_byte, needed in SEI */
    i32 *table;         /* Video packet or Gob sizes */
    i32 tableSize;      /* Size of above table */
    i32 tableCnt;       /* Table counter of above table */
} stream_s;

/* General tools */
#define ABS(x)          ((x) < (0) ? -(x) : (x))
#define MAX(a, b)       ((a) > (b) ?  (a) : (b))
#define MIN(a, b)       ((a) < (b) ?  (a) : (b))
#define SIGN(a)         ((a) < (0) ? (-1) : (1))

#endif
