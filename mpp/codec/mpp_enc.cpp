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

#define  MODULE_TAG "mpp_enc"

#include "string.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "hal_h264e_api.h"

static MPP_RET release_task_in_port(MppPort port)
{
    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    MppFrame frame = NULL;
    MppTask mpp_task;

    do {
        ret = mpp_port_dequeue(port, &mpp_task);
        if (ret)
            break;

        if (mpp_task) {
            packet = NULL;
            frame = NULL;
            ret = mpp_task_meta_get_frame(mpp_task, MPP_META_KEY_INPUT_FRM,  &frame);
            if (frame) {
                mpp_frame_deinit(&frame);
                frame = NULL;
            }
            ret = mpp_task_meta_get_packet(mpp_task, MPP_META_KEY_OUTPUT_PKT, &packet);
            if (packet) {
                mpp_packet_deinit(&packet);
                packet = NULL;
            }

            mpp_port_enqueue(port, mpp_task);
            mpp_task = NULL;
        } else
            break;
    } while (1);

    return ret;
}

void *mpp_enc_control_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEnc *enc = mpp->mEnc;
    MppThread *thd_enc  = mpp->mThreadCodec;
    EncTask task;  // TODO
    HalTaskInfo task_info;
    MppPort input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
    MppPort output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    MppTask mpp_task = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;

    memset(&task_info, 0, sizeof(HalTaskInfo));

    while (MPP_THREAD_RUNNING == thd_enc->get_status()) {
        thd_enc->lock();
        ret = mpp_port_dequeue(input, &mpp_task);
        if (ret || NULL == mpp_task) {
            thd_enc->wait();
        }
        thd_enc->unlock();

        if (mpp_task != NULL) {
            mpp_task_meta_get_frame (mpp_task, MPP_META_KEY_INPUT_FRM,  &frame);
            mpp_task_meta_get_packet(mpp_task, MPP_META_KEY_OUTPUT_PKT, &packet);

            if (NULL == frame) {
                mpp_port_enqueue(input, mpp_task);
                continue;
            }

            memset(&task, 0, sizeof(EncTask));

            if (mpp_frame_get_buffer(frame)) {
                /*
                 * if there is available buffer in the input frame do encoding
                 */
                if (NULL == packet) {
                    RK_U32 width  = enc->enc_cfg.width;
                    RK_U32 height = enc->enc_cfg.height;
                    RK_U32 size = width * height;
                    MppBuffer buffer = NULL;

                    mpp_buffer_get(mpp->mPacketGroup, &buffer, size);
                    mpp_log("create buffer size %d fd %d\n", size, mpp_buffer_get_fd(buffer));
                    mpp_packet_init_with_buffer(&packet, buffer);
                    mpp_buffer_put(buffer);
                }
                mpp_assert(packet);

                mpp_packet_set_pts(packet, mpp_frame_get_pts(frame));

                task.ctrl_frm_buf_in = mpp_frame_get_buffer(frame);
                task.ctrl_pkt_buf_out = mpp_packet_get_buffer(packet);
                controller_encode(mpp->mEnc->controller, &task);

                task_info.enc.syntax.data = (void *)(&(task.syntax_data));
                mpp_hal_reg_gen((mpp->mEnc->hal), &task_info);
                mpp_hal_hw_start((mpp->mEnc->hal), &task_info);
                /*vpuWaitResult = */mpp_hal_hw_wait((mpp->mEnc->hal), &task_info); // TODO   need to check the return value

                RK_U32 outputStreamSize = 0;
                controller_config(mpp->mEnc->controller, GET_OUTPUT_STREAM_SIZE, (void*)&outputStreamSize);

                mpp_packet_set_length(packet, outputStreamSize);
            } else {
                /*
                 * else init a empty packet for output
                 */
                mpp_packet_new(&packet);
            }

            if (mpp_frame_get_eos(frame))
                mpp_packet_set_eos(packet);

            /*
             * first clear output packet
             * then enqueue task back to input port
             * final user will release the mpp_frame they had input
             */
            mpp_task_meta_set_frame(mpp_task, MPP_META_KEY_INPUT_FRM, frame);
            mpp_port_enqueue(input, mpp_task);
            mpp_task = NULL;

            // send finished task to output port
            mpp_port_dequeue(output, &mpp_task);
            mpp_task_meta_set_packet(mpp_task, MPP_META_KEY_OUTPUT_PKT, packet);

            {
                RK_S32 is_intra = task.syntax_data.frame_coding_type;
                RK_U32 flag = mpp_packet_get_flag(packet);

                mpp_task_meta_set_s32(mpp_task, MPP_META_KEY_OUTPUT_INTRA, is_intra);
                if (is_intra) {
                    mpp_packet_set_flag(packet, flag | MPP_PACKET_FLAG_INTRA);
                }
            }

            // setup output task here
            mpp_port_enqueue(output, mpp_task);
            mpp_task = NULL;
            packet = NULL;
            frame = NULL;
        }
    }

    // clear remain task in output port
    release_task_in_port(mpp->mInputPort);
    release_task_in_port(input);
    release_task_in_port(mpp->mOutputPort);

    return NULL;
}

