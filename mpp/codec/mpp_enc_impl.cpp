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

#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "mpp_time.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_buffer_impl.h"

#include "mpp_enc_refs.h"
#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"

#include "mpp.h"
#include "mpp_enc_debug.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_enc_impl.h"
#include "mpp_enc_cb_param.h"

typedef union EncAsyncWait_u {
    RK_U32          val;
    struct {
        RK_U32      enc_frm_in      : 1;   // 0x0001 MPP_ENC_NOTIFY_FRAME_ENQUEUE
        RK_U32      reserv0002      : 1;   // 0x0002
        RK_U32      reserv0004      : 1;   // 0x0004
        RK_U32      enc_pkt_out     : 1;   // 0x0008 MPP_ENC_NOTIFY_PACKET_ENQUEUE

        RK_U32      task_hnd        : 1;   // 0x0010
        RK_U32      reserv0020      : 1;   // 0x0020
        RK_U32      reserv0040      : 1;   // 0x0040
        RK_U32      reserv0080      : 1;   // 0x0080

        RK_U32      reserv0100      : 1;   // 0x0100
        RK_U32      reserv0200      : 1;   // 0x0200
        RK_U32      reserv0400      : 1;   // 0x0400
        RK_U32      reserv0800      : 1;   // 0x0800

        RK_U32      reserv1000      : 1;   // 0x1000
        RK_U32      reserv2000      : 1;   // 0x2000
        RK_U32      reserv4000      : 1;   // 0x4000
        RK_U32      reserv8000      : 1;   // 0x8000
    };
} EncAsyncWait;

static RK_U8 uuid_version[16] = {
    0x3d, 0x07, 0x6d, 0x45, 0x73, 0x0f, 0x41, 0xa8,
    0xb1, 0xc4, 0x25, 0xd7, 0x97, 0x6b, 0xf1, 0xac,
};

static RK_U8 uuid_rc_cfg[16] = {
    0xd7, 0xdc, 0x03, 0xc3, 0xc5, 0x6f, 0x40, 0xe0,
    0x8e, 0xa9, 0x17, 0x1a, 0xd2, 0xef, 0x5e, 0x23,
};

static RK_U8 uuid_usr_data[16] = {
    0xfe, 0x39, 0xac, 0x4c, 0x4a, 0x8e, 0x4b, 0x4b,
    0x85, 0xd9, 0xb2, 0xa2, 0x4f, 0xa1, 0x19, 0x5b,
};

static RK_U8 uuid_debug_info[16] = {
    0x57, 0x68, 0x97, 0x80, 0xe7, 0x0c, 0x4b, 0x65,
    0xa9, 0x06, 0xae, 0x29, 0x94, 0x11, 0xcd, 0x9a
};

RK_U8 uuid_refresh_cfg[16] = {
    0x72, 0x65, 0x63, 0x6f, 0x76, 0x65, 0x72, 0x79,
    0x5f, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x00, 0x00
};

static MPP_RET enc_async_wait_task(MppEncImpl *enc, EncAsyncTaskInfo *info);
static void reset_hal_enc_task(HalEncTask *task)
{
    memset(task, 0, sizeof(*task));
}

static void reset_enc_rc_task(EncRcTask *task)
{
    memset(task, 0, sizeof(*task));
}

static void reset_enc_task(MppEncImpl *enc)
{
    enc->task_in = NULL;
    enc->task_out = NULL;
    enc->packet = NULL;
    enc->frame = NULL;

    enc->frm_buf = NULL;
    enc->pkt_buf = NULL;

    /* NOTE: clear add_by flags */
    enc->hdr_status.val = enc->hdr_status.ready;
}

static void update_enc_hal_info(MppEncImpl *enc)
{
    MppDevInfoCfg data[32];
    RK_S32 size = sizeof(data);
    RK_S32 i;

    if (NULL == enc->hal_info || NULL == enc->dev)
        return ;

    hal_info_from_enc_cfg(enc->hal_info, &enc->cfg);
    hal_info_get(enc->hal_info, data, &size);

    if (size) {
        size /= sizeof(data[0]);
        for (i = 0; i < size; i++)
            mpp_dev_ioctl(enc->dev, MPP_DEV_SET_INFO, &data[i]);
    }
}

static void update_hal_info_fps(MppEncImpl *enc)
{
    MppDevInfoCfg cfg;

    RK_S32 time_diff = ((RK_S32)(enc->time_end - enc->time_base) / 1000);
    RK_U64 fps = hal_info_to_float(enc->frame_count * 1000, time_diff);

    cfg.type = ENC_INFO_FPS_CALC;
    cfg.flag = CODEC_INFO_FLAG_STRING;
    cfg.data = fps;

    mpp_dev_ioctl(enc->dev, MPP_DEV_SET_INFO, &cfg);

    enc->time_base = enc->time_end;
    enc->frame_count = 0;
}

