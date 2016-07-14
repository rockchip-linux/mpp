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

#ifndef ENC_GET_OPTION
#define ENC_GET_OPTION

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"  // mask by lance 2016.05.05

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
    char *longOpt;
    char shortOpt;
    i32 enableArg;
} option_s;

typedef struct {
    i32 optCnt;
    char *optArg;
    char shortOpt;
    i32 enableArg;
} argument_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncGetOption(i32 argc, char **argv, option_s *, argument_s *);

#endif
