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

#ifndef __PARSER_API_H__
#define __PARSER_API_H__

#include "mpp_packet.h"
#include "mpp_buf_slot.h"
#include "hal_task.h"

/*
 * slots    - all decoder need a slots interface to sync its internal dpb management
 *            with buffer group in mpp_dec
 *
 * the reset wait for extension
 */
typedef struct DecParserInitCfg_t {
    // input
    MppCodingType   coding;
    MppBufSlots     frame_slots;
    MppBufSlots     packet_slots;

    // output
    RK_S32          task_count;
    RK_U32          need_split;
    RK_U32          internal_pts;
} ParserCfg;


/*
 * Parser is the data structure provided from different decoders
 *
 * They will be static register to mpp_dec for scaning
 * name     - decoder name
 * coding   - decoder coding type
 * ctx_size - decoder context size, mpp_dec will use this to malloc memory
 * flag     - reserve
 *
 * init     - decoder initialization function
 * deinit   - decoder de-initialization function
 * parse    - decoder main working function, mpp_dec will input packet and get output syntax
 * reset    - decoder reset function
 * flush    - decoder output all frames
 * control  - decoder configure function
 */
typedef struct ParserApi_t {
    char            *name;
    MppCodingType   coding;
    RK_U32          ctx_size;
    RK_U32          flag;

    MPP_RET (*init)(void *ctx, ParserCfg *cfg);
    MPP_RET (*deinit)(void *ctx);

    MPP_RET (*prepare)(void *ctx, MppPacket pkt, HalDecTask *task);
    MPP_RET (*parse)(void *ctx, HalDecTask *task);

    MPP_RET (*reset)(void *ctx);
    MPP_RET (*flush)(void *ctx);
    MPP_RET (*control)(void *ctx, RK_S32 cmd, void *param);
    MPP_RET (*callback)(void *ctx, void *err_info);
} ParserApi;


#endif /*__PARSER_API_H__*/
