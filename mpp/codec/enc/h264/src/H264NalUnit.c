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
--  Abstract  :   NAL unit handling
--
--------------------------------------------------------------------------------
--
--
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "H264NalUnit.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    H264NalUnit

------------------------------------------------------------------------------*/
void H264NalUnitHdr(stream_s * stream, i32 nalRefIdc, nalUnitType_e nalUnitType,
                    true_e byteStream)
{
    if (byteStream == ENCHW_YES) {
        H264PutBits(stream, 0, 8);
        COMMENT("BYTE STREAM: leadin_zero_8bits");

        H264PutBits(stream, 0, 8);
        COMMENT("BYTE STREAM: Start_code_prefix");

        H264PutBits(stream, 0, 8);
        COMMENT("BYTE STREAM: Start_code_prefix");

        H264PutBits(stream, 1, 8);
        COMMENT("BYTE STREAM: Start_code_prefix");
    }

    H264PutBits(stream, 0, 1);
    COMMENT("forbidden_zero_bit");

    H264PutBits(stream, nalRefIdc, 2);
    COMMENT("nal_ref_idc");

    H264PutBits(stream, (i32) nalUnitType, 5);
    COMMENT("nal_unit_type");

    stream->zeroBytes = 0; /* we start new counter for zero bytes */

    return;
}

/*------------------------------------------------------------------------------

    H264NalUnitTrailinBits

------------------------------------------------------------------------------*/
void H264NalUnitTrailinBits(stream_s * stream, true_e byteStream)
{
    H264RbspTrailingBits(stream);

    if (byteStream == ENCHW_YES) {
#if 0   /* system model has removed this */
        H264PutBits(stream, 0, 8);
        COMMENT("BYTE STREAM: trailing_zero_8bits");
#endif
    }

    return;
}

u32 H264FillerNALU(stream_s * sp, i32 cnt, true_e byteStream)
{
    i32 i = cnt;
    u32 nal_size;

    nal_size = sp->byteCnt;

    ASSERT(sp != NULL);

    H264NalUnitHdr(sp, 0, FILLERDATA, byteStream);

    for (; i > 0; i--) {
        H264NalBits(sp, 0xFF, 8);
        COMMENT("filler ff_byte");
    }
    H264RbspTrailingBits(sp);

    nal_size = sp->byteCnt - nal_size;

    return nal_size;
}
