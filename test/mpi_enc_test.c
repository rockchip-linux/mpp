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

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_enc_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "utils.h"
#include "mpi_enc_utils.h"

typedef struct {
    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frame_count;
    RK_U64 stream_size;

    // src and dst
    FILE *fp_input;
    FILE *fp_output;

    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;
    MppEncSliceSplit split_cfg;
    MppEncOSDPltCfg osd_plt_cfg;
    MppEncOSDPlt    osd_plt;
    MppEncOSDData   osd_data;

    // input / output
    MppBuffer frm_buf;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;
    MppBuffer osd_idx_buf;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_U32 num_frames;

    // resources
    size_t frame_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;
    /*
     * osd idx size range from 16x16 bytes(pixels) to hor_stride*ver_stride(bytes).
     * for general use, 1/8 Y buffer is enough.
     */
    size_t osd_idx_size;
    RK_U32 plt_table[8];

    RK_U32 osd_enable;
    RK_U32 osd_mode;
    RK_U32 split_mode;
    RK_U32 split_arg;

    // rate control runtime parameter
    RK_S32 gop;
    RK_S32 fps;
    RK_S32 bps;
    RK_S32 gop_mode;
    MppEncGopRef ref;
} MpiEncTestData;

MPP_RET test_ctx_init(MpiEncTestData **data, MpiEncTestArgs *cmd)
{
    MpiEncTestData *p = NULL;
    MPP_RET ret = MPP_OK;

    if (!data || !cmd) {
        mpp_err_f("invalid input data %p cmd %p\n", data, cmd);
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_calloc(MpiEncTestData, 1);
    if (!p) {
        mpp_err_f("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
        goto RET;
    }

    // get paramter from cmd
    p->width        = cmd->width;
    p->height       = cmd->height;
    p->hor_stride   = (cmd->hor_stride) ? (cmd->hor_stride) :
                      (MPP_ALIGN(cmd->width, 16));
    p->ver_stride   = (cmd->ver_stride) ? (cmd->ver_stride) :
                      (MPP_ALIGN(cmd->height, 16));
    p->fmt          = cmd->format;
    p->type         = cmd->type;
    p->bps          = cmd->bps_target;
    if (cmd->type == MPP_VIDEO_CodingMJPEG)
        cmd->num_frames = 1;
    p->num_frames   = cmd->num_frames;
    p->gop_mode     =  cmd->gop_mode;

    if (cmd->file_input) {
        p->fp_input = fopen(cmd->file_input, "rb");
        if (NULL == p->fp_input) {
            mpp_err("failed to open input file %s\n", cmd->file_input);
            mpp_err("create default yuv image for test\n");
        }
    }

    if (cmd->file_output) {
        p->fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == p->fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            ret = MPP_ERR_OPEN_FILE;
        }
    }

    // update resource parameter
    switch (p->fmt) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY :
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 2;
    } break;

    default: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 4;
    } break;
    }

    /*
     * osd idx size range from 16x16 bytes(pixels) to hor_stride*ver_stride(bytes).
     * for general use, 1/8 Y buffer is enough.
     */
    p->osd_idx_size  = p->hor_stride * p->ver_stride / 8;
    p->plt_table[0] = MPP_ENC_OSD_PLT_RED;
    p->plt_table[1] = MPP_ENC_OSD_PLT_YELLOW;
    p->plt_table[2] = MPP_ENC_OSD_PLT_BLUE;
    p->plt_table[3] = MPP_ENC_OSD_PLT_GREEN;
    p->plt_table[4] = MPP_ENC_OSD_PLT_CYAN;
    p->plt_table[5] = MPP_ENC_OSD_PLT_TRANS;
    p->plt_table[6] = MPP_ENC_OSD_PLT_BLACK;
    p->plt_table[7] = MPP_ENC_OSD_PLT_WHITE;

RET:
    *data = p;
    return ret;
}

