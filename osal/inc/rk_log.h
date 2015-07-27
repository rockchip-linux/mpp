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

#ifndef __RK_LOG_H__
#define __RK_LOG_H__

#include "rk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * STATIC_LOG_LEVE is for
 */
#define STATIC_LOG_LEVE             (0xffffffff)

#ifdef  STATIC_LOG_LEVE
#define rk_debug                    STATIC_LOG_LEVE
#else
extern  RK_U32 rk_debug;
#endif

void rk_set_log_flag(RK_U32 flag);
RK_U32 rk_get_log_flag();

/*
 * Send the specified message to the log
 * rk_log : general log function
 * rk_err : log function for error information
 */
void rk_log(const char *fmt, ...);
void rk_err(const char *fmt, ...);

/*
 * debug flag usage:
 * +------+-------------------+
 * | 8bit |      24bit        |
 * +------+-------------------+
 *  0~15 bit: software debug print
 * 16~23 bit: hardware debug print
 * 24~31 bit: information print format
 */
/*
 *  0~ 3 bit:
 */
#define rk_dbg(flag, debug, fmt, ...) \
    do { \
        if (debug & flag) { \
            rk_log(fmt, ## __VA_ARGS__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif


#endif /*__RK_LOG_H__*/
