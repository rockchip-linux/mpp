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

#define MODULE_TAG "h264d_test"

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

#include "h264d_log.h"
#include "h264d_rwfile.h"
#include "h264d_api.h"
#include "hal_h264d_api.h"


#define MODULE_TAG "h264d_test"


static MPP_RET manual_set_env(void)
{
#if defined(_MSC_VER)
    mpp_env_set_u32("h264d_log_help",     1             );
    mpp_env_set_u32("h264d_log_show",     1             );
    mpp_env_set_u32("h264d_log_ctrl",     0x007B        );
    mpp_env_set_u32("h264d_log_level",    5             );
    mpp_env_set_u32("h264d_log_decframe", 0             );
    mpp_env_set_u32("h264d_log_begframe", 0             );
    mpp_env_set_u32("h264d_log_endframe", 0             );
    mpp_env_set_str("h264d_log_outpath",  "F:/h264_log" );
#endif
    return MPP_OK;
}

static MPP_RET decoder_deinit(MppDec *pApi)
{

    if (pApi->slots) {
        mpp_buf_slot_deinit(pApi->slots);
        pApi->slots = NULL;
    }
    if (pApi->parser_api && pApi->parser_ctx) {
        pApi->parser_api->deinit(pApi->parser_ctx);
    }
    if (pApi->hal_ctx) {
        mpp_hal_deinit(pApi->hal_ctx);
        pApi->hal_ctx = NULL;
    }
    mpp_free(pApi->parser_ctx);

    return MPP_OK;
}

static MPP_RET decoder_init(MppDec *pApi)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    MppParserInitCfg  parser_cfg;
    MppHalCfg         hal_cfg;
    // set decoder
    pApi->parser_api = &api_h264d_parser;
    pApi->parser_ctx = mpp_calloc_size(void, pApi->parser_api->ctx_size);
    MEM_CHECK(ret, pApi->parser_ctx);
    // malloc slot
    FUN_CHECK(ret = mpp_buf_slot_init(&pApi->slots));
    MEM_CHECK(ret, pApi->slots);
    // init parser part
    memset(&parser_cfg, 0, sizeof(parser_cfg));
    parser_cfg.slots = pApi->slots;
    FUN_CHECK(ret = pApi->parser_api->init(pApi->parser_ctx, &parser_cfg));
    // init hal part
    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.type = MPP_CTX_DEC;
    hal_cfg.coding = pApi->parser_api->coding;
    hal_cfg.task_count = parser_cfg.task_count;
    FUN_CHECK(ret = mpp_hal_init(&pApi->hal_ctx, &hal_cfg));
    pApi->tasks = hal_cfg.tasks;

    return MPP_OK;
__FAILED:
    decoder_deinit(pApi);

    return ret;
}



int main(int argc, char **argv)
{
    MPP_RET        ret = MPP_ERR_UNKNOW;

    InputParams   *pIn = mpp_calloc(InputParams, 1);
    MppDec       *pApi = mpp_calloc(MppDec, 1);
    MppPacketImpl *pkt = mpp_calloc_size(MppPacketImpl, sizeof(MppPacketImpl));
    HalTask      *task = mpp_calloc_size(HalTask, sizeof(HalTask));
    MEM_CHECK(ret, pIn && pApi && pkt && task);
    mpp_log("== test start == \n");
    // set debug mode
    FUN_CHECK(ret = manual_set_env());
    // input paramters configure
    FUN_CHECK(ret = h264d_configure(pIn, argc, argv));
    // open files
    FUN_CHECK(ret = h264d_open_files(pIn));
    FUN_CHECK(ret = h264d_alloc_frame_buffer(pIn));
    // init
    FUN_CHECK(ret = decoder_init(pApi));
    do {
        if (!pkt->size) {
            FUN_CHECK(ret = h264d_read_one_frame(pIn, (MppPacket)pkt));
        }
        FUN_CHECK(ret = pApi->parser_api->parse(pApi->parser_ctx, pkt, &task->dec));
        if (((HalDecTask*)&task->dec)->valid) {
            FUN_CHECK(ret = mpp_hal_reg_gen(pApi->hal_ctx, task));
            //mpp_log("---- decoder, Frame_no = %d \n", pIn->iFrmdecoded);
            //!< end of stream
            if (!pkt->size && (pkt->flag & MPP_PACKET_FLAG_EOS)) {
                break;
            }
            pIn->iFrmdecoded++;
        }
    } while (!pIn->iDecFrmNum || (pIn->iFrmdecoded < pIn->iDecFrmNum));

    mpp_log("+++++++ all test return +++++++ \n");
    ret = MPP_OK;
__FAILED:
    decoder_deinit(pApi);
    h264d_free_frame_buffer(pIn);
    h264d_write_fpga_data(pIn); //!< for fpga debug
    h264d_close_files(pIn);
    mpp_free(pIn);
    mpp_free(pApi);
    mpp_free(pkt);
    mpp_free(task);

    return ret;
}

