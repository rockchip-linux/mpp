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
--  Description : MAD threshold calculation
--
--------------------------------------------------------------------------------
--
--
--
------------------------------------------------------------------------------*/

#ifndef H264_MAD_H
#define H264_MAD_H

#include "enccommon.h"

#define DSCY                      32 /* n * 32 */
#define I32_MAX           2147483647 /* 2 ^ 31 - 1 */
#define DIV(a, b)       (((a) + (SIGN(a) * (b)) / 2) / (b))
#define MAD_TABLE_LEN              5

typedef struct {
    i32  a1;               /* model parameter, y = a1*x + a2 */
    i32  a2;               /* model parameter */
    i32  th[MAD_TABLE_LEN];     /* mad threshold */
    i32  count[MAD_TABLE_LEN];  /* number of macroblocks under threshold */
    i32  pos;              /* current position */
    i32  len;              /* current lenght */
    i32  threshold;        /* current frame threshold */
    i32  mbPerFrame;       /* number of macroblocks per frame */
} madTable_s;

/*------------------------------------------------------------------------------
    Function prototypes
------------------------------------------------------------------------------*/

void H264MadInit(madTable_s *mad, u32 mbPerFrame);

void H264MadThreshold(madTable_s *madTable, u32 madCount);

#endif

