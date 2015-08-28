/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __MPP_LOG_H__
#define __MPP_LOG_H__

#include "rk_type.h"
#include <stdlib.h>

/*
 * mpp runtime log system usage:
 * mpp_err is for error status message, it will print for sure.
 * mpp_log is for important message like open/close/reset/flush, it will print too.
 * mpp_dbg is for all optional message. it can be controlled by debug and flag.
 */

#define mpp_log(fmt, ...) _mpp_log(MODULE_TAG, fmt, ## __VA_ARGS__)
#define mpp_err(fmt, ...) _mpp_err(MODULE_TAG, fmt, ## __VA_ARGS__)

#define _mpp_dbg(debug, flag, fmt, ...) \
             __mpp_dbg(debug, flag, MODULE_TAG, fmt, ## __VA_ARGS__)

/*
 * mpp_dbg usage:
 *
 * in h264d module define module debug flag variable like: h265d_debug
 * then define h265d_dbg macro as follow :
 *
 * extern RK_U32 h265d_debug;
 *
 * #define H265D_DBG_FUNCTION          (0x00000001)
 * #define H265D_DBG_VPS               (0x00000002)
 * #define H265D_DBG_SPS               (0x00000004)
 * #define H265D_DBG_PPS               (0x00000008)
 * #define H265D_DBG_SLICE_HDR         (0x00000010)
 *
 * #define h265d_dbg(flag, fmt, ...) mpp_dbg(h265d_debug, flag, fmt, ## __VA_ARGS__)
 *
 * finally use environment control the debug flag
 *
 * mpp_get_env_u32("h264d_debug", &h265d_debug, 0)
 *
 */
/*
 * sub-module debug flag usage example:
 * +------+-------------------+
 * | 8bit |      24bit        |
 * +------+-------------------+
 *  0~15 bit: software debug print
 * 16~23 bit: hardware debug print
 * 24~31 bit: information print format
 */

#define MPP_STRINGS(x)      MPP_TO_STRING(x)
#define MPP_TO_STRING(x)    #x

#define mpp_assert(cond) do {                                           \
    if (!(cond)) {                                                      \
        mpp_err("Assertion %s failed at %s:%d\n",                       \
               MPP_STRINGS(cond), __FILE__, __LINE__);                  \
        abort();                                                        \
    }                                                                   \
} while (0)


#ifdef __cplusplus
extern "C" {
#endif

void mpp_log_set_flag(RK_U32 flag);
RK_U32 mpp_log_get_flag();

void _mpp_log(const char *tag, const char *fmt, ...);
void _mpp_err(const char *tag, const char *fmt, ...);
void __mpp_dbg(RK_U32 debug, RK_U32 flag, const char *tag, const char *fmt, ...);

#ifdef __cplusplus
}
#endif


#endif /*__MPP_LOG_H__*/