static MPP_RET release_task_in_port(MppPort port)
{
    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    MppFrame frame = NULL;
    MppTask mpp_task;

    do {
        ret = mpp_port_poll(port, MPP_POLL_NON_BLOCK);
        if (ret < 0)
            break;

        mpp_port_dequeue(port, &mpp_task);
        mpp_assert(mpp_task);
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

static void check_hal_task_pkt_len(HalEncTask *task, const char *reason)
{
    RK_U32 task_length = task->length;
    RK_U32 packet_length = mpp_packet_get_length(task->packet);

    if (task_length != packet_length) {
        mpp_err_f("%s check failed: task length is not match to packet length %d vs %d\n",
                  reason, task_length, packet_length);
    }
}

static MPP_RET check_enc_task_wait(MppEncImpl *enc, EncAsyncWait *wait)
{
    MPP_RET ret = MPP_OK;
    RK_U32 notify = enc->notify_flag;
    RK_U32 last_wait = enc->status_flag;
    RK_U32 curr_wait = wait->val;
    RK_U32 wait_chg  = last_wait & (~curr_wait);
    RK_U32 keep_notify = 0;

    do {
        if (enc->reset_flag)
            break;

        // NOTE: User control should always be processed
        if (notify & MPP_ENC_CONTROL) {
            keep_notify = notify & (~MPP_ENC_CONTROL);
            break;
        }

        // NOTE: When condition is not fulfilled check nofify flag again
        if (!curr_wait || (curr_wait & notify))
            break;

        ret = MPP_NOK;
    } while (0);

    enc_dbg_status("%p %08x -> %08x [%08x] notify %08x -> %s\n", enc,
                   last_wait, curr_wait, wait_chg, notify, (ret) ? ("wait") : ("work"));

    enc->status_flag = wait->val;
    enc->notify_flag = keep_notify;

    if (ret) {
        enc->wait_count++;
    } else {
        enc->work_count++;
    }

    return ret;
}

static RK_S32 check_codec_to_resend_hdr(MppEncCodecCfg *codec)
{
    switch (codec->coding) {
    case MPP_VIDEO_CodingAVC : {
        if (codec->h264.change)
            return 1;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        if (codec->h265.change)
            return 1;
    } break;
    case MPP_VIDEO_CodingVP8 :
    case MPP_VIDEO_CodingMJPEG :
    default : {
    } break;
    }
    return 0;
}

static RK_S32 check_resend_hdr(MpiCmd cmd, void *param, MppEncCfgSet *cfg, RK_BOOL *encode_idr)
{
    RK_S32 resend = 0;
    static const char *resend_reason[] = {
        "unchanged",
        "codec/prep cfg change",
        "rc cfg change rc_mode/fps/gop",
        "set cfg change input/format/color",
        "set cfg change rc_mode/fps/gop",
        "set cfg change codec",
    };

    *encode_idr = RK_TRUE;

    if (cfg->codec.coding == MPP_VIDEO_CodingMJPEG) {
        *encode_idr = RK_FALSE;
        return 0;
    }

    do {
        if (cmd == MPP_ENC_SET_IDR_FRAME)
            return 1;

        if (cmd == MPP_ENC_SET_CODEC_CFG ||
            cmd == MPP_ENC_SET_PREP_CFG) {
            resend = 1;
            break;
        }

        if (cmd == MPP_ENC_SET_RC_CFG) {
            RK_U32 change = *(RK_U32 *)param;
            RK_U32 check_flag = MPP_ENC_RC_CFG_CHANGE_RC_MODE |
                                MPP_ENC_RC_CFG_CHANGE_FPS_OUT |
                                MPP_ENC_RC_CFG_CHANGE_GOP;

            if (change & check_flag) {
                resend = 2;

                if (cfg->rc.fps_chg_no_idr && (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT))
                    *encode_idr = RK_FALSE;

                break;
            }
        }

        if (cmd == MPP_ENC_SET_CFG) {
            RK_U32 change = cfg->prep.change;
            RK_U32 check_flag = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                                MPP_ENC_PREP_CFG_CHANGE_FORMAT |
                                MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                                MPP_ENC_PREP_CFG_CHANGE_COLOR_RANGE |
                                MPP_ENC_PREP_CFG_CHANGE_COLOR_SPACE |
                                MPP_ENC_PREP_CFG_CHANGE_COLOR_PRIME |
                                MPP_ENC_PREP_CFG_CHANGE_COLOR_TRC;

            if (change & check_flag) {
                resend = 3;
                break;
            }

            change = cfg->rc.change;
            check_flag = MPP_ENC_RC_CFG_CHANGE_RC_MODE |
                         MPP_ENC_RC_CFG_CHANGE_FPS_OUT |
                         MPP_ENC_RC_CFG_CHANGE_GOP;

            if (change & check_flag) {
                resend = 4;

                if (cfg->rc.fps_chg_no_idr && (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT))
                    *encode_idr = RK_FALSE;

                break;
            }
            if (check_codec_to_resend_hdr(&cfg->codec)) {
                resend = 5;
                break;
            }
        }
    } while (0);

    if (resend)
        enc_dbg_detail("send header for %s\n", resend_reason[resend]);
    else
        *encode_idr = RK_FALSE;

    return resend;
}

static RK_S32 check_rc_cfg_update(MpiCmd cmd, MppEncCfgSet *cfg)
{
    if (cmd == MPP_ENC_SET_RC_CFG ||
        cmd == MPP_ENC_SET_PREP_CFG ||
        cmd == MPP_ENC_SET_REF_CFG) {
        return 1;
    }

    if (cmd == MPP_ENC_SET_CFG) {
        RK_U32 change = cfg->prep.change;
        RK_U32 check_flag = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                            MPP_ENC_PREP_CFG_CHANGE_FORMAT;

        if (change & check_flag)
            return 1;

        change = cfg->rc.change;
        check_flag = MPP_ENC_RC_CFG_CHANGE_ALL &
                     (~MPP_ENC_RC_CFG_CHANGE_QUALITY);

        if (change & check_flag)
            return 1;
    }

    return 0;
}

static RK_S32 check_rc_gop_update(MpiCmd cmd, MppEncCfgSet *cfg)
{
    if (((cmd == MPP_ENC_SET_RC_CFG) || (cmd == MPP_ENC_SET_CFG)) &&
        (cfg->rc.change & MPP_ENC_RC_CFG_CHANGE_GOP))
        return 1;

    return 0;
}

static RK_S32 check_hal_info_update(MpiCmd cmd)
{
    if (cmd == MPP_ENC_SET_CFG ||
        cmd == MPP_ENC_SET_RC_CFG ||
        cmd == MPP_ENC_SET_CODEC_CFG ||
        cmd == MPP_ENC_SET_PREP_CFG ||
        cmd == MPP_ENC_SET_REF_CFG) {
        return 1;
    }

    return 0;
}

static void check_low_delay_part_mode(MppEncImpl *enc)
{
    MppEncCfgSet *cfg = &enc->cfg;

    enc->low_delay_part_mode = 0;

    if (!(cfg->base.low_delay))
        return;

    if (!cfg->split.split_mode)
        return;

    if (mpp_enc_hal_check_part_mode(enc->enc_hal))
        return;

    enc->low_delay_part_mode = 1;
}

static void check_low_delay_output(MppEncImpl *enc)
{
    MppEncCfgSet *cfg = &enc->cfg;

    enc->low_delay_output = 0;

    if (!cfg->split.split_mode || !(cfg->split.split_out & MPP_ENC_SPLIT_OUT_LOWDELAY))
        return;

    if (cfg->rc.max_reenc_times) {
        mpp_log_f("can not enable lowdelay output with reencode enabled\n");
        cfg->rc.max_reenc_times = 0;
    }

    if (cfg->rc.drop_mode) {
        mpp_log_f("can not enable lowdelay output with drop mode enabled\n");
        cfg->rc.drop_mode = MPP_ENC_RC_DROP_FRM_DISABLED;
    }

    if (cfg->rc.super_mode) {
        mpp_log_f("can not enable lowdelay output with super frame mode enabled\n");
        cfg->rc.super_mode = MPP_ENC_RC_SUPER_FRM_NONE;
    }

    enc->low_delay_output = 1;
}

MPP_RET mpp_enc_callback(const char *caller, void *ctx, RK_S32 cmd, void *param)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;
    EncOutParam *out = (EncOutParam *)param;
    HalEncTask *task = NULL;
    MppPacket packet = NULL;
    MppPacketImpl *impl = NULL;
    RK_U8 *last_pos = NULL;
    RK_S32 slice_length = 0;
    RK_U32 part_first = 0;
    MPP_RET ret = MPP_OK;
    Mpp *mpp = (Mpp*)enc->mpp;
    (void) caller;

    if (!enc->low_delay_output)
        return ret;

    task = (HalEncTask *)out->task;
    mpp_assert(task);
    part_first = task->part_first;
    packet = task->packet;

    if (part_first) {
        task->part_pos = (RK_U8 *)mpp_packet_get_pos(packet);
        task->part_length = mpp_packet_get_length(packet);

        enc_dbg_slice("first slice previous length %d\n", task->part_length);
        mpp_assert(task->part_pos);
        task->part_first = 0;
        slice_length = task->part_length;
    }

    last_pos = (RK_U8 *)task->part_pos;
    slice_length += out->length;

    enc_dbg_slice("last_pos %p len %d:%d\n", last_pos, out->length, slice_length);

    switch (cmd) {
    case ENC_OUTPUT_FINISH : {
        enc_dbg_slice("slice pos %p len %5d last\n", last_pos, slice_length);

        impl = (MppPacketImpl *)packet;

        impl->pos = last_pos;
        impl->length = slice_length;
        impl->status.val = 0;
        impl->status.partition = 1;
        impl->status.soi = part_first;
        impl->status.eoi = 1;

        task->part_pos += slice_length;
        task->part_length += slice_length;
        task->part_count++;
        task->part_last = 1;
    } break;
    case ENC_OUTPUT_SLICE : {
        enc_dbg_slice("slice pos %p len %5d\n", last_pos, slice_length);

        mpp_packet_new((MppPacket *)&impl);
        mpp_assert(impl);

        /* copy the source data */
        memcpy(impl, packet, sizeof(*impl));

        impl->pos = last_pos;
        impl->length = slice_length;
        impl->status.val = 0;
        impl->status.partition = 1;
        impl->status.soi = part_first;
        impl->status.eoi = 0;

        if (impl->buffer)
            mpp_buffer_inc_ref(impl->buffer);

        mpp_meta_get(&impl->meta);
        if (impl->meta) {
            EncFrmStatus *frm = &task->rc_task->frm;

            mpp_meta_set_s32(impl->meta, KEY_OUTPUT_INTRA, frm->is_intra);
        }

        mpp_packet_copy_segment_info(impl, packet);
        mpp_packet_reset_segment(packet);

        enc_dbg_detail("pkt %d new pos %p len %d\n", task->part_count,
                       last_pos, slice_length);

        task->part_pos += slice_length;
        task->part_length += slice_length;
        task->part_count++;
        if (!mpp->mEncAyncProc) {
            mpp_task_meta_set_packet(enc->task_out, KEY_OUTPUT_PACKET, impl);
            mpp_port_enqueue(enc->output, enc->task_out);

            ret = mpp_port_poll(enc->output, MPP_POLL_BLOCK);
            mpp_assert(ret > 0);
            ret = mpp_port_dequeue(enc->output, &enc->task_out);
            mpp_assert(enc->task_out);
        } else {
            if (mpp->mPktOut) {
                MppList *pkt_out = mpp->mPktOut;

                mpp_mutex_cond_lock(&pkt_out->cond_lock);
                mpp_list_add_at_tail(pkt_out, &impl, sizeof(impl));
                mpp->mPacketPutCount++;
                mpp_list_signal(pkt_out);
                mpp_mutex_cond_unlock(&pkt_out->cond_lock);
            }
        }
    } break;
    default : {
    } break;
    }

    return ret;
}

MPP_RET mpp_enc_proc_rc_cfg(MppCodingType coding, MppEncRcCfg *dst, MppEncRcCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change) {
        MppEncRcCfg bak = *dst;

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
            dst->fps_in_denom = src->fps_in_denom;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
            dst->fps_out_flex = src->fps_out_flex;
            dst->fps_out_num = src->fps_out_num;
            dst->fps_out_denom = src->fps_out_denom;
            dst->fps_chg_no_idr = src->fps_chg_no_idr;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_GOP) {
            /*
             * If GOP is changed smaller, disable Intra-Refresh
             * and User level should reconfig Intra-Refresh
             */
            if (dst->gop < src->gop && dst->refresh_en) {
                dst->refresh_en = 0;
            }
            dst->gop = src->gop;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_GOP_REF_CFG)
            dst->ref_cfg = src->ref_cfg;

        if (change & MPP_ENC_RC_CFG_CHANGE_MAX_REENC)
            dst->max_reenc_times = src->max_reenc_times;

        if (change & MPP_ENC_RC_CFG_CHANGE_DROP_FRM) {
            dst->drop_mode = src->drop_mode;
            dst->drop_threshold = src->drop_threshold;
            dst->drop_gap = src->drop_gap;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_PRIORITY) {
            if (src->rc_priority >= MPP_ENC_RC_PRIORITY_BUTT) {
                mpp_err("invalid rc_priority %d should be[%d, %d] \n",
                        src->rc_priority, MPP_ENC_RC_BY_BITRATE_FIRST, MPP_ENC_RC_PRIORITY_BUTT);
                ret = MPP_ERR_VALUE;
            }
            dst->rc_priority = src->rc_priority;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_SUPER_FRM) {
            if (src->super_mode >= MPP_ENC_RC_SUPER_FRM_BUTT) {
                mpp_err("invalid super_mode %d should be[%d, %d] \n",
                        src->super_mode, MPP_ENC_RC_SUPER_FRM_NONE, MPP_ENC_RC_SUPER_FRM_BUTT);
                ret = MPP_ERR_VALUE;
            }
            dst->super_mode = src->super_mode;
            dst->super_i_thd = src->super_i_thd;
            dst->super_p_thd = src->super_p_thd;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_DEBREATH) {
            dst->debreath_en    = src->debreath_en;
            dst->debre_strength = src->debre_strength;
            if (dst->debreath_en && dst->debre_strength > 35) {
                mpp_err("invalid debre_strength should be[%d, %d] \n", 0, 35);
                ret = MPP_ERR_VALUE;
            }
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_MAX_I_PROP)
            dst->max_i_prop = src->max_i_prop;

        if (change & MPP_ENC_RC_CFG_CHANGE_MIN_I_PROP)
            dst->min_i_prop = src->min_i_prop;

        if (change & MPP_ENC_RC_CFG_CHANGE_INIT_IP_RATIO)
            dst->init_ip_ratio = src->init_ip_ratio;

        if (change & MPP_ENC_RC_CFG_CHANGE_QP_INIT)
            dst->qp_init = src->qp_init;

        if (change & MPP_ENC_RC_CFG_CHANGE_QP_RANGE) {
            dst->qp_min = src->qp_min;
            dst->qp_max = src->qp_max;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I) {
            dst->qp_min_i = src->qp_min_i;
            dst->qp_max_i = src->qp_max_i;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP)
            dst->qp_max_step = src->qp_max_step;

        if (change & MPP_ENC_RC_CFG_CHANGE_QP_IP)
            dst->qp_delta_ip = src->qp_delta_ip;

        if (change & MPP_ENC_RC_CFG_CHANGE_QP_VI)
            dst->qp_delta_vi = src->qp_delta_vi;

        if (change & MPP_ENC_RC_CFG_CHANGE_FQP) {
            dst->fqp_min_i = src->fqp_min_i;
            dst->fqp_min_p = src->fqp_min_p;
            dst->fqp_max_i = src->fqp_max_i;
            dst->fqp_max_p = src->fqp_max_p;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_HIER_QP) {
            dst->hier_qp_en = src->hier_qp_en;
            memcpy(dst->hier_qp_delta, src->hier_qp_delta, sizeof(src->hier_qp_delta));
            memcpy(dst->hier_frame_num, src->hier_frame_num, sizeof(src->hier_frame_num));
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_ST_TIME)
            dst->stats_time = src->stats_time;

        if (change & MPP_ENC_RC_CFG_CHANGE_REFRESH) {
            if (dst->debreath_en) {
                mpp_err_f("Turn off Debreath first.");
                ret = MPP_ERR_VALUE;
            }
            dst->refresh_en = src->refresh_en;
            dst->refresh_mode = src->refresh_mode;
            // Make sure refresh_num is legal
            dst->refresh_num = src->refresh_num;
        }

        // parameter checking
        if (dst->rc_mode >= MPP_ENC_RC_MODE_BUTT) {
            mpp_err("invalid rc mode %d should be RC_MODE_VBR or RC_MODE_CBR\n",
                    src->rc_mode);
            ret = MPP_ERR_VALUE;
        }
        if (dst->quality >= MPP_ENC_RC_QUALITY_BUTT) {
            mpp_err("invalid quality %d should be from QUALITY_WORST to QUALITY_BEST\n",
                    dst->quality);
            ret = MPP_ERR_VALUE;
        }

        if (dst->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
            RK_S32 bps_min = MPP_ENC_MIN_BPS;
            RK_S32 bps_max = MPP_ENC_MAX_BPS;

            if (coding == MPP_VIDEO_CodingMJPEG) {
                bps_min *= 4;
                bps_max *= 4;
                if (bps_max < 0)
                    bps_max = INT_MAX;
            }

            if ((dst->bps_target >= bps_max || dst->bps_target <= bps_min) ||
                (dst->bps_max    >= bps_max || dst->bps_max    <= bps_min) ||
                (dst->bps_min    >= bps_max || dst->bps_min    <= bps_min)) {
                mpp_err("invalid bit per second %x:%u min %x:%u max %x:%u out of range %dK~%dM\n",
                        dst->bps_target, dst->bps_target, dst->bps_min,
                        dst->bps_min, dst->bps_max, dst->bps_max,
                        bps_min / SZ_1K,  bps_max / SZ_1M);
                ret = MPP_ERR_VALUE;
            }
        }

        if (dst->fps_in_num < 0 || dst->fps_in_denom < 0 ||
            dst->fps_out_num < 0 || dst->fps_out_denom < 0) {
            mpp_err("invalid fps cfg [number:denom:flex]: in [%d:%d:%d] out [%d:%d:%d]\n",
                    dst->fps_in_num, dst->fps_in_denom, dst->fps_in_flex,
                    dst->fps_out_num, dst->fps_out_denom, dst->fps_out_flex);
            ret = MPP_ERR_VALUE;
        }

        // if I frame min/max is not set use normal case
        if (dst->qp_min_i <= 0)
            dst->qp_min_i = dst->qp_min;
        if (dst->qp_max_i <= 0)
            dst->qp_max_i = dst->qp_max;
        if (dst->qp_min < 0 || dst->qp_max < 0 || dst->qp_min > dst->qp_max ||
            dst->qp_min_i < 0 || dst->qp_max_i < 0 ||
            dst->qp_min_i > dst->qp_max_i ||
            (dst->qp_init > 0 &&
             (dst->qp_init > dst->qp_max_i || dst->qp_init < dst->qp_min_i))) {
            mpp_err("invalid qp range: init %d i [%d:%d] p [%d:%d]\n",
                    dst->qp_init, dst->qp_min_i, dst->qp_max_i,
                    dst->qp_min, dst->qp_max);

            dst->qp_init  = bak.qp_init;
            dst->qp_min_i = bak.qp_min_i;
            dst->qp_max_i = bak.qp_max_i;
            dst->qp_min   = bak.qp_min;
            dst->qp_max   = bak.qp_max;

            mpp_err("restore qp range: init %d i [%d:%d] p [%d:%d]\n",
                    dst->qp_init, dst->qp_min_i, dst->qp_max_i,
                    dst->qp_min, dst->qp_max);
        }
        if (MPP_ABS(dst->qp_delta_ip) > 8) {
            mpp_err("invalid qp delta ip %d restore to %d\n",
                    dst->qp_delta_ip, bak.qp_delta_ip);
            dst->qp_delta_ip = bak.qp_delta_ip;
        }
        if (MPP_ABS(dst->qp_delta_vi) > 6) {
            mpp_err("invalid qp delta vi %d restore to %d\n",
                    dst->qp_delta_vi, bak.qp_delta_vi);
            dst->qp_delta_vi = bak.qp_delta_vi;
        }
        if (dst->qp_max_step < 0) {
            mpp_err("invalid qp max step %d restore to %d\n",
                    dst->qp_max_step, bak.qp_max_step);
            dst->qp_max_step = bak.qp_max_step;
        }
        if (dst->stats_time && dst->stats_time > 60) {
            mpp_err("warning: bitrate statistic time %d is larger than 60s\n",
                    dst->stats_time);
        }

        dst->change |= change;

        if (ret) {
            mpp_err_f("failed to accept new rc config\n");
            *dst = bak;
        } else {
            mpp_log("MPP_ENC_SET_RC_CFG bps %d [%d : %d] fps [%d:%d] gop %d\n",
                    dst->bps_target, dst->bps_min, dst->bps_max,
                    dst->fps_in_num, dst->fps_out_num, dst->gop);
        }
    }

    return ret;
}

MPP_RET mpp_enc_proc_hw_cfg(MppEncHwCfg *dst, MppEncHwCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change) {
        MppEncHwCfg bak = *dst;

        if (change & MPP_ENC_HW_CFG_CHANGE_QP_ROW)
            dst->qp_delta_row = src->qp_delta_row;

        if (change & MPP_ENC_HW_CFG_CHANGE_QP_ROW_I)
            dst->qp_delta_row_i = src->qp_delta_row_i;

        if (change & MPP_ENC_HW_CFG_CHANGE_AQ_THRD_I)
            memcpy(dst->aq_thrd_i, src->aq_thrd_i, sizeof(dst->aq_thrd_i));

        if (change & MPP_ENC_HW_CFG_CHANGE_AQ_THRD_P)
            memcpy(dst->aq_thrd_p, src->aq_thrd_p, sizeof(dst->aq_thrd_p));

        if (change & MPP_ENC_HW_CFG_CHANGE_AQ_STEP_I)
            memcpy(dst->aq_step_i, src->aq_step_i, sizeof(dst->aq_step_i));

        if (change & MPP_ENC_HW_CFG_CHANGE_AQ_STEP_P)
            memcpy(dst->aq_step_p, src->aq_step_p, sizeof(dst->aq_step_p));

        if (change & MPP_ENC_HW_CFG_CHANGE_QBIAS_I)
            dst->qbias_i = src->qbias_i;

        if (change & MPP_ENC_HW_CFG_CHANGE_QBIAS_P)
            dst->qbias_p = src->qbias_p;

        if (change & MPP_ENC_HW_CFG_CHANGE_QBIAS_EN)
            dst->qbias_en = src->qbias_en;

        if (change & MPP_ENC_HW_CFG_CHANGE_MB_RC)
            dst->mb_rc_disable = src->mb_rc_disable;

        if (change & MPP_ENC_HW_CFG_CHANGE_CU_MODE_BIAS)
            memcpy(dst->mode_bias, src->mode_bias, sizeof(dst->mode_bias));

        if (change & MPP_ENC_HW_CFG_CHANGE_CU_SKIP_BIAS) {
            dst->skip_bias_en = src->skip_bias_en;
            dst->skip_sad = src->skip_sad;
            dst->skip_bias = src->skip_bias;
        }

        if (dst->qp_delta_row < 0 || dst->qp_delta_row_i < 0) {
            mpp_err("invalid hw qp delta row [%d:%d]\n",
                    dst->qp_delta_row_i, dst->qp_delta_row);
            ret = MPP_ERR_VALUE;
        }

        dst->change |= change;

        if (ret) {
            mpp_err_f("failed to accept new hw config\n");
            *dst = bak;
        }
    }

    return ret;
}

