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

#ifndef __H264_PUT_BITS_H__
#define __H264_PUT_BITS_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#if 1
#define H264NalBits(stream, val, num) H264PutNalBits(stream, val, num)
#else
#define H264NalBits(stream, val, num) \
    if (stream->bufferedBits + num < 8) { \
        i32 bits = stream->bufferedBits + (num); \
        stream->bufferedBits += (num); \
        stream->byteBuffer = stream->byteBuffer | ((u32)(val) << (32-bits)); \
    } else { \
        H264PutNalBits(stream, val, num); \
    }
#endif

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
bool_e H264SetBuffer(stream_s * buffer, u8 * stream, i32 size);
void H264PutBits(stream_s *, i32, i32);
void H264PutNalBits(stream_s *, i32, i32);
void H264ExpGolombUnsigned(stream_s * stream, u32 val);
void H264ExpGolombSigned(stream_s * stream, i32 val);
void H264RbspTrailingBits(stream_s * stream);
void H264Comment(char *comment);

#endif