MPP_RET test_ctx_deinit(MpiEncTestData **data)
{
    MpiEncTestData *p = NULL;

    if (!data) {
        mpp_err_f("invalid input data %p\n", data);
        return MPP_ERR_NULL_PTR;
    }

    p = *data;
    if (p) {
        if (p->fp_input) {
            fclose(p->fp_input);
            p->fp_input = NULL;
        }
        if (p->fp_output) {
            fclose(p->fp_output);
            p->fp_output = NULL;
        }
        MPP_FREE(p);
        *data = NULL;
    }

    return MPP_OK;
}

static void setup_gop_ref(MppEncGopRef *ref, RK_S32 gop_mode)
{
    // rockchip tsvc config
    MppGopRefInfo *gop = &ref->gop_info[0];

    mpp_log("gop_mode %d", gop_mode);

    ref->change = 1;
    ref->gop_cfg_enable = 1;

    // default no LTR
    ref->lt_ref_interval = 0;
    ref->max_lt_ref_cnt = 0;

    if (gop_mode == 3) {
        // tsvc4
        //      /-> P1      /-> P3        /-> P5      /-> P7
        //     /           /             /           /
        //    //--------> P2            //--------> P6
        //   //                        //
        //  ///---------------------> P4
        // ///
        // P0 ------------------------------------------------> P8
        ref->ref_gop_len    = 8;
        ref->layer_weight[0] = 800;
        ref->layer_weight[1] = 400;
        ref->layer_weight[2] = 400;
        ref->layer_weight[3] = 400;

        gop[0].temporal_id  = 0;
        gop[0].ref_idx      = 0;
        gop[0].is_non_ref   = 0;
        gop[0].is_lt_ref    = 1;
        gop[0].lt_idx       = 0;

        gop[1].temporal_id  = 3;
        gop[1].ref_idx      = 0;
        gop[1].is_non_ref   = 1;
        gop[1].is_lt_ref    = 0;
        gop[1].lt_idx       = 0;

        gop[2].temporal_id  = 2;
        gop[2].ref_idx      = 0;
        gop[2].is_non_ref   = 0;
        gop[2].is_lt_ref    = 0;
        gop[2].lt_idx       = 0;

        gop[3].temporal_id  = 3;
        gop[3].ref_idx      = 2;
        gop[3].is_non_ref   = 1;
        gop[3].is_lt_ref    = 0;
        gop[3].lt_idx       = 0;

        gop[4].temporal_id  = 1;
        gop[4].ref_idx      = 0;
        gop[4].is_non_ref   = 0;
        gop[4].is_lt_ref    = 1;
        gop[4].lt_idx       = 1;

        gop[5].temporal_id  = 3;
        gop[5].ref_idx      = 4;
        gop[5].is_non_ref   = 1;
        gop[5].is_lt_ref    = 0;
        gop[5].lt_idx       = 0;

        gop[6].temporal_id  = 2;
        gop[6].ref_idx      = 4;
        gop[6].is_non_ref   = 0;
        gop[6].is_lt_ref    = 0;
        gop[6].lt_idx       = 0;

        gop[7].temporal_id  = 3;
        gop[7].ref_idx      = 6;
        gop[7].is_non_ref   = 1;
        gop[7].is_lt_ref    = 0;
        gop[7].lt_idx       = 0;

        gop[8].temporal_id  = 0;
        gop[8].ref_idx      = 0;
        gop[8].is_non_ref   = 0;
        gop[8].is_lt_ref    = 1;
        gop[8].lt_idx       = 0;

        ref->max_lt_ref_cnt = 2;
    } else if (gop_mode == 2) {
        // tsvc3
        //     /-> P1      /-> P3
        //    /           /
        //   //--------> P2
        //  //
        // P0/---------------------> P4
        ref->ref_gop_len    = 4;
        ref->layer_weight[0] = 1000;
        ref->layer_weight[1] = 500;
        ref->layer_weight[2] = 500;
        ref->layer_weight[3] = 0;

        gop[0].temporal_id  = 0;
        gop[0].ref_idx      = 0;
        gop[0].is_non_ref   = 0;
        gop[0].is_lt_ref    = 0;
        gop[0].lt_idx       = 0;

        gop[1].temporal_id  = 2;
        gop[1].ref_idx      = 0;
        gop[1].is_non_ref   = 1;
        gop[1].is_lt_ref    = 0;
        gop[1].lt_idx       = 0;

        gop[2].temporal_id  = 1;
        gop[2].ref_idx      = 0;
        gop[2].is_non_ref   = 0;
        gop[2].is_lt_ref    = 0;
        gop[2].lt_idx       = 0;

        gop[3].temporal_id  = 2;
        gop[3].ref_idx      = 2;
        gop[3].is_non_ref   = 1;
        gop[3].is_lt_ref    = 0;
        gop[3].lt_idx       = 0;

        gop[4].temporal_id  = 0;
        gop[4].ref_idx      = 0;
        gop[4].is_non_ref   = 0;
        gop[4].is_lt_ref    = 0;
        gop[4].lt_idx       = 0;

        // set to lt_ref interval with looping LTR idx
        ref->lt_ref_interval = 10;
        ref->max_lt_ref_cnt = 3;
    } else if (gop_mode == 1) {
        // tsvc2
        //   /-> P1
        //  /
        // P0--------> P2
        ref->ref_gop_len    = 2;
        ref->layer_weight[0] = 1400;
        ref->layer_weight[1] = 600;
        ref->layer_weight[2] = 0;
        ref->layer_weight[3] = 0;

        gop[0].temporal_id  = 0;
        gop[0].ref_idx      = 0;
        gop[0].is_non_ref   = 0;
        gop[0].is_lt_ref    = 0;
        gop[0].lt_idx       = 0;

        gop[1].temporal_id  = 1;
        gop[1].ref_idx      = 0;
        gop[1].is_non_ref   = 1;
        gop[1].is_lt_ref    = 0;
        gop[1].lt_idx       = 0;

        gop[2].temporal_id  = 0;
        gop[2].ref_idx      = 0;
        gop[2].is_non_ref   = 0;
        gop[2].is_lt_ref    = 0;
        gop[2].lt_idx       = 0;
    }
}

