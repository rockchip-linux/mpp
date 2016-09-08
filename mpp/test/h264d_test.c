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

#include "vpu_api.h"

#include "h264d_log.h"
#include "mpp_env.h"
#include "mpp_dec.h"
#include "mpp_hal.h"
#include "mpp_mem.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer.h"
#include "mpp_buffer_impl.h"
#include "mpp_hal.h"

#include "h264d_log.h"
#include "h264d_rwfile.h"
#include "h264d_api.h"
#include "hal_task.h"
#include "hal_h264d_api.h"
#include "mpp_thread.h"

typedef enum {
    HAL_THREAD_UNINITED,
    HAL_THREAD_RUNNING,
    HAL_THREAD_WAITING,
    HAL_THREAD_STOPPING,
} HalStatus;

typedef struct hal_thread_param_t {
    HalStatus hal_status;
    pthread_t halThread;
    pthread_mutex_t parLock;
    pthread_cond_t parCond;
    pthread_mutex_t halLock;
    pthread_cond_t halCond;
} HalThreadPar_t;

typedef struct h264d_test_ctx_t {
    MppDec         m_api;
    InputParams    m_in;
    HalTaskInfo    m_task;
    HalThreadPar_t m_hal;
    MppBuffer      m_dec_pkt_buf;
    MppBuffer      m_dec_pic_buf;
    MppBufferGroup mFrameGroup;
    MppBufferGroup mStreamGroup;
} H264dTestCtx_t;

static MPP_RET manual_set_env(void)
{
#if defined(_MSC_VER)
    mpp_env_set_u32("h264d_log_help", 1);
    mpp_env_set_u32("h264d_log_show", 1);
    mpp_env_set_u32("h264d_log_ctrl", 0x800B);
    mpp_env_set_u32("h264d_log_level", 5);
    mpp_env_set_u32("h264d_log_decframe", 0);
    mpp_env_set_u32("h264d_log_begframe", 0);
    mpp_env_set_u32("h264d_log_endframe", 0);
    mpp_env_set_u32("h264d_log_yuv", 0);
    mpp_env_set_u32("h264d_chg_org", 0);  //!< 0:VDPU 1: RKVDEC
    mpp_env_set_str("h264d_log_outpath", "F:/h264_log/allegro_dat");
    mpp_env_set_str("h264d_log_cmppath", "F:/h264_log_driver/trunk_dat");
#endif
    return MPP_OK;
}


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
    }
    if (pctx->m_dec_pic_buf) {
        mpp_buffer_put(pctx->m_dec_pic_buf);
    }
    if (pctx->mFrameGroup != NULL) {
        mpp_err("mFrameGroup deInit");
        mpp_buffer_group_put(pctx->mFrameGroup);
    }
    if (pctx->mStreamGroup != NULL) {
        mpp_err("mStreamGroup deInit");
        mpp_buffer_group_put(pctx->mStreamGroup);
    }
    return MPP_OK;
}

static MPP_RET decoder_init(H264dTestCtx_t *pctx)
{

    MPP_RET ret = MPP_ERR_UNKNOW;
    ParserCfg parser_cfg;
    MppHalCfg hal_cfg;
    MppDec *pApi = &pctx->m_api;

    manual_set_env(); // set env, in windows

    mpp_env_get_u32("rkv_h264d_test_debug", &rkv_h264d_test_debug, 0);

    if (pctx->mFrameGroup == NULL) {
        ret = mpp_buffer_group_get_internal(&pctx->mFrameGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("h264d mpp_buffer_group_get failed\n");
            goto __FAILED;
        }
    }
    if (pctx->mStreamGroup == NULL) {
        ret = mpp_buffer_group_get_internal(&pctx->mStreamGroup, MPP_BUFFER_TYPE_ION);
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

static MPP_RET flush_decoded_frames(MppDec *pApi, FILE *fp)
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
            if ((rkv_h264d_test_debug & H264D_TEST_DUMPYUV) && fp) {
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
            H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] ---- decoder, read_one_frame Frame_no = %d", pIn->iFrmdecoded);
            pIn->iFrmdecoded++;
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
                end_of_flag = 1; //!< end of stream
                task->dec.valid = 0;
                mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
                mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
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
            flush_decoded_frames(pApi, pIn->fp_yuv_data);
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

        }
    } while (!end_of_flag);
    //!< flush dpb and send to display
    FUN_CHECK(ret = mpp_dec_flush(pApi));
    flush_decoded_frames(pApi, pIn->fp_yuv_data);

    ret = MPP_OK;
