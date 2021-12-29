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
#include "mpp_common.h"

#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"

#include "mpp.h"
#include "mpp_enc_debug.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_enc_impl.h"

typedef union EncTaskWait_u {
    RK_U32          val;
    struct {
        RK_U32      enc_frm_in      : 1;   // 0x0001 MPP_ENC_NOTIFY_FRAME_ENQUEUE
        RK_U32      reserv0002      : 1;   // 0x0002
        RK_U32      reserv0004      : 1;   // 0x0004
        RK_U32      enc_pkt_out     : 1;   // 0x0008 MPP_ENC_NOTIFY_PACKET_ENQUEUE

        RK_U32      reserv0010      : 1;   // 0x0010
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
} EncTaskWait;

/* encoder internal work flow */
typedef union EncTaskStatus_u {
    RK_U32          val;
    struct {
        RK_U32      task_in_rdy         : 1;
        RK_U32      task_out_rdy        : 1;

        RK_U32      frm_pkt_rdy         : 1;

        RK_U32      hal_task_reset_rdy  : 1;    // reset hal task to start
        RK_U32      rc_check_frm_drop   : 1;    // rc  stage
        RK_U32      pkt_buf_rdy         : 1;    // prepare pkt buf

        RK_U32      enc_start           : 1;    // enc stage
        RK_U32      refs_force_update   : 1;    // enc stage
        RK_U32      low_delay_again     : 1;    // enc stage low delay output again

        RK_U32      enc_backup          : 1;    // enc stage
        RK_U32      enc_restore         : 1;    // reenc flow start point
        RK_U32      enc_proc_dpb        : 1;    // enc stage
        RK_U32      rc_frm_start        : 1;    // rc  stage
        RK_U32      check_type_reenc    : 1;    // flow checkpoint if reenc -> enc_restore
        RK_U32      enc_proc_hal        : 1;    // enc stage
        RK_U32      hal_get_task        : 1;    // hal stage
        RK_U32      rc_hal_start        : 1;    // rc  stage
        RK_U32      hal_gen_reg         : 1;    // hal stage
        RK_U32      hal_start           : 1;    // hal stage
        RK_U32      hal_wait            : 1;    // hal stage NOTE: special in low delay mode
        RK_U32      rc_hal_end          : 1;    // rc  stage
        RK_U32      hal_ret_task        : 1;    // hal stage
        RK_U32      enc_update_hal      : 1;    // enc stage
        RK_U32      rc_frm_end          : 1;    // rc  stage
        RK_U32      check_rc_reenc      : 1;    // flow checkpoint if reenc -> enc_restore
    };
} EncTaskStatus;

typedef struct EncTask_t {
    RK_S32          seq_idx;
    EncTaskStatus   status;
    EncTaskWait     wait;
    HalEncTask      info;
} EncTask;

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

static MPP_RET check_enc_task_wait(MppEncImpl *enc, EncTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 notify = enc->notify_flag;
    RK_U32 last_wait = enc->status_flag;
    RK_U32 curr_wait = task->wait.val;
    RK_U32 wait_chg  = last_wait & (~curr_wait);

    do {
        if (enc->reset_flag)
            break;

        // NOTE: User control should always be processed
        if (notify & MPP_ENC_CONTROL)
            break;

        // NOTE: When condition is not fulfilled check nofify flag again
        if (!curr_wait || (curr_wait & notify))
            break;

        ret = MPP_NOK;
    } while (0);

    enc_dbg_status("%p %08x -> %08x [%08x] notify %08x -> %s\n", enc,
                   last_wait, curr_wait, wait_chg, notify, (ret) ? ("wait") : ("work"));

    enc->status_flag = task->wait.val;
    enc->notify_flag = 0;

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

static RK_S32 check_resend_hdr(MpiCmd cmd, void *param, MppEncCfgSet *cfg)
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

    if (cfg->codec.coding == MPP_VIDEO_CodingMJPEG ||
        cfg->codec.coding == MPP_VIDEO_CodingVP8)
        return 0;

    do {
        if (cmd == MPP_ENC_SET_CODEC_CFG ||
            cmd == MPP_ENC_SET_PREP_CFG ||
            cmd == MPP_ENC_SET_IDR_FRAME) {
            resend = 1;
            break;
        }

        if (cmd == MPP_ENC_SET_RC_CFG) {
            RK_U32 change = *(RK_U32 *)param;
            RK_U32 check_flag = MPP_ENC_RC_CFG_CHANGE_RC_MODE |
                                MPP_ENC_RC_CFG_CHANGE_FPS_IN |
                                MPP_ENC_RC_CFG_CHANGE_FPS_OUT |
                                MPP_ENC_RC_CFG_CHANGE_GOP;

            if (change & check_flag) {
                resend = 2;
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
                         MPP_ENC_RC_CFG_CHANGE_FPS_IN |
                         MPP_ENC_RC_CFG_CHANGE_FPS_OUT |
                         MPP_ENC_RC_CFG_CHANGE_GOP;

            if (change & check_flag) {
                resend = 4;
                break;
            }
            if (check_codec_to_resend_hdr(&cfg->codec)) {
                resend = 5;
                break;
            }
        }
    } while (0);

    if (resend)
        mpp_log("send header for %s\n", resend_reason[resend]);

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

static RK_S32 check_low_delay_part_mode(MppEncImpl *enc)
{
    MppEncCfgSet *cfg = &enc->cfg;

    if (!cfg->base.low_delay)
        return 0;

    if (!cfg->split.split_mode)
        return 0;

    if (mpp_enc_hal_check_part_mode(enc->enc_hal))
        return 0;

    return 1;
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
            dst->fps_in_denorm = src->fps_in_denorm;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
            dst->fps_out_flex = src->fps_out_flex;
            dst->fps_out_num = src->fps_out_num;
            dst->fps_out_denorm = src->fps_out_denorm;
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_GOP)
            dst->gop = src->gop;

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

        if (change & MPP_ENC_RC_CFG_CHANGE_HIER_QP) {
            dst->hier_qp_en = src->hier_qp_en;
            memcpy(dst->hier_qp_delta, src->hier_qp_delta, sizeof(src->hier_qp_delta));
            memcpy(dst->hier_frame_num, src->hier_frame_num, sizeof(src->hier_frame_num));
        }

        if (change & MPP_ENC_RC_CFG_CHANGE_ST_TIME)
            dst->stats_time = src->stats_time;

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

        if (change & MPP_ENC_HW_CFG_CHANGE_MB_RC)
            dst->mb_rc_disable = src->mb_rc_disable;

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

MPP_RET mpp_enc_proc_cfg(MppEncImpl *enc, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        MppEncCfgImpl *impl = (MppEncCfgImpl *)param;
        MppEncCfgSet *src = &impl->cfg;
        RK_U32 change = src->base.change;

        /* get base cfg here */
        if (change) {
            MppEncCfgSet *dst = &enc->cfg;

            if (change & MPP_ENC_BASE_CFG_CHANGE_LOW_DELAY)
                dst->base.low_delay = src->base.low_delay;

            src->base.change = 0;
        }

        /* process rc cfg at mpp_enc module */
        if (src->rc.change) {
            ret = mpp_enc_proc_rc_cfg(enc->coding, &enc->cfg.rc, &src->rc);
            src->rc.change = 0;
        }

        /* process hardware cfg at mpp_enc module */
        if (src->hw.change) {
            ret = mpp_enc_proc_hw_cfg(&enc->cfg.hw, &src->hw);
            src->hw.change = 0;
        }

        /* Then process the rest config */
        ret = enc_impl_proc_cfg(enc->impl, cmd, param);
    } break;
    case MPP_ENC_SET_RC_CFG : {
        MppEncRcCfg *src = (MppEncRcCfg *)param;
        if (src)
            ret = mpp_enc_proc_rc_cfg(enc->coding, &enc->cfg.rc, src);
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

    if (check_resend_hdr(cmd, param, &enc->cfg)) {
        enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
        enc->hdr_status.val = 0;
    }
    if (check_rc_cfg_update(cmd, &enc->cfg))
        enc->rc_status.rc_api_user_cfg = 1;
    if (check_rc_gop_update(cmd, &enc->cfg))
        mpp_enc_refs_set_rc_igop(enc->refs, enc->cfg.rc.gop);

    if (check_hal_info_update(cmd))
        enc->hal_info_updated = 0;

    enc->low_delay_part_mode = check_low_delay_part_mode(enc);

    return ret;
}

static const char *name_of_rc_mode[] = {
    "vbr",
    "cbr",
    "fixqp",
    "avbr",
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
    default : {
        cfg->mode = RC_AVBR;
    } break;
    }

    cfg->fps.fps_in_flex    = rc->fps_in_flex;
    cfg->fps.fps_in_num     = rc->fps_in_num;
    cfg->fps.fps_in_denorm  = rc->fps_in_denorm;
    cfg->fps.fps_out_flex   = rc->fps_out_flex;
    cfg->fps.fps_out_num    = rc->fps_out_num;
    cfg->fps.fps_out_denorm = rc->fps_out_denorm;
    cfg->igop               = rc->gop;
    cfg->max_i_bit_prop     = rc->max_i_prop;
    cfg->min_i_bit_prop     = rc->min_i_prop;
    cfg->init_ip_ratio      = rc->init_ip_ratio;

    cfg->bps_target = rc->bps_target;
    cfg->bps_max    = rc->bps_max;
    cfg->bps_min    = rc->bps_min;

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
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        MppEncJpegCfg *jpeg = &codec->jpeg;

        cfg->init_quality = jpeg->q_factor;
        cfg->max_quality = jpeg->qf_max;
        cfg->min_quality = jpeg->qf_min;
        cfg->max_i_quality = jpeg->qf_max;
        cfg->min_i_quality = jpeg->qf_min;
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

    if (info->st_gop) {
        cfg->vgop = info->st_gop;
        if (cfg->vgop >= rc->fps_out_num / rc->fps_out_denorm &&
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
                cfg->fps.fps_in_num, cfg->fps.fps_in_denorm,
                cfg->fps.fps_out_flex ? "flex" : "fix",
                cfg->fps.fps_out_num, cfg->fps.fps_out_denorm,
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

static MPP_RET mpp_enc_normal(Mpp *mpp, EncTask *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &enc->rc_task;
    MppEncHeaderStatus *hdr_status = &enc->hdr_status;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->info;
    MppFrame frame = hal_task->frame;
    MppPacket packet = hal_task->packet;
    MPP_RET ret = MPP_OK;

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
    if (enc->hdr_mode == MPP_ENC_HEADER_MODE_EACH_IDR &&
        frm->is_intra &&
        !hdr_status->added_by_change &&
        !hdr_status->added_by_ctrl &&
        !hdr_status->added_by_mode) {
        enc_dbg_detail("task %d IDR header length %d\n",
                       frm->seq_idx, enc->hdr_len);

        mpp_packet_append(packet, enc->hdr_pkt);

        hal_task->header_length = enc->hdr_len;
        hal_task->length += enc->hdr_len;
        hdr_status->added_by_mode = 1;
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

static MPP_RET mpp_enc_reenc_simple(Mpp *mpp, EncTask *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &enc->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->info;
    MPP_RET ret = MPP_OK;

    enc_dbg_func("enter\n");

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

static MPP_RET mpp_enc_reenc_drop(Mpp *mpp, EncTask *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncRcTask *rc_task = &enc->rc_task;
    EncRcTaskInfo *info = &rc_task->info;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->info;
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

static MPP_RET mpp_enc_reenc_force_pskip(Mpp *mpp, EncTask *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncRefFrmUsrCfg *frm_cfg = &enc->frm_cfg;
    EncRcTask *rc_task = &enc->rc_task;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->info;
    MPP_RET ret = MPP_OK;

    enc_dbg_func("enter\n");

    frm_cfg->force_pskip++;
    frm_cfg->force_flag |= ENC_FORCE_PSKIP;

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

static void mpp_enc_terminate_task(MppEncImpl *enc, EncTask *task)
{
    HalEncTask *hal_task = &task->info;
    EncFrmStatus *frm = &enc->rc_task.frm;

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

static MPP_RET try_get_enc_task(MppEncImpl *enc, EncTask *task)
{
    EncRcTask *rc_task = &enc->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    MppEncRefFrmUsrCfg *frm_cfg = &enc->frm_cfg;
    MppEncHeaderStatus *hdr_status = &enc->hdr_status;
    EncTaskStatus *status = &task->status;
    EncTaskWait *wait = &task->wait;
    HalEncTask *hal_task = &task->info;
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
        mpp_task_meta_get_buffer(enc->task_in, KEY_MOTION_INFO, &hal_task->mv_info);

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

        hal_task->rc_task   = rc_task;
        hal_task->frm_cfg   = frm_cfg;
        hal_task->frame     = enc->frame;
        hal_task->input     = enc->frm_buf;
        hal_task->packet    = enc->packet;
        hal_task->output    = enc->pkt_buf;
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

        // when the frame should be dropped just return empty packet
        if (frm->drop) {
            mpp_stopwatch_record(stopwatch, "invalid on frame rate drop");
            hal_task->valid = 0;
            hal_task->length = 0;
            ret = MPP_NOK;
            goto TASK_DONE;
        }
        hal_task->valid = 1;
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

static MPP_RET try_proc_low_deley_task(Mpp *mpp, EncTask *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &enc->rc_task;
    MppEncHeaderStatus *hdr_status = &enc->hdr_status;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    EncTaskStatus *status = &task->status;
    EncTaskWait *wait = &task->wait;
    HalEncTask *hal_task = &task->info;
    MppFrame frame = hal_task->frame;
    MppPacket packet = hal_task->packet;
    MPP_RET ret = MPP_OK;

    if (status->low_delay_again)
        goto GET_OUTPUT_TASK;

    enc_dbg_detail("task %d enc proc dpb\n", frm->seq_idx);
    mpp_enc_refs_get_cpb(enc->refs, cpb);

    enc_dbg_frm_status("frm %d start ***********************************\n", cpb->curr.seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_dpb, impl, hal_task, mpp, ret);

    enc_dbg_detail("task %d rc frame start\n", frm->seq_idx);
    ENC_RUN_FUNC2(rc_frm_start, enc->rc_ctx, rc_task, mpp, ret);

    // 16. generate header before hardware stream
    if (enc->hdr_mode == MPP_ENC_HEADER_MODE_EACH_IDR &&
        frm->is_intra &&
        !hdr_status->added_by_change &&
        !hdr_status->added_by_ctrl &&
        !hdr_status->added_by_mode) {
        enc_dbg_detail("task %d IDR header length %d\n",
                       frm->seq_idx, enc->hdr_len);

        mpp_packet_append(packet, enc->hdr_pkt);

        hal_task->header_length = enc->hdr_len;
        hal_task->length += enc->hdr_len;
        hdr_status->added_by_mode = 1;
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
    enc->frm_cfg.force_flag = 0;

    return ret;
}

static MPP_RET try_proc_normal_task(MppEncImpl *enc, EncTask *task)
{
    Mpp *mpp = (Mpp*)enc->mpp;
    EncRcTask *rc_task = &enc->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    MppEncRefFrmUsrCfg *frm_cfg = &enc->frm_cfg;
    HalEncTask *hal_task = &task->info;
    MPP_RET ret = MPP_OK;

    // 17. normal encode
    ENC_RUN_FUNC2(mpp_enc_normal, mpp, task, mpp, ret);

    // 18. drop, force pskip and reencode  process
    while (frm->reencode && frm->reencode_times < enc->cfg.rc.max_reenc_times) {
        hal_task->length -= hal_task->hw_length;
        hal_task->hw_length = 0;

        enc_dbg_detail("task %d reenc %d times %d\n", frm->seq_idx, frm->reencode, frm->reencode_times);

        if (frm->drop) {
            mpp_enc_reenc_drop(mpp, task);
            break;
        }

        if (frm->force_pskip && !frm->is_idr && !frm->is_lt_ref) {
            mpp_enc_reenc_force_pskip(mpp, task);
            break;
        }

        mpp_enc_reenc_simple(mpp, task);
    }
    enc_dbg_detail("task %d rc enc->frame end\n", frm->seq_idx);
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

    /* setup output packet and meta data */
    mpp_packet_set_length(enc->packet, hal_task->length);

    {
        MppMeta meta = mpp_packet_get_meta(enc->packet);

        if (hal_task->mv_info)
            mpp_meta_set_buffer(meta, KEY_MOTION_INFO, hal_task->mv_info);

        mpp_meta_set_s32(meta, KEY_OUTPUT_INTRA, frm->is_intra);
        if (rc_task->info.quality_real)
            mpp_meta_set_s32(meta, KEY_ENC_AVERAGE_QP, rc_task->info.quality_real);
    }

    /*
     * First return output packet.
     * Then enqueue task back to input port.
     * Final user will release the mpp_frame they had input.
     */
    enc_dbg_detail("task %d enqueue packet pts %lld\n", frm->seq_idx, enc->task_pts);

    mpp_task_meta_set_packet(enc->task_out, KEY_OUTPUT_PACKET, enc->packet);
    mpp_port_enqueue(enc->output, enc->task_out);

    enc_dbg_detail("task %d enqueue frame pts %lld\n", frm->seq_idx, enc->task_pts);

    mpp_task_meta_set_frame(enc->task_in, KEY_INPUT_FRAME, enc->frame);
    mpp_port_enqueue(enc->input, enc->task_in);

    reset_enc_task(enc);
    task->status.val = 0;

    return ret;
}

void *mpp_enc_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MppThread *thd_enc  = enc->thread_enc;
    EncTask task;
    EncTaskStatus *status = &task.status;
    MPP_RET ret = MPP_OK;

    memset(&task, 0, sizeof(task));

    enc->time_base = mpp_time();

    while (1) {
        {
            AutoMutex autolock(thd_enc->mutex());
            if (MPP_THREAD_RUNNING != thd_enc->get_status())
                break;

            if (check_enc_task_wait(enc, &task))
                thd_enc->wait();
        }

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
                continue;
            }

            // 2. process reset
            if (enc->reset_flag) {
                enc_dbg_detail("thread reset start\n");
                {
                    AutoMutex autolock(thd_enc->mutex());
                    enc->status_flag = 0;
                }

                AutoMutex autolock(thd_enc->mutex(THREAD_CONTROL));
                enc->reset_flag = 0;
                sem_post(&enc->enc_reset);
                enc_dbg_detail("thread reset done\n");
                continue;
            }

            // 3. try get a task to encode
            ret = try_get_enc_task(enc, &task);
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
            try_proc_low_deley_task(mpp, &task);
        else
            try_proc_normal_task(enc, &task);
    }

    // clear remain task in enc->output port
    release_task_in_port(enc->input);
    release_task_in_port(mpp->mUsrOutPort);

    return NULL;
}