void *mpp_enc_hal_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *hal      = mpp->mThreadHal;
    mpp_list *frames    = mpp->mFrames;
    mpp_list *tasks     = mpp->mTasks;

    while (MPP_THREAD_RUNNING == hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        hal->lock();
        if (0 == tasks->list_size())
            hal->wait();
        hal->unlock();

        // get_config
        // register genertation
        if (tasks->list_size()) {
            HalDecTask *task;
            tasks->lock();
            tasks->del_at_head(&task, sizeof(task));
            mpp->mTaskGetCount++;
            tasks->unlock();

            // hal->mpp_hal_reg_gen(current);

            /*
             * wait previous register set done
             */
            // hal->mpp_hal_hw_wait(previous);

            /*
             * send current register set to hardware
             */
            // hal->mpp_hal_hw_start(current);

            /*
             * mark previous buffer is complete
             */
            // change dpb slot status
            // signal()
            // mark frame in output queue
            // wait up output thread to get a output frame

            // for test
            MppBuffer buffer;
            mpp_buffer_get(mpp->mFrameGroup, &buffer, SZ_1M);

            MppFrame frame;
            mpp_frame_init(&frame);
            mpp_frame_set_buffer(frame, buffer);
            frames->lock();
            frames->add_at_tail(&frame, sizeof(frame));
            mpp->mFramePutCount++;
            frames->signal();
            frames->unlock();
        }
    }

    return NULL;
}

static MPP_RET mpp_extra_info_generate(MppEnc *enc)
{
    h264e_control_extra_info_cfg *info  = &enc->extra_info_cfg;
    H264EncConfig *enc_cfg              = &enc->enc_cfg;
    H264EncRateCtrl *enc_rc_cfg         = &enc->enc_rc_cfg;

    info->chroma_qp_index_offset        = enc_cfg->chroma_qp_index_offset;
    info->enable_cabac                  = enc_cfg->enable_cabac;
    info->pic_init_qp                   = enc_cfg->pic_init_qp;
    info->pic_luma_height               = enc_cfg->height;
    info->pic_luma_width                = enc_cfg->width;
    info->transform8x8_mode             = enc_cfg->transform8x8_mode;

    info->input_image_format            = enc_cfg->input_image_format;
    info->profile_idc                   = enc_cfg->profile;
    info->level_idc                     = enc_cfg->level;
    info->keyframe_max_interval         = enc_rc_cfg->keyframe_max_interval;
    info->second_chroma_qp_index_offset = enc_cfg->second_chroma_qp_index_offset;
    info->pps_id                        = enc_cfg->pps_id;
    return MPP_OK;
}

MPP_RET mpp_enc_init(MppEnc **enc, MppCodingType coding)
{
    MPP_RET ret;
    MppBufSlots frame_slots = NULL;
    MppBufSlots packet_slots = NULL;
    Controller controller = NULL;
    MppHal hal = NULL;
    MppEnc *p = NULL;
    RK_S32 task_count = 2;
    IOInterruptCB cb = {NULL, NULL};

    if (NULL == enc) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    *enc = NULL;

    p = mpp_calloc(MppEnc, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_MALLOC;
    }

    do {
        ret = mpp_buf_slot_init(&frame_slots);
        if (ret) {
            mpp_err_f("could not init frame buffer slot\n");
            break;
        }

        ret = mpp_buf_slot_init(&packet_slots);
        if (ret) {
            mpp_err_f("could not init packet buffer slot\n");
            break;
        }

        mpp_buf_slot_setup(packet_slots, task_count);
        cb.callBack = mpp_enc_notify;
        cb.opaque = p;

        ControllerCfg controller_cfg = {
            coding,
            task_count,
            cb,
        };

        ret = controller_init(&controller, &controller_cfg);
        if (ret) {
            mpp_err_f("could not init parser\n");
            break;
        }
        cb.callBack = hal_enc_callback;
        cb.opaque = controller;
        // then init hal with task count from parser
        MppHalCfg hal_cfg = {
            MPP_CTX_ENC,
            coding,
            HAL_MODE_LIBVPU,
            HAL_VEPU,
            frame_slots,
            packet_slots,
            NULL,
            1/*controller_cfg.task_count*/,  // TODO
            0,
            cb,
        };

        ret = mpp_hal_init(&hal, &hal_cfg);
        if (ret) {
            mpp_err_f("could not init hal\n");
            break;
        }

        p->coding = coding;
        p->controller = controller;
        p->hal    = hal;
        p->tasks  = hal_cfg.tasks;
        p->frame_slots  = frame_slots;
        p->packet_slots = packet_slots;
        *enc = p;
        return MPP_OK;
    } while (0);

    mpp_enc_deinit(p);
    return MPP_NOK;

}

