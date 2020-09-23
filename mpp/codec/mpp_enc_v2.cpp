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

#define  MODULE_TAG "mpp_enc_v2"

#include <string.h>
#include <stdarg.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_info.h"
#include "mpp_common.h"

#include "mpp_packet_impl.h"

#include "mpp.h"
#include "mpp_enc_debug.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_enc_impl.h"
#include "mpp_enc_hal.h"
#include "mpp_enc_ref.h"
#include "mpp_enc_refs.h"

#include "rc.h"

RK_U32 mpp_enc_debug = 0;

typedef union MppEncHeaderStatus_u {
    RK_U32 val;
    struct {
        RK_U32          ready           : 1;

        RK_U32          added_by_ctrl   : 1;
        RK_U32          added_by_mode   : 1;
        RK_U32          added_by_change : 1;
    };
} MppEncHeaderStatus;

typedef struct RcApiStatus_t {
    RK_U32              rc_api_inited   : 1;
    RK_U32              rc_api_updated  : 1;
    RK_U32              rc_api_user_cfg : 1;
} RcApiStatus;

typedef struct MppEncImpl_t {
    MppCodingType       coding;
    EncImpl             impl;
    MppEncHal           enc_hal;

    /* cpb parameters */
    MppEncRefs          refs;
    MppEncRefFrmUsrCfg  frm_cfg;

    /*
     * Rate control plugin parameters
     */
    RcApiStatus         rc_status;
    RK_S32              rc_api_updated;
    RK_S32              rc_cfg_updated;
    RcApiBrief          rc_brief;
    RcCtx               rc_ctx;
    EncRcTask           rc_task;

    MppThread           *thread_enc;
    void                *mpp;

    // internal status and protection
    Mutex               lock;
    RK_U32              reset_flag;
    sem_t               enc_reset;

    RK_U32              wait_count;
    RK_U32              work_count;
    RK_U32              status_flag;
    RK_U32              notify_flag;

    /* Encoder configure set */
    MppEncCfgSet        cfg;

    /* control process */
    RK_U32              cmd_send;
    RK_U32              cmd_recv;
    MpiCmd              cmd;
    void                *param;
    MPP_RET             *cmd_ret;
    sem_t               enc_ctrl;

    // legacy support for MPP_ENC_GET_EXTRA_INFO
    MppPacket           hdr_pkt;
    void                *hdr_buf;
    RK_U32              hdr_len;
    MppEncHeaderStatus  hdr_status;
    MppEncHeaderMode    hdr_mode;
    MppEncSeiMode       sei_mode;

    /* information for debug prefix */
    const char          *version_info;
    RK_S32              version_length;
    char                *rc_cfg_info;
    RK_S32              rc_cfg_pos;
    RK_S32              rc_cfg_length;
    RK_S32              rc_cfg_size;
} MppEncImpl;

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

        RK_U32      rc_check_frm_drop   : 1;    // rc  stage
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
    EncFrmStatus    frm;
    HalTaskInfo     info;
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

static void reset_hal_enc_task(HalEncTask *task)
{
    memset(task, 0, sizeof(*task));
}

static void reset_enc_rc_task(EncRcTask *task)
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
    MppCodingType coding = codec->coding;
    RK_U32 skip_flag = 0;

    switch (coding) {
    case MPP_VIDEO_CodingAVC : {
        MppEncH264Cfg *h264 = &codec->h264;

        skip_flag = MPP_ENC_H264_CFG_CHANGE_QP_LIMIT | MPP_ENC_H264_CFG_CHANGE_MAX_TID;

        if (h264->change & (~skip_flag))
            return 1;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        MppEncH265Cfg *h265 = &codec->h265;
        if (h265->change & (~MPP_ENC_H265_CFG_RC_QP_CHANGE))
            return 1;
    }
    break;
    case MPP_VIDEO_CodingVP8 : {
    } break;
    case MPP_VIDEO_CodingMJPEG : {
    } break;
    default : {
    } break;
    }
    return 0;
}

static const char *resend_reason[] = {
    "unchanged",
    "codec/prep cfg change",
    "rc cfg change rc_mode/fps/gop",
    "set cfg change input/format ",
    "set cfg change rc_mode/fps/gop",
    "set cfg change codec",
};

