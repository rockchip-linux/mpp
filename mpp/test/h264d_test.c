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

#define MODULE_TAG "h264d_test"


#if defined(_WIN32)
#include "vld.h"
#endif
#include <string.h>

#include "mpp_env.h"
#include "mpp_dec.h"
#include "mpp_hal.h"
#include "mpp_mem.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer.h"
#include "mpp_buffer_impl.h"
#include "mpp_hal.h"

#include "h264d_rwfile.h"
#include "h264d_api.h"
#include "hal_task.h"
#include "hal_h264d_api.h"



typedef struct h264d_test_ctx_t {
    MppDec         m_api;
    InputParams    m_in;
    HalTaskInfo    m_task;
    MppBuffer      m_dec_pkt_buf;
    MppBuffer      m_dec_pic_buf;
    MppBufferGroup mFrameGroup;
    MppBufferGroup mStreamGroup;
} H264dTestCtx_t;


static MPP_RET decoder_deinit(H264dTestCtx_t *pctx)
{
    MppDec *pApi = &pctx->m_api;

    if (pApi->parser) {
        parser_deinit(pApi->parser);
        pApi->parser = NULL;
    }
    if (pApi->hal) {
        mpp_hal_deinit(pApi->hal);
        pApi->hal = NULL;
    }
    if (pApi->frame_slots) {
        mpp_buf_slot_deinit(pApi->frame_slots);
        pApi->frame_slots = NULL;
    }
    if (pApi->packet_slots) {
        mpp_buf_slot_deinit(pApi->packet_slots);
        pApi->packet_slots = NULL;
    }
    if (pctx->m_dec_pkt_buf) {
        mpp_buffer_put(pctx->m_dec_pkt_buf);
        pctx->m_dec_pkt_buf = NULL;
    }
    if (pctx->m_dec_pic_buf) {
        mpp_buffer_put(pctx->m_dec_pic_buf);
        pctx->m_dec_pic_buf = NULL;
    }
    if (pctx->mFrameGroup) {
        mpp_err("mFrameGroup deInit");
        mpp_buffer_group_put(pctx->mFrameGroup);
        pctx->mFrameGroup = NULL;
    }
    if (pctx->mStreamGroup) {
        mpp_err("mStreamGroup deInit");
        mpp_buffer_group_put(pctx->mStreamGroup);
        pctx->mStreamGroup = NULL;
    }
    return MPP_OK;
}

static MPP_RET decoder_init(H264dTestCtx_t *pctx)
{

    MPP_RET ret = MPP_ERR_UNKNOW;
    ParserCfg parser_cfg;
    MppHalCfg hal_cfg;
    MppDec *pApi = &pctx->m_api;

    mpp_env_get_u32("rkv_h264d_test_debug", &rkv_h264d_test_debug, 0);

    if (pctx->mFrameGroup == NULL) {
#ifdef RKPLATFORM
        mpp_log_f("mFrameGroup used ion In");
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mFrameGroup, MPP_BUFFER_TYPE_ION));
#else
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mFrameGroup, MPP_BUFFER_TYPE_NORMAL));
#endif
        if (MPP_OK != ret) {
            mpp_err("h264d mpp_buffer_group_get failed\n");
            goto __FAILED;
        }
    }
    if (pctx->mStreamGroup == NULL) {
#ifdef RKPLATFORM
        mpp_log_f("mStreamGroup used ion In");
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mStreamGroup, MPP_BUFFER_TYPE_ION));
#else
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mStreamGroup, MPP_BUFFER_TYPE_NORMAL));
#endif
        if (MPP_OK != ret) {
            mpp_err("h264d mpp_buffer_group_get failed\n");
            goto __FAILED;
        }
    }
    // codec
    pApi->coding = MPP_VIDEO_CodingAVC;
    // malloc slot
    FUN_CHECK(ret = mpp_buf_slot_init(&pApi->frame_slots));
    MEM_CHECK(ret, pApi->frame_slots);
    ret = mpp_buf_slot_init(&pApi->packet_slots);
    MEM_CHECK(ret, pApi->packet_slots);
    mpp_buf_slot_setup(pApi->packet_slots, 2);
    // init parser part
    memset(&parser_cfg, 0, sizeof(parser_cfg));
    parser_cfg.coding = pApi->coding;
    parser_cfg.frame_slots = pApi->frame_slots;
    parser_cfg.packet_slots = pApi->packet_slots;
    parser_cfg.task_count = 2;
    FUN_CHECK(ret = parser_init(&pApi->parser, &parser_cfg));
    // init hal part
    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.type = MPP_CTX_DEC;
    hal_cfg.coding = pApi->coding;
    hal_cfg.work_mode = HAL_MODE_LIBVPU;
    {
        RK_U32 hal_device_id = 0;
        mpp_env_get_u32("h264d_chg_org", &hal_device_id, 1);
        if (hal_device_id == 1) {
            hal_cfg.device_id = HAL_RKVDEC;
        } else {
            hal_cfg.device_id = HAL_VDPU;
        }
    }
    hal_cfg.frame_slots = pApi->frame_slots;
    hal_cfg.packet_slots = pApi->packet_slots;
    hal_cfg.task_count = parser_cfg.task_count;
    FUN_CHECK(ret = mpp_hal_init(&pApi->hal, &hal_cfg));
    pApi->tasks = hal_cfg.tasks;

    return MPP_OK;

