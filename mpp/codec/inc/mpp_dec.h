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

#ifndef __MPP_DEC_H__
#define __MPP_DEC_H__

#include "mpp_parser.h"
#include "mpp_hal.h"

typedef struct MppDec_t MppDec;

struct MppDec_t {
    MppCodingType       coding;

    Parser              parser;
    MppHal              hal;

    // common resource
    MppBufSlots         frame_slots;
    MppBufSlots         packet_slots;
    HalTaskGroup        tasks;
    RK_U32              reset_flag;
};


#ifdef __cplusplus
extern "C" {
#endif

/*
 * main thread for all decoder. This thread will connect parser / hal / mpp
 */
void *mpp_dec_parser_thread(void *data);
void *mpp_dec_hal_thread(void *data);

/*
 *
 */
MPP_RET mpp_dec_init(MppDec **dec, MppCodingType coding);
MPP_RET mpp_dec_deinit(MppDec *dec);

MPP_RET mpp_dec_reset(MppDec *dec);
MPP_RET mpp_dec_flush(MppDec *dec);
MPP_RET mpp_dec_control(MppDec *dec, MpiCmd cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_DEC_H__*/
