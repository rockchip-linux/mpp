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

#define MODULE_TAG "avsd_test"


#if defined(_WIN32)
#include "vld.h"
#endif
#include <string.h>
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_dec.h"
#include "mpp_hal.h"
#include "mpp_mem.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_hal.h"

#include "avsd_api.h"
#include "hal_avsd_api.h"

#if 1

//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
if (!(val)) {   \
        ret = MPP_ERR_MALLOC; \
        mpp_log("Function:%s:%d, ERROR: malloc buffer.\n", __FUNCTION__, __LINE__); \
        mpp_assert(0); goto __FAILED; \
}} while (0)
//!< function return check
#define FUN_CHECK(val)\
do{\
if ((val) < 0) { \
        goto __FAILED; \
}} while (0)


static MPP_RET avs_decoder_deinit(MppDec *pApi)
{
    if (pApi->frame_slots) {
        mpp_buf_slot_deinit(pApi->frame_slots);
        pApi->frame_slots = NULL;
    }
    if (pApi->parser) {
        parser_deinit(pApi->parser);
        pApi->parser = NULL;
    }
    if (pApi->hal) {
        mpp_hal_deinit(pApi->hal);
        pApi->hal = NULL;
    }

    return MPP_OK;
}

static MPP_RET avs_decoder_init(MppDec *pApi)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    ParserCfg       parser_cfg;
    MppHalCfg       hal_cfg;

    pApi->coding = MPP_VIDEO_CodingAVS;
    // malloc slot
    FUN_CHECK(ret = mpp_buf_slot_init(&pApi->frame_slots));
    MEM_CHECK(ret, pApi->frame_slots);
    // init parser part
    memset(&parser_cfg, 0, sizeof(parser_cfg));
    parser_cfg.coding = pApi->coding;
    parser_cfg.frame_slots = pApi->frame_slots;
    parser_cfg.packet_slots = pApi->packet_slots;
    FUN_CHECK(ret = parser_init(&pApi->parser, &parser_cfg));

    // init hal part
    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.type = MPP_CTX_DEC;
    hal_cfg.coding = pApi->coding;
    hal_cfg.work_mode = HAL_MODE_LIBVPU;
    hal_cfg.device_id = HAL_RKVDEC;
    hal_cfg.frame_slots = pApi->frame_slots;
    hal_cfg.packet_slots = pApi->packet_slots;
    hal_cfg.task_count = parser_cfg.task_count;
    FUN_CHECK(ret = mpp_hal_init(&pApi->hal, &hal_cfg));
    pApi->tasks = hal_cfg.tasks;

    return MPP_OK;
__FAILED:
    avs_decoder_deinit(pApi);

    return ret;
}

int main(int argc, char **argv)
{
    MPP_RET        ret = MPP_ERR_UNKNOW;
    MppPacket      pkt = NULL;
    MppDec       *pApi = mpp_calloc(MppDec, 1);
    HalTaskInfo  *task = mpp_calloc_size(HalTaskInfo, sizeof(HalTaskInfo));
    mpp_log("+++++++ all test begin +++++++ \n");
    //!< init decoder
    FUN_CHECK(ret = avs_decoder_init(pApi));
    //!< initial packet
    mpp_packet_init(&pkt, NULL, 0);
    //!< initial task
    memset(task, 0, sizeof(HalTaskInfo));
    memset(task->dec.refer, -1, sizeof(task->dec.refer));
    //!< prepare
    FUN_CHECK(ret = parser_prepare(pApi->parser, pkt, &task->dec));
    //!< free packet
    mpp_packet_deinit(&pkt);
    //!< parse
    FUN_CHECK(ret = parser_parse(pApi->parser, &task->dec));
    //!< hal module
    FUN_CHECK(ret = mpp_hal_reg_gen(pApi->hal, task));
    FUN_CHECK(ret = mpp_hal_hw_start(pApi->hal, task));
    FUN_CHECK(ret = mpp_hal_hw_wait(pApi->hal, task));

    (void)argv;
    (void)argc;

    mpp_log("+++++++ all test end +++++++ \n");
    ret = MPP_OK;
__FAILED:
    avs_decoder_deinit(pApi);
    MPP_FREE(pApi);
    MPP_FREE(task);

    return ret;
}

#else


int main(int argc, char **argv)
{
	return avsd_test_main(argc, argv);
}

#endif