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

#ifndef __H264_CODE_FRAME_H__
#define __H264_CODE_FRAME_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "H264Instance.h"
#include "H264Slice.h"
#include "H264RateControl.h"
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

typedef enum {
    H264ENCODE_OK = 0,
    H264ENCODE_TIMEOUT = 1,
    H264ENCODE_DATA_ERROR = 2,
    H264ENCODE_HW_ERROR = 3,
    H264ENCODE_SYSTEM_ERROR = 4,
    H264ENCODE_HW_RESET = 5
} h264EncodeFrame_e;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void H264CodeFrame(h264Instance_s * inst, h264e_syntax *syntax_data);

#endif