MPP_RET test_mpp_setup(MpiEncTestData *p)
{
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
    MppEncCodecCfg *codec_cfg;
    MppEncPrepCfg *prep_cfg;
    MppEncRcCfg *rc_cfg;
    MppEncSliceSplit *split_cfg;

    if (NULL == p)
        return MPP_ERR_NULL_PTR;

    mpi = p->mpi;
    ctx = p->ctx;
    codec_cfg = &p->codec_cfg;
    prep_cfg = &p->prep_cfg;
    rc_cfg = &p->rc_cfg;
    split_cfg = &p->split_cfg;

    /* setup default parameter */
    p->fps = 30;
    p->gop = 60;

    if (!p->bps)
        p->bps = p->width * p->height / 8 * p->fps;

    prep_cfg->change        = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                              MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                              MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg->width         = p->width;
    prep_cfg->height        = p->height;
    prep_cfg->hor_stride    = p->hor_stride;
    prep_cfg->ver_stride    = p->ver_stride;
    prep_cfg->format        = p->fmt;
    prep_cfg->rotation      = MPP_ENC_ROT_0;
    ret = mpi->control(ctx, MPP_ENC_SET_PREP_CFG, prep_cfg);
    if (ret) {
        mpp_err("mpi control enc set prep cfg failed ret %d\n", ret);
        goto RET;
    }

    rc_cfg->change  = MPP_ENC_RC_CFG_CHANGE_ALL;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_CBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;

    if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        /* constant QP does not have bps */
        rc_cfg->bps_target   = -1;
        rc_cfg->bps_max      = -1;
        rc_cfg->bps_min      = -1;
    } else if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_CBR) {
        /* constant bitrate has very small bps range of 1/16 bps */
        rc_cfg->bps_target   = p->bps;
        rc_cfg->bps_max      = p->bps * 17 / 16;
        rc_cfg->bps_min      = p->bps * 15 / 16;
    } else if (rc_cfg->rc_mode ==  MPP_ENC_RC_MODE_VBR) {
        /* variable bitrate has large bps range */
        rc_cfg->bps_target   = p->bps;
        rc_cfg->bps_max      = p->bps * 17 / 16;
        rc_cfg->bps_min      = p->bps * 1 / 16;
    }

    /* fix input / output frame rate */
    rc_cfg->fps_in_flex      = 0;
    rc_cfg->fps_in_num       = p->fps;
    rc_cfg->fps_in_denorm    = 1;
    rc_cfg->fps_out_flex     = 0;
    rc_cfg->fps_out_num      = p->fps;
    rc_cfg->fps_out_denorm   = 1;

    rc_cfg->gop              = p->gop;
    rc_cfg->skip_cnt         = 0;

    mpp_log("mpi_enc_test bps %d fps %d gop %d\n",
            rc_cfg->bps_target, rc_cfg->fps_out_num, rc_cfg->gop);
    ret = mpi->control(ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
    if (ret) {
        mpp_err("mpi control enc set rc cfg failed ret %d\n", ret);
        goto RET;
    }

    codec_cfg->coding = p->type;
    switch (codec_cfg->coding) {
    case MPP_VIDEO_CodingAVC : {
        codec_cfg->h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                                 MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                                 MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        codec_cfg->h264.profile  = 100;
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        codec_cfg->h264.level    = 40;
        codec_cfg->h264.entropy_coding_mode  = 1;
        codec_cfg->h264.cabac_init_idc  = 0;
        codec_cfg->h264.transform8x8_mode = 1;
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        codec_cfg->jpeg.change  = MPP_ENC_JPEG_CFG_CHANGE_QP;
        codec_cfg->jpeg.quant   = 10;
    } break;
    case MPP_VIDEO_CodingVP8 : {
    } break;
    case MPP_VIDEO_CodingHEVC : {
        codec_cfg->h265.change = MPP_ENC_H265_CFG_INTRA_QP_CHANGE | MPP_ENC_H265_CFG_RC_QP_CHANGE;
        if (rc_cfg->rc_mode != MPP_ENC_RC_MODE_FIXQP)
            codec_cfg->h265.qp_init = -1;
        else
            codec_cfg->h265.qp_init = 26;
        codec_cfg->h265.max_i_qp = 46;
        codec_cfg->h265.min_i_qp = 24;
        codec_cfg->h265.max_qp = 51;
        codec_cfg->h265.min_qp = 10;
        if (0) {
            codec_cfg->h265.change |= MPP_ENC_H265_CFG_SLICE_CHANGE;
            codec_cfg->h265.slice_cfg.split_enable = 1;
            codec_cfg->h265.slice_cfg.split_mode = 1;
            codec_cfg->h265.slice_cfg.slice_size = 10;
        }
    } break;
    default : {
        mpp_err_f("support encoder coding type %d\n", codec_cfg->coding);
    } break;
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CODEC_CFG, codec_cfg);
    if (ret) {
        mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
        goto RET;
    }

    p->split_mode = 0;
    p->split_arg = 0;

    mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
    mpp_env_get_u32("split_arg", &p->split_arg, 0);

    if (p->split_mode) {
        split_cfg->change = MPP_ENC_SPLIT_CFG_CHANGE_ALL;
        split_cfg->split_mode = p->split_mode;
        split_cfg->split_arg = p->split_arg;

        mpp_log("split_mode %d split_arg %d\n", p->split_mode, p->split_arg);

        ret = mpi->control(ctx, MPP_ENC_SET_SPLIT, split_cfg);
        if (ret) {
            mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
            goto RET;
        }
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        goto RET;
    }

    if (p->gop_mode && p->gop_mode < 4) {
        setup_gop_ref(&p->ref, p->gop_mode);

        mpp_log_f("MPP_ENC_SET_GOPREF start gop mode %d\n", p->gop_mode);

        ret = mpi->control(ctx, MPP_ENC_SET_GOPREF, &p->ref);
        mpp_log_f("MPP_ENC_SET_GOPREF done ret %d\n", ret);
        if (ret) {
            mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
            goto RET;
        }
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            goto RET;
        }
    }

    p->osd_enable = 0;
    p->osd_mode = 0;

    mpp_env_get_u32("osd_enable", &p->osd_enable, 0);
    mpp_env_get_u32("osd_mode", &p->osd_mode, MPP_ENC_OSD_PLT_TYPE_DEFAULT);

    if (p->osd_enable) {
        /* gen and cfg osd plt */
        mpi_enc_gen_osd_plt(&p->osd_plt, p->plt_table);
        p->osd_plt_cfg.change = MPP_ENC_OSD_PLT_CFG_CHANGE_ALL;
        p->osd_plt_cfg.type = MPP_ENC_OSD_PLT_TYPE_USERDEF;
        p->osd_plt_cfg.plt = &p->osd_plt;

        ret = mpi->control(ctx, MPP_ENC_SET_OSD_PLT_CFG, &p->osd_plt_cfg);
        if (ret) {
            mpp_err("mpi control enc set osd plt failed ret %d\n", ret);
            goto RET;
        }
    }