static RK_S32 check_resend_hdr(MpiCmd cmd, void *param, MppEncCfgSet *cfg)
{
    RK_S32 resend = 0;

    if (cfg->codec.coding == MPP_VIDEO_CodingMJPEG ||
        cfg->codec.coding == MPP_VIDEO_CodingVP8)
        return 0;

    do {
        if (cmd == MPP_ENC_SET_CODEC_CFG ||
            cmd == MPP_ENC_SET_PREP_CFG) {
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
                                MPP_ENC_PREP_CFG_CHANGE_ROTATION;

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

static RK_S32 check_codec_to_rc_cfg_update(MppEncCodecCfg *codec)
{
    MppCodingType coding = codec->coding;

    switch (coding) {
    case MPP_VIDEO_CodingAVC : {
        MppEncH264Cfg *h264 = &codec->h264;
        if (h264->change | MPP_ENC_H264_CFG_CHANGE_QP_LIMIT)
            return 1;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        MppEncH265Cfg *h265 = &codec->h265;
        if (h265->change | MPP_ENC_H265_CFG_RC_QP_CHANGE)
            return 1;
    } break;
    case MPP_VIDEO_CodingVP8 : {
    } break;
    case MPP_VIDEO_CodingMJPEG : {
    } break;
    default : {
    } break;
    }

    return 0;
}

static RK_S32 check_rc_cfg_update(MpiCmd cmd, MppEncCfgSet *cfg)
{
    if (cmd == MPP_ENC_SET_RC_CFG ||
        cmd == MPP_ENC_SET_PREP_CFG ||
        cmd == MPP_ENC_SET_REF_CFG) {
        return 1;
    }

    if (cmd == MPP_ENC_SET_CODEC_CFG) {
        if (check_codec_to_rc_cfg_update(&cfg->codec))
            return 1;
    }

    if (cmd == MPP_ENC_SET_CFG) {
        RK_U32 change = cfg->prep.change;
        RK_U32 check_flag = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                            MPP_ENC_PREP_CFG_CHANGE_FORMAT;

        if (change & check_flag)
            return 1;

        change = cfg->rc.change;
        check_flag = MPP_ENC_RC_CFG_CHANGE_RC_MODE |
                     MPP_ENC_RC_CFG_CHANGE_BPS |
                     MPP_ENC_RC_CFG_CHANGE_FPS_IN |
                     MPP_ENC_RC_CFG_CHANGE_FPS_OUT |
                     MPP_ENC_RC_CFG_CHANGE_GOP;

        if (change & check_flag)
            return 1;

        if (check_codec_to_rc_cfg_update(&cfg->codec))
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

static void mpp_enc_proc_cfg(MppEncImpl *enc)
{
    switch (enc->cmd) {
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

        if (enc->cmd == MPP_ENC_GET_EXTRA_INFO) {
            mpp_err("Please use MPP_ENC_GET_HDR_SYNC instead of unsafe MPP_ENC_GET_EXTRA_INFO\n");
            mpp_err("NOTE: MPP_ENC_GET_HDR_SYNC needs MppPacket input\n");

            *(MppPacket *)enc->param = enc->hdr_pkt;
        } else {
            mpp_packet_copy((MppPacket)enc->param, enc->hdr_pkt);
        }

        enc->hdr_status.added_by_ctrl = 1;
    } break;
    case MPP_ENC_GET_RC_API_ALL : {
        RcApiQueryAll *query = (RcApiQueryAll *)enc->param;

        rc_brief_get_all(query);
    } break;
    case MPP_ENC_GET_RC_API_BY_TYPE : {
        RcApiQueryType *query = (RcApiQueryType *)enc->param;

        rc_brief_get_by_type(query);
    } break;
    case MPP_ENC_SET_RC_API_CFG : {
        const RcImplApi *api = (const RcImplApi *)enc->param;

        rc_api_add(api);
    } break;
    case MPP_ENC_GET_RC_API_CURRENT : {
        RcApiBrief *dst = (RcApiBrief *)enc->param;

        *dst = enc->rc_brief;
    } break;
    case MPP_ENC_SET_RC_API_CURRENT : {
        RcApiBrief *src = (RcApiBrief *)enc->param;

        mpp_assert(src->type == enc->coding);
        enc->rc_brief = *src;
        enc->rc_status.rc_api_user_cfg = 1;
        enc->rc_status.rc_api_updated = 1;
    } break;
    case MPP_ENC_SET_HEADER_MODE : {
        if (enc->param) {
            MppEncHeaderMode mode = *((MppEncHeaderMode *)enc->param);

            if (mode < MPP_ENC_HEADER_MODE_BUTT) {
                enc->hdr_mode = mode;
                enc_dbg_ctrl("header mode set to %d\n", mode);
            } else {
                mpp_err_f("invalid header mode %d\n", mode);
                *enc->cmd_ret = MPP_NOK;
            }
        } else {
            mpp_err_f("invalid NULL ptr on setting header mode\n");
            *enc->cmd_ret = MPP_NOK;
        }
    } break;
    case MPP_ENC_SET_SEI_CFG : {
        if (enc->param) {
            MppEncSeiMode mode = *((MppEncSeiMode *)enc->param);

            if (mode <= MPP_ENC_SEI_MODE_ONE_FRAME) {
                enc->sei_mode = mode;
                enc_dbg_ctrl("sei mode set to %d\n", mode);
            } else {
                mpp_err_f("invalid sei mode %d\n", mode);
                *enc->cmd_ret = MPP_NOK;
            }
        } else {
            mpp_err_f("invalid NULL ptr on setting header mode\n");
            *enc->cmd_ret = MPP_NOK;
        }
    } break;
    case MPP_ENC_SET_REF_CFG : {
        MPP_RET ret = MPP_OK;
        MppEncRefCfg src = (MppEncRefCfg)enc->param;
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
            *enc->cmd_ret = ret;
        }

        ret = mpp_enc_refs_set_cfg(enc->refs, dst);
        if (ret) {
            mpp_err_f("failed to set ref cfg ret %d\n", ret);
            *enc->cmd_ret = ret;
        }

        if (mpp_enc_refs_update_hdr(enc->refs))
            enc->hdr_status.val = 0;
    } break;
    case MPP_ENC_SET_OSD_PLT_CFG : {
        MppEncOSDPltCfg *src = (MppEncOSDPltCfg *)enc->param;
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
        enc_impl_proc_cfg(enc->impl, enc->cmd, enc->param);
    } break;
    }

    if (check_resend_hdr(enc->cmd, enc->param, &enc->cfg)) {
        enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
        enc->hdr_status.val = 0;
    }
    if (check_rc_cfg_update(enc->cmd, &enc->cfg))
        enc->rc_status.rc_api_user_cfg = 1;
    if (check_rc_gop_update(enc->cmd, &enc->cfg))
        mpp_enc_refs_set_rc_igop(enc->refs, enc->cfg.rc.gop);
}

#define ENC_RUN_FUNC2(func, ctx, task, mpp, ret)        \
    ret = func(ctx, task);                              \
    if (ret) {                                          \
        mpp_err("mpp %p "#func":%-4d failed return %d", \
                mpp, __LINE__, ret);                    \
        goto TASK_DONE;                                 \
    }

static const char *name_of_rc_mode[] = {
    "cbr",
    "vbr",
    "avbr",
    "cvbr",
    "qvbr",
    "fixqp",
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

    cfg->bps_target = rc->bps_target;
    cfg->bps_max    = rc->bps_max;
    cfg->bps_min    = rc->bps_min;
    cfg->stat_times = 3;

    /* quality configure */
    switch (codec->coding) {
    case MPP_VIDEO_CodingAVC : {
        MppEncH264Cfg *h264 = &codec->h264;

        cfg->init_quality = h264->qp_init;
        cfg->max_quality = h264->qp_max;
        cfg->min_quality = h264->qp_min;
        cfg->max_i_quality = h264->qp_max_i ? h264->qp_max_i : h264->qp_max;
        cfg->min_i_quality = h264->qp_min_i ? h264->qp_min_i : h264->qp_min;
        cfg->i_quality_delta = h264->qp_delta_ip;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        MppEncH265Cfg *h265 = &codec->h265;

        cfg->init_quality = h265->qp_init;
        cfg->max_quality = h265->max_qp;
        cfg->min_quality = h265->min_qp;
        cfg->max_i_quality = h265->max_i_qp;
        cfg->min_i_quality = h265->min_i_qp;
        cfg->i_quality_delta = h265->ip_qp_delta;
    } break;
    case MPP_VIDEO_CodingVP8 : {
        MppEncVp8Cfg *vp8 = &codec->vp8;

        cfg->init_quality   = vp8->qp_init;
        cfg->max_quality    = vp8->qp_max;
        cfg->min_quality    = vp8->qp_min;
        cfg->max_i_quality  = vp8->qp_max_i ? vp8->qp_max_i : vp8->qp_max;
        cfg->min_i_quality  = vp8->qp_min_i ? vp8->qp_min_i : vp8->qp_min;
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        MppEncJpegCfg *jpeg = &codec->jpeg;

        cfg->init_quality = jpeg->quant;
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

static MPP_RET mpp_enc_normal(Mpp *mpp, EncTask *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &enc->rc_task;
    EncCpbStatus *cpb = &rc_task->cpb;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->info.enc;
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
        !enc->hdr_status.added_by_change &&
        !enc->hdr_status.added_by_ctrl &&
        !enc->hdr_status.added_by_mode) {
        enc_dbg_detail("task %d IDR header length %d\n",
                       frm->seq_idx, enc->hdr_len);

        mpp_packet_append(packet, enc->hdr_pkt);

        hal_task->header_length = enc->hdr_len;
        hal_task->length += enc->hdr_len;
        enc->hdr_status.added_by_mode = 1;
    }

    // check for header adding
    if (hal_task->length != mpp_packet_get_length(packet)) {
        mpp_err_f("header adding check failed: task length is not match to packet length %d vs %d\n",
                  hal_task->length, mpp_packet_get_length(packet));
    }

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
        MppMeta frm_meta = mpp_frame_get_meta(frame);
        MppEncUserData *user_data = NULL;

        mpp_meta_get_ptr(frm_meta, KEY_USER_DATA, (void**)&user_data);

        if (user_data) {
            if (user_data->pdata && user_data->len) {
                RK_S32 length = 0;

                enc_impl_add_prefix(impl, packet, &length, uuid_usr_data,
                                    user_data->pdata, user_data->len);

                hal_task->sei_length += length;
                hal_task->length += length;
            } else
                mpp_err_f("failed to insert user data %p len %d\n",
                          user_data->pdata, user_data->len);
        }
    }

    // check for user data adding
    if (hal_task->length != mpp_packet_get_length(packet)) {
        mpp_err_f("user data adding check failed: task length is not match to packet length %d vs %d\n",
                  hal_task->length, mpp_packet_get_length(packet));
    }

    enc_dbg_detail("task %d enc proc hal\n", frm->seq_idx);
    ENC_RUN_FUNC2(enc_impl_proc_hal, impl, hal_task, mpp, ret);

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

TASK_DONE:
    return ret;
}

static MPP_RET mpp_enc_reenc_simple(Mpp *mpp, EncTask *task)
{
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MppEncHal hal = enc->enc_hal;
    EncRcTask *rc_task = &enc->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    HalEncTask *hal_task = &task->info.enc;
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
    HalEncTask *hal_task = &task->info.enc;
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
    HalEncTask *hal_task = &task->info.enc;
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

void *mpp_enc_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppThread *thd_enc  = enc->thread_enc;
    MppEncCfgSet *cfg = &enc->cfg;
    MppEncRcCfg *rc_cfg = &cfg->rc;
    MppEncPrepCfg *prep_cfg = &cfg->prep;
    EncRcTask *rc_task = &enc->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    MppEncRefFrmUsrCfg *frm_cfg = &enc->frm_cfg;
    EncTask task;
    HalTaskInfo *task_info = &task.info;
    HalEncTask *hal_task = &task_info->enc;
    MppPort input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
    MppPort output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    MppTask task_in = NULL;
    MppTask task_out = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;

    memset(&task, 0, sizeof(task));

    while (1) {
        {
            AutoMutex autolock(thd_enc->mutex());
            if (MPP_THREAD_RUNNING != thd_enc->get_status())
                break;

            if (check_enc_task_wait(enc, &task))
                thd_enc->wait();
        }

        // 1. process user control
        if (enc->cmd_send != enc->cmd_recv) {
            enc_dbg_detail("ctrl proc %d cmd %08x\n", enc->cmd_recv, enc->cmd);
            mpp_enc_proc_cfg(enc);
            sem_post(&enc->enc_ctrl);
            enc->cmd_recv++;
            enc_dbg_detail("ctrl proc %d done send %d\n", enc->cmd_recv,
                           enc->cmd_send);
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

        // 3. check and update rate control api
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

        // 4. check input task
        if (!task.status.task_in_rdy) {
            ret = mpp_port_poll(input, MPP_POLL_NON_BLOCK);
            if (ret) {
                task.wait.enc_frm_in = 1;
                continue;
            }

            task.status.task_in_rdy = 1;
            task.wait.enc_frm_in = 0;
            enc_dbg_detail("task in ready\n");
        }

        // 5. check output task
        if (!task.status.task_out_rdy) {
            ret = mpp_port_poll(output, MPP_POLL_NON_BLOCK);
            if (ret) {
                task.wait.enc_pkt_out = 1;
                continue;
            }

            task.status.task_out_rdy = 1;
            task.wait.enc_pkt_out = 0;
            enc_dbg_detail("task out ready\n");
        }

        // get tasks from both input and output
        ret = mpp_port_dequeue(input, &task_in);
        mpp_assert(task_in);

        ret = mpp_port_dequeue(output, &task_out);
        mpp_assert(task_out);

        /*
         * frame will be return to input.
         * packet will be sent to output.
         */
        mpp_task_meta_get_frame (task_in, KEY_INPUT_FRAME,  &frame);
        mpp_task_meta_get_packet(task_in, KEY_OUTPUT_PACKET, &packet);

        enc_dbg_detail("task dequeue done frm %p pkt %p\n", frame, packet);

        /*
         * 6. check empty task for signaling
         * If there is no input frame just return empty packet task
         */
        if (NULL == frame)
            goto TASK_RETURN;

        if (NULL == mpp_frame_get_buffer(frame))
            goto TASK_RETURN;

        // 7. check and update rate control config
        if (enc->rc_status.rc_api_user_cfg) {
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

        // 8. all task ready start encoding one frame
        reset_hal_enc_task(hal_task);
        reset_enc_rc_task(rc_task);
        hal_task->rc_task = rc_task;
        hal_task->frm_cfg = frm_cfg;
        frm->seq_idx = task.seq_idx++;
        rc_task->frame = frame;

        enc_dbg_detail("task seq idx %d start\n", frm->seq_idx);

        /*
         * 9. check and create packet for output
         * if there is available buffer in the input frame do encoding
         */
        if (NULL == packet) {
            /* NOTE: set buffer w * h * 1.5 to avoid buffer overflow */
            RK_U32 width  = enc->cfg.prep.width;
            RK_U32 height = enc->cfg.prep.height;
            RK_U32 size = MPP_ALIGN(width, 16) * MPP_ALIGN(height, 16) * 3 / 2;
            MppBuffer buffer = NULL;

            mpp_assert(size);
            mpp_buffer_get(mpp->mPacketGroup, &buffer, size);
            mpp_packet_init_with_buffer(&packet, buffer);
            /* NOTE: clear length for output */
            mpp_packet_set_length(packet, 0);
            mpp_buffer_put(buffer);

            enc_dbg_detail("create output pkt %p buf %p\n", packet, buffer);
        }

        mpp_assert(packet);

        // 10. bypass pts to output
        {
            RK_S64 pts = mpp_frame_get_pts(frame);
            mpp_packet_set_pts(packet, pts);
            enc_dbg_detail("task %d pts %lld\n", frm->seq_idx, pts);
        }

        // 11. check frame drop by frame rate conversion
        ENC_RUN_FUNC2(rc_frm_check_drop, enc->rc_ctx, rc_task, mpp, ret);
        task.status.rc_check_frm_drop = 1;
        enc_dbg_detail("task %d drop %d\n", frm->seq_idx, frm->drop);

        // when the frame should be dropped just return empty packet
        if (frm->drop) {
            hal_task->valid = 0;
            hal_task->length = 0;
            goto TASK_DONE;
        }

        // start encoder task process here
        hal_task->valid = 1;

        // 12. generate header before hardware stream
        if (!enc->hdr_status.ready) {
            /* config cpb before generating header */
            enc_impl_gen_hdr(impl, enc->hdr_pkt);
            enc->hdr_len = mpp_packet_get_length(enc->hdr_pkt);
            enc->hdr_status.ready = 1;

            enc_dbg_detail("task %d update header length %d\n",
                           frm->seq_idx, enc->hdr_len);

            mpp_packet_append(packet, enc->hdr_pkt);
            hal_task->header_length = enc->hdr_len;
            hal_task->length += enc->hdr_len;
            enc->hdr_status.added_by_change = 1;
        }

        mpp_assert(hal_task->length == mpp_packet_get_length(packet));

        // 13. setup input frame and output packet
        hal_task->frame  = frame;
        hal_task->input  = mpp_frame_get_buffer(frame);
        hal_task->packet = packet;
        hal_task->output = mpp_packet_get_buffer(packet);
        hal_task->length = mpp_packet_get_length(packet);
        mpp_task_meta_get_buffer(task_in, KEY_MOTION_INFO, &hal_task->mv_info);

        /* 14. check frm_meta data force key in input frame and start one frame */
        enc_dbg_detail("task %d enc start\n", frm->seq_idx);
        ENC_RUN_FUNC2(enc_impl_start, impl, hal_task, mpp, ret);

        // 14. setup user_cfg to dpb
        if (frm_cfg->force_flag)
            mpp_enc_refs_set_usr_cfg(enc->refs, frm_cfg);

        // 15. backup dpb
        mpp_enc_refs_stash(enc->refs);
        task.status.enc_backup = 1;

        ENC_RUN_FUNC2(mpp_enc_normal, mpp, &task, mpp, ret);

        // reencode process
        while (frm->reencode && frm->reencode_times < rc_cfg->max_reenc_times) {
            hal_task->length -= hal_task->hw_length;
            hal_task->hw_length = 0;

            enc_dbg_detail("task %d reenc %d times %d\n", frm->seq_idx, frm->reencode, frm->reencode_times);

            if (frm->drop) {
                mpp_enc_reenc_drop(mpp, &task);
                break;
            }

            if (frm->force_pskip && !frm->is_idr && !frm->is_lt_ref) {
                mpp_enc_reenc_force_pskip(mpp, &task);
                break;
            }

            mpp_enc_reenc_simple(mpp, &task);
        }
        enc_dbg_detail("task %d rc frame end\n", frm->seq_idx);
        ENC_RUN_FUNC2(rc_frm_end, enc->rc_ctx, rc_task, mpp, ret);

        frm->reencode = 0;
        frm->reencode_times = 0;
        frm_cfg->force_flag = 0;

    TASK_DONE:
        /* setup output packet and meta data */
        mpp_packet_set_length(packet, hal_task->length);

        {
            MppMeta meta = mpp_packet_get_meta(packet);

            if (hal_task->mv_info)
                mpp_meta_set_buffer(meta, KEY_MOTION_INFO, hal_task->mv_info);

            mpp_meta_set_s32(meta, KEY_OUTPUT_INTRA, frm->is_intra);
        }

    TASK_RETURN:
        /*
         * First return output packet.
         * Then enqueue task back to input port.
         * Final user will release the mpp_frame they had input.
         */
        if (NULL == packet)
            mpp_packet_new(&packet);

        if (frame && mpp_frame_get_eos(frame))
            mpp_packet_set_eos(packet);
        else
            mpp_packet_clr_eos(packet);

        enc_dbg_detail("task %d enqueue packet pts %lld\n", frm->seq_idx, mpp_packet_get_pts(packet));

        mpp_task_meta_set_packet(task_out, KEY_OUTPUT_PACKET, packet);
        mpp_port_enqueue(output, task_out);

        enc_dbg_detail("task %d enqueue frame pts %lld\n", frm->seq_idx, mpp_frame_get_pts(frame));

        mpp_task_meta_set_frame(task_in, KEY_INPUT_FRAME, frame);
        mpp_port_enqueue(input, task_in);

        task_in = NULL;
        task_out = NULL;
        packet = NULL;
        frame = NULL;

        task.status.val = 0;
        enc->hdr_status.val = 0;
        enc->hdr_status.ready = 1;
    }

    // clear remain task in output port
    release_task_in_port(input);
    release_task_in_port(mpp->mOutputPort);

    return NULL;
}

MPP_RET mpp_enc_init_v2(MppEnc *enc, MppEncInitCfg *cfg)
{
    MPP_RET ret;
    MppCodingType coding = cfg->coding;
    EncImpl impl = NULL;
    MppEncImpl *p = NULL;
    MppEncHal enc_hal = NULL;
    MppEncHalCfg enc_hal_cfg;
    EncImplCfg ctrl_cfg;

    mpp_env_get_u32("mpp_enc_debug", &mpp_enc_debug, 0);

    if (NULL == enc) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    *enc = NULL;

    p = mpp_calloc(MppEncImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_MALLOC;
    }

    ret = mpp_enc_refs_init(&p->refs);
    if (ret) {
        mpp_err_f("could not init enc refs\n");
        goto ERR_RET;
    }

    // H.264 encoder use mpp_enc_hal path
    // create hal first
    enc_hal_cfg.coding = coding;
    enc_hal_cfg.set = NULL;
    enc_hal_cfg.cfg = &p->cfg;
    enc_hal_cfg.work_mode = HAL_MODE_LIBVPU;
    enc_hal_cfg.device_id = DEV_VEPU;

    ctrl_cfg.coding = coding;
    ctrl_cfg.dev_id = DEV_VEPU;
    ctrl_cfg.cfg = &p->cfg;
    ctrl_cfg.set = NULL;
    ctrl_cfg.refs = p->refs;
    ctrl_cfg.task_count = 2;

    ret = mpp_enc_hal_init(&enc_hal, &enc_hal_cfg);
    if (ret) {
        mpp_err_f("could not init enc hal\n");
        goto ERR_RET;
    }

    ctrl_cfg.dev_id = enc_hal_cfg.device_id;
    ctrl_cfg.task_count = -1;

    ret = enc_impl_init(&impl, &ctrl_cfg);
    if (ret) {
        mpp_err_f("could not init impl\n");
        goto ERR_RET;
    }

    p->coding   = coding;
    p->impl     = impl;
    p->enc_hal  = enc_hal;
    p->mpp      = cfg->mpp;
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_SEQ;
    p->version_info = get_mpp_version();
    p->version_length = strlen(p->version_info);
    p->rc_cfg_size = SZ_1K;
    p->rc_cfg_info = mpp_calloc_size(char, p->rc_cfg_size);

    {
        // create header packet storage
        size_t size = SZ_1K;
        p->hdr_buf = mpp_calloc_size(void, size);

        mpp_packet_init(&p->hdr_pkt, p->hdr_buf, size);
        mpp_packet_set_length(p->hdr_pkt, 0);
    }

    /* NOTE: setup configure coding for check */
    p->cfg.codec.coding = coding;
    p->cfg.plt_cfg.plt = &p->cfg.plt_data;
    mpp_enc_ref_cfg_init(&p->cfg.ref_cfg);
    ret = mpp_enc_ref_cfg_copy(p->cfg.ref_cfg, mpp_enc_ref_default());
    ret = mpp_enc_refs_set_cfg(p->refs, mpp_enc_ref_default());

    sem_init(&p->enc_reset, 0, 0);
    sem_init(&p->enc_ctrl, 0, 0);

    *enc = p;
    return ret;
ERR_RET:
    mpp_enc_deinit_v2(p);
    return ret;
}

MPP_RET mpp_enc_deinit_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    if (NULL == enc) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (enc->impl) {
        enc_impl_deinit(enc->impl);
        enc->impl = NULL;
    }

    if (enc->enc_hal) {
        mpp_enc_hal_deinit(enc->enc_hal);
        enc->enc_hal = NULL;
    }

    if (enc->hdr_pkt)
        mpp_packet_deinit(&enc->hdr_pkt);

    MPP_FREE(enc->hdr_buf);

    if (enc->cfg.ref_cfg) {
        mpp_enc_ref_cfg_deinit(&enc->cfg.ref_cfg);
        enc->cfg.ref_cfg = NULL;
    }

    if (enc->refs) {
        mpp_enc_refs_deinit(&enc->refs);
        enc->refs = NULL;
    }

    if (enc->rc_ctx) {
        rc_deinit(enc->rc_ctx);
        enc->rc_ctx = NULL;
    }

    MPP_FREE(enc->rc_cfg_info);
    enc->rc_cfg_size = 0;
    enc->rc_cfg_length = 0;

    sem_destroy(&enc->enc_reset);
    sem_destroy(&enc->enc_ctrl);

    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_start_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);

    enc->thread_enc = new MppThread(mpp_enc_thread,
                                    enc->mpp, "mpp_enc");
    enc->thread_enc->start();

    enc_dbg_func("%p out\n", enc);

    return MPP_OK;
}

MPP_RET mpp_enc_stop_v2(MppEnc ctx)
{
    MPP_RET ret = MPP_OK;
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);

    if (enc->thread_enc) {
        enc->thread_enc->stop();
        delete enc->thread_enc;
        enc->thread_enc = NULL;
    }

    enc_dbg_func("%p out\n", enc);
    return ret;

}

MPP_RET mpp_enc_reset_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);
    if (NULL == enc) {
        mpp_err_f("found NULL input enc\n");
        return MPP_ERR_NULL_PTR;
    }

    MppThread *thd = enc->thread_enc;

    thd->lock(THREAD_CONTROL);
    enc->reset_flag = 1;
    mpp_enc_notify_v2(enc, MPP_ENC_RESET);
    thd->unlock(THREAD_CONTROL);
    sem_wait(&enc->enc_reset);
    mpp_assert(enc->reset_flag == 0);

    return MPP_OK;
}

MPP_RET mpp_enc_notify_v2(MppEnc ctx, RK_U32 flag)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in flag %08x\n", enc, flag);
    MppThread *thd  = enc->thread_enc;

    thd->lock();
    if (flag == MPP_ENC_CONTROL) {
        enc->notify_flag |= flag;
        enc_dbg_notify("%p status %08x notify control signal\n", enc,
                       enc->status_flag);
        thd->signal();
    } else {
        RK_U32 old_flag = enc->notify_flag;

        enc->notify_flag |= flag;
        if ((old_flag != enc->notify_flag) &&
            (enc->notify_flag & enc->status_flag)) {
            enc_dbg_notify("%p status %08x notify %08x signal\n", enc,
                           enc->status_flag, enc->notify_flag);
            thd->signal();
        }
    }
    thd->unlock();
    enc_dbg_func("%p out\n", enc);
    return MPP_OK;
}