MPP_RET mpp_enc_deinit(MppEnc *enc)
{
    if (NULL == enc) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (enc->controller) {
        controller_deinit(enc->controller);
        enc->controller = NULL;
    }

    if (enc->hal) {
        mpp_hal_deinit(enc->hal);
        enc->hal = NULL;
    }

    if (enc->frame_slots) {
        mpp_buf_slot_deinit(enc->frame_slots);
        enc->frame_slots = NULL;
    }

    if (enc->packet_slots) {
        mpp_buf_slot_deinit(enc->packet_slots);
        enc->packet_slots = NULL;
    }

    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_reset(MppEnc *enc)
{
    (void)enc;
    return MPP_OK;
}

MPP_RET mpp_enc_notify(void *ctx, void *info)
{
    // TODO
    (void)ctx;
    (void)info;
    return MPP_OK;
}

MPP_RET mpp_enc_control(MppEnc *enc, MpiCmd cmd, void *param)
{
    if (NULL == enc) {
        mpp_err_f("found NULL input enc %p\n", enc);
        return MPP_ERR_NULL_PTR;
    }

    // TODO
    MppEncConfig    *mpp_cfg    = (MppEncConfig*)param;
    H264EncConfig   *enc_cfg    = &(enc->enc_cfg);
    H264EncRateCtrl *enc_rc_cfg = &(enc->enc_rc_cfg);

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        //H264ENC_NAL_UNIT_STREAM;  // decide whether stream start with start code,e.g."00 00 00 01"
        enc_cfg->streamType = H264ENC_BYTE_STREAM;
        enc_cfg->frameRateDenom = 1;
        if (mpp_cfg->profile)
            enc_cfg->profile = (h264e_profile)mpp_cfg->profile;
        else
            enc_cfg->profile = H264_PROFILE_BASELINE;
        if (mpp_cfg->level)
            enc_cfg->level = (H264EncLevel)mpp_cfg->level;
        else
            enc_cfg->level = H264ENC_LEVEL_4_0;
        if (mpp_cfg->width && mpp_cfg->height) {
            enc_cfg->width = mpp_cfg->width;
            enc_cfg->height = mpp_cfg->height;
        } else
            mpp_err("width %d height %d is not available\n", mpp_cfg->width, mpp_cfg->height);
        if (mpp_cfg->fps_in)
            enc_cfg->frameRateNum = mpp_cfg->fps_in;
        else
            enc_cfg->frameRateNum = 30;
        if (mpp_cfg->cabac_en)
            enc_cfg->enable_cabac = mpp_cfg->cabac_en;
        else
            enc_cfg->enable_cabac = 0;

        enc_cfg->transform8x8_mode = (enc_cfg->profile >= H264_PROFILE_HIGH) ? (1) : (0);
        enc_cfg->chroma_qp_index_offset = 2;
        enc_cfg->pic_init_qp = mpp_cfg->qp;
        enc_cfg->second_chroma_qp_index_offset = 2;
        enc_cfg->pps_id = 0;
        enc_cfg->input_image_format = H264ENC_YUV420_SEMIPLANAR;

        controller_config(enc->controller, SET_ENC_CFG, (void *)enc_cfg);

        if (mpp_cfg->rc_mode) {
            /* VBR / CBR mode */
            RK_S32 max_qp = MPP_MAX(mpp_cfg->qp + 6, 48);
            RK_S32 min_qp = MPP_MIN(mpp_cfg->qp - 6, 16);

            enc_rc_cfg->pictureRc       = 1;
            enc_rc_cfg->mbRc            = 1;
            enc_rc_cfg->qpHdr           = mpp_cfg->qp;
            enc_rc_cfg->qpMax           = max_qp;
            enc_rc_cfg->qpMin           = min_qp;
            enc_rc_cfg->hrd             = 1;
            enc_rc_cfg->intraQpDelta    = 3;
        } else {
            /* CQP mode */
            enc_rc_cfg->pictureRc       = 0;
            enc_rc_cfg->mbRc            = 0;
            enc_rc_cfg->qpHdr           = mpp_cfg->qp;
            enc_rc_cfg->qpMax           = mpp_cfg->qp;
            enc_rc_cfg->qpMin           = mpp_cfg->qp;
            enc_rc_cfg->hrd             = 0;
            enc_rc_cfg->intraQpDelta    = 0;
        }
        enc_rc_cfg->pictureSkip = mpp_cfg->skip_cnt;

        if (mpp_cfg->gop > 0)
            enc_rc_cfg->intraPicRate = mpp_cfg->gop;
        else
            enc_rc_cfg->intraPicRate = 30;

        enc_rc_cfg->keyframe_max_interval = 150;
        enc_rc_cfg->bitPerSecond = mpp_cfg->bps;
        enc_rc_cfg->gopLen = mpp_cfg->gop;
        enc_rc_cfg->fixedIntraQp = 0;
        enc_rc_cfg->mbQpAdjustment = 3;
        enc_rc_cfg->hrdCpbSize = mpp_cfg->bps / 8;

        controller_config(enc->controller, SET_ENC_RC_CFG,  (void *)enc_rc_cfg);

        mpp_extra_info_generate(enc);

        mpp_hal_control(enc->hal, MPP_ENC_SET_EXTRA_INFO, (void*)(&(enc->extra_info_cfg)));

    } break;
    case MPP_ENC_GET_EXTRA_INFO : {
        mpp_hal_control(enc->hal, cmd, param);
    } break;
    default : {
    } break;
    }

    return MPP_OK;
}