__FAILED:
    return ret;
}

static void *hal_thread_run(void *in_ctx)
{
    H264dTestCtx_t *pctx = (H264dTestCtx_t *)in_ctx;

    MppDec *pApi = &pctx->m_api;
    InputParams *pIn = &pctx->m_in;
    HalTaskInfo *task = &pctx->m_task;
    HalThreadPar_t *p_hal = &pctx->m_hal;
    HalStatus hal_status = p_hal->hal_status;

    do {
        //!< wait
        {
            pthread_mutex_lock(&p_hal->halLock);
            H264D_TEST_LOG(H264D_TEST_TRACE, "[hal_thread] pthread_cond_wait(par_Cond).");
            pthread_cond_wait(&p_hal->parCond, &p_hal->halLock);
            pthread_mutex_unlock(&p_hal->halLock);
        }
        //!< break
        {
            pthread_mutex_lock(&p_hal->halLock);
            hal_status = p_hal->hal_status;
            pthread_mutex_unlock(&p_hal->halLock);
        }
        H264D_TEST_LOG(H264D_TEST_TRACE, "[hal_thread] hal_status=%d.", hal_status);
        if (hal_status == HAL_THREAD_STOPPING) {
            H264D_TEST_LOG(H264D_TEST_TRACE, "[hal_thread] 2222222222.");
            break;
        }
        //!< signal
        {
            pthread_mutex_lock(&p_hal->halLock);
            mpp_hal_hw_wait(pctx->m_api.hal, &pctx->m_task);
            //pthread_cond_signal(&p_hal->halCond);
            H264D_TEST_LOG(H264D_TEST_TRACE, "[hal_thread] pthread_cond_signal(hal_Cond).");
            pthread_mutex_unlock(&p_hal->halLock);
        }
        {
            if (pctx->m_dec_pkt_buf) {
                mpp_buffer_put(pctx->m_dec_pkt_buf);
            }
            mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
            mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
            //!< write frame out
            flush_decoded_frames(pApi, pIn->fp_yuv_data);
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
        }
        pthread_cond_signal(&p_hal->halCond);

    } while (hal_status != HAL_THREAD_STOPPING);
    H264D_TEST_LOG(H264D_TEST_TRACE, "[hal_thread] 4444444444444444");

    return NULL;
}

static MPP_RET decoder_muti_deinit(H264dTestCtx_t *pctx)
{
    void *ptr = NULL;
    HalThreadPar_t *p_hal = &pctx->m_hal;

    H264D_TEST_LOG(H264D_TEST_TRACE, "decoder_deinit In.");
    pthread_join(p_hal->halThread, &ptr);
    H264D_TEST_LOG(H264D_TEST_TRACE, "halThread(%p)destroy success", &p_hal->halThread);

    pthread_cond_destroy(&p_hal->parCond);
    pthread_cond_destroy(&p_hal->halCond);
    pthread_mutex_destroy(&p_hal->parLock);
    pthread_mutex_destroy(&p_hal->halLock);
    H264D_TEST_LOG(H264D_TEST_TRACE, "3333333333333");
    H264D_TEST_LOG(H264D_TEST_TRACE, "decoder_deinit Ok.");

    return MPP_OK;
}