RET:
    return ret;
}

MPP_RET test_mpp_run(MpiEncTestData *p)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi;
    MppCtx ctx;

    if (NULL == p)
        return MPP_ERR_NULL_PTR;

    mpi = p->mpi;
    ctx = p->ctx;

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        MppPacket packet = NULL;
        ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
            goto RET;
        }

        /* get and write sps/pps for H.264 */
        if (packet) {
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);

            if (p->fp_output)
                fwrite(ptr, 1, len, p->fp_output);

            packet = NULL;
        }
    }

    while (!p->pkt_eos) {
        MppFrame frame = NULL;
        MppPacket packet = NULL;
        void *buf = mpp_buffer_get_ptr(p->frm_buf);

        if (p->fp_input) {
            ret = read_image(buf, p->fp_input, p->width, p->height,
                             p->hor_stride, p->ver_stride, p->fmt);
            if (ret == MPP_NOK || feof(p->fp_input)) {
                mpp_log("found last frame. feof %d\n", feof(p->fp_input));
                p->frm_eos = 1;
            } else if (ret == MPP_ERR_VALUE)
                goto RET;
        } else {
            ret = fill_image(buf, p->width, p->height, p->hor_stride,
                             p->ver_stride, p->fmt, p->frame_count);
            if (ret)
                goto RET;
        }

        ret = mpp_frame_init(&frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            goto RET;
        }

        mpp_frame_set_width(frame, p->width);
        mpp_frame_set_height(frame, p->height);
        mpp_frame_set_hor_stride(frame, p->hor_stride);
        mpp_frame_set_ver_stride(frame, p->ver_stride);
        mpp_frame_set_fmt(frame, p->fmt);
        mpp_frame_set_eos(frame, p->frm_eos);

        if (p->ref.max_lt_ref_cnt) {
            // force idr as reference every 15 frames
            //RK_S32 quotient = p->frame_count / 15;
            RK_S32 remainder = p->frame_count % 15;

            if (p->frame_count && remainder == 0) {
                MppMeta meta = mpp_frame_get_meta(frame);
                static RK_S32 lt_ref_idx = 0;

                mpp_log("force lt_ref %d\n", lt_ref_idx);
                mpp_meta_set_s32(meta, KEY_LONG_REF_IDX, lt_ref_idx);
                lt_ref_idx++;
                if (lt_ref_idx >= p->ref.max_lt_ref_cnt)
                    lt_ref_idx = 0;
            } else
                mpp_frame_set_meta(frame, NULL);
        }
        if (p->fp_input && feof(p->fp_input))
            mpp_frame_set_buffer(frame, NULL);
        else
            mpp_frame_set_buffer(frame, p->frm_buf);

        MppEncUserData user_data;
        char *str = "Hello world hahahaha lalalala\n";

        {
            MppMeta meta = mpp_frame_get_meta(frame);

            if ((p->frame_count & 2) == 0) {
                user_data.pdata = str;
                user_data.len = strlen(str) + 1;
                mpp_meta_set_ptr(meta, KEY_USER_DATA, &user_data);
            }

            if (p->osd_enable) {
                /* gen and cfg osd plt */
                mpi_enc_gen_osd_data(&p->osd_data, p->osd_idx_buf, p->frame_count);
                mpp_meta_set_ptr(meta, KEY_OSD_DATA, (void*)&p->osd_data);
            }
        }

        /*
         * NOTE: in non-block mode the frame can be resent.
         * The default input timeout mode is block.
         *
         * User should release the input frame to meet the requirements of
         * resource creator must be the resource destroyer.
         */
        ret = mpi->encode_put_frame(ctx, frame);
        if (ret) {
            mpp_err("mpp encode put frame failed\n");
            mpp_frame_deinit(&frame);
            goto RET;
        }
        mpp_frame_deinit(&frame);

        ret = mpi->encode_get_packet(ctx, &packet);
        if (ret) {
            mpp_err("mpp encode get packet failed\n");
            goto RET;
        }

        mpp_assert(packet);

        if (packet) {
            // write packet to file here
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);
            MppMeta meta = mpp_packet_get_meta(packet);
            RK_S32 temporal_id = 0;

            ret = mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id);
            if (ret == MPP_OK)
                mpp_log_f("get temporal id %d\n", temporal_id);
            ret = MPP_OK;

            p->pkt_eos = mpp_packet_get_eos(packet);

            if (p->fp_output)
                fwrite(ptr, 1, len, p->fp_output);
            mpp_packet_deinit(&packet);

            mpp_log_f("encoded frame %d size %d\n", p->frame_count, len);
            p->stream_size += len;
            p->frame_count++;

            if (p->pkt_eos) {
                mpp_log("found last packet\n");
                mpp_assert(p->frm_eos);
            }
        }

        if (p->num_frames && p->frame_count >= p->num_frames) {
            mpp_log_f("encode max %d frames", p->frame_count);
            break;
        }
        if (p->frm_eos && p->pkt_eos)
            break;
    }