__FAILED:
    decoder_deinit(pctx);

    return ret;
}

static MPP_RET flush_frames(MppDec *pApi, FILE *fp)
{
    RK_S32 slot_idx = 0;
    MppFrame out_frame = NULL;

    while (MPP_OK == mpp_buf_slot_dequeue(pApi->frame_slots, &slot_idx, QUEUE_DISPLAY)) {
        mpp_buf_slot_get_prop(pApi->frame_slots, slot_idx, SLOT_FRAME, &out_frame);
        if (out_frame) {
            RK_U32 stride_w, stride_h;
            void *ptr = NULL;
            MppBuffer framebuf;
            stride_w = mpp_frame_get_hor_stride(out_frame);
            stride_h = mpp_frame_get_ver_stride(out_frame);
            framebuf = mpp_frame_get_buffer(out_frame);
            ptr = mpp_buffer_get_ptr(framebuf);
            if (fp) {
                fwrite(ptr, 1, stride_w * stride_h * 3 / 2, fp);
                fflush(fp);
            }
            mpp_frame_deinit(&out_frame);
            out_frame = NULL;
        }
        mpp_buf_slot_clr_flag(pApi->frame_slots, slot_idx, SLOT_QUEUE_USE);
    }

    return MPP_OK;
}

static MPP_RET decoder_single_test(H264dTestCtx_t *pctx)
{
    MppPacket pkt = NULL;
    RK_U32 end_of_flag = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    MppDec *pApi = &pctx->m_api;
    InputParams *pIn = &pctx->m_in;
    HalTaskInfo *task = &pctx->m_task;
    //!< initial task
    memset(task, 0, sizeof(HalTaskInfo));
    memset(task->dec.refer, -1, sizeof(task->dec.refer));
    task->dec.input = -1;
    do {
        //!< get one packet
        if (pkt == NULL) {
            if (pIn->is_eof ||
                (pIn->iDecFrmNum && (pIn->iFrmdecoded >= pIn->iDecFrmNum))) {
                mpp_packet_init(&pkt, NULL, 0);
                mpp_packet_set_eos(pkt);
            } else {
                FUN_CHECK(ret = h264d_read_one_frame(pIn));
                mpp_packet_init(&pkt, pIn->strm.pbuf, pIn->strm.strmbytes);
            }
        }
        //!< prepare
        FUN_CHECK(ret = parser_prepare(pApi->parser, pkt, &task->dec));

        //!< parse
        if (task->dec.valid) {
            MppBufferImpl *buf = NULL;

            task->dec.valid = 0;
            if (task->dec.input < 0) {
                mpp_buf_slot_get_unused(pApi->packet_slots, &task->dec.input);
            }
            mpp_buf_slot_get_prop(pApi->packet_slots, task->dec.input, SLOT_BUFFER, &pctx->m_dec_pkt_buf);
            if (NULL == pctx->m_dec_pkt_buf) {
                size_t stream_size = mpp_packet_get_size(task->dec.input_packet);
                mpp_buffer_get(pctx->mStreamGroup, &pctx->m_dec_pkt_buf, stream_size);
                if (pctx->m_dec_pkt_buf)
                    mpp_buf_slot_set_prop(pApi->packet_slots, task->dec.input, SLOT_BUFFER, pctx->m_dec_pkt_buf);
            }
            buf = (MppBufferImpl *)pctx->m_dec_pkt_buf;
            memcpy(buf->info.ptr, mpp_packet_get_data(task->dec.input_packet), mpp_packet_get_length(task->dec.input_packet));

            mpp_buf_slot_set_flag(pApi->packet_slots, task->dec.input, SLOT_CODEC_READY);
            mpp_buf_slot_set_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
            FUN_CHECK(ret = parser_parse(pApi->parser, &task->dec));
        }
        //!< deinit packet
        if (mpp_packet_get_length(pkt) == 0) {
            if (mpp_packet_get_eos(pkt)) {
                if (task->dec.valid) {
                    mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
                    mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
                    task->dec.valid = 0;
                }
                end_of_flag = 1; //!< end of stream
            }
            mpp_packet_deinit(&pkt);
            pkt = NULL;
        }
        //!< run hal module
        if (task->dec.valid) {
            if (mpp_buf_slot_is_changed(pApi->frame_slots)) {
                mpp_buf_slot_ready(pApi->frame_slots);
            }
            mpp_buf_slot_get_prop(pApi->frame_slots, task->dec.output, SLOT_BUFFER, &pctx->m_dec_pic_buf);
            if (NULL == pctx->m_dec_pic_buf) {
                RK_U32 size = (RK_U32)mpp_buf_slot_get_size(pApi->frame_slots);
                mpp_buffer_get(pctx->mFrameGroup, &pctx->m_dec_pic_buf, size);
                if (pctx->m_dec_pic_buf)
                    mpp_buf_slot_set_prop(pApi->frame_slots, task->dec.output, SLOT_BUFFER, pctx->m_dec_pic_buf);
            }
            FUN_CHECK(ret = mpp_hal_reg_gen(pApi->hal, task));

            FUN_CHECK(ret = mpp_hal_hw_start(pApi->hal, task));
            FUN_CHECK(ret = mpp_hal_hw_wait(pApi->hal, task));
            if (pctx->m_dec_pkt_buf) {
                mpp_buffer_put(pctx->m_dec_pkt_buf);
                pctx->m_dec_pkt_buf = NULL;
            }
            mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
            mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
            //!< write frame out
            flush_frames(pApi, pIn->fp_yuv_data);
            //!< clear refrece flag
            {
                RK_U32 i = 0;
                for (i = 0; i < MPP_ARRAY_ELEMS(task->dec.refer); i++) {
                    if (task->dec.refer[i] >= 0) {
                        mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.refer[i], SLOT_HAL_INPUT);
                    }
                }
            }
            //!< reset task
            memset(task, 0, sizeof(HalTaskInfo));
            memset(task->dec.refer, -1, sizeof(task->dec.refer));
            task->dec.input = -1;
            H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] ---- had decode frame_no = %d", pIn->iFrmdecoded);
            pIn->iFrmdecoded++;

        }
    } while (!end_of_flag);
    //!< flush dpb and send to display
    FUN_CHECK(ret = mpp_dec_flush(pApi));
    flush_frames(pApi, pIn->fp_yuv_data);

    ret = MPP_OK;
__FAILED:
    return ret;
}


int main(int argc, char **argv)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    InputParams *pInput = NULL;
    H264dTestCtx_t *pctx = NULL;

    MEM_CHECK(ret, pctx = mpp_calloc(H264dTestCtx_t, 1));
    // prepare
    pInput = &pctx->m_in;
    FUN_CHECK(ret = h264d_configure(pInput, argc, argv));
    FUN_CHECK(ret = h264d_open_files(pInput));
    FUN_CHECK(ret = h264d_alloc_frame_buffer(pInput));
    // init
    decoder_init(pctx);
    // decode
    ret = decoder_single_test(pctx);
    if (ret != MPP_OK) {
        H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] Single-thread test error.");
        goto __FAILED;
    }
    ret = MPP_OK;

__FAILED:
    decoder_deinit(pctx);
    h264d_free_frame_buffer(pInput);

    h264d_close_files(pInput);
    MPP_FREE(pctx);
    H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] decoder_deinit over.");

    return ret;
}

