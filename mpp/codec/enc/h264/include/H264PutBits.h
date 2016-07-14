/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Rockchip Products .                             --
--                                                                            --
--                   (C) COPYRIGHT 2014 ROCKCHIP PRODUCTS                     --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--
--------------------------------------------------------------------------------
--
--
--
------------------------------------------------------------------------------*/

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