static MPP_RET decoder_muti_init(H264dTestCtx_t *pctx)
{
    HalThreadPar_t *p_hal = &pctx->m_hal;

    p_hal->hal_status = HAL_THREAD_RUNNING;
    //!< create lock and condition
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&p_hal->parLock, &attr);
        pthread_mutex_init(&p_hal->halLock, &attr);
        pthread_mutexattr_destroy(&attr);

        pthread_cond_init(&p_hal->parCond, NULL);
        pthread_cond_init(&p_hal->halCond, NULL);
        H264D_TEST_LOG(H264D_TEST_TRACE, "mutex and condition create success.");
        pthread_mutex_lock(&p_hal->halLock);
        pthread_cond_signal(&p_hal->halCond);
        H264D_TEST_LOG(H264D_TEST_TRACE, "pthread_cond_signal(hal_Cond).");
        pthread_mutex_unlock(&p_hal->halLock);

    }
    //!< create hal thread
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if (0 == pthread_create(&p_hal->halThread, &attr, hal_thread_run, pctx)) {
            H264D_TEST_LOG(H264D_TEST_TRACE, "hal_thread(%p) create success.", &p_hal->halThread);
        }
        pthread_attr_destroy(&attr);
    }
    return MPP_OK;
}

static MPP_RET decoder_muti_test(H264dTestCtx_t *pctx)
{
    MppPacket pkt = NULL;
    RK_U32 end_of_flag = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    MppDec *pApi = &pctx->m_api;
    InputParams *pIn = &pctx->m_in;
    HalTaskInfo *task = &pctx->m_task;
    //!< init decoder
    FUN_CHECK(ret = decoder_muti_init(pctx));
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
            H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] ---- decoder, read_one_frame Frame_no = %d", pIn->iFrmdecoded);
            pIn->iFrmdecoded++;
        }
        //!< prepare
        FUN_CHECK(ret = parser_prepare(pApi->parser, pkt, &task->dec));
        {
            HalThreadPar_t *p_hal = &pctx->m_hal;
            pthread_mutex_lock(&p_hal->parLock);
            H264D_TEST_LOG(H264D_TEST_TRACE, "pthread_cond_wait(hal Cond).");
            pthread_cond_wait(&p_hal->halCond, &p_hal->parLock);
            pthread_mutex_unlock(&p_hal->parLock);
        }
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
                end_of_flag = 1; //!< end of stream
                task->dec.valid = 0;
                {
                    HalThreadPar_t *p_hal = &pctx->m_hal;
                    pthread_mutex_lock(&p_hal->parLock);
                    p_hal->hal_status = HAL_THREAD_STOPPING;
                    pthread_cond_signal(&p_hal->parCond);
                    H264D_TEST_LOG(H264D_TEST_TRACE, "1111111111111111");
                    pthread_mutex_unlock(&p_hal->parLock);
                }

                mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
                mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
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
            {
                HalThreadPar_t *p_hal = &pctx->m_hal;

                pthread_mutex_lock(&p_hal->parLock);
                pthread_cond_signal(&p_hal->parCond);
                H264D_TEST_LOG(H264D_TEST_TRACE, "pthread_cond_single(par Cond).");
                pthread_mutex_unlock(&p_hal->parLock);
            }
        }
    } while (!end_of_flag);
    //!< flush dpb and send to display
    FUN_CHECK(ret = mpp_dec_flush(pApi));
    flush_decoded_frames(pApi, pIn->fp_yuv_data);

    ret = MPP_OK;
__FAILED:
    decoder_muti_deinit(pctx);

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
    if (rkv_h264d_test_debug & H264D_TEST_MUTI_THREAD) {
        ret = decoder_muti_test(pctx);
        if (ret != MPP_OK) {
            H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] Muti-thread test error.");
            goto __FAILED;
        }
    } else {
        ret = decoder_single_test(pctx);
        if (ret != MPP_OK) {
            H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] Single-thread test error.");
            goto __FAILED;
        }
    }
    ret = MPP_OK;

__FAILED:
    decoder_deinit(pctx);
    h264d_free_frame_buffer(pInput);
    if ((ret == MPP_OK)
        && (H264D_TEST_FPGA & rkv_h264d_test_debug)) {
        h264d_write_fpga_data(pInput);  //!< for fpga debug
    }
    h264d_close_files(pInput);
    MPP_FREE(pctx);
    H264D_TEST_LOG(H264D_TEST_TRACE, "[H264D_TEST] decoder_deinit over.");

    return ret;
}

