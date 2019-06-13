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

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp_packet_impl.h"

#include "mpp.h"
#include "hal_h264e_api.h"

#define MPP_ENC_DBG_FUNCTION            (0x00000001)
#define MPP_ENC_DBG_CONTROL             (0x00000002)

RK_U32 mpp_enc_debug = 0;

#define mpp_enc_dbg(flag, fmt, ...)     _mpp_dbg(mpp_enc_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_enc_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpp_enc_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_enc_dbg_func(fmt, ...)      mpp_enc_dbg_f(MPP_ENC_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define mpp_enc_dbg_ctrl(fmt, ...)      mpp_enc_dbg_f(MPP_ENC_DBG_CONTROL, fmt, ## __VA_ARGS__)

static void reset_hal_enc_task(HalEncTask *task)
{
    memset(task, 0, sizeof(*task));
}

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
            ret = mpp_task_meta_get_frame(mpp_task, KEY_INPUT_FRAME,  &frame);
            if (frame) {
                mpp_frame_deinit(&frame);
                frame = NULL;
            }
            ret = mpp_task_meta_get_packet(mpp_task, KEY_OUTPUT_PACKET, &packet);
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
    HalTaskInfo task_info;
    HalEncTask *enc_task = &task_info.enc;
    MppPort input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
    MppPort output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    MppTask mpp_task = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;
    MppBuffer mv_info = NULL;

    memset(&task_info, 0, sizeof(HalTaskInfo));

    while (MPP_THREAD_RUNNING == thd_enc->get_status()) {
        thd_enc->lock();
        ret = mpp_port_dequeue(input, &mpp_task);
        if (ret || NULL == mpp_task) {
            thd_enc->wait();
        }
        thd_enc->unlock();

        if (mpp_task != NULL) {
            mpp_task_meta_get_frame (mpp_task, KEY_INPUT_FRAME,  &frame);
            mpp_task_meta_get_packet(mpp_task, KEY_OUTPUT_PACKET, &packet);
            mpp_task_meta_get_buffer(mpp_task, KEY_MOTION_INFO, &mv_info);

            if (NULL == frame) {
                mpp_port_enqueue(input, mpp_task);
                continue;
            }

            reset_hal_enc_task(enc_task);

            if (mpp_frame_get_buffer(frame)) {
                /*
                 * if there is available buffer in the input frame do encoding
                 */
                if (NULL == packet) {
                    RK_U32 width  = enc->cfg.prep.width;
                    RK_U32 height = enc->cfg.prep.height;
                    RK_U32 size = width * height;
                    MppBuffer buffer = NULL;

                    mpp_assert(size);
                    mpp_buffer_get(mpp->mPacketGroup, &buffer, size);
                    mpp_packet_init_with_buffer(&packet, buffer);
                    mpp_buffer_put(buffer);
                }
                mpp_assert(packet);

                mpp_packet_set_pts(packet, mpp_frame_get_pts(frame));

                enc_task->input  = mpp_frame_get_buffer(frame);
                enc_task->output = mpp_packet_get_buffer(packet);
                enc_task->mv_info = mv_info;

                {
                    AutoMutex auto_lock(&enc->lock);
                    ret = controller_encode(mpp->mEnc->controller, enc_task);
                    if (ret) {
                        mpp_err("mpp %p controller_encode failed return %d", mpp, ret);
                        goto TASK_END;
                    }
                }
                ret = mpp_hal_reg_gen((mpp->mEnc->hal), &task_info);
                if (ret) {
                    mpp_err("mpp %p hal_reg_gen failed return %d", mpp, ret);
                    goto TASK_END;
                }
                ret = mpp_hal_hw_start((mpp->mEnc->hal), &task_info);
                if (ret) {
                    mpp_err("mpp %p hal_hw_start failed return %d", mpp, ret);
                    goto TASK_END;
                }
                ret = mpp_hal_hw_wait((mpp->mEnc->hal), &task_info);
                if (ret) {
                    mpp_err("mpp %p hal_hw_wait failed return %d", mpp, ret);
                    goto TASK_END;
                }
            TASK_END:
                mpp_packet_set_length(packet, task_info.enc.length);
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
            mpp_task_meta_set_frame(mpp_task, KEY_INPUT_FRAME, frame);
            mpp_port_enqueue(input, mpp_task);
            mpp_task = NULL;

            // send finished task to output port
            mpp_port_poll(output, MPP_POLL_BLOCK);
            mpp_port_dequeue(output, &mpp_task);

            /*
             * mpp_task may be null if output port is awaken by Mpp::clear()
             */
            if (mpp_task) {
                //set motion info buffer to output task
                if (mv_info)
                    mpp_task_meta_set_buffer(mpp_task, KEY_MOTION_INFO, mv_info);

                mpp_task_meta_set_packet(mpp_task, KEY_OUTPUT_PACKET, packet);

                {
                    RK_S32 is_intra = enc_task->is_intra;
                    RK_U32 flag = mpp_packet_get_flag(packet);

                    mpp_task_meta_set_s32(mpp_task, KEY_OUTPUT_INTRA, is_intra);
                    if (is_intra) {
                        mpp_packet_set_flag(packet, flag | MPP_PACKET_FLAG_INTRA);
                    }
                }

                // setup output task here
                mpp_port_enqueue(output, mpp_task);
            } else {
                mpp_packet_deinit(&packet);
            }
            mpp_task = NULL;
            packet = NULL;
            frame = NULL;
        }
    }

    // clear remain task in output port
    release_task_in_port(input);
    release_task_in_port(mpp->mOutputPort);

    return NULL;
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

    mpp_env_get_u32("mpp_enc_debug", &mpp_enc_debug, 0);

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

        ControllerCfg ctrl_cfg = {
            coding,
            &p->cfg,
            &p->set,
            task_count,
        };

        ret = controller_init(&controller, &ctrl_cfg);
        if (ret) {
            mpp_err_f("could not init controller\n");
            break;
        }
        cb.callBack = hal_enc_callback;
        cb.opaque = controller;
        // then init hal with task count from controller
        MppHalCfg hal_cfg = {
            MPP_CTX_ENC,
            coding,
            HAL_MODE_LIBVPU,
            HAL_VEPU,
            frame_slots,
            packet_slots,
            &p->cfg,
            &p->set,
            NULL,
            1/*ctrl_cfg.task_count*/,  // TODO
            0,
            cb,
        };

        ret = mpp_hal_init(&hal, &hal_cfg);
        if (ret) {
            mpp_err_f("could not init hal\n");
            break;
        }

        p->coding       = coding;
        p->controller   = controller;
        p->hal          = hal;
        p->tasks        = hal_cfg.tasks;
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
    AutoMutex auto_lock(&enc->lock);

    return MPP_OK;
}

