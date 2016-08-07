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
void H264NalUnitHdr(stream_s * stream, RK_S32 nalRefIdc, nalUnitType_e nalUnitType,
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

    H264PutBits(stream, (RK_S32) nalUnitType, 5);
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

RK_U32 H264FillerNALU(stream_s * sp, RK_S32 cnt, true_e byteStream)
{
    RK_S32 i = cnt;
    RK_U32 nal_size;

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
