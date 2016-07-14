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

#ifndef __H264_NAL_UNIT_H__
#define __H264_NAL_UNIT_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "H264PutBits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void H264NalUnitHdr(stream_s * stream, i32 nalRefIdc, nalUnitType_e
                    nalUnitType, true_e byteStream);
void H264NalUnitTrailinBits(stream_s * stream, true_e byteStream);
u32 H264FillerNALU(stream_s * sp, i32 cnt, true_e byteStream);

#endif