RET:

    return ret;
}

int mpi_enc_test(MpiEncTestArgs *cmd)
{
    MPP_RET ret = MPP_OK;
    MpiEncTestData *p = NULL;
    MppPollType timeout = MPP_POLL_BLOCK;

    mpp_log("mpi_enc_test start\n");

    ret = test_ctx_init(&p, cmd);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(NULL, &p->frm_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(NULL, &p->osd_idx_buf, p->osd_idx_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input osd index ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mpp_log("mpi_enc_test encoder test start w %d h %d type %d\n",
            p->width, p->height, p->type);

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = test_mpp_setup(p);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = test_mpp_run(p);
    if (ret) {
        mpp_err_f("test mpp run failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->reset(p->ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->frm_buf) {
        mpp_buffer_put(p->frm_buf);
        p->frm_buf = NULL;
    }

    if (p->osd_idx_buf) {
        mpp_buffer_put(p->osd_idx_buf);
        p->osd_idx_buf = NULL;
    }

    if (MPP_OK == ret)
        mpp_log("mpi_enc_test success total frame %d bps %lld\n",
                p->frame_count, (RK_U64)((p->stream_size * 8 * p->fps) / p->frame_count));
    else
        mpp_err("mpi_enc_test failed ret %d\n", ret);

    test_ctx_deinit(&p);

    return ret;
}

int main(int argc, char **argv)
{
    RK_S32 ret = MPP_NOK;
    MpiEncTestArgs* cmd = mpi_enc_test_cmd_get();

    memset((void*)cmd, 0, sizeof(*cmd));

    // parse the cmd option
    if (argc > 1)
        ret = mpi_enc_test_cmd_update_by_args(cmd, argc, argv);

    if (ret) {
        mpi_enc_test_help();
        return ret;
    }

    mpi_enc_test_cmd_show_opt(cmd);

    ret = mpi_enc_test(cmd);

    mpi_enc_test_cmd_put(cmd);

    return ret;
}

