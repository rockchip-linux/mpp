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
--  Description : Preprocessor setup
--
--------------------------------------------------------------------------------
--
--
--
------------------------------------------------------------------------------*/
#ifndef __ENC_PRE_PROCESS_H__
#define __ENC_PRE_PROCESS_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define    ROTATE_0     0U
#define    ROTATE_90R   1U  /* Rotate 90 degrees clockwise */
#define    ROTATE_90L   2U  /* Rotate 90 degrees counter-clockwise */

/* maximum input picture width set by the available bits in ASIC regs */
#define    MAX_INPUT_IMAGE_WIDTH   (8192)

typedef struct {
    u32 lumWidthSrc;    /* Source input image width */
    u32 lumHeightSrc;   /* Source input image height */
    u32 lumWidth;   /* Encoded image width */
    u32 lumHeight;  /* Encoded image height */
    u32 horOffsetSrc;   /* Encoded frame offset, reference is ... */
    u32 verOffsetSrc;   /* ...top  left corner of source image */
    u32 inputFormat;
    u32 rotation;
    u32 videoStab;
    u32 colorConversionType;    /* 0 = bt601, 1 = bt709, 2 = user defined */
    u32 colorConversionCoeffA;
    u32 colorConversionCoeffB;
    u32 colorConversionCoeffC;
    u32 colorConversionCoeffE;
    u32 colorConversionCoeffF;
} preProcess_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncPreProcessCheck(const preProcess_s * preProcess);
void EncPreProcess(asicData_s * asic, const preProcess_s * preProcess);
void EncSetColorConversion(preProcess_s * preProcess, asicData_s * asic);

#endif
