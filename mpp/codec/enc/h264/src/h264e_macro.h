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

#ifndef _H264E_MACRO_H_
#define _H264E_MACRO_H_

//#define SYNTAX_DATA_IN_FILE 1

#define H264E_CONTROL_LOG_MODE              0x00001111

#define H264E_CONTROL_LOG_ERROR             0x00000001
#define H264E_CONTROL_LOG_ASSERT            0x00000010
#define H264E_CONTROL_LOG_WARNNING          0x00000100
#define H264E_CONTROL_LOG_FLOW              0x00001000
#define H264E_CONTROL_LOG_DETAIL            0x00010000
#define H264E_CONTROL_LOG_FILE              0x00100000


#define h264e_control_debug_enter() \
    do {\
        if (H264E_CONTROL_LOG_MODE & H264E_CONTROL_LOG_FLOW)\
            { mpp_log("line(%d), func(%s), enter", __LINE__, __FUNCTION__); }\
    } while (0)

#define h264e_control_debug_leave() \
    do {\
        if (H264E_CONTROL_LOG_MODE & H264E_CONTROL_LOG_FLOW)\
            { mpp_log("line(%d), func(%s), leave", __LINE__, __FUNCTION__); }\
    } while (0)


#endif