MPP_RET mpp_enc_notify(MppEnc *enc, RK_U32 flag)
{
    // TODO
    (void)enc;
    (void)flag;
    return MPP_OK;
}

MPP_RET mpp_enc_control(MppEnc *enc, MpiCmd cmd, void *param)
{
    if (NULL == enc || (NULL == param && cmd != MPP_ENC_SET_IDR_FRAME)) {
        mpp_err_f("found NULL input enc %p cmd %x param %d\n", enc, cmd, param);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    AutoMutex auto_lock(&enc->lock);

    switch (cmd) {
    case MPP_ENC_SET_ALL_CFG : {
        mpp_enc_dbg_ctrl("set all config\n");
        memcpy(&enc->set, param, sizeof(enc->set));

        ret = controller_config(enc->controller, MPP_ENC_SET_RC_CFG, param);
        if (!ret)
            ret = mpp_hal_control(enc->hal, MPP_ENC_SET_RC_CFG, &enc->set.rc);

        if (!ret)
            mpp_enc_update_rc_cfg(&enc->cfg.rc, &enc->set.rc);

        if (!ret)
            ret = mpp_hal_control(enc->hal, MPP_ENC_SET_PREP_CFG, &enc->set.prep);

        if (!ret)
            ret = mpp_hal_control(enc->hal, MPP_ENC_SET_CODEC_CFG, &enc->set.codec);
    } break;
    case MPP_ENC_GET_ALL_CFG : {
        MppEncCfgSet *p = (MppEncCfgSet *)param;

        mpp_enc_dbg_ctrl("get all config\n");
        memcpy(p, &enc->cfg, sizeof(*p));
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        mpp_enc_dbg_ctrl("set prep config\n");
        memcpy(&enc->set.prep, param, sizeof(enc->set.prep));

        ret = mpp_hal_control(enc->hal, cmd, param);
        if (!ret)
            mpp_enc_update_prep_cfg(&enc->cfg.prep, &enc->set.prep);
    } break;
    case MPP_ENC_GET_PREP_CFG : {
        MppEncPrepCfg *p = (MppEncPrepCfg *)param;

        mpp_enc_dbg_ctrl("get prep config\n");
        memcpy(p, &enc->cfg.prep, sizeof(*p));
    } break;
    case MPP_ENC_SET_RC_CFG : {
        mpp_enc_dbg_ctrl("set rc config\n");
        memcpy(&enc->set.rc, param, sizeof(enc->set.rc));

        ret = controller_config(enc->controller, cmd, param);
        if (!ret)
            ret = mpp_hal_control(enc->hal, cmd, param);

        if (!ret)
            mpp_enc_update_rc_cfg(&enc->cfg.rc, &enc->set.rc);
    } break;
    case MPP_ENC_GET_RC_CFG : {
        MppEncRcCfg *p = (MppEncRcCfg *)param;

        mpp_enc_dbg_ctrl("get rc config\n");
        memcpy(p, &enc->cfg.rc, sizeof(*p));
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        mpp_enc_dbg_ctrl("set codec config\n");
        memcpy(&enc->set.codec, param, sizeof(enc->set.codec));

        ret = mpp_hal_control(enc->hal, cmd, param);
        /* NOTE: codec information will be update by encoder hal */
    } break;
    case MPP_ENC_GET_CODEC_CFG : {
        MppEncCodecCfg *p = (MppEncCodecCfg *)param;

        mpp_enc_dbg_ctrl("get codec config\n");
        memcpy(p, &enc->cfg.codec, sizeof(*p));
    } break;

    case MPP_ENC_SET_IDR_FRAME : {
        mpp_enc_dbg_ctrl("idr request\n");
        ret = controller_config(enc->controller, SET_IDR_FRAME, param);
    } break;
    case MPP_ENC_GET_EXTRA_INFO : {
        mpp_enc_dbg_ctrl("get extra info\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_OSD_PLT_CFG : {
        mpp_enc_dbg_ctrl("set osd plt\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_OSD_DATA_CFG : {
        mpp_enc_dbg_ctrl("set osd data\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_ROI_CFG : {
        mpp_enc_dbg_ctrl("set roi data\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_SEI_CFG : {
        mpp_enc_dbg_ctrl("set sei\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_GET_SEI_DATA : {
        mpp_enc_dbg_ctrl("get sei\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_PRE_ALLOC_BUFF : {
        mpp_enc_dbg_ctrl("pre alloc buff\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_QP_RANGE : {
        mpp_enc_dbg_ctrl("set qp range\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_CTU_QP: {
        mpp_enc_dbg_ctrl("set ctu qp\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    default : {
        mpp_log_f("unsupported cmd id %08x param %p\n", cmd, param);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}

void mpp_enc_update_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src)
{
    RK_U32 change = src->change;

    if (change) {

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT)
            dst->format = src->format;

        if (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)
            dst->rotation = src->rotation;

        if (change & MPP_ENC_PREP_CFG_CHANGE_MIRRORING)
            dst->mirroring = src->mirroring;

        if (change & MPP_ENC_PREP_CFG_CHANGE_DENOISE)
            dst->denoise = src->denoise;

        if (change & MPP_ENC_PREP_CFG_CHANGE_SHARPEN)
            dst->sharpen = src->sharpen;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            if (dst->rotation == MPP_ENC_ROT_90 || dst->rotation == MPP_ENC_ROT_270) {
                dst->width = src->height;
                dst->height = src->width;
            } else {
                dst->width = src->width;
                dst->height = src->height;
            }
            dst->hor_stride = src->hor_stride;
            dst->ver_stride = src->ver_stride;
        }

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    }
}

void mpp_enc_update_rc_cfg(MppEncRcCfg *dst, MppEncRcCfg *src)
{
    RK_U32 change = src->change;
    if (change) {
        if (change & MPP_ENC_RC_CFG_CHANGE_RC_MODE)
            dst->rc_mode = src->rc_mode;

        if (change & MPP_ENC_RC_CFG_CHANGE_QUALITY)
            dst->quality = src->quality;

        if (change & MPP_ENC_RC_CFG_CHANGE_BPS) {
            dst->bps_target = src->bps_target;
            dst->bps_max = src->bps_max;
            dst->bps_min = src->bps_min;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_FPS_IN) {
            dst->fps_in_flex = src->fps_in_flex;
            dst->fps_in_num = src->fps_in_num;
            dst->fps_in_denorm = src->fps_in_denorm;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
            dst->fps_out_flex = src->fps_out_flex;
            dst->fps_out_num = src->fps_out_num;
            dst->fps_out_denorm = src->fps_out_denorm;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_GOP)
            dst->gop = src->gop;

        if (change & MPP_ENC_RC_CFG_CHANGE_SKIP_CNT)
            dst->skip_cnt = src->skip_cnt;

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    }
}