MPP_RET mpp_enc_proc_tune_cfg(MppEncFineTuneCfg *dst, MppEncFineTuneCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change) {
        MppEncFineTuneCfg bak = *dst;

        if (change & MPP_ENC_TUNE_CFG_CHANGE_SCENE_MODE)
            dst->scene_mode = src->scene_mode;

        if (dst->scene_mode < MPP_ENC_SCENE_MODE_DEFAULT ||
            dst->scene_mode >= MPP_ENC_SCENE_MODE_BUTT) {
            mpp_err("invalid scene mode %d not in range [%d, %d]\n", dst->scene_mode,
                    MPP_ENC_SCENE_MODE_DEFAULT, MPP_ENC_SCENE_MODE_BUTT - 1);
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_SE_MODE)
            dst->se_mode = src->se_mode;

        if (change & MPP_ENC_TUNE_CFG_CHANGE_DEBLUR_EN)
            dst->deblur_en = src->deblur_en;

        if (change & MPP_ENC_TUNE_CFG_CHANGE_DEBLUR_STR)
            dst->deblur_str = src->deblur_str;

        if (dst->deblur_str < 0 || dst->deblur_str > 7) {
            mpp_err("invalid deblur strength not in range [0, 7]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_ANTI_FLICKER_STR)
            dst->anti_flicker_str = src->anti_flicker_str;

        if (dst->anti_flicker_str < 0 || dst->anti_flicker_str > 3) {
            mpp_err("invalid anti_flicker_str not in range [0 : 3]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_ATR_STR_I)
            dst->atr_str_i = src->atr_str_i;

        if (dst->atr_str_i < 0 || dst->atr_str_i > 3) {
            mpp_err("invalid atr_str not in range [0 : 3]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_ATR_STR_P)
            dst->atr_str_p = src->atr_str_p;

        if (dst->atr_str_p < 0 || dst->atr_str_p > 3) {
            mpp_err("invalid atr_str not in range [0 : 3]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_ATL_STR)
            dst->atl_str = src->atl_str;

        if (dst->atl_str < 0 || dst->atl_str > 3) {
            mpp_err("invalid atr_str not in range [0 : 3]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_SAO_STR_I)
            dst->sao_str_i = src->sao_str_i;

        if (dst->sao_str_i < 0 || dst->sao_str_i > 3) {
            mpp_err("invalid atr_str not in range [0 : 3]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_SAO_STR_P)
            dst->sao_str_p = src->sao_str_p;

        if (dst->sao_str_p < 0 || dst->sao_str_p > 3) {
            mpp_err("invalid atr_str not in range [0 : 3]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_LAMBDA_IDX_I)
            dst->lambda_idx_i = src->lambda_idx_i;

        if (dst->lambda_idx_i < 0 || dst->lambda_idx_i > 8) {
            mpp_err("invalid lambda idx i not in range [0, 8]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_LAMBDA_IDX_P)
            dst->lambda_idx_p = src->lambda_idx_p;

        if (dst->lambda_idx_p < 0 || dst->lambda_idx_p > 8) {
            mpp_err("invalid lambda idx i not in range [0, 8]\n");
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_RC_CONTAINER)
            dst->rc_container = src->rc_container;

        if (dst->rc_container < 0 || dst->rc_container > 2) {
            mpp_err("invalid rc_container %d not in range [0, 2]\n", dst->rc_container);
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_VMAF_OPT)
            dst->vmaf_opt = src->vmaf_opt;

        if (dst->vmaf_opt < 0 || dst->vmaf_opt > 1) {
            mpp_err("invalid vmaf_opt %d not in range [0, 1]\n", dst->vmaf_opt);
            ret = MPP_ERR_VALUE;
        }

        if (change & MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG) {
            dst->bg_delta_qp_i = src->bg_delta_qp_i;
            dst->bg_delta_qp_p = src->bg_delta_qp_p;
            dst->fg_delta_qp_i = src->fg_delta_qp_i;
            dst->fg_delta_qp_p = src->fg_delta_qp_p;
            dst->bmap_qpmin_i = src->bmap_qpmin_i;
            dst->bmap_qpmin_p = src->bmap_qpmin_p;
            dst->bmap_qpmax_i = src->bmap_qpmax_i;
            dst->bmap_qpmax_p = src->bmap_qpmax_p;
            dst->min_bg_fqp = src->min_bg_fqp;
            dst->max_bg_fqp = src->max_bg_fqp;
            dst->min_fg_fqp = src->min_fg_fqp;
            dst->max_fg_fqp = src->max_fg_fqp;
        }

        dst->change |= change;

        if (ret) {
            mpp_err_f("failed to accept new tuning config\n");
            *dst = bak;
        }
    }

    return ret;
}

static MPP_RET mpp_enc_control_set_ref_cfg(MppEncImpl *enc, void *param)
{
    MPP_RET ret = MPP_OK;
    MppEncRefCfg src = (MppEncRefCfg)param;
    MppEncRefCfg dst = enc->cfg.ref_cfg;

    if (NULL == src)
        src = mpp_enc_ref_default();

    if (NULL == dst) {
        mpp_enc_ref_cfg_init(&dst);
        enc->cfg.ref_cfg = dst;
    }

    ret = mpp_enc_ref_cfg_copy(dst, src);
    if (ret) {
        mpp_err_f("failed to copy ref cfg ret %d\n", ret);
    }

    ret = mpp_enc_refs_set_cfg(enc->refs, dst);
    if (ret) {
        mpp_err_f("failed to set ref cfg ret %d\n", ret);
    }

    if (mpp_enc_refs_update_hdr(enc->refs))
        enc->hdr_status.val = 0;

    return ret;
}

MPP_RET mpp_enc_proc_cfg(MppEncImpl *enc, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    RK_BOOL encode_idr = RK_FALSE;

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        MppEncCfgSet *src = (MppEncCfgSet *)param;
        RK_U32 change = src->base.change;
        MPP_RET ret_tmp = MPP_OK;

        /* get base cfg here */
        if (change) {
            MppEncCfgSet *dst = &enc->cfg;

            if (change & MPP_ENC_BASE_CFG_CHANGE_LOW_DELAY)
                dst->base.low_delay = src->base.low_delay;

            src->base.change = 0;
        }

        /* process rc cfg at mpp_enc module */
        if (src->rc.change) {
            ret_tmp = mpp_enc_proc_rc_cfg(enc->coding, &enc->cfg.rc, &src->rc);
            if (ret_tmp != MPP_OK)
                ret = ret_tmp;
            // update ref cfg
            if ((enc->cfg.rc.change & MPP_ENC_RC_CFG_CHANGE_GOP_REF_CFG) &&
                (enc->cfg.rc.gop > 0))
                mpp_enc_control_set_ref_cfg(enc, enc->cfg.rc.ref_cfg);

            src->rc.change = 0;
        }

        /* process hardware cfg at mpp_enc module */
        if (src->hw.change) {
            ret_tmp = mpp_enc_proc_hw_cfg(&enc->cfg.hw, &src->hw);
            if (ret_tmp != MPP_OK)
                ret = ret_tmp;
            src->hw.change = 0;
        }

        /* process hardware cfg at mpp_enc module */
        if (src->tune.change) {
            ret_tmp = mpp_enc_proc_tune_cfg(&enc->cfg.tune, &src->tune);
            if (ret_tmp != MPP_OK)
                ret = ret_tmp;
            src->tune.change = 0;
        }

        /* Then process the rest config */
        ret_tmp = enc_impl_proc_cfg(enc->impl, cmd, param);
        if (ret_tmp != MPP_OK)
            ret = ret_tmp;
    } break;
    case MPP_ENC_SET_RC_CFG : {
        MppEncRcCfg *src = (MppEncRcCfg *)param;
        if (src) {
            ret = mpp_enc_proc_rc_cfg(enc->coding, &enc->cfg.rc, src);

            // update ref cfg
            if ((enc->cfg.rc.change & MPP_ENC_RC_CFG_CHANGE_GOP_REF_CFG) &&
                (enc->cfg.rc.gop > 0))
                mpp_enc_control_set_ref_cfg(enc, enc->cfg.rc.ref_cfg);
        }
    } break;
    case MPP_ENC_SET_IDR_FRAME : {
        enc->frm_cfg.force_idr++;
    } break;
    case MPP_ENC_GET_HDR_SYNC :
    case MPP_ENC_GET_EXTRA_INFO : {
        /*
         * NOTE: get stream header should use user's MppPacket
         * If we provide internal MppPacket to external user
         * we do not known when the buffer usage is finished.
         * So encoder always write its header to external buffer
         * which is provided by user.
         */
        if (!enc->hdr_status.ready) {
            enc_impl_gen_hdr(enc->impl, enc->hdr_pkt);
            enc->hdr_len = mpp_packet_get_length(enc->hdr_pkt);
            enc->hdr_status.ready = 1;
        }

        if (cmd == MPP_ENC_GET_EXTRA_INFO) {
            mpp_err("Please use MPP_ENC_GET_HDR_SYNC instead of unsafe MPP_ENC_GET_EXTRA_INFO\n");
            mpp_err("NOTE: MPP_ENC_GET_HDR_SYNC needs MppPacket input\n");

            *(MppPacket *)param = enc->hdr_pkt;
        } else {
            mpp_packet_copy((MppPacket)param, enc->hdr_pkt);
        }

        if (enc->hdr_pkt) {
            Mpp *mpp = (Mpp *)enc->mpp;
            // dump output
            mpp_ops_enc_get_pkt(mpp->mDump, enc->hdr_pkt);
        }

        enc->hdr_status.added_by_ctrl = 1;
    } break;
    case MPP_ENC_PRE_ALLOC_BUFF : {
        /* deprecated control */
        mpp_log("deprecated MPP_ENC_PRE_ALLOC_BUFF control\n");
    } break;
    case MPP_ENC_GET_RC_API_ALL : {
        RcApiQueryAll *query = (RcApiQueryAll *)param;

        rc_brief_get_all(query);
    } break;
    case MPP_ENC_GET_RC_API_BY_TYPE : {
        RcApiQueryType *query = (RcApiQueryType *)param;

        rc_brief_get_by_type(query);
    } break;
    case MPP_ENC_SET_RC_API_CFG : {
        const RcImplApi *api = (const RcImplApi *)param;

        rc_api_add(api);
    } break;
    case MPP_ENC_GET_RC_API_CURRENT : {
        RcApiBrief *dst = (RcApiBrief *)param;

        *dst = enc->rc_brief;
    } break;
    case MPP_ENC_SET_RC_API_CURRENT : {
        RcApiBrief *src = (RcApiBrief *)param;

        mpp_assert(src->type == enc->coding);
        enc->rc_brief = *src;
        enc->rc_status.rc_api_user_cfg = 1;
        enc->rc_status.rc_api_updated = 1;
    } break;
    case MPP_ENC_SET_HEADER_MODE : {
        if (param) {
            MppEncHeaderMode mode = *((MppEncHeaderMode *)param);

            if (mode < MPP_ENC_HEADER_MODE_BUTT) {
                enc->hdr_mode = mode;
                enc_dbg_ctrl("header mode set to %d\n", mode);
            } else {
                mpp_err_f("invalid header mode %d\n", mode);
                ret = MPP_NOK;
            }
        } else {
            mpp_err_f("invalid NULL ptr on setting header mode\n");
            ret = MPP_NOK;
        }
    } break;
    case MPP_ENC_SET_SEI_CFG : {
        if (param) {
            MppEncSeiMode mode = *((MppEncSeiMode *)param);

            if (mode <= MPP_ENC_SEI_MODE_ONE_FRAME) {
                enc->sei_mode = mode;
                enc_dbg_ctrl("sei mode set to %d\n", mode);
            } else {
                mpp_err_f("invalid sei mode %d\n", mode);
                ret = MPP_NOK;
            }
        } else {
            mpp_err_f("invalid NULL ptr on setting header mode\n");
            ret = MPP_NOK;
        }
    } break;
    case MPP_ENC_SET_REF_CFG : {
        ret = mpp_enc_control_set_ref_cfg(enc, param);
    } break;
    case MPP_ENC_SET_OSD_PLT_CFG : {
        MppEncOSDPltCfg *src = (MppEncOSDPltCfg *)param;
        MppEncOSDPltCfg *dst = &enc->cfg.plt_cfg;
        RK_U32 change = src->change;

        if (change) {
            RK_S32 cfg_err = 0;

            if (change & MPP_ENC_OSD_PLT_CFG_CHANGE_MODE) {
                if (src->type >= MPP_ENC_OSD_PLT_TYPE_BUTT) {
                    mpp_err_f("invalid osd plt type %d\n", src->type);
                    cfg_err |= MPP_ENC_OSD_PLT_CFG_CHANGE_MODE;
                } else
                    dst->type = src->type;
            }

            if (change & MPP_ENC_OSD_PLT_CFG_CHANGE_PLT_VAL) {
                if (src->plt == NULL) {
                    mpp_err_f("invalid osd plt NULL pointer\n");
                    cfg_err |= MPP_ENC_OSD_PLT_CFG_CHANGE_PLT_VAL;
                } else {
                    memcpy(dst->plt, src->plt, sizeof(MppEncOSDPlt));
                }
            }

            dst->change = cfg_err ? 0 : change;
            enc_dbg_ctrl("plt type %d data %p\n", dst->type, src->plt);
        }
    } break;
    default : {
        ret = enc_impl_proc_cfg(enc->impl, cmd, param);
    } break;
    }

    if (check_resend_hdr(cmd, param, &enc->cfg, &encode_idr)) {
        if (encode_idr)
            enc->frm_cfg.force_flag |= ENC_FORCE_IDR;

        enc->hdr_status.val = 0;
    }
    if (check_rc_cfg_update(cmd, &enc->cfg))
        enc->rc_status.rc_api_user_cfg = 1;
    if (check_rc_gop_update(cmd, &enc->cfg))
        mpp_enc_refs_set_rc_igop(enc->refs, enc->cfg.rc.gop);

    if (enc->cfg.rc.refresh_en)
        mpp_enc_refs_set_refresh_length(enc->refs, enc->cfg.rc.refresh_length);

    if (check_hal_info_update(cmd))
        enc->hal_info_updated = 0;

    check_low_delay_part_mode(enc);
    check_low_delay_output(enc);

    return ret;
}

static const char *name_of_rc_mode[] = {
    "vbr",
    "cbr",
    "fixqp",
    "avbr",
    "smtrc",
    "sp_enc"
};

static void update_rc_cfg_log(MppEncImpl *impl, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    RK_S32 size = impl->rc_cfg_size;
    RK_S32 length = impl->rc_cfg_length;
    char *base = impl->rc_cfg_info + length;

    length += vsnprintf(base, size - length, fmt, args);
    if (length >= size)
        mpp_log_f("rc cfg log is full\n");

    impl->rc_cfg_length = length;

    va_end(args);
}

static void update_user_datas(EncImpl impl, MppPacket packet, MppFrame frame, HalEncTask *hal_task)
{
    MppMeta frm_meta = mpp_frame_get_meta(frame);
    MppEncUserData *user_data = NULL;
    MppEncUserDataSet *user_datas = NULL;
    RK_S32 length = 0;

    mpp_meta_get_ptr(frm_meta, KEY_USER_DATA, (void**)&user_data);
    if (user_data) {
        if (user_data->pdata && user_data->len) {
            enc_impl_add_prefix(impl, packet, &length, uuid_usr_data,
                                user_data->pdata, user_data->len);

            hal_task->sei_length += length;
            hal_task->length += length;
        } else
            mpp_err_f("failed to insert user data %p len %d\n",
                      user_data->pdata, user_data->len);
    }

    mpp_meta_get_ptr(frm_meta, KEY_USER_DATAS, (void**)&user_datas);
    if (user_datas && user_datas->count) {
        RK_U32 i = 0;

        for (i = 0; i < user_datas->count; i++) {
            MppEncUserDataFull *user_data_v2 = &user_datas->datas[i];
            if (user_data_v2->pdata && user_data_v2->len) {
                if (user_data_v2->uuid)
                    enc_impl_add_prefix(impl, packet, &length, user_data_v2->uuid,
                                        user_data_v2->pdata, user_data_v2->len);
                else
                    enc_impl_add_prefix(impl, packet, &length, uuid_debug_info,
                                        user_data_v2->pdata, user_data_v2->len);

                hal_task->sei_length += length;
                hal_task->length += length;
            }
        }
    }
}

static void set_rc_cfg(RcCfg *cfg, MppEncCfgSet *cfg_set)
{
    MppEncRcCfg *rc = &cfg_set->rc;
    MppEncPrepCfg *prep = &cfg_set->prep;
    MppEncCodecCfg *codec = &cfg_set->codec;
    MppEncRefCfgImpl *ref = (MppEncRefCfgImpl *)cfg_set->ref_cfg;
    MppEncCpbInfo *info = &ref->cpb_info;

    cfg->width = prep->width;
    cfg->height = prep->height;

    switch (rc->rc_mode) {
    case MPP_ENC_RC_MODE_CBR : {
        cfg->mode = RC_CBR;
    } break;
    case MPP_ENC_RC_MODE_VBR : {
        cfg->mode = RC_VBR;
    } break;
    case MPP_ENC_RC_MODE_AVBR : {
        cfg->mode = RC_AVBR;
    } break;
    case MPP_ENC_RC_MODE_FIXQP: {
        cfg->mode = RC_FIXQP;
    } break;
    case MPP_ENC_RC_MODE_SMTRC: {
        cfg->mode = RC_SMT;
    } break;
    case MPP_ENC_RC_MODE_SE: {
        cfg->mode = RC_SE;
    } break;
    default : {
        cfg->mode = RC_AVBR;
    } break;
    }

    cfg->fps.fps_in_flex    = rc->fps_in_flex;
    cfg->fps.fps_in_num     = rc->fps_in_num;
    cfg->fps.fps_in_denom  = rc->fps_in_denom;
    cfg->fps.fps_out_flex   = rc->fps_out_flex;
    cfg->fps.fps_out_num    = rc->fps_out_num;
    cfg->fps.fps_out_denom = rc->fps_out_denom;
    cfg->igop               = rc->gop;
    cfg->max_i_bit_prop     = rc->max_i_prop;
    cfg->min_i_bit_prop     = rc->min_i_prop;
    cfg->init_ip_ratio      = rc->init_ip_ratio;

    cfg->bps_target = rc->bps_target;
    cfg->bps_max    = rc->bps_max;
    cfg->bps_min    = rc->bps_min;
    cfg->scene_mode = cfg_set->tune.scene_mode;
    cfg->rc_container = cfg_set->tune.rc_container;

    cfg->hier_qp_cfg.hier_qp_en = rc->hier_qp_en;
    memcpy(cfg->hier_qp_cfg.hier_frame_num, rc->hier_frame_num, sizeof(rc->hier_frame_num));
    memcpy(cfg->hier_qp_cfg.hier_qp_delta, rc->hier_qp_delta, sizeof(rc->hier_qp_delta));

    mpp_assert(rc->fps_out_num);
    cfg->stats_time = rc->stats_time ? rc->stats_time : 3;
    cfg->stats_time = mpp_clip(cfg->stats_time, 1, 60);

    /* quality configure */
    switch (codec->coding) {
    case MPP_VIDEO_CodingAVC :
    case MPP_VIDEO_CodingHEVC :
    case MPP_VIDEO_CodingVP8 : {
        cfg->init_quality = rc->qp_init;
        cfg->max_quality = rc->qp_max;
        cfg->min_quality = rc->qp_min;
        cfg->max_i_quality = rc->qp_max_i ? rc->qp_max_i : rc->qp_max;
        cfg->min_i_quality = rc->qp_min_i ? rc->qp_min_i : rc->qp_min;
        cfg->i_quality_delta = rc->qp_delta_ip;
        cfg->vi_quality_delta = rc->qp_delta_vi;
        cfg->fqp_min_p = rc->fqp_min_p == INT_MAX ? cfg->min_quality : rc->fqp_min_p;
        cfg->fqp_min_i = rc->fqp_min_i == INT_MAX ? cfg->min_i_quality : rc->fqp_min_i;
        cfg->fqp_max_p = rc->fqp_max_p == INT_MAX ? cfg->max_quality : rc->fqp_max_p;
        cfg->fqp_max_i = rc->fqp_max_i == INT_MAX ? cfg->max_i_quality : rc->fqp_max_i;
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        MppEncJpegCfg *jpeg = &codec->jpeg;

        cfg->init_quality = jpeg->q_factor;
        cfg->max_quality = jpeg->qf_max;
        cfg->min_quality = jpeg->qf_min;
        cfg->max_i_quality = jpeg->qf_max;
        cfg->min_i_quality = jpeg->qf_min;
        cfg->fqp_min_i = 100 - jpeg->qf_max;
        cfg->fqp_max_i = 100 - jpeg->qf_min;
        cfg->fqp_min_p = 100 - jpeg->qf_max;
        cfg->fqp_max_p = 100 - jpeg->qf_min;
    } break;
    default : {
        mpp_err_f("unsupport coding type %d\n", codec->coding);
    } break;
    }

    cfg->layer_bit_prop[0] = 256;
    cfg->layer_bit_prop[1] = 0;
    cfg->layer_bit_prop[2] = 0;
    cfg->layer_bit_prop[3] = 0;

    cfg->max_reencode_times = rc->max_reenc_times;
    cfg->drop_mode = rc->drop_mode;
    cfg->drop_thd = rc->drop_threshold;
    cfg->drop_gap = rc->drop_gap;

    cfg->super_cfg.rc_priority = rc->rc_priority;
    cfg->super_cfg.super_mode = rc->super_mode;
    cfg->super_cfg.super_i_thd = rc->super_i_thd;
    cfg->super_cfg.super_p_thd = rc->super_p_thd;

    cfg->debreath_cfg.enable   = rc->debreath_en;
    cfg->debreath_cfg.strength = rc->debre_strength;

    cfg->refresh_len = rc->refresh_length;

    if (info->st_gop) {
        cfg->vgop = info->st_gop;
        if (cfg->vgop >= rc->fps_out_num / rc->fps_out_denom &&
            cfg->vgop < cfg->igop ) {
            cfg->gop_mode = SMART_P;
            if (!cfg->vi_quality_delta)
                cfg->vi_quality_delta = 2;
        }
    }

    if (codec->coding == MPP_VIDEO_CodingAVC ||
        codec->coding == MPP_VIDEO_CodingHEVC) {
        mpp_log("mode %s bps [%d:%d:%d] fps %s [%d/%d] -> %s [%d/%d] gop i [%d] v [%d]\n",
                name_of_rc_mode[cfg->mode],
                rc->bps_min, rc->bps_target, rc->bps_max,
                cfg->fps.fps_in_flex ? "flex" : "fix",
                cfg->fps.fps_in_num, cfg->fps.fps_in_denom,
                cfg->fps.fps_out_flex ? "flex" : "fix",
                cfg->fps.fps_out_num, cfg->fps.fps_out_denom,
                cfg->igop, cfg->vgop);
    }
}

MPP_RET mpp_enc_proc_rc_update(MppEncImpl *enc)
{
    MPP_RET ret = MPP_OK;

    // check and update rate control api
    if (!enc->rc_status.rc_api_inited || enc->rc_status.rc_api_updated) {
        RcApiBrief *brief = &enc->rc_brief;

        if (enc->rc_ctx) {
            enc_dbg_detail("rc deinit %p\n", enc->rc_ctx);
            rc_deinit(enc->rc_ctx);
            enc->rc_ctx = NULL;
        }

        /* NOTE: default name is NULL */
        ret = rc_init(&enc->rc_ctx, enc->coding, &brief->name);
        if (ret)
            mpp_err("enc %p fail to init rc %s\n", enc, brief->name);
        else
            enc->rc_status.rc_api_inited = 1;

        enc_dbg_detail("rc init %p name %s ret %d\n", enc->rc_ctx, brief->name, ret);
        enc->rc_status.rc_api_updated = 0;

        enc->rc_cfg_length = 0;
        update_rc_cfg_log(enc, "%s:", brief->name);
        enc->rc_cfg_pos = enc->rc_cfg_length;
    }

    // check and update rate control config
    if (enc->rc_status.rc_api_user_cfg) {
        MppEncCfgSet *cfg = &enc->cfg;
        MppEncRcCfg *rc_cfg = &cfg->rc;
        MppEncPrepCfg *prep_cfg = &cfg->prep;
        RcCfg usr_cfg;

        enc_dbg_detail("rc update cfg start\n");

        memset(&usr_cfg, 0 , sizeof(usr_cfg));
        set_rc_cfg(&usr_cfg, cfg);
        ret = rc_update_usr_cfg(enc->rc_ctx, &usr_cfg);
        rc_cfg->change = 0;
        prep_cfg->change = 0;

        enc_dbg_detail("rc update cfg done\n");
        enc->rc_status.rc_api_user_cfg = 0;

        enc->rc_cfg_length = enc->rc_cfg_pos;
        update_rc_cfg_log(enc, "%s-b:%d[%d:%d]-g:%d-q:%d:[%d:%d]:[%d:%d]:%d\n",
                          name_of_rc_mode[usr_cfg.mode],
                          usr_cfg.bps_target,
                          usr_cfg.bps_min, usr_cfg.bps_max, usr_cfg.igop,
                          usr_cfg.init_quality,
                          usr_cfg.min_quality, usr_cfg.max_quality,
                          usr_cfg.min_i_quality, usr_cfg.max_i_quality,
                          usr_cfg.i_quality_delta);
    }

    return ret;
}

#define ENC_RUN_FUNC2(func, ctx, task, mpp, ret)        \
    ret = func(ctx, task);                              \
    if (ret) {                                          \
        mpp_err("mpp %p "#func":%-4d failed return %d", \
                mpp, __LINE__, ret);                    \
        goto TASK_DONE;                                 \
    }

static MPP_RET mpp_enc_check_frm_pkt(MppEncImpl *enc)
{
    enc->frm_buf = NULL;
    enc->pkt_buf = NULL;

    if (enc->packet)
        enc->pkt_buf = mpp_packet_get_buffer(enc->packet);
    else
        mpp_packet_new(&enc->packet);

    if (enc->frame) {
        RK_S64 pts = mpp_frame_get_pts(enc->frame);
        MppBuffer frm_buf = mpp_frame_get_buffer(enc->frame);

        enc->task_pts = pts;
        enc->frm_buf = frm_buf;

        mpp_packet_set_pts(enc->packet, pts);
        mpp_packet_set_dts(enc->packet, mpp_frame_get_dts(enc->frame));

        if (mpp_frame_get_eos(enc->frame))
            mpp_packet_set_eos(enc->packet);
        else
            mpp_packet_clr_eos(enc->packet);
    }

    return (NULL == enc->frame || NULL == enc->frm_buf) ? MPP_NOK : MPP_OK;
}

static MPP_RET mpp_enc_check_pkt_buf(MppEncImpl *enc)
{
    if (NULL == enc->pkt_buf) {
        /* NOTE: set buffer w * h * 1.5 to avoid buffer overflow */
        Mpp *mpp = (Mpp *)enc->mpp;
        MppEncPrepCfg *prep = &enc->cfg.prep;
        RK_U32 width  = MPP_ALIGN(prep->width, 16);
        RK_U32 height = MPP_ALIGN(prep->height, 16);
        RK_U32 size = (enc->coding == MPP_VIDEO_CodingMJPEG) ?
                      (width * height * 3 / 2) : (width * height);
        MppPacketImpl *pkt = (MppPacketImpl *)enc->packet;
        MppBuffer buffer = NULL;

        mpp_assert(size);
        mpp_buffer_get(mpp->mPacketGroup, &buffer, size);
        mpp_buffer_attach_dev(buffer, enc->dev);
        mpp_assert(buffer);
        enc->pkt_buf = buffer;
        pkt->data   = mpp_buffer_get_ptr(buffer);
        pkt->pos    = pkt->data;
        pkt->size   = mpp_buffer_get_size(buffer);
        pkt->length = 0;
        pkt->buffer = buffer;

        enc_dbg_detail("create output pkt %p buf %p\n", enc->packet, buffer);
    } else {
        enc_dbg_detail("output to pkt %p buf %p pos %p length %d\n",
                       enc->packet, enc->pkt_buf,
                       mpp_packet_get_pos(enc->packet),
                       mpp_packet_get_length(enc->packet));
    }

    return MPP_OK;
}

static MPP_RET mpp_enc_proc_two_pass(Mpp *mpp, EncAsyncTaskInfo *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MPP_RET ret = MPP_OK;

    if (mpp_enc_refs_next_frm_is_intra(enc->refs)) {
        EncRcTask *rc_task = &task->rc;
        EncFrmStatus frm_bak = rc_task->frm;
        EncRcTaskInfo rc_info = rc_task->info;
        EncCpbStatus *cpb = &rc_task->cpb;
        EncFrmStatus *frm = &rc_task->frm;
        HalEncTask *hal_task = &task->task;
        EncImpl impl = enc->impl;
        MppEncHal hal = enc->enc_hal;
        MppPacket packet = hal_task->packet;
        RK_S32 task_len = hal_task->length;
        RK_S32 hw_len = hal_task->hw_length;
        RK_S32 pkt_len = mpp_packet_get_length(packet);

        enc_dbg_detail("task %d two pass mode enter\n", frm->seq_idx);
        rc_task->info = enc->rc_info_prev;
        hal_task->segment_nb = mpp_packet_get_segment_nb(hal_task->packet);

        enc_dbg_detail("task %d enc proc dpb\n", frm->seq_idx);
        mpp_enc_refs_get_cpb_pass1(enc->refs, cpb);

        enc_dbg_frm_status("frm %d start ***********************************\n", cpb->curr.seq_idx);
        ENC_RUN_FUNC2(enc_impl_proc_dpb, impl, hal_task, mpp, ret);

        enc_dbg_detail("task %d enc proc hal\n", frm->seq_idx);
        ENC_RUN_FUNC2(enc_impl_proc_hal, impl, hal_task, mpp, ret);

        enc_dbg_detail("task %d hal get task\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_get_task, hal, hal_task, mpp, ret);

        enc_dbg_detail("task %d hal generate reg\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_gen_regs, hal, hal_task, mpp, ret);

        enc_dbg_detail("task %d hal start\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_start, hal, hal_task, mpp, ret);

        enc_dbg_detail("task %d hal wait\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_wait,  hal, hal_task, mpp, ret);

        enc_dbg_detail("task %d hal ret task\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_ret_task, hal, hal_task, mpp, ret);

        //recover status & packet
        mpp_packet_set_length(packet, pkt_len);
        mpp_packet_set_segment_nb(packet, hal_task->segment_nb);
        hal_task->hw_length = hw_len;
        hal_task->length = task_len;

        *frm = frm_bak;
        rc_task->info = rc_info;

        enc_dbg_detail("task %d two pass mode leave\n", frm->seq_idx);
    }
TASK_DONE:
    return ret;
}

static void mpp_enc_rc_info_backup(MppEncImpl *enc, EncAsyncTaskInfo *task)
{
    if (!enc->support_hw_deflicker || !enc->cfg.rc.debreath_en)
        return;

    enc->rc_info_prev = task->rc.info;
}

static MPP_RET mpp_enc_force_pskip_check(Mpp *mpp, EncAsyncTaskInfo *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncRcTask *rc_task = &task->rc;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    MppEncCpbInfo cpb_info;
    RK_U32 max_tid = 0;
    MPP_RET ret = MPP_OK;

    mpp_enc_refs_get_cpb_info(enc->refs, &cpb_info);
    max_tid = cpb_info.max_st_tid;

    if (task->usr.force_flag & ENC_FORCE_IDR) {
        enc_dbg_detail("task %d, FORCE IDR should not be set as pskip frames", frm->seq_idx);
        ret = MPP_NOK;
    }
    if (cpb->curr.is_idr) {
        enc_dbg_detail("task %d, IDR frames should not be set as pskip frames", frm->seq_idx);
        ret = MPP_NOK;
    }
    if (cpb->curr.is_lt_ref) {
        enc_dbg_detail("task %d, LTR frames should not be set as pskip frames", frm->seq_idx);
        ret = MPP_NOK;
    }
    if (cpb->curr.temporal_id != max_tid) {
        enc_dbg_detail("task %d, Only top-layer frames can be set as pskip frames in TSVC mode", frm->seq_idx);
        ret = MPP_NOK;
    }
    if (cpb->curr.ref_mode != REF_TO_PREV_REF_FRM) {
        enc_dbg_detail("task %d, Only frames with reference mode set to prev_ref can be set as pskip frames", frm->seq_idx);
        ret = MPP_NOK;
    }

    return ret;
}

static MPP_RET mpp_enc_force_pskip(Mpp *mpp, EncAsyncTaskInfo *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncRefFrmUsrCfg *frm_cfg = &task->usr;
    EncRcTask *rc_task = &task->rc;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->task;
    MPP_RET ret = MPP_OK;

    enc_dbg_func("enter\n");

    frm_cfg->force_pskip++;
    if (frm->force_pskip)
        frm_cfg->force_flag |= ENC_FORCE_PSKIP_NON_REF;
    else if (frm->force_pskip_is_ref)
        frm_cfg->force_flag |= ENC_FORCE_PSKIP_IS_REF;

    /* NOTE: in some condition the pskip should not happen */
    mpp_enc_refs_set_usr_cfg(enc->refs, frm_cfg);

    enc_dbg_detail("task %d enc proc dpb\n", frm->seq_idx);
    mpp_enc_refs_get_cpb(enc->refs, cpb);

    ret = mpp_enc_force_pskip_check(mpp, task);

    if (ret) {
        mpp_enc_refs_rollback(enc->refs);
        frm_cfg->force_pskip--;
        if (frm->force_pskip)
            frm_cfg->force_flag &= ~ENC_FORCE_PSKIP_NON_REF;
        else if (frm->force_pskip_is_ref)
            frm_cfg->force_flag &= ~ENC_FORCE_PSKIP_IS_REF;
        return MPP_NOK;
    }

    enc_dbg_frm_status("frm %d start ***********************************\n", cpb->curr.seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_dpb, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_start, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d rc hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_start, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d enc sw enc start\n", frm->seq_idx);
    ENC_RUN_FUNC2(enc_impl_sw_enc, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc hal end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_end, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d rc frame end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_end, enc->rc_ctx, rc_task, mpp, ret);

TASK_DONE:
    enc_dbg_func("leave\n");
    return ret;
}

static MPP_RET mpp_enc_get_pskip_mode(Mpp *mpp, EncAsyncTaskInfo *task, MppPskipMode *skip_mode)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncRcTask *rc_task = &task->rc;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->task;
    MPP_RET ret = MPP_OK;

    skip_mode->pskip_is_ref = 0;
    skip_mode->pskip_is_non_ref = 0;
    enc->frame = hal_task->frame;

    if (mpp_frame_has_meta(enc->frame)) {
        MppMeta frm_meta = mpp_frame_get_meta(enc->frame);
        if (frm_meta) {
            mpp_meta_get_s32(frm_meta, KEY_INPUT_PSKIP, &skip_mode->pskip_is_ref);
            mpp_meta_get_s32(frm_meta, KEY_INPUT_PSKIP_NON_REF, &skip_mode->pskip_is_non_ref);
        }
    }

    if (skip_mode->pskip_is_ref == 1 && skip_mode->pskip_is_non_ref == 1) {
        mpp_err("task %d, Don't cfg pskip frame to be both a ref and non-ref at the same time");
        ret = MPP_NOK;
    } else {
        frm->force_pskip = skip_mode->pskip_is_non_ref;
        frm->force_pskip_is_ref = skip_mode->pskip_is_ref;
        ret = MPP_OK;
    }

    return ret;
}


static void mpp_enc_add_sw_header(MppEncImpl *enc, HalEncTask *hal_task)
{
    EncImpl impl = enc->impl;
    MppEncHeaderStatus *hdr_status = &enc->hdr_status;
    EncRcTask *rc_task = hal_task->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    MppPacket packet = hal_task->packet;
    MppFrame frame = hal_task->frame;

    if (!(hdr_status->val & HDR_ADDED_MASK)) {
        RK_U32 add_header = 0;

        if (enc->hdr_mode == MPP_ENC_HEADER_MODE_EACH_IDR && frm->is_intra)
            add_header |= 1;

        if (enc->cfg.rc.refresh_en && frm->is_i_recovery && !frm->is_idr)
            add_header |= 2;

        if (add_header) {
            enc_dbg_detail("task %d IDR header length %d\n",
                           frm->seq_idx, enc->hdr_len);

            mpp_packet_append(packet, enc->hdr_pkt);

            hal_task->header_length = enc->hdr_len;
            hal_task->length += enc->hdr_len;
            hdr_status->added_by_mode = 1;
        }

        if ((add_header & 2) && enc->sei_mode >= MPP_ENC_SEI_MODE_ONE_SEQ) {
            RK_S32 length = 0;

            enc_impl_add_prefix(impl, packet, &length, uuid_refresh_cfg,
                                &enc->cfg.rc.refresh_length, 0);

            if (length) {
                enc_dbg_detail("task %d refresh header length %d\n",
                               frm->seq_idx, enc->hdr_len);

                hal_task->sei_length += length;
                hal_task->length += length;
            }
        }
    }

    // check for header adding
    check_hal_task_pkt_len(hal_task, "header adding");

    /* 17. Add all prefix info before encoding */
    if (frm->is_idr && enc->sei_mode >= MPP_ENC_SEI_MODE_ONE_SEQ) {
        RK_S32 length = 0;

        enc_impl_add_prefix(impl, packet, &length, uuid_version,
                            enc->version_info, enc->version_length);

        hal_task->sei_length += length;
        hal_task->length += length;

        length = 0;
        enc_impl_add_prefix(impl, packet, &length, uuid_rc_cfg,
                            enc->rc_cfg_info, enc->rc_cfg_length);

        hal_task->sei_length += length;
        hal_task->length += length;
    }

    if (mpp_frame_has_meta(frame)) {
        update_user_datas(impl, packet, frame, hal_task);
    }

    // check for user data adding
    check_hal_task_pkt_len(hal_task, "user data adding");
}

static MPP_RET mpp_enc_normal(Mpp *mpp, EncAsyncTaskInfo *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &task->rc;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->task;
    MPP_RET ret = MPP_OK;
    EncAsyncStatus *status = &task->status;

    if (enc->support_hw_deflicker && enc->cfg.rc.debreath_en) {
        ret = mpp_enc_proc_two_pass(mpp, task);
        if (ret)
            return ret;
    }

    enc_dbg_detail("task %d check force pskip start\n", frm->seq_idx);
    if (!status->check_frm_pskip) {
        MppPskipMode skip_mode;
        status->check_frm_pskip = 1;

        mpp_enc_get_pskip_mode((Mpp*)enc->mpp, task, &skip_mode);
        if (skip_mode.pskip_is_ref || skip_mode.pskip_is_non_ref) {
            ret = mpp_enc_force_pskip((Mpp*)enc->mpp, task);
            if (ret)
                enc_dbg_detail("task %d set force pskip failed.", frm->seq_idx);
            else
                goto TASK_DONE;
        }
    }

    enc_dbg_detail("task %d enc proc dpb\n", frm->seq_idx);
    mpp_enc_refs_get_cpb(enc->refs, cpb);

    enc_dbg_frm_status("frm %d start ***********************************\n", cpb->curr.seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_dpb, impl, hal_task, mpp, ret);

    enc_dbg_frm_status("frm %d compare\n", cpb->curr.seq_idx);
    enc_dbg_frm_status("seq_idx      %d vs %d\n", frm->seq_idx, cpb->curr.seq_idx);
    enc_dbg_frm_status("is_idr       %d vs %d\n", frm->is_idr, cpb->curr.is_idr);
    enc_dbg_frm_status("is_intra     %d vs %d\n", frm->is_intra, cpb->curr.is_intra);
    enc_dbg_frm_status("is_non_ref   %d vs %d\n", frm->is_non_ref, cpb->curr.is_non_ref);
    enc_dbg_frm_status("is_lt_ref    %d vs %d\n", frm->is_lt_ref, cpb->curr.is_lt_ref);
    enc_dbg_frm_status("lt_idx       %d vs %d\n", frm->lt_idx, cpb->curr.lt_idx);
    enc_dbg_frm_status("temporal_id  %d vs %d\n", frm->temporal_id, cpb->curr.temporal_id);
    enc_dbg_frm_status("frm %d done  ***********************************\n", cpb->curr.seq_idx);

    enc_dbg_detail("task %d rc frame start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_start, enc->rc_ctx, rc_task, mpp, ret);

    // 16. generate header before hardware stream
    mpp_enc_add_sw_header(enc, hal_task);

    enc_dbg_detail("task %d enc proc hal\n", frm->seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_hal, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d hal get task\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_get_task, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_start, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d hal generate reg\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_gen_regs, hal, hal_task, mpp, ret);

    hal_task->segment_nb = mpp_packet_get_segment_nb(hal_task->packet);
    mpp_stopwatch_record(hal_task->stopwatch, "encode hal start");
    enc_dbg_detail("task %d hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_start, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d hal wait\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_wait,  hal, hal_task, mpp, ret);

    mpp_stopwatch_record(hal_task->stopwatch, "encode hal finish");

    enc_dbg_detail("task %d rc hal end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_end, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d hal ret task\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_ret_task, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame check reenc\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_check_reenc, enc->rc_ctx, rc_task, mpp, ret);

TASK_DONE:
    return ret;
}

static void mpp_enc_clr_rc_cb_info(EncRcTask *rc_task)
{
    EncRcTaskInfo *hal_rc = (EncRcTaskInfo *) &rc_task->info;
    EncRcTaskInfo bak = rc_task->info;

    memset(hal_rc, 0, sizeof(rc_task->info));

    hal_rc->frame_type = bak.frame_type;
    hal_rc->bit_target = bak.bit_target;
    hal_rc->bit_max = bak.bit_max;
    hal_rc->bit_min = bak.bit_min;
    hal_rc->quality_target = bak.quality_target;
    hal_rc->quality_max = bak.quality_max;
    hal_rc->quality_min = bak.quality_min;
}

static MPP_RET mpp_enc_reenc_simple(Mpp *mpp, EncAsyncTaskInfo *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &task->rc;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->task;
    MPP_RET ret = MPP_OK;

    enc_dbg_func("enter\n");

    mpp_enc_clr_rc_cb_info(rc_task);

    enc_dbg_detail("task %d enc proc hal\n", frm->seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_hal, enc->impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d hal get task\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_get_task, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_start, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d hal generate reg\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_gen_regs, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_start, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d hal wait\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_wait,  hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc hal end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_end, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d hal ret task\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_ret_task, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame check reenc\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_check_reenc, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d reenc %d times %d\n", frm->seq_idx, frm->reencode, frm->reencode_times);
    enc_dbg_func("leave\n");

TASK_DONE:
    return ret;
}

static MPP_RET mpp_enc_reenc_drop(Mpp *mpp, EncAsyncTaskInfo *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncRcTask *rc_task = &task->rc;
    EncRcTaskInfo *info = &rc_task->info;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->task;
    MPP_RET ret = MPP_OK;

    enc_dbg_func("enter\n");
    mpp_enc_refs_rollback(enc->refs);

    info->bit_real = hal_task->length;
    info->quality_real = info->quality_target;

    enc_dbg_detail("task %d rc frame end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_end, enc->rc_ctx, rc_task, mpp, ret);

TASK_DONE:
    enc_dbg_func("leave\n");
    return ret;
}

static MPP_RET mpp_enc_reenc_force_pskip(Mpp *mpp, EncAsyncTaskInfo *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncRefFrmUsrCfg *frm_cfg = &task->usr;
    EncRcTask *rc_task = &task->rc;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->task;
    MPP_RET ret = MPP_OK;

    enc_dbg_func("enter\n");

    frm_cfg->force_pskip++;
    frm_cfg->force_flag |= ENC_FORCE_PSKIP_NON_REF;

    /* NOTE: in some condition the pskip should not happen */

    mpp_enc_refs_rollback(enc->refs);
    mpp_enc_refs_set_usr_cfg(enc->refs, frm_cfg);

    enc_dbg_detail("task %d enc proc dpb\n", frm->seq_idx);
    mpp_enc_refs_get_cpb(enc->refs, cpb);

    enc_dbg_frm_status("frm %d start ***********************************\n", cpb->curr.seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_dpb, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d enc sw enc start\n", frm->seq_idx);
    ENC_RUN_FUNC2(enc_impl_sw_enc, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_end, enc->rc_ctx, rc_task, mpp, ret);

TASK_DONE:
    enc_dbg_func("leave\n");
    return ret;
}

static void mpp_enc_terminate_task(MppEncImpl *enc, EncAsyncTaskInfo *task)
{
    HalEncTask *hal_task = &task->task;
    EncFrmStatus *frm = &task->rc.frm;

    mpp_stopwatch_record(hal_task->stopwatch, "encode task done");

    if (enc->packet) {
        /* setup output packet and meta data */
        mpp_packet_set_length(enc->packet, hal_task->length);

        /*
         * First return output packet.
         * Then enqueue task back to input port.
         * Final user will release the mpp_frame they had input.
         */
        enc_dbg_detail("task %d enqueue packet pts %lld\n", frm->seq_idx, enc->task_pts);

        mpp_task_meta_set_packet(enc->task_out, KEY_OUTPUT_PACKET, enc->packet);
        mpp_port_enqueue(enc->output, enc->task_out);
    }

    if (enc->task_in) {
        enc_dbg_detail("task %d enqueue frame pts %lld\n", frm->seq_idx, enc->task_pts);

        mpp_task_meta_set_frame(enc->task_in, KEY_INPUT_FRAME, enc->frame);
        mpp_port_enqueue(enc->input, enc->task_in);
    }

    reset_enc_task(enc);
    task->status.val = 0;
}

static MPP_RET try_get_enc_task(MppEncImpl *enc, EncAsyncTaskInfo *task, EncAsyncWait *wait)
{
    EncRcTask *rc_task = &task->rc;
    EncFrmStatus *frm = &rc_task->frm;
    MppEncRefFrmUsrCfg *frm_cfg = &task->usr;
    MppEncHeaderStatus *hdr_status = &enc->hdr_status;
    EncAsyncStatus *status = &task->status;
    HalEncTask *hal_task = &task->task;
    MppStopwatch stopwatch = NULL;
    MPP_RET ret = MPP_OK;

    if (!status->task_in_rdy) {
        ret = mpp_port_poll(enc->input, MPP_POLL_NON_BLOCK);
        if (ret < 0) {
            wait->enc_frm_in = 1;
            return ret;
        }

        status->task_in_rdy = 1;
        wait->enc_frm_in = 0;
        enc_dbg_detail("task in ready\n");
    }

    // 5. check output task
    if (!status->task_out_rdy) {
        ret = mpp_port_poll(enc->output, MPP_POLL_NON_BLOCK);
        if (ret < 0) {
            wait->enc_pkt_out = 1;
            return ret;
        }

        status->task_out_rdy = 1;
        wait->enc_pkt_out = 0;
        enc_dbg_detail("task out ready\n");
    }

    /*
     * 6. get frame and packet for one task
     * If there is no input frame just return empty packet task
     */
    if (!status->frm_pkt_rdy) {
        // get tasks from both input and output
        ret = mpp_port_dequeue(enc->input, &enc->task_in);
        mpp_assert(enc->task_in);

        ret = mpp_port_dequeue(enc->output, &enc->task_out);
        mpp_assert(enc->task_out);

        /*
         * frame will be return to input.
         * packet will be sent to output.
         */
        mpp_task_meta_get_frame (enc->task_in, KEY_INPUT_FRAME,  &enc->frame);
        mpp_task_meta_get_packet(enc->task_in, KEY_OUTPUT_PACKET, &enc->packet);
        mpp_task_meta_get_buffer(enc->task_in, KEY_MOTION_INFO, &enc->md_info);

        enc_dbg_detail("task dequeue done frm %p pkt %p\n", enc->frame, enc->packet);

        stopwatch = mpp_frame_get_stopwatch(enc->frame);
        mpp_stopwatch_record(stopwatch, "encode task start");

        if (mpp_enc_check_frm_pkt(enc)) {
            mpp_stopwatch_record(stopwatch, "invalid on check frm pkt");
            reset_hal_enc_task(hal_task);
            ret = MPP_NOK;
            goto TASK_DONE;
        }

        status->frm_pkt_rdy = 1;
        enc_dbg_detail("task frame packet ready\n");
    }

    // 8. all task ready start encoding one frame
    if (!status->hal_task_reset_rdy) {
        reset_hal_enc_task(hal_task);
        reset_enc_rc_task(rc_task);
        task->usr = enc->frm_cfg;
        enc->frm_cfg.force_flag = 0;

        hal_task->rc_task   = rc_task;
        hal_task->frm_cfg   = frm_cfg;
        hal_task->frame     = enc->frame;
        hal_task->input     = enc->frm_buf;
        hal_task->packet    = enc->packet;
        hal_task->output    = enc->pkt_buf;
        hal_task->md_info   = enc->md_info;
        hal_task->stopwatch = stopwatch;

        frm->seq_idx = task->seq_idx++;
        rc_task->frame = enc->frame;
        enc_dbg_detail("task seq idx %d start\n", frm->seq_idx);
    }

    // 9. check frame drop by frame rate conversion
    if (!status->rc_check_frm_drop) {
        ENC_RUN_FUNC2(rc_frm_check_drop, enc->rc_ctx, rc_task, enc->mpp, ret);
        status->rc_check_frm_drop = 1;
        enc_dbg_detail("task %d drop %d\n", frm->seq_idx, frm->drop);

        hal_task->valid = 1;
        // when the frame should be dropped just return empty packet
        if (frm->drop) {
            mpp_stopwatch_record(stopwatch, "invalid on frame rate drop");
            hal_task->length = 0;
            hal_task->flags.drop_by_fps = 1;
            status->enc_start = 1;
            if (!status->refs_force_update) {
                if (frm_cfg->force_flag)
                    mpp_enc_refs_set_usr_cfg(enc->refs, frm_cfg);

                status->refs_force_update = 1;
            }
            ret = MPP_OK;
            goto TASK_DONE;
        }
    }

    // start encoder task process here
    mpp_assert(hal_task->valid);

    // 10. check and create packet for output
    if (!status->pkt_buf_rdy) {
        mpp_enc_check_pkt_buf(enc);
        status->pkt_buf_rdy = 1;

        hal_task->output = enc->pkt_buf;
    }
    mpp_assert(enc->packet);
    mpp_assert(enc->pkt_buf);

    // 11. check hal info update
    if (!enc->hal_info_updated) {
        update_enc_hal_info(enc);
        enc->hal_info_updated = 1;
    }

    // 12. generate header before hardware stream
    if (!hdr_status->ready) {
        /* config cpb before generating header */
        enc_impl_gen_hdr(enc->impl, enc->hdr_pkt);
        enc->hdr_len = mpp_packet_get_length(enc->hdr_pkt);
        hdr_status->ready = 1;

        enc_dbg_detail("task %d update header length %d\n",
                       frm->seq_idx, enc->hdr_len);

        mpp_packet_append(enc->packet, enc->hdr_pkt);
        hal_task->header_length = enc->hdr_len;
        hal_task->length += enc->hdr_len;
        hdr_status->added_by_change = 1;
    }

    check_hal_task_pkt_len(hal_task, "gen_hdr and adding");

    // 13. check frm_meta data force key in input frame and start one frame
    if (!status->enc_start) {
        enc_dbg_detail("task %d enc start\n", frm->seq_idx);
        ENC_RUN_FUNC2(enc_impl_start, enc->impl, hal_task, enc->mpp, ret);
        status->enc_start = 1;
    }

    // 14. setup user_cfg to dpb
    if (!status->refs_force_update) {
        if (frm_cfg->force_flag)
            mpp_enc_refs_set_usr_cfg(enc->refs, frm_cfg);

        status->refs_force_update = 1;
    }

    // 15. backup dpb
    if (!status->enc_backup) {
        mpp_enc_refs_stash(enc->refs);
        status->enc_backup = 1;
    }

TASK_DONE:
    if (ret)
        mpp_enc_terminate_task(enc, task);
    return ret;
}

static MPP_RET try_proc_low_deley_task(Mpp *mpp, EncAsyncTaskInfo *task, EncAsyncWait *wait)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &task->rc;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    EncAsyncStatus *status = &task->status;
    HalEncTask *hal_task = &task->task;
    MppPacket packet = hal_task->packet;
    MPP_RET ret = MPP_OK;

    if (hal_task->flags.drop_by_fps)
        goto TASK_DONE;

    if (status->low_delay_again)
        goto GET_OUTPUT_TASK;

    enc_dbg_detail("task %d check force pskip start\n", frm->seq_idx);
    if (!status->check_frm_pskip) {
        MppPskipMode skip_mode;
        status->check_frm_pskip = 1;

        mpp_enc_get_pskip_mode((Mpp*)enc->mpp, task, &skip_mode);
        if (skip_mode.pskip_is_ref || skip_mode.pskip_is_non_ref) {
            ret = mpp_enc_force_pskip((Mpp*)enc->mpp, task);
            if (ret)
                enc_dbg_detail("task %d set force pskip failed.", frm->seq_idx);
            else
                goto TASK_DONE;
        }
    }

    enc_dbg_detail("task %d enc proc dpb\n", frm->seq_idx);
    mpp_enc_refs_get_cpb(enc->refs, cpb);

    enc_dbg_frm_status("frm %d start ***********************************\n", cpb->curr.seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_dpb, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_start, enc->rc_ctx, rc_task, mpp, ret);

    // 16. generate header before hardware stream
    mpp_enc_add_sw_header(enc, hal_task);

    enc_dbg_detail("task %d enc proc hal\n", frm->seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_hal, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d hal get task\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_get_task, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_start, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d hal generate reg\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_gen_regs, hal, hal_task, mpp, ret);

    hal_task->part_first = 0;
    hal_task->part_last = 0;

    do {
        enc_dbg_detail("task %d hal start\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_part_start, hal, hal_task, mpp, ret);

        enc_dbg_detail("task %d hal wait\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_part_wait,  hal, hal_task, mpp, ret);

        enc_dbg_detail("task %d hal ret task\n", frm->seq_idx);
        ENC_RUN_FUNC2(mpp_enc_hal_ret_task, hal, hal_task, mpp, ret);

        if (hal_task->part_first) {
            hal_task->part_pos = (RK_U8 *)mpp_packet_get_pos(packet);
            hal_task->part_length = 0;
        }

        mpp_packet_set_length(packet, hal_task->length);

        enc_dbg_detail("task %d task_out %p\n", frm->seq_idx, enc->task_out);

    GET_OUTPUT_TASK:
        /* output task as soon as  */
        if (NULL == enc->task_out) {
            enc_dbg_detail("task %d poll new task for part output\n", frm->seq_idx);
            ret = mpp_port_poll(enc->output, MPP_POLL_NON_BLOCK);
            if (ret < 0) {
                wait->enc_pkt_out = 1;
                status->low_delay_again = 1;
                return MPP_NOK;
            }

            status->task_out_rdy = 1;
            wait->enc_pkt_out = 0;
            enc_dbg_detail("task out ready\n");

            ret = mpp_port_dequeue(enc->output, &enc->task_out);
        }

        mpp_assert(enc->task_out);

        /* copy new packet for multiple packet output */
        if (!hal_task->part_last) {
            RK_U8 *part_pos = (RK_U8 *)mpp_packet_get_pos(packet);
            RK_U32 part_length = hal_task->length;
            RK_U8 *last_pos = (RK_U8 *)hal_task->part_pos;
            RK_U32 pkt_len = 0;
            MppPacketImpl *part_pkt = NULL;

            enc_dbg_detail("pkt %d last %p part %p len %d\n", hal_task->part_count,
                           last_pos, part_pos, part_length);

            part_pos += part_length;
            pkt_len = (RK_U32)(part_pos - last_pos);

            mpp_packet_copy_init((MppPacket *)&part_pkt, packet);
            part_pkt->pos = last_pos;
            part_pkt->length = pkt_len;
            part_pkt->status.val = 0;
            part_pkt->status.partition = 1;
            part_pkt->status.soi = hal_task->part_first;
            part_pkt->status.eoi = hal_task->part_last;

            enc_dbg_detail("pkt %d new pos %p len %d\n", hal_task->part_count,
                           last_pos, pkt_len);

            hal_task->part_pos = part_pos;

            enc_dbg_detail("task %d enqueue packet pts %lld part %d\n",
                           frm->seq_idx, enc->task_pts, hal_task->part_count);
            mpp_task_meta_set_packet(enc->task_out, KEY_OUTPUT_PACKET, part_pkt);
            mpp_port_enqueue(enc->output, enc->task_out);
            enc->task_out = NULL;
            hal_task->part_count++;
        }
    } while (!hal_task->part_last);

TASK_DONE:
    /* output last task */
    {
        RK_U8 *part_pos = (RK_U8 *)mpp_packet_get_pos(packet);
        size_t part_length = hal_task->length;
        RK_U8 *last_pos = (RK_U8 *)hal_task->part_pos;
        RK_U32 pkt_len = 0;
        MppPacketImpl *pkt = (MppPacketImpl *)packet;

        enc_dbg_detail("pkt %d last %p part %p len %d\n", hal_task->part_count,
                       last_pos, part_pos, part_length);

        part_pos += part_length;
        pkt_len = part_pos - last_pos;

        enc_dbg_detail("pkt %d new pos %p len %d\n", hal_task->part_count,
                       last_pos, pkt_len);

        pkt->length = pkt_len;
        pkt->pos = last_pos;
        pkt->status.val = 0;
        pkt->status.partition = 1;
        pkt->status.soi = hal_task->part_first;
        pkt->status.eoi = hal_task->part_last;

        enc_dbg_detail("pkt %d new pos %p len %d\n", hal_task->part_count,
                       last_pos, pkt_len);

        hal_task->part_pos = part_pos;

        enc_dbg_detail("task %d enqueue packet pts %lld part %d\n",
                       frm->seq_idx, enc->task_pts, hal_task->part_count);
        mpp_task_meta_set_packet(enc->task_out, KEY_OUTPUT_PACKET, packet);
        mpp_port_enqueue(enc->output, enc->task_out);
        enc->task_out = NULL;
        hal_task->part_count = 0;
    }
    status->low_delay_again = 0;

    enc->time_end = mpp_time();
    enc->frame_count++;

    if (enc->dev && enc->time_base && enc->time_end &&
        ((enc->time_end - enc->time_base) >= (RK_S64)(1000 * 1000)))
        update_hal_info_fps(enc);

    enc_dbg_detail("task %d rc hal end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_end, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d enqueue frame pts %lld\n", frm->seq_idx, enc->task_pts);

    mpp_task_meta_set_frame(enc->task_in, KEY_INPUT_FRAME, enc->frame);
    mpp_port_enqueue(enc->input, enc->task_in);

    reset_enc_task(enc);
    status->val = 0;
    task->usr.force_flag = 0;

    return ret;
}

static MPP_RET set_enc_info_to_packet(MppEncImpl *enc, HalEncTask *hal_task)
{
    Mpp *mpp = (Mpp*)enc->mpp;
    EncRcTask *rc_task = hal_task->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    MppPacket packet = hal_task->packet;
    MppMeta meta = mpp_packet_get_meta(packet);

    if (!meta) {
        mpp_err_f("failed to get meta from packet\n");
        return MPP_NOK;
    }

    if (enc->coding == MPP_VIDEO_CodingHEVC || enc->coding == MPP_VIDEO_CodingAVC) {
        RK_U32 is_pskip = !(rc_task->info.lvl64_inter_num ||
                            rc_task->info.lvl32_inter_num ||
                            rc_task->info.lvl16_inter_num ||
                            rc_task->info.lvl8_inter_num  ||
                            rc_task->info.lvl32_intra_num ||
                            rc_task->info.lvl16_intra_num ||
                            rc_task->info.lvl8_intra_num  ||
                            rc_task->info.lvl4_intra_num);

        /* num of inter different size predicted block */
        mpp_meta_set_s32(meta, KEY_LVL64_INTER_NUM, rc_task->info.lvl64_inter_num);
        mpp_meta_set_s32(meta, KEY_LVL32_INTER_NUM, rc_task->info.lvl32_inter_num);
        mpp_meta_set_s32(meta, KEY_LVL16_INTER_NUM, rc_task->info.lvl16_inter_num);
        mpp_meta_set_s32(meta, KEY_LVL8_INTER_NUM,  rc_task->info.lvl8_inter_num);
        /* num of intra different size predicted block */
        mpp_meta_set_s32(meta, KEY_LVL32_INTRA_NUM, rc_task->info.lvl32_intra_num);
        mpp_meta_set_s32(meta, KEY_LVL16_INTRA_NUM, rc_task->info.lvl16_intra_num);
        mpp_meta_set_s32(meta, KEY_LVL8_INTRA_NUM,  rc_task->info.lvl8_intra_num);
        mpp_meta_set_s32(meta, KEY_LVL4_INTRA_NUM,  rc_task->info.lvl4_intra_num);

        mpp_meta_set_s64(meta, KEY_ENC_SSE,  rc_task->info.sse);
        mpp_meta_set_s32(meta, KEY_OUTPUT_PSKIP,    frm->force_pskip || is_pskip);
        mpp_meta_set_s32(meta, KEY_ENC_BPS_RT, rc_task->info.rt_bits);

        if (rc_task->info.frame_type == INTER_VI_FRAME)
            mpp_meta_set_s32(meta, KEY_ENC_USE_LTR, rc_task->cpb.refr.lt_idx);
    }
    /* frame type */
    mpp_meta_set_s32(meta, KEY_OUTPUT_INTRA,    frm->is_intra);

    /* start qp and average qp */
    mpp_meta_set_s32(meta, KEY_ENC_START_QP,    rc_task->info.quality_target);
    mpp_meta_set_s32(meta, KEY_ENC_AVERAGE_QP,  rc_task->info.quality_real);

    if (hal_task->md_info)
        mpp_meta_set_buffer(meta, KEY_MOTION_INFO, hal_task->md_info);

    if (mpp->mEncAyncIo)
        mpp_meta_set_frame(meta, KEY_INPUT_FRAME, hal_task->frame);

    return MPP_OK;
}

static MPP_RET try_proc_normal_task(MppEncImpl *enc, EncAsyncTaskInfo *task)
{
    Mpp *mpp = (Mpp*)enc->mpp;
    EncRcTask *rc_task = &task->rc;
    EncFrmStatus *frm = &rc_task->frm;
    MppEncRefFrmUsrCfg *frm_cfg = &task->usr;
    HalEncTask *hal_task = &task->task;
    MppPacket packet = hal_task->packet;
    MPP_RET ret = MPP_OK;

    if (hal_task->flags.drop_by_fps)
        goto TASK_DONE;

    // 17. normal encode
    ENC_RUN_FUNC2(mpp_enc_normal, mpp, task, mpp, ret);

    // 18. drop, force pskip and reencode  process
    while (frm->reencode && frm->reencode_times < enc->cfg.rc.max_reenc_times) {
        hal_task->length -= hal_task->hw_length;
        hal_task->hw_length = 0;

        mpp_packet_set_segment_nb(hal_task->packet, hal_task->segment_nb);

        enc_dbg_detail("task %d reenc %d times %d\n", frm->seq_idx, frm->reencode, frm->reencode_times);

        if (frm->drop) {
            mpp_enc_reenc_drop(mpp, task);
            break;
        }

        if (frm->force_pskip && !frm->is_idr && !frm->is_lt_ref) {
            mpp_enc_reenc_force_pskip(mpp, task);
            break;
        }
        frm->force_pskip = 0;
        mpp_enc_reenc_simple(mpp, task);
    }
    enc_dbg_detail("task %d rc frame end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_end, enc->rc_ctx, rc_task, mpp, ret);

    enc->time_end = mpp_time();
    enc->frame_count++;

    if (enc->dev && enc->time_base && enc->time_end &&
        ((enc->time_end - enc->time_base) >= (RK_S64)(1000 * 1000)))
        update_hal_info_fps(enc);

    frm->reencode = 0;
    frm->reencode_times = 0;
    frm_cfg->force_flag = 0;

TASK_DONE:
    mpp_stopwatch_record(hal_task->stopwatch, "encode task done");

    if (!mpp_packet_is_partition(packet)) {
        /* setup output packet and meta data */
        mpp_packet_set_length(packet, hal_task->length);
    }

    /* enc failed force idr */
    if (ret) {
        enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
        enc->hdr_status.val = 0;
        mpp_packet_set_length(packet, 0);
        mpp_err_f("enc failed force idr!\n");
    } else
        set_enc_info_to_packet(enc, hal_task);
    /*
     * First return output packet.
     * Then enqueue task back to input port.
     * Final user will release the mpp_frame they had input.
     */
    enc_dbg_detail("task %d enqueue packet pts %lld\n", frm->seq_idx, enc->task_pts);

    mpp_task_meta_set_packet(enc->task_out, KEY_OUTPUT_PACKET, packet);
    mpp_port_enqueue(enc->output, enc->task_out);

    enc_dbg_detail("task %d enqueue frame pts %lld\n", frm->seq_idx, enc->task_pts);

    mpp_task_meta_set_frame(enc->task_in, KEY_INPUT_FRAME, enc->frame);
    mpp_port_enqueue(enc->input, enc->task_in);

    mpp_enc_rc_info_backup(enc, task);
    reset_enc_task(enc);
    task->status.val = 0;

    return ret;
}

void *mpp_enc_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MppThread *thd_enc  = enc->thread_enc;
    EncAsyncTaskInfo task;
    EncAsyncWait wait;
    EncAsyncStatus *status = &task.status;
    MPP_RET ret = MPP_OK;

    memset(&task, 0, sizeof(task));
    wait.val = 0;

    enc->time_base = mpp_time();

    while (1) {
        mpp_thread_lock(thd_enc, THREAD_WORK);

        if (MPP_THREAD_RUNNING != mpp_thread_get_status(thd_enc, THREAD_WORK)) {
            mpp_thread_unlock(thd_enc, THREAD_WORK);
            break;
        }

        if (check_enc_task_wait(enc, &wait))
            mpp_thread_wait(thd_enc, THREAD_WORK);

        mpp_thread_unlock(thd_enc, THREAD_WORK);

        // When encoder is not on encoding process external config and reset
        if (!status->enc_start) {
            // 1. process user control
            if (enc->cmd_send != enc->cmd_recv) {
                enc_dbg_detail("ctrl proc %d cmd %08x\n", enc->cmd_recv, enc->cmd);
                sem_wait(&enc->cmd_start);
                ret = mpp_enc_proc_cfg(enc, enc->cmd, enc->param);
                if (ret)
                    *enc->cmd_ret = ret;
                enc->cmd_recv++;
                enc_dbg_detail("ctrl proc %d done send %d\n", enc->cmd_recv,
                               enc->cmd_send);
                mpp_assert(enc->cmd_send == enc->cmd_send);
                enc->param = NULL;
                enc->cmd = (MpiCmd)0;
                sem_post(&enc->cmd_done);

                // async cfg update process for hal
                // mainly for buffer prepare
                mpp_enc_hal_prepare(enc->enc_hal);

                /* NOTE: here will clear change flag of rc and prep cfg */
                mpp_enc_proc_rc_update(enc);
                wait.val = 0;
                continue;
            }

            // 2. process reset
            if (enc->reset_flag) {
                enc_dbg_detail("thread reset start\n");

                mpp_thread_lock(thd_enc, THREAD_WORK);
                enc->status_flag = 0;
                mpp_thread_unlock(thd_enc, THREAD_WORK);

                enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
                enc->frm_cfg.force_idr++;

                mpp_thread_lock(thd_enc, THREAD_CONTROL);
                enc->reset_flag = 0;
                sem_post(&enc->enc_reset);
                enc_dbg_detail("thread reset done\n");
                wait.val = 0;
                mpp_thread_unlock(thd_enc, THREAD_CONTROL);
                continue;
            }

            // 3. try get a task to encode
            ret = try_get_enc_task(enc, &task, &wait);
            if (ret)
                continue;
        }

        mpp_assert(status->enc_start);
        if (!status->enc_start)
            continue;

        // check partition low delay encoding
        /*
         * NOTE: Only when both split encoding and low delay mode are set then
         *       use special low delay path
         */
        if (enc->low_delay_part_mode)
            try_proc_low_deley_task(mpp, &task, &wait);
        else
            try_proc_normal_task(enc, &task);
    }

    // clear remain task in enc->output port
    release_task_in_port(enc->input);
    release_task_in_port(mpp->mUsrOutPort);

    return NULL;
}

static void async_task_reset(EncAsyncTaskInfo *task)
{
    memset(task, 0, sizeof(*task));
    task->task.rc_task = &task->rc;
    task->task.frm_cfg = &task->usr;
    task->usr.force_flag = 0;
}

static void async_task_terminate(MppEncImpl *enc, EncAsyncTaskInfo *async)
{
    HalEncTask *hal_task = &async->task;
    EncFrmStatus *frm = &async->rc.frm;
    Mpp *mpp = (Mpp *)enc->mpp;

    mpp_stopwatch_record(hal_task->stopwatch, "encode task done");

    if (hal_task->packet) {
        MppPacket pkt = hal_task->packet;

        /* setup output packet and meta data */
        mpp_packet_set_length(pkt, hal_task->length);

        /*
         * First return output packet.
         * Then enqueue task back to input port.
         * Final user will release the mpp_frame they had input.
         */
        enc_dbg_detail("task %d enqueue packet pts %lld\n", frm->seq_idx, enc->task_pts);

        if (mpp->mPktOut) {
            MppList *pkt_out = mpp->mPktOut;

            if (enc->frame) {
                MppMeta meta = mpp_packet_get_meta(pkt);
                MppStopwatch stopwatch = mpp_frame_get_stopwatch(enc->frame);

                mpp_stopwatch_record(stopwatch, "encode task terminate");

                mpp_meta_set_frame(meta, KEY_INPUT_FRAME, enc->frame);
                enc->frame = NULL;
            }

            mpp_mutex_cond_lock(&pkt_out->cond_lock);

            mpp_list_add_at_tail(pkt_out, &pkt, sizeof(pkt));
            mpp->mPacketPutCount++;
            mpp_list_signal(pkt_out);
            mpp_assert(pkt);

            mpp_mutex_cond_unlock(&pkt_out->cond_lock);
            enc_dbg_detail("packet out ready\n");
        }
    }

    async_task_reset(async);
}

static void async_task_skip(MppEncImpl *enc)
{
    Mpp *mpp = (Mpp*)enc->mpp;
    MppStopwatch stopwatch = NULL;
    MppMeta meta = NULL;
    MppFrame frm = NULL;
    MppPacket pkt = NULL;

    mpp_list_del_at_head(mpp->mFrmIn, &frm, sizeof(frm));
    mpp->mFrameGetCount++;

    mpp_assert(frm);

    enc_dbg_detail("skip input frame start\n");

    stopwatch = mpp_frame_get_stopwatch(frm);
    mpp_stopwatch_record(stopwatch, "skip task start");

    if (mpp_frame_has_meta(frm)) {
        meta = mpp_frame_get_meta(frm);
        if (meta)
            mpp_meta_get_packet(meta, KEY_OUTPUT_PACKET, &pkt);
    }

    if (NULL == pkt)
        mpp_packet_new(&pkt);

    mpp_assert(pkt);

    mpp_packet_set_length(pkt, 0);
    mpp_packet_set_pts(pkt, mpp_frame_get_pts(frm));
    mpp_packet_set_dts(pkt, mpp_frame_get_dts(frm));

    if (mpp_frame_get_eos(frm))
        mpp_packet_set_eos(pkt);
    else
        mpp_packet_clr_eos(pkt);

    meta = mpp_packet_get_meta(pkt);
    mpp_assert(meta);

    mpp_meta_set_frame(meta, KEY_INPUT_FRAME, frm);

    if (mpp->mPktOut) {
        MppList *pkt_out = mpp->mPktOut;

        mpp_mutex_cond_lock(&pkt_out->cond_lock);
        mpp_stopwatch_record(stopwatch, "skip task output");
        mpp_list_add_at_tail(pkt_out, &pkt, sizeof(pkt));
        mpp->mPacketPutCount++;
        mpp_list_signal(pkt_out);
        mpp_mutex_cond_unlock(&pkt_out->cond_lock);
    }

    enc_dbg_detail("packet skip ready\n");
}

static MPP_RET check_async_frm_pkt(EncAsyncTaskInfo *async)
{
    HalEncTask *hal_task = &async->task;
    MppPacket packet = hal_task->packet;
    MppFrame frame = hal_task->frame;

    hal_task->input = NULL;
    hal_task->output = NULL;

    if (packet)
        hal_task->output = mpp_packet_get_buffer(packet);
    else {
        mpp_packet_new(&packet);
        hal_task->packet = packet;
    }

    if (frame) {
        RK_S64 pts = mpp_frame_get_pts(frame);
        MppBuffer frm_buf = mpp_frame_get_buffer(frame);

        hal_task->input = frm_buf;

        mpp_packet_set_pts(packet, pts);
        mpp_packet_set_dts(packet, mpp_frame_get_dts(frame));

        if (mpp_frame_get_eos(frame))
            mpp_packet_set_eos(packet);
        else
            mpp_packet_clr_eos(packet);
    }

    return (NULL == frame || NULL == hal_task->input) ? MPP_NOK : MPP_OK;
}

static MPP_RET check_async_pkt_buf(MppEncImpl *enc, EncAsyncTaskInfo *async)
{
    HalEncTask *hal_task = &async->task;

    if (NULL == hal_task->output) {
        /* NOTE: set buffer w * h * 1.5 to avoid buffer overflow */
        Mpp *mpp = (Mpp *)enc->mpp;
        MppEncPrepCfg *prep = &enc->cfg.prep;
        RK_U32 width  = MPP_ALIGN(prep->width, 16);
        RK_U32 height = MPP_ALIGN(prep->height, 16);
        RK_U32 size = (enc->coding == MPP_VIDEO_CodingMJPEG) ?
                      (width * height * 3 / 2) : (width * height);
        MppPacketImpl *pkt = (MppPacketImpl *)hal_task->packet;
        MppBuffer buffer = NULL;

        mpp_assert(size);
        mpp_buffer_get(mpp->mPacketGroup, &buffer, size);
        mpp_buffer_attach_dev(buffer, enc->dev);
        mpp_assert(buffer);
        enc->pkt_buf = buffer;
        pkt->data   = mpp_buffer_get_ptr(buffer);
        pkt->pos    = pkt->data;
        pkt->size   = mpp_buffer_get_size(buffer);
        pkt->length = 0;
        pkt->buffer = buffer;
        hal_task->output = buffer;

        enc_dbg_detail("create output pkt %p buf %p\n", hal_task->packet, buffer);
    } else {
        enc_dbg_detail("output to pkt %p buf %p pos %p length %d\n",
                       hal_task->packet, hal_task->output,
                       mpp_packet_get_pos(hal_task->packet),
                       mpp_packet_get_length(hal_task->packet));
    }

    return MPP_OK;
}

static MPP_RET try_get_async_task(MppEncImpl *enc, EncAsyncWait *wait)
{
    Mpp *mpp = (Mpp *)enc->mpp;
    EncAsyncTaskInfo *async = enc->async;
    EncRcTask *rc_task = NULL;
    MppEncHeaderStatus *hdr_status = &enc->hdr_status;
    HalEncTask *hal_task = NULL;
    EncAsyncStatus *status = NULL;
    MppStopwatch stopwatch = NULL;
    MppPacket packet = NULL;
    MppFrame frame = NULL;
    RK_U32 seq_idx = 0;
    MPP_RET ret = MPP_OK;

    if (NULL == enc->hnd) {
        hal_task_get_hnd(enc->tasks, TASK_IDLE, &enc->hnd);
        if (enc->hnd) {
            wait->task_hnd = 0;
            enc_dbg_detail("get hnd success\n");

            mpp_assert(enc->async == NULL);
            async = (EncAsyncTaskInfo *)hal_task_hnd_get_data(enc->hnd);
            async_task_reset(async);
            enc->async = async;
        } else {
            wait->task_hnd = 1;
            enc_dbg_detail("get hnd failed\n");
            return MPP_NOK;
        }
    }

    mpp_assert(enc->hnd);
    mpp_assert(enc->async);

    hal_task = &async->task;
    rc_task = &async->rc;
    status = &async->status;
    packet = hal_task->packet;
    frame = hal_task->frame;

    if (NULL == frame) {
        if (mpp->mFrmIn) {
            MppList *frm_in = mpp->mFrmIn;

            mpp_mutex_cond_lock(&frm_in->cond_lock);

            if (mpp_list_size(frm_in)) {
                mpp_list_del_at_head(frm_in, &frame, sizeof(frame));
                mpp_list_signal(frm_in);

                mpp->mFrameGetCount++;

                mpp_assert(frame);

                status->task_in_rdy = 1;
                wait->enc_frm_in = 0;

                enc_dbg_detail("get input frame success\n");

                stopwatch = mpp_frame_get_stopwatch(frame);
                mpp_stopwatch_record(stopwatch, "encode task start");

                hal_task->frame = frame;
            }

            mpp_mutex_cond_unlock(&frm_in->cond_lock);
        }

        if (NULL == frame) {
            enc_dbg_detail("get input frame failed\n");
            wait->enc_frm_in = 1;
            return MPP_NOK;
        }
    }

    if (NULL == packet) {
        if (mpp_frame_has_meta(frame)) {
            MppMeta meta = mpp_frame_get_meta(frame);

            mpp_meta_get_packet(meta, KEY_OUTPUT_PACKET, &packet);
        }

        if (packet) {
            status->task_out_rdy = 1;
            wait->enc_pkt_out = 0;

            enc_dbg_detail("get output packet success\n");

            hal_task->packet = packet;
        }
    }

    mpp_assert(frame);

    stopwatch = mpp_frame_get_stopwatch(frame);

    /*
     * 6. get frame and packet for one task
     * If there is no input frame just return empty packet task
     */
    if (!status->frm_pkt_rdy) {
        async->seq_idx = enc->task_idx++;
        async->pts = mpp_frame_get_pts(hal_task->frame);

        hal_task->stopwatch = stopwatch;
        rc_task->frame = async->task.frame;

        enc_dbg_detail("task seq idx %d start\n", seq_idx);

        if (check_async_frm_pkt(async)) {
            mpp_stopwatch_record(stopwatch, "empty frame on check frm pkt");
            hal_task->valid = 1;
            hal_task->length = 0;
            hal_task->flags.drop_by_fps = 1;
            ret = MPP_OK;
            goto TASK_DONE;
        }

        status->frm_pkt_rdy = 1;
        enc_dbg_detail("task frame packet ready\n");
    }

    seq_idx = async->seq_idx;

    // 9. check frame drop by frame rate conversion
    if (!status->rc_check_frm_drop) {
        EncFrmStatus *frm = &rc_task->frm;

        ENC_RUN_FUNC2(rc_frm_check_drop, enc->rc_ctx, rc_task, enc->mpp, ret);
        status->rc_check_frm_drop = 1;
        enc_dbg_detail("task %d drop %d\n", seq_idx, frm->drop);

        // when the frame should be dropped just return empty packet
        hal_task->valid = 1;
        if (frm->drop) {
            mpp_stopwatch_record(stopwatch, "invalid on frame rate drop");
            hal_task->length = 0;
            hal_task->flags.drop_by_fps = 1;
            ret = MPP_OK;
            goto TASK_DONE;
        }

        *hal_task->frm_cfg = enc->frm_cfg;
        enc->frm_cfg.force_flag = 0;
    }

    // start encoder task process here
    mpp_assert(hal_task->valid);

    // 10. check and create packet for output
    if (!status->pkt_buf_rdy) {
        check_async_pkt_buf(enc, async);
        status->pkt_buf_rdy = 1;

        enc_dbg_detail("task %d check pkt buffer success\n", seq_idx);
    }
    mpp_assert(hal_task->packet);
    mpp_assert(hal_task->output);

    // 11. check hal info update
    if (!enc->hal_info_updated) {
        update_enc_hal_info(enc);
        enc->hal_info_updated = 1;

        enc_dbg_detail("task %d update enc hal info success\n", seq_idx);
    }

    // 12. generate header before hardware stream
    if (!hdr_status->ready) {
        /* config cpb before generating header */
        enc_impl_gen_hdr(enc->impl, enc->hdr_pkt);
        enc->hdr_len = mpp_packet_get_length(enc->hdr_pkt);
        hdr_status->ready = 1;

        enc_dbg_detail("task %d update header length %d\n",
                       seq_idx, enc->hdr_len);

        mpp_packet_append(hal_task->packet, enc->hdr_pkt);
        hal_task->header_length = enc->hdr_len;
        hal_task->length += enc->hdr_len;
        hdr_status->added_by_change = 1;
    }

    check_hal_task_pkt_len(hal_task, "gen_hdr and adding");

    // 13. check frm_meta data force key in input frame and start one frame
    if (!status->enc_start) {
        enc_dbg_detail("task %d enc start\n", seq_idx);
        ENC_RUN_FUNC2(enc_impl_start, enc->impl, hal_task, enc->mpp, ret);
        status->enc_start = 1;
    }

    // 14. setup user_cfg to dpb
    if (!status->refs_force_update) {
        MppEncRefFrmUsrCfg *frm_cfg = &async->usr;

        if (frm_cfg->force_flag) {
            mpp_enc_refs_set_usr_cfg(enc->refs, frm_cfg);
            frm_cfg->force_flag = 0;
        }

        enc_dbg_detail("task %d refs force update success\n", seq_idx);
        status->refs_force_update = 1;
    }

    // 15. backup dpb
    if (!status->enc_backup) {
        mpp_enc_refs_stash(enc->refs);
        enc_dbg_detail("task %d refs stash success\n", seq_idx);
        status->enc_backup = 1;
    }

TASK_DONE:
    if (ret) {
        enc_dbg_detail("task %d terminate\n", seq_idx);
        async_task_terminate(enc, async);
    }

    return ret;
}

static MPP_RET try_proc_processing_task(MppEncImpl *enc, EncAsyncWait *wait)
{
    HalTaskHnd hnd = NULL;
    EncAsyncTaskInfo *info = NULL;
    MPP_RET ret = MPP_NOK;

    ret = hal_task_get_hnd(enc->tasks, TASK_PROCESSING, &hnd);
    if (ret)
        return ret;

    info = (EncAsyncTaskInfo *)hal_task_hnd_get_data(hnd);

    mpp_assert(!info->status.enc_done);

    enc_async_wait_task(enc, info);
    hal_task_hnd_set_status(hnd, TASK_IDLE);
    wait->task_hnd = 0;

    return MPP_OK;
}

static MPP_RET proc_async_task(MppEncImpl *enc, EncAsyncWait *wait)
{
    Mpp *mpp = (Mpp*)enc->mpp;
    EncImpl impl = enc->impl;
    MppEncHal hal = enc->enc_hal;
    EncAsyncTaskInfo *async = enc->async;
    EncAsyncStatus *status = &async->status;
    HalEncTask *hal_task = &async->task;
    EncRcTask *rc_task = hal_task->rc_task;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    RK_U32 seq_idx = async->seq_idx;
    MPP_RET ret = MPP_OK;

    mpp_assert(hal_task->valid);

    if (hal_task->flags.drop_by_fps)
        goto SEND_TASK_INFO;

    if (!status->check_frm_pskip) {
        MppPskipMode skip_mode;
        status->check_frm_pskip = 1;

        mpp_enc_get_pskip_mode((Mpp*)enc->mpp, async, &skip_mode);
        if (skip_mode.pskip_is_ref || skip_mode.pskip_is_non_ref) {
            ret = mpp_enc_force_pskip((Mpp*)enc->mpp, async);
            if (ret)
                enc_dbg_detail("task %d set force pskip failed.", frm->seq_idx);
            else
                goto SEND_TASK_INFO;
        }
    }

    if (enc->support_hw_deflicker && enc->cfg.rc.debreath_en) {
        bool two_pass_en = mpp_enc_refs_next_frm_is_intra(enc->refs);

        if (two_pass_en) {
            /* wait all tasks done */
            while (MPP_OK == try_proc_processing_task(enc, wait));

            ret = mpp_enc_proc_two_pass(mpp, async);
            if (ret)
                return ret;
        }
    }

    enc_dbg_detail("task %d enc proc dpb\n", seq_idx);
    mpp_enc_refs_get_cpb(enc->refs, cpb);

    enc_dbg_frm_status("frm %d start ***********************************\n", seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_dpb, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_start, enc->rc_ctx, rc_task, mpp, ret);

    // 16. generate header before hardware stream
    mpp_enc_add_sw_header(enc, hal_task);

    enc_dbg_detail("task %d enc proc hal\n", frm->seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_hal, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d hal get task\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_get_task, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_start, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d hal generate reg\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_gen_regs, hal, hal_task, mpp, ret);

    mpp_stopwatch_record(hal_task->stopwatch, "encode hal start");
    enc_dbg_detail("task %d hal start\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_start, hal, hal_task, mpp, ret);

SEND_TASK_INFO:
    status->enc_done = 0;
    hal_task_hnd_set_status(enc->hnd, TASK_PROCESSING);
    enc_dbg_detail("task %d on processing ret %d\n", frm->seq_idx, ret);

    enc->hnd = NULL;
    enc->async = NULL;

TASK_DONE:

    /* NOTE: clear add_by flags */
    enc->hdr_status.val = enc->hdr_status.ready;

    return ret;
}

static MPP_RET check_enc_async_wait(MppEncImpl *enc, EncAsyncWait *wait)
{
    MPP_RET ret = MPP_OK;
    RK_U32 notify = enc->notify_flag;
    RK_U32 last_wait = enc->status_flag;
    RK_U32 curr_wait = wait->val;
    RK_U32 wait_chg  = last_wait & (~curr_wait);
    RK_U32 keep_notify = 0;

    do {
        if (enc->reset_flag)
            break;

        // NOTE: User control should always be processed
        if (notify & MPP_ENC_CONTROL) {
            keep_notify = notify & (~MPP_ENC_CONTROL);
            break;
        }

        // NOTE: When condition is not fulfilled check nofify flag again
        if (!curr_wait || (curr_wait & notify))
            break;

        ret = MPP_NOK;
    } while (0);

    enc_dbg_status("%p %08x -> %08x [%08x] notify %08x -> %s\n", enc,
                   last_wait, curr_wait, wait_chg, notify, (ret) ? ("wait") : ("work"));

    enc->status_flag = wait->val;
    enc->notify_flag = keep_notify;

    if (ret) {
        enc->wait_count++;
    } else {
        enc->work_count++;
    }

    return ret;
}

static MPP_RET enc_async_wait_task(MppEncImpl *enc, EncAsyncTaskInfo *info)
{
    Mpp *mpp = (Mpp*)enc->mpp;
    MppEncHal hal = enc->enc_hal;
    HalEncTask *hal_task = &info->task;
    EncRcTask *rc_task = hal_task->rc_task;
    EncFrmStatus *frm = &info->rc.frm;
    MppPacket pkt = hal_task->packet;
    MPP_RET ret = MPP_OK;

    if (hal_task->flags.drop_by_fps || hal_task->frm_cfg->force_pskip)
        goto TASK_DONE;

    enc_dbg_detail("task %d hal wait\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_wait, hal, hal_task, mpp, ret);

    mpp_stopwatch_record(hal_task->stopwatch, "encode hal finish");

    enc_dbg_detail("task %d rc hal end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_hal_end, enc->rc_ctx, rc_task, mpp, ret);

    enc_dbg_detail("task %d hal ret task\n", frm->seq_idx);
    ENC_RUN_FUNC2(mpp_enc_hal_ret_task, hal, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame end\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_end, enc->rc_ctx, rc_task, mpp, ret);

TASK_DONE:
    if (!mpp_packet_is_partition(pkt)) {
        /* setup output packet and meta data */
        mpp_packet_set_length(pkt, hal_task->length);

        check_hal_task_pkt_len(hal_task, "hw finish");
    }

    enc_dbg_detail("task %d output packet pts %lld\n", info->seq_idx, info->pts);

    /*
     * async encode process, there may have multiple frames encoding.
     * if last frame enc failed, drop non-idr frames and force idr for next frame.
     */
    if (enc->enc_failed_drop && !hal_task->rc_task->frm.is_idr) {
        mpp_packet_set_length(pkt, 0);
        mpp_err_f("last frame enc failed, drop this P frame\n");
        enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
        enc->hdr_status.val = 0;
    } else
        enc->enc_failed_drop = 0;
    /* enc failed, force idr for next frame */
    if (ret) {
        enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
        enc->hdr_status.val = 0;
        mpp_packet_set_length(pkt, 0);
        enc->enc_failed_drop = 1;

        mpp_err_f("enc failed force idr!\n");
    } else
        set_enc_info_to_packet(enc, hal_task);

    if (mpp->mPktOut) {
        MppList *pkt_out = mpp->mPktOut;

        mpp_mutex_cond_lock(&pkt_out->cond_lock);

        mpp_list_add_at_tail(pkt_out, &pkt, sizeof(pkt));
        mpp->mPacketPutCount++;
        mpp_list_signal(pkt_out);

        mpp_mutex_cond_unlock(&pkt_out->cond_lock);
    }

    return ret;
}

void *mpp_enc_async_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MppThread *thd_enc = enc->thread_enc;
    EncAsyncWait wait;
    MPP_RET ret = MPP_OK;

    enc_dbg_func("thread start\n");

    wait.val = 0;

    while (1) {
        mpp_thread_lock(thd_enc, THREAD_WORK);
        if (MPP_THREAD_RUNNING != mpp_thread_get_status(thd_enc, THREAD_WORK)) {
            mpp_thread_unlock(thd_enc, THREAD_WORK);
            break;
        }

        if (check_enc_async_wait(enc, &wait)) {
            enc_dbg_detail("wait start\n");
            mpp_thread_wait(thd_enc, THREAD_WORK);
            enc_dbg_detail("wait done\n");
        }
        mpp_thread_unlock(thd_enc, THREAD_WORK);

        // When encoder is not on encoding process external config and reset
        // 1. process user control and reset flag
        if (enc->cmd_send != enc->cmd_recv || enc->reset_flag) {
            MppList *frm_in = mpp->mFrmIn;

            /* when process cmd or reset hold frame input */
            mpp_mutex_cond_lock(&frm_in->cond_lock);

            enc_dbg_detail("ctrl proc %d cmd %08x\n", enc->cmd_recv, enc->cmd);

            // wait all tasks done
            while (MPP_OK == try_proc_processing_task(enc, &wait));

            if (enc->cmd_send != enc->cmd_recv) {
                sem_wait(&enc->cmd_start);
                ret = mpp_enc_proc_cfg(enc, enc->cmd, enc->param);
                if (ret)
                    *enc->cmd_ret = ret;
                enc->cmd_recv++;
                enc_dbg_detail("ctrl proc %d done send %d\n", enc->cmd_recv,
                               enc->cmd_send);
                mpp_assert(enc->cmd_send == enc->cmd_send);
                enc->param = NULL;
                enc->cmd = (MpiCmd)0;
                sem_post(&enc->cmd_done);

                // async cfg update process for hal
                // mainly for buffer prepare
                mpp_enc_hal_prepare(enc->enc_hal);

                /* NOTE: here will clear change flag of rc and prep cfg */
                mpp_enc_proc_rc_update(enc);
                goto SYNC_DONE;
            }

            if (enc->reset_flag) {
                enc_dbg_detail("thread reset start\n");

                /* skip the frames in input queue */
                while (mpp_list_size(frm_in))
                    async_task_skip(enc);

                {
                    mpp_thread_lock(thd_enc, THREAD_WORK);
                    enc->status_flag = 0;
                    mpp_thread_unlock(thd_enc, THREAD_WORK);
                }

                enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
                enc->frm_cfg.force_idr++;

                mpp_thread_lock(thd_enc, THREAD_CONTROL);
                enc->reset_flag = 0;
                sem_post(&enc->enc_reset);
                enc_dbg_detail("thread reset done\n");
                mpp_thread_unlock(thd_enc, THREAD_CONTROL);
            }
        SYNC_DONE:
            mpp_mutex_cond_unlock(&frm_in->cond_lock);
            wait.val = 0;
            continue;
        }

        // 2. try get a task to encode
        ret = try_get_async_task(enc, &wait);
        enc_dbg_detail("try_get_async_task ret %d\n", ret);
        if (ret) {
            try_proc_processing_task(enc, &wait);
            continue;
        }

        mpp_assert(enc->async);
        mpp_assert(enc->async->task.valid);

        proc_async_task(enc, &wait);
    }
    /* wait all task done */
    while (MPP_OK == try_proc_processing_task(enc, &wait));

    enc_dbg_func("thread finish\n");

    return NULL;
}
