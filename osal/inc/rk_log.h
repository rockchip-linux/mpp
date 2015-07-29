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

/*
 * C log functions
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_TAG
#define LOG_TAG                     "rk_log"
#endif

void rk_set_log_flag(RK_U32 flag);
RK_U32 rk_get_log_flag();

#define rk_log(fmt, ...) _rk_log(LOG_TAG, fmt, ## __VA_ARGS__)
#define rk_err(fmt, ...) _rk_err(LOG_TAG, fmt, ## __VA_ARGS__)

#define rk_dbg(debug, flag, fmt, ...)   \
    do {                                            \
        if (debug & flag) {                         \
            _rk_log(LOG_TAG, fmt, ## __VA_ARGS__);  \
        }                                           \
    } while(0)

/*
 * Send the specified message to the log
 * _rk_log : general log function, send log to stdout
 * _rk_err : log function for error information, send log to stderr
 */
void _rk_log(const char *tag, const char *fmt, ...);
void _rk_err(const char *tag, const char *fmt, ...);

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
 * dynamic debug function
 * rk_dbg_add_flag  : add a new debug flag associated with module name
 * rk_dbg_set_flag  : set a existing debug flag associated with module name
 * rk_dbg_show_flag : show all existing debug flags
 */

//void rk_dbg(RK_U32 debug, RK_U32 flag, const char *tag, const char *fmt, ...);

/*
 * submodules suggest to use macro as below:
 * #define h264d_dbg(flag, const char *fmt, ...) \
 *     rk_dbg(h264d_debug, flag, fmt, ## __VA_ARGS__)
 */

#ifdef __cplusplus
}
#endif


#endif /*__RK_LOG_H__*/
