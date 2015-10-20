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
#include "hal_task.h"
#include "hal_h264d_api.h"


static MPP_RET manual_set_env(void)
{
#if defined(_MSC_VER)
    mpp_env_set_u32("h264d_log_help",     1             );
    mpp_env_set_u32("h264d_log_show",     1             );
    mpp_env_set_u32("h264d_log_ctrl",     0x900B        );
    mpp_env_set_u32("h264d_log_level",    5             );
    mpp_env_set_u32("h264d_log_decframe", 0             );
    mpp_env_set_u32("h264d_log_begframe", 0             );
    mpp_env_set_u32("h264d_log_endframe", 0             );
    mpp_env_set_str("h264d_log_outpath",  "F:/h264_log" );
    mpp_env_set_str("h264d_log_cmppath",  "F:/h264_log_driver/trunk_dat" );
#endif
    return MPP_OK;
}

static MPP_RET decoder_deinit(MppDec *pApi)
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

static MPP_RET decoder_init(MppDec *pApi)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    ParserCfg       parser_cfg;
    MppHalCfg       hal_cfg;

    pApi->coding = MPP_VIDEO_CodingAVC;
    // malloc slot
    FUN_CHECK(ret = mpp_buf_slot_init(&pApi->frame_slots));
    MEM_CHECK(ret, pApi->frame_slots);
    // init parser part
    memset(&parser_cfg, 0, sizeof(parser_cfg));
    parser_cfg.coding = pApi->coding;
    parser_cfg.frame_slots  = pApi->frame_slots;
    parser_cfg.packet_slots = pApi->packet_slots;
    FUN_CHECK(ret = parser_init(&pApi->parser, &parser_cfg));

    // init hal part
    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.type = MPP_CTX_DEC;
    hal_cfg.coding = pApi->coding;
    hal_cfg.work_mode = HAL_MODE_LIBVPU;
    hal_cfg.device_id = HAL_VDPU;
    hal_cfg.frame_slots = pApi->frame_slots;
    hal_cfg.packet_slots = pApi->packet_slots;
    hal_cfg.task_count = parser_cfg.task_count;
    FUN_CHECK(ret = mpp_hal_init(&pApi->hal, &hal_cfg));
    pApi->tasks = hal_cfg.tasks;

    return MPP_OK;
__FAILED:
    decoder_deinit(pApi);

    return ret;
}



int main(int argc, char **argv)
{
    //FILE *g_debug_file = NULL;
    //RK_S32 m_max_bytes = 0;
    //RK_S32 m_max_slice_data = 0;

    MPP_RET        ret = MPP_ERR_UNKNOW;
    MppPacket      pkt = NULL;
    InputParams   *pIn = mpp_calloc(InputParams, 1);
    MppDec       *pApi = mpp_calloc(MppDec, 1);
    HalTaskInfo  *task = mpp_calloc_size(HalTaskInfo, sizeof(HalTaskInfo));
    RK_U32 end_of_flag = 0;

    //if (g_debug_file0 == NULL) {
    //    g_debug_file0 = fopen("rk_debugfile_view0.txt", "wb");
    //}

    if (g_debug_file1 == NULL) {
        g_debug_file1 = fopen("rk_debugfile_view1.txt", "wb");
    }


    MEM_CHECK(ret, pIn && pApi && task);
    mpp_log("== test start == \n");
    // set debug mode
    FUN_CHECK(ret = manual_set_env());
    // input paramters configure
    FUN_CHECK(ret = h264d_configure(pIn, argc, argv));
    // open files
    FUN_CHECK(ret = h264d_open_files(pIn));
    FUN_CHECK(ret = h264d_alloc_frame_buffer(pIn));

    //!< init decoder
    FUN_CHECK(ret = decoder_init(pApi));
    do {
        //!< get one packet
        if (pkt == NULL) {
            if (pIn->is_eof) {
                mpp_packet_init(&pkt, NULL, 0);
                mpp_packet_set_eos(pkt);
            } else {
                FUN_CHECK(ret = h264d_read_one_frame(pIn));
                mpp_packet_init(&pkt, pIn->strm.pbuf, pIn->strm.strmbytes);
            }
            mpp_log("---- decoder, read_one_frame Frame_no = %d \n", pIn->iFrmdecoded++);
        }
        //!< initial task
        memset(task, 0, sizeof(HalTaskInfo));
        memset(task->dec.refer, -1, sizeof(task->dec.refer));
        //!< prepare
        FUN_CHECK(ret = parser_prepare(pApi->parser, pkt, &task->dec));
        //!< parse
        if (task->dec.valid) {
            task->dec.valid = 0;
            FUN_CHECK(ret = parser_parse(pApi->parser, &task->dec));
        }
        //!< deinit packet
        if (mpp_packet_get_length(pkt) == 0) {
            if (mpp_packet_get_eos(pkt)) {
                end_of_flag = 1; //!< end of stream
            }
            mpp_packet_deinit(&pkt);
            pkt = NULL;
        }
        //!< run hal module
        if (task->dec.valid) {
            FUN_CHECK(ret = mpp_hal_reg_gen(pApi->hal, task));
            FUN_CHECK(ret = mpp_hal_hw_start(pApi->hal, task));
            FUN_CHECK(ret = mpp_hal_hw_wait(pApi->hal, task));
            //!< write frame out
        }
        if (end_of_flag) {
            break;
        }
    } while (!pIn->iDecFrmNum || (pIn->iFrmdecoded < pIn->iDecFrmNum));
    //!< flush dpb and send to display
    FUN_CHECK(ret = mpp_dec_flush(pApi));
    mpp_log("+++++++ all test return +++++++ \n");
    ret = MPP_OK;
__FAILED:
    decoder_deinit(pApi);
    h264d_free_frame_buffer(pIn);
    h264d_write_fpga_data(pIn); //!< for fpga debug
    h264d_close_files(pIn);
    MPP_FREE(pIn);
    MPP_FREE(pApi);
    MPP_FREE(task);

    //g_debug_file = fopen("rk_debugfile_view0.txt", "rb");
    //if(g_debug_file)
    //  fscanf(g_debug_file, "(max_bytes)%d, (max_slice_data)%d \n", &m_max_bytes, &m_max_slice_data);
    //FCLOSE(g_debug_file);
    //if (m_max_bytes < g_max_bytes)
    //{
    //  m_max_bytes = g_max_bytes;
    //}
    //if (m_max_slice_data < g_max_slice_data)
    //{
    //  m_max_slice_data = g_max_slice_data;
    //}
    //g_debug_file = fopen("rk_debugfile_view0.txt", "ab+");
    //if(g_debug_file)
    //  fprintf(g_debug_file, "(max_bytes)%d, (max_slice_data)%d \n", g_max_bytes, g_max_slice_data);

    //FCLOSE(g_debug_file);

    return ret;
}