/*
 * preprocess config and rate-control config is common config then they will
 * be done in mpp_enc layer
 *
 * codec related config will be set in each hal component
 */
MPP_RET mpp_enc_control_v2(MppEnc ctx, MpiCmd cmd, void *param)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    if (NULL == enc) {
        mpp_err_f("found NULL enc\n");
        return MPP_ERR_NULL_PTR;
    }

    if (NULL == param && cmd != MPP_ENC_SET_IDR_FRAME && cmd != MPP_ENC_SET_REF_CFG) {
        mpp_err_f("found NULL param enc %p cmd %x\n", enc, cmd);
        return MPP_ERR_NULL_PTR;
    }

    AutoMutex auto_lock(&enc->lock);
    MPP_RET ret = MPP_OK;

    enc_dbg_ctrl("sending cmd %d param %p\n", cmd, param);

    switch (cmd) {
    case MPP_ENC_GET_CFG : {
        MppEncCfgImpl *p = (MppEncCfgImpl *)param;

        enc_dbg_ctrl("get all config\n");
        memcpy(&p->cfg, &enc->cfg, sizeof(enc->cfg));
    } break;
    case MPP_ENC_GET_PREP_CFG : {
        enc_dbg_ctrl("get prep config\n");
        memcpy(param, &enc->cfg.prep, sizeof(enc->cfg.prep));
    } break;
    case MPP_ENC_GET_RC_CFG : {
        enc_dbg_ctrl("get rc config\n");
        memcpy(param, &enc->cfg.rc, sizeof(enc->cfg.rc));
    } break;
    case MPP_ENC_GET_CODEC_CFG : {
        enc_dbg_ctrl("get codec config\n");
        memcpy(param, &enc->cfg.codec, sizeof(enc->cfg.codec));
    } break;
    case MPP_ENC_SET_IDR_FRAME : {
        enc_dbg_ctrl("set idr frame\n");
        enc->frm_cfg.force_flag |= ENC_FORCE_IDR;
        enc->frm_cfg.force_idr++;
    } break;
    case MPP_ENC_GET_HEADER_MODE : {
        enc_dbg_ctrl("get header mode\n");
        memcpy(param, &enc->hdr_mode, sizeof(enc->hdr_mode));
    } break;
    case MPP_ENC_GET_OSD_PLT_CFG : {
        enc_dbg_ctrl("get osd plt cfg\n");
        memcpy(param, &enc->cfg.plt_cfg, sizeof(enc->cfg.plt_cfg));
    } break;
    default : {
        // Cmd which is not get configure will handle by enc_impl
        enc->cmd = cmd;
        enc->param = param;
        enc->cmd_ret = &ret;

        enc->cmd_send++;
        mpp_enc_notify_v2(ctx, MPP_ENC_CONTROL);
        sem_wait(&enc->enc_ctrl);
    } break;
    }

    enc_dbg_ctrl("sending cmd %d done\n", cmd);
    return ret;
}
