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

#define MODULE_TAG "vpu_api_legacy"

#include <fcntl.h>
#include "string.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "vpu_api_legacy.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer_impl.h"
#include "mpp_frame.h"

#define VPU_API_ENC_INPUT_TIMEOUT 100

RK_U32 vpu_api_debug = 0;

static MppFrameFormat vpu_pic_type_remap_to_mpp(EncInputPictureType type)
{
    MppFrameFormat ret = MPP_FMT_BUTT;
    switch (type) {
    case ENC_INPUT_YUV420_PLANAR : {
        ret = MPP_FMT_YUV420P;
    } break;
    case ENC_INPUT_YUV420_SEMIPLANAR : {
        ret = MPP_FMT_YUV420SP;
    } break;
    case ENC_INPUT_YUV422_INTERLEAVED_YUYV : {
        ret = MPP_FMT_YUV422_YUYV;
    } break;
    case ENC_INPUT_YUV422_INTERLEAVED_UYVY : {
        ret = MPP_FMT_YUV422_UYVY;
    } break;
    case ENC_INPUT_RGB565 : {
        ret = MPP_FMT_RGB565;
    } break;
    case ENC_INPUT_BGR565 : {
        ret = MPP_FMT_BGR565;
    } break;
    case ENC_INPUT_RGB555 : {
        ret = MPP_FMT_RGB555;
    } break;
    case ENC_INPUT_BGR555 : {
        ret = MPP_FMT_BGR555;
    } break;
    case ENC_INPUT_RGB444 : {
        ret = MPP_FMT_RGB444;
    } break;
    case ENC_INPUT_BGR444 : {
        ret = MPP_FMT_BGR444;
    } break;
    case ENC_INPUT_RGB888 : {
        ret = MPP_FMT_RGBA8888;
    } break;
    case ENC_INPUT_BGR888 : {
        ret = MPP_FMT_BGRA8888;
    } break;
    case ENC_INPUT_RGB101010 : {
        ret = MPP_FMT_RGB101010;
    } break;
    case ENC_INPUT_BGR101010 : {
        ret = MPP_FMT_BGR101010;
    } break;
    default : {
        mpp_err("There is no match format, err!!!!!!");
    } break;
    }
    return ret;
}

static MPP_RET vpu_api_set_enc_cfg(MppCtx mpp_ctx, MppApi *mpi, MppEncCfg enc_cfg,
                                   MppCodingType coding, MppFrameFormat fmt,
                                   EncParameter_t *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_S32 width    = cfg->width;
    RK_S32 height   = cfg->height;
    RK_S32 bps      = cfg->bitRate;
    RK_S32 fps_in   = cfg->framerate;
    RK_S32 fps_out  = (cfg->framerateout) ? (cfg->framerateout) : (fps_in);
    RK_S32 gop      = (cfg->intraPicRate) ? (cfg->intraPicRate) : (fps_out);
    RK_S32 qp_init  = (coding == MPP_VIDEO_CodingAVC) ? (26) :
                      (coding == MPP_VIDEO_CodingMJPEG) ? (10) :
                      (coding == MPP_VIDEO_CodingVP8) ? (56) :
                      (coding == MPP_VIDEO_CodingHEVC) ? (26) : (0);
    RK_S32 qp       = (cfg->qp) ? (cfg->qp) : (qp_init);
    RK_S32 profile  = cfg->profileIdc;
    RK_S32 level    = cfg->levelIdc;
    RK_S32 cabac_en = cfg->enableCabac;
    RK_S32 rc_mode  = cfg->rc_mode;
    RK_U32 is_fix_qp = (rc_mode == MPP_ENC_RC_MODE_FIXQP) ? 1 : 0;

    mpp_log("setup encoder rate control config:\n");
    mpp_log("width %4d height %4d format %d:%x\n", width, height, cfg->format, fmt);
    mpp_log("rc_mode %s qp %d bps %d\n", (rc_mode) ? ("CBR") : ("CQP"), qp, bps);
    mpp_log("fps in %d fps out %d gop %d\n", fps_in, fps_out, gop);
    mpp_log("setup encoder stream feature config:\n");
    mpp_log("profile %d level %d cabac %d\n", profile, level, cabac_en);

    mpp_assert(width);
    mpp_assert(height);
    mpp_assert(qp);

    mpp_enc_cfg_set_s32(enc_cfg, "prep:width", width);
    mpp_enc_cfg_set_s32(enc_cfg, "prep:height", height);
    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP :
    case MPP_FMT_YUV420SP_VU : {
        mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride", MPP_ALIGN(width, 16));
    } break;
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555: {
        mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride", 2 * MPP_ALIGN(width, 16));
    } break;
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 : {
        mpp_enc_cfg_set_s32(enc_cfg, "prep:hor_stride", 4 * MPP_ALIGN(width, 16));
    } break;
    default: {
        mpp_err("unsupport format 0x%x\n", fmt & MPP_FRAME_FMT_MASK);
    } break;
    }
    mpp_enc_cfg_set_s32(enc_cfg, "prep:ver_stride", MPP_ALIGN(height, 8));
    mpp_enc_cfg_set_s32(enc_cfg, "prep:format", fmt);

    mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", is_fix_qp ? MPP_ENC_RC_MODE_FIXQP :
                        (rc_mode ? MPP_ENC_RC_MODE_CBR : MPP_ENC_RC_MODE_VBR));
    mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps * 17 / 16);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", rc_mode ? bps * 15 / 16 : bps * 1 / 16);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_flex", 0);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_num", fps_in);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_in_denorm", 1);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_flex", 0);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_num", fps_out);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:fps_out_denorm", 1);
    mpp_enc_cfg_set_s32(enc_cfg, "rc:gop", gop);

    mpp_enc_cfg_set_s32(enc_cfg, "codec:type", coding);
    switch (coding) {
    case MPP_VIDEO_CodingAVC : {
        mpp_enc_cfg_set_s32(enc_cfg, "h264:profile", profile);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:level", level);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:cabac_en", cabac_en);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:qp_init", is_fix_qp ? qp : -1);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:qp_min", is_fix_qp  ? qp : 10);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:qp_max", is_fix_qp ? qp : 51);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:qp_min_i", 10);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:qp_max_i", 51);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:qp_step", 4);
        mpp_enc_cfg_set_s32(enc_cfg, "h264:qp_delta_ip", 3);
    } break;
    case MPP_VIDEO_CodingVP8 : {
        mpp_enc_cfg_set_s32(enc_cfg, "vp8:qp_init", -1);
        mpp_enc_cfg_set_s32(enc_cfg, "vp8:qp_min", 0);
        mpp_enc_cfg_set_s32(enc_cfg, "vp8:qp_max", 127);
        mpp_enc_cfg_set_s32(enc_cfg, "vp8:qp_min_i", 0);
        mpp_enc_cfg_set_s32(enc_cfg, "vp8:qp_max_i", 127);
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        mpp_enc_cfg_set_s32(enc_cfg, "jpeg:quant", qp);
    } break;
    default : {
        mpp_err_f("support encoder coding type %d\n", coding);
    } break;
    }

    ret = mpi->control(mpp_ctx, MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
        mpp_err("setup enc config failed ret %d\n", ret);
        goto RET;
    }
RET:
    return ret;
}

static int is_valid_dma_fd(int fd)
{
    int ret = 1;
    /* detect input file handle */
    int fs_flag = fcntl(fd, F_GETFL, NULL);
    int fd_flag = fcntl(fd, F_GETFD, NULL);

    if (fs_flag == -1 || fd_flag == -1) {
        ret = 0;
    }

    return ret;
}

static int copy_align_raw_buffer_to_dest(RK_U8 *dst, RK_U8 *src, RK_U32 width,
                                         RK_U32 height, MppFrameFormat fmt)
{
    int ret = 1;
    RK_U32 index = 0;
    RK_U8 *dst_buf = dst;
    RK_U8 *src_buf = src;
    RK_U32 row = 0;
    RK_U32 hor_stride = MPP_ALIGN(width, 16);
    RK_U32 ver_stride = MPP_ALIGN(height, 8);
    RK_U8 *dst_u = dst_buf + hor_stride * ver_stride;
    RK_U8 *dst_v = dst_u + hor_stride * ver_stride / 4;

    switch (fmt) {
    case MPP_FMT_YUV420SP : {
        for (row = 0; row < height; row++) {
            memcpy(dst_buf + row * hor_stride, src_buf + index, width);
            index += width;
        }
        for (row = 0; row < height / 2; row++) {
            memcpy(dst_u + row * hor_stride, src_buf + index, width);
            index += width;
        }
    } break;
    case MPP_FMT_YUV420P : {
        for (row = 0; row < height; row++) {
            memcpy(dst_buf + row * hor_stride, src_buf + index, width);
            index += width;
        }
        for (row = 0; row < height / 2; row++) {
            memcpy(dst_u + row * hor_stride / 2, src_buf + index, width / 2);
            index += width / 2;
        }
        for (row = 0; row < height / 2; row++) {
            memcpy(dst_v + row * hor_stride / 2, src_buf + index, width / 2);
            index += width / 2;
        }
    } break;
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_ARGB8888 : {
        for (row = 0; row < height; row++) {
            memcpy(dst_buf + row * hor_stride * 4, src_buf + row * width * 4, width * 4);
        }
    } break;
    default : {
        mpp_err("unsupport align fmt:%d now\n", fmt);
    } break;
    }

    return ret;
}

VpuApiLegacy::VpuApiLegacy() :
    mpp_ctx(NULL),
    mpi(NULL),
    init_ok(0),
    frame_count(0),
    set_eos(0),
    memGroup(NULL),
    format(MPP_FMT_YUV420P),
    fd_input(-1),
    fd_output(-1),
    mEosSet(0),
    enc_cfg(NULL),
    enc_hdr_pkt(NULL),
    enc_hdr_buf(NULL),
    enc_hdr_buf_size(0)
{
    vpu_api_dbg_func("enter\n");

    mpp_create(&mpp_ctx, &mpi);

    memset(&enc_param, 0, sizeof(enc_param));

    mlvec = NULL;
    memset(&mlvec_dy_cfg, 0, sizeof(mlvec_dy_cfg));

    vpu_api_dbg_func("leave\n");
}

VpuApiLegacy::~VpuApiLegacy()
{
    vpu_api_dbg_func("enter\n");

    mpp_destroy(mpp_ctx);

    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    if (enc_cfg) {
        mpp_enc_cfg_deinit(enc_cfg);
        enc_cfg = NULL;
    }

    if (mlvec) {
        vpu_api_mlvec_deinit(mlvec);
        mlvec = NULL;
    }

    if (enc_hdr_pkt) {
        mpp_packet_deinit(&enc_hdr_pkt);
        enc_hdr_pkt = NULL;
    }
    MPP_FREE(enc_hdr_buf);
    enc_hdr_buf_size = 0;

    vpu_api_dbg_func("leave\n");
}

static RK_S32 init_frame_info(VpuCodecContext *ctx,
                              MppCtx mpp_ctx, MppApi *mpi, VPU_GENERIC *p)
{
    RK_S32 ret = -1;
    MppFrame frame = NULL;
    RK_U32 fbcOutFmt = 0;

    if (ctx->private_data)
        fbcOutFmt = *(RK_U32 *)ctx->private_data;

    if (ctx->extra_cfg.bit_depth
        || ctx->extra_cfg.yuv_format) {
        if (ctx->extra_cfg.bit_depth == 10)
            p->CodecType = (ctx->extra_cfg.yuv_format == 1)
                           ? MPP_FMT_YUV422SP_10BIT : MPP_FMT_YUV420SP_10BIT;
        else
            p->CodecType = (ctx->extra_cfg.yuv_format == 1)
                           ? MPP_FMT_YUV422SP : MPP_FMT_YUV420SP;
    } else {
        /**hightest of p->ImgWidth bit show current dec bitdepth
          * 0 - 8bit
          * 1 - 10bit
          **/
        if (p->ImgWidth & 0x80000000)
            p->CodecType = (p->ImgWidth & 0x40000000)
                           ? MPP_FMT_YUV422SP_10BIT : MPP_FMT_YUV420SP_10BIT;
        else
            p->CodecType = (p->ImgWidth & 0x40000000)
                           ? MPP_FMT_YUV422SP : MPP_FMT_YUV420SP;
    }
    p->ImgWidth = (p->ImgWidth & 0xFFFF);

    mpp_frame_init(&frame);

    mpp_frame_set_width(frame, p->ImgWidth);
    mpp_frame_set_height(frame, p->ImgHeight);
    mpp_frame_set_fmt(frame, (MppFrameFormat)(p->CodecType | fbcOutFmt));

    ret = mpi->control(mpp_ctx, MPP_DEC_SET_FRAME_INFO, (MppParam)frame);
    /* output the parameters used */
    p->ImgHorStride = mpp_frame_get_hor_stride(frame);
    p->ImgVerStride = mpp_frame_get_ver_stride(frame);
    p->BufSize = mpp_frame_get_buf_size(frame);

    mpp_frame_deinit(&frame);

    return ret;
}


RK_S32 VpuApiLegacy::init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size)
{
    vpu_api_dbg_func("enter\n");

    MPP_RET ret = MPP_OK;
    MppCtxType type;

    if (mpp_ctx == NULL || mpi == NULL) {
        mpp_err("found invalid context input");
        return MPP_ERR_NULL_PTR;
    }

    if (CODEC_DECODER == ctx->codecType) {
        type = MPP_CTX_DEC;
    } else if (CODEC_ENCODER == ctx->codecType) {
        type = MPP_CTX_ENC;
    } else {
        mpp_err("found invalid codec type %d\n", ctx->codecType);
        return MPP_ERR_VPU_CODEC_INIT;
    }

    ret = mpp_init(mpp_ctx, type, (MppCodingType)ctx->videoCoding);
    if (ret) {
        mpp_err_f(" init error. \n");
        return ret;
    }

    if (MPP_CTX_ENC == type) {
        EncParameter_t *param = (EncParameter_t*)ctx->private_data;
        MppCodingType coding = (MppCodingType)ctx->videoCoding;
        MppPollType block = (MppPollType)VPU_API_ENC_INPUT_TIMEOUT;
        MppEncSeiMode sei_mode = MPP_ENC_SEI_MODE_DISABLE;

        /* setup input / output block mode */
        ret = mpi->control(mpp_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
        if (MPP_OK != ret)
            mpp_err("mpi control MPP_SET_INPUT_TIMEOUT failed\n");

        /* disable sei by default */
        ret = mpi->control(mpp_ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
        if (ret)
            mpp_err("mpi control MPP_ENC_SET_SEI_CFG failed ret %d\n", ret);

        if (memGroup == NULL) {
            ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_ION);
            if (ret) {
                mpp_err("memGroup mpp_buffer_group_get failed %d\n", ret);
                return ret;
            }
        }

        ret = mpp_enc_cfg_init(&enc_cfg);
        if (ret) {
            mpp_err("mpp_enc_cfg_init failed %d\n", ret);
            mpp_buffer_group_put(memGroup);
            memGroup = NULL;
            return ret;
        }

        format = vpu_pic_type_remap_to_mpp((EncInputPictureType)param->format);

        memcpy(&enc_param, param, sizeof(enc_param));

        if (MPP_OK == vpu_api_mlvec_check_cfg(param)) {
            if (NULL == mlvec) {
                vpu_api_mlvec_init(&mlvec);
                vpu_api_mlvec_setup(mlvec, mpp_ctx, mpi, enc_cfg);
            }
        }

        if (mlvec)
            vpu_api_mlvec_set_st_cfg(mlvec, (VpuApiMlvecStaticCfg *)param);

        vpu_api_set_enc_cfg(mpp_ctx, mpi, enc_cfg, coding, format, param);

        if (!mlvec) {
            if (NULL == enc_hdr_pkt) {
                if (NULL == enc_hdr_buf) {
                    enc_hdr_buf_size = SZ_1K;
                    enc_hdr_buf = mpp_calloc_size(RK_U8, enc_hdr_buf_size);
                }

                if (enc_hdr_buf)
                    mpp_packet_init(&enc_hdr_pkt, enc_hdr_buf, enc_hdr_buf_size);
            }

            mpp_assert(enc_hdr_pkt);
            if (enc_hdr_pkt) {
                ret = mpi->control(mpp_ctx, MPP_ENC_GET_HDR_SYNC, enc_hdr_pkt);
                ctx->extradata_size = mpp_packet_get_length(enc_hdr_pkt);
                ctx->extradata      = mpp_packet_get_data(enc_hdr_pkt);
            }
        }
    } else { /* MPP_CTX_DEC */
        vpug.CodecType  = ctx->codecType;
        vpug.ImgWidth   = ctx->width;
        vpug.ImgHeight  = ctx->height;

        init_frame_info(ctx, mpp_ctx, mpi, &vpug);

        if (extraData != NULL) {
            MppPacket pkt = NULL;

            mpp_packet_init(&pkt, extraData, extra_size);
            mpp_packet_set_extra_data(pkt);
            mpi->decode_put_packet(mpp_ctx, pkt);
            mpp_packet_deinit(&pkt);
        }

        RK_U32 flag = 0;
        ret = mpi->control(mpp_ctx, MPP_DEC_SET_ENABLE_DEINTERLACE, &flag);
        if (ret)
            mpp_err_f("disable mpp deinterlace failed ret %d\n", ret);
    }

    init_ok = 1;

    vpu_api_dbg_func("leave\n");
    return ret;
}

RK_S32 VpuApiLegacy::flush(VpuCodecContext *ctx)
{
    (void)ctx;
    vpu_api_dbg_func("enter\n");
    if (mpi && mpi->reset && init_ok) {
        mpi->reset(mpp_ctx);
        set_eos = 0;
        mEosSet = 0;
    }
    vpu_api_dbg_func("leave\n");
    return 0;
}

static void setup_VPU_FRAME_from_mpp_frame(VPU_FRAME *vframe, MppFrame mframe)
{
    MppBuffer buf = mpp_frame_get_buffer(mframe);
    RK_U64 pts  = mpp_frame_get_pts(mframe);
    RK_U32 mode = mpp_frame_get_mode(mframe);

    MppFrameColorRange colorRan = mpp_frame_get_color_range(mframe);
    MppFrameColorTransferCharacteristic colorTrc = mpp_frame_get_color_trc(mframe);
    MppFrameColorPrimaries colorPri = mpp_frame_get_color_primaries(mframe);
    MppFrameColorSpace colorSpa = mpp_frame_get_colorspace(mframe);

    if (buf)
        mpp_buffer_inc_ref(buf);

    vframe->DisplayWidth = mpp_frame_get_width(mframe);
    vframe->DisplayHeight = mpp_frame_get_height(mframe);
    vframe->FrameWidth = mpp_frame_get_hor_stride(mframe);
    vframe->FrameHeight = mpp_frame_get_ver_stride(mframe);

    vframe->ColorRange = (colorRan == MPP_FRAME_RANGE_JPEG);
    vframe->ColorPrimaries = colorPri;
    vframe->ColorTransfer = colorTrc;
    vframe->ColorCoeffs = colorSpa;

    if (mode == MPP_FRAME_FLAG_FRAME)
        vframe->FrameType = 0;
    else {
        RK_U32 field_order = mode & MPP_FRAME_FLAG_FIELD_ORDER_MASK;
        if (field_order == MPP_FRAME_FLAG_TOP_FIRST)
            vframe->FrameType = 1;
        else if (field_order == MPP_FRAME_FLAG_BOT_FIRST)
            vframe->FrameType = 2;
        else if (field_order == MPP_FRAME_FLAG_DEINTERLACED)
            vframe->FrameType = 4;
    }
    vframe->ErrorInfo = mpp_frame_get_errinfo(mframe) | mpp_frame_get_discard(mframe);
    vframe->ShowTime.TimeHigh = (RK_U32)(pts >> 32);
    vframe->ShowTime.TimeLow = (RK_U32)pts;
    switch (mpp_frame_get_fmt(mframe) & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP: {
        vframe->ColorType = VPU_OUTPUT_FORMAT_YUV420_SEMIPLANAR;
        vframe->OutputWidth = 0x20;
    } break;
    case MPP_FMT_YUV420SP_10BIT: {
        vframe->ColorType = VPU_OUTPUT_FORMAT_YUV420_SEMIPLANAR;
        vframe->ColorType |= VPU_OUTPUT_FORMAT_BIT_10;
        vframe->OutputWidth = 0x22;
    } break;
    case MPP_FMT_YUV422SP: {
        vframe->ColorType = VPU_OUTPUT_FORMAT_YUV422;
        vframe->OutputWidth = 0x10;
    } break;
    case MPP_FMT_YUV422SP_10BIT: {
        vframe->ColorType = VPU_OUTPUT_FORMAT_YUV422;
        vframe->ColorType |= VPU_OUTPUT_FORMAT_BIT_10;
        vframe->OutputWidth = 0x23;
    } break;
    default: {
    } break;
    }

    switch (colorPri) {
    case MPP_FRAME_PRI_BT2020: {
        vframe->ColorType |= VPU_OUTPUT_FORMAT_COLORSPACE_BT2020;
    } break;
    case MPP_FRAME_PRI_BT709: {
        vframe->ColorType |= VPU_OUTPUT_FORMAT_COLORSPACE_BT709;
    } break;
    default: {
    } break;
    }

    switch (colorTrc) {
    case MPP_FRAME_TRC_SMPTEST2084: {
        vframe->ColorType |= VPU_OUTPUT_FORMAT_DYNCRANGE_HDR10; //HDR10
    } break;
    case MPP_FRAME_TRC_ARIB_STD_B67: {
        vframe->ColorType |= VPU_OUTPUT_FORMAT_DYNCRANGE_HDR_HLG; //HDR_HLG
    } break;
    case MPP_FRAME_TRC_BT2020_10: {
        vframe->ColorType |= VPU_OUTPUT_FORMAT_COLORSPACE_BT2020; //BT2020
    } break;
    default: {
    } break;
    }

    if (buf) {
        MppBufferImpl *p = (MppBufferImpl*)buf;
        void *ptr = (p->mode == MPP_BUFFER_INTERNAL) ?
                    mpp_buffer_get_ptr(buf) : NULL;
        RK_S32 fd = mpp_buffer_get_fd(buf);

        vframe->FrameBusAddr[0] = fd;
        vframe->FrameBusAddr[1] = fd;
        vframe->vpumem.vir_addr = (RK_U32*)ptr;
        vframe->vpumem.phy_addr = fd;

        vframe->vpumem.size = vframe->FrameWidth * vframe->FrameHeight * 3 / 2;
        vframe->vpumem.offset = (RK_U32*)buf;
    }
}

RK_S32 VpuApiLegacy::decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut)
{
    MPP_RET ret = MPP_OK;
    MppFrame mframe = NULL;
    MppPacket packet = NULL;

    vpu_api_dbg_func("enter\n");

    if (ctx->videoCoding == OMX_RK_VIDEO_CodingMJPEG) {
        MppTask task = NULL;

        if (!init_ok) {
            mpp_err("init failed!\n");
            return VPU_API_ERR_VPU_CODEC_INIT;
        }

        /* check input param */
        if (!pkt || !aDecOut) {
            mpp_err("invalid input %p and output %p\n", pkt, aDecOut);
            return VPU_API_ERR_UNKNOW;
        }

        if (pkt->size <= 0) {
            mpp_err("invalid input size %d\n", pkt->size);
            return VPU_API_ERR_UNKNOW;
        }

        /* try import input buffer and output buffer */
        RK_S32 fd           = -1;
        RK_U32 width        = ctx->width;
        RK_U32 height       = ctx->height;
        RK_U32 hor_stride   = MPP_ALIGN(width,  16);
        RK_U32 ver_stride   = MPP_ALIGN(height, 16);
        MppBuffer   str_buf = NULL; /* input */
        MppBuffer   pic_buf = NULL; /* output */

        ret = mpp_frame_init(&mframe);
        if (MPP_OK != ret) {
            mpp_err_f("mpp_frame_init failed\n");
            goto DECODE_OUT;
        }

        fd = (RK_S32)(pkt->pts & 0xffffffff);
        if (fd_input < 0) {
            fd_input = is_valid_dma_fd(fd);
        }

        if (fd_input) {
            MppBufferInfo   inputCommit;

            memset(&inputCommit, 0, sizeof(inputCommit));
            inputCommit.type = MPP_BUFFER_TYPE_ION;
            inputCommit.size = pkt->size;
            inputCommit.fd = fd;

            ret = mpp_buffer_import(&str_buf, &inputCommit);
            if (ret) {
                mpp_err_f("import input picture buffer failed\n");
                goto DECODE_OUT;
            }
        } else {
            if (NULL == pkt->data) {
                ret = MPP_ERR_NULL_PTR;
                goto DECODE_OUT;
            }

            ret = mpp_buffer_get(memGroup, &str_buf, pkt->size);
            if (ret) {
                mpp_err_f("allocate input picture buffer failed\n");
                goto DECODE_OUT;
            }
            memcpy((RK_U8*) mpp_buffer_get_ptr(str_buf), pkt->data, pkt->size);
        }

        fd = (RK_S32)(aDecOut->timeUs & 0xffffffff);
        if (fd_output < 0) {
            fd_output = is_valid_dma_fd(fd);
        }

        if (fd_output) {
            MppBufferInfo outputCommit;

            memset(&outputCommit, 0, sizeof(outputCommit));
            /* in order to avoid interface change use space in output to transmit information */
            outputCommit.type = MPP_BUFFER_TYPE_ION;
            outputCommit.fd = fd;
            outputCommit.size = width * height * 3 / 2;
            outputCommit.ptr = (void*)aDecOut->data;

            ret = mpp_buffer_import(&pic_buf, &outputCommit);
            if (ret) {
                mpp_err_f("import output stream buffer failed\n");
                goto DECODE_OUT;
            }
        } else {
            ret = mpp_buffer_get(memGroup, &pic_buf, hor_stride * ver_stride * 3 / 2);
            if (ret) {
                mpp_err_f("allocate output stream buffer failed\n");
                goto DECODE_OUT;
            }
        }

        mpp_packet_init_with_buffer(&packet, str_buf); /* input */
        mpp_frame_set_buffer(mframe, pic_buf); /* output */

        vpu_api_dbg_func("mpp import input fd %d output fd %d",
                         mpp_buffer_get_fd(str_buf), mpp_buffer_get_fd(pic_buf));

        ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
        if (ret) {
            mpp_err("mpp input poll failed\n");
            goto DECODE_OUT;
        }

        ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
        if (ret) {
            mpp_err("mpp task input dequeue failed\n");
            goto DECODE_OUT;
        }

        mpp_task_meta_set_packet(task, KEY_INPUT_PACKET, packet);
        mpp_task_meta_set_frame (task, KEY_OUTPUT_FRAME, mframe);

        ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
        if (ret) {
            mpp_err("mpp task input enqueue failed\n");
            goto DECODE_OUT;
        }

        pkt->size = 0;
        task = NULL;

        ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
        if (ret) {
            mpp_err("mpp output poll failed\n");
            goto DECODE_OUT;
        }

        ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task);
        if (ret) {
            mpp_err("ret %d mpp task output dequeue failed\n", ret);
            goto DECODE_OUT;
        }

        if (task) {
            MppFrame frame_out = NULL;

            mpp_task_meta_get_frame(task, KEY_OUTPUT_FRAME, &frame_out);
            mpp_assert(frame_out == mframe);
            vpu_api_dbg_func("decoded frame %d\n", frame_count);
            frame_count++;

            ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
            if (ret) {
                mpp_err("mpp task output enqueue failed\n");
                goto DECODE_OUT;
            }
            task = NULL;
        }

        // copy decoded frame into output buffer, and set outpub frame size
        if (mframe != NULL) {
            MppBuffer buf_out = mpp_frame_get_buffer(mframe);
            size_t len  = mpp_buffer_get_size(buf_out);
            aDecOut->size = len;

            if (fd_output) {
                mpp_log_f("fd for output is invalid!\n");
                // TODO: check frame format and allocate correct buffer
                aDecOut->data = mpp_malloc(RK_U8, width * height * 3 / 2);
                memcpy(aDecOut->data, (RK_U8*) mpp_buffer_get_ptr(pic_buf), aDecOut->size);
            }

            vpu_api_dbg_func("get frame %p size %d\n", mframe, len);

            mpp_frame_deinit(&mframe);
        } else {
            mpp_log("outputPacket is NULL!");
        }

    DECODE_OUT:
        if (str_buf) {
            mpp_buffer_put(str_buf);
            str_buf = NULL;
        }

        if (pic_buf) {
            mpp_buffer_put(pic_buf);
            pic_buf = NULL;
        }
    } else {
        mpp_packet_init(&packet, pkt->data, pkt->size);
        mpp_packet_set_pts(packet, pkt->pts);
        if (pkt->nFlags & OMX_BUFFERFLAG_EOS) {
            mpp_packet_set_eos(packet);
        }

        vpu_api_dbg_input("input size %-6d flag %x pts %lld\n",
                          pkt->size, pkt->nFlags, pkt->pts);

        ret = mpi->decode(mpp_ctx, packet, &mframe);
        if (MPP_OK == ret) {
            pkt->size = 0;
        }
        if (ret || NULL == mframe) {
            aDecOut->size = 0;
        } else {
            VPU_FRAME *vframe = (VPU_FRAME *)aDecOut->data;
            MppBuffer buf = mpp_frame_get_buffer(mframe);

            setup_VPU_FRAME_from_mpp_frame(vframe, mframe);

            aDecOut->size = sizeof(VPU_FRAME);
            aDecOut->timeUs = mpp_frame_get_pts(mframe);
            frame_count++;

            if (mpp_frame_get_eos(mframe)) {
                set_eos = 1;
                if (buf == NULL) {
                    aDecOut->size = 0;
                }
            }
            if (vpu_api_debug & VPU_API_DBG_OUTPUT) {
                mpp_log("get one frame pts %lld, fd 0x%x, poc %d, errinfo %x, discard %d, eos %d, verr %d",
                        aDecOut->timeUs,
                        ((buf) ? (mpp_buffer_get_fd(buf)) : (-1)),
                        mpp_frame_get_poc(mframe),
                        mpp_frame_get_errinfo(mframe),
                        mpp_frame_get_discard(mframe),
                        mpp_frame_get_eos(mframe), vframe->ErrorInfo);
            }

            /*
             * IMPORTANT: mframe is malloced from mpi->decode_get_frame
             * So we need to deinit mframe here. But the buffer in the frame should not be free with mframe.
             * Because buffer need to be set to vframe->vpumem.offset and send to display.
             * The we have to clear the buffer pointer in mframe then release mframe.
             */
            mpp_frame_deinit(&mframe);
        }
    }

    if (packet)
        mpp_packet_deinit(&packet);

    if (mframe)
        mpp_frame_deinit(&mframe);

    vpu_api_dbg_func("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::decode_sendstream(VideoPacket_t *pkt)
{
    vpu_api_dbg_func("enter\n");

    RK_S32 ret = MPP_OK;
    MppPacket mpkt = NULL;

    if (!init_ok) {
        return VPU_API_ERR_VPU_CODEC_INIT;
    }

    mpp_packet_init(&mpkt, pkt->data, pkt->size);
    mpp_packet_set_pts(mpkt, pkt->pts);
    if (pkt->nFlags & OMX_BUFFERFLAG_EOS) {
        mpp_packet_set_eos(mpkt);
    }

    vpu_api_dbg_input("input size %-6d flag %x pts %lld\n",
                      pkt->size, pkt->nFlags, pkt->pts);

    ret = mpi->decode_put_packet(mpp_ctx, mpkt);
    if (ret == MPP_OK) {
        pkt->size = 0;
    } else {
        /* reduce cpu overhead here */
        msleep(1);
    }

    mpp_packet_deinit(&mpkt);

    vpu_api_dbg_func("leave ret %d\n", ret);
    /* NOTE: always return success for old player compatibility */
    return MPP_OK;
}

RK_S32 VpuApiLegacy::decode_getoutframe(DecoderOut_t *aDecOut)
{
    RK_S32 ret = 0;
    VPU_FRAME *vframe = (VPU_FRAME *)aDecOut->data;
    MppFrame  mframe = NULL;

    vpu_api_dbg_func("enter\n");

    if (!init_ok) {
        return VPU_API_ERR_VPU_CODEC_INIT;
    }

    memset(vframe, 0, sizeof(VPU_FRAME));

    if (NULL == mpi) {
        aDecOut->size = 0;
        return 0;
    }

    if (set_eos) {
        aDecOut->size = 0;
        mEosSet = 1;
        return VPU_API_EOS_STREAM_REACHED;
    }

    ret = mpi->decode_get_frame(mpp_ctx, &mframe);
    if (ret || NULL == mframe) {
        aDecOut->size = 0;
    } else {
        MppBuffer buf = mpp_frame_get_buffer(mframe);

        setup_VPU_FRAME_from_mpp_frame(vframe, mframe);

        aDecOut->size = sizeof(VPU_FRAME);
        aDecOut->timeUs = mpp_frame_get_pts(mframe);
        frame_count++;

        if (mpp_frame_get_eos(mframe) && !mpp_frame_get_info_change(mframe)) {
            set_eos = 1;
            if (buf == NULL) {
                aDecOut->size = 0;
                mEosSet = 1;
                ret = VPU_API_EOS_STREAM_REACHED;
            } else {
                aDecOut->nFlags |= VPU_API_EOS_STREAM_REACHED;
            }
        }
        if (vpu_api_debug & VPU_API_DBG_OUTPUT) {
            mpp_log("get one frame pts %lld, fd 0x%x, poc %d, errinfo %x, discard %d, eos %d, verr %d",
                    aDecOut->timeUs,
                    ((buf) ? (mpp_buffer_get_fd(buf)) : (-1)),
                    mpp_frame_get_poc(mframe),
                    mpp_frame_get_errinfo(mframe),
                    mpp_frame_get_discard(mframe),
                    mpp_frame_get_eos(mframe), vframe->ErrorInfo);
        }

        /*
         * IMPORTANT: mframe is malloced from mpi->decode_get_frame
         * So we need to deinit mframe here. But the buffer in the frame should not be free with mframe.
         * Because buffer need to be set to vframe->vpumem.offset and send to display.
         * The we have to clear the buffer pointer in mframe then release mframe.
         */
        mpp_frame_deinit(&mframe);
    }

    vpu_api_dbg_func("leave ret %d\n", ret);

    return ret;
}

RK_S32 VpuApiLegacy::encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut)
{
    MPP_RET ret = MPP_OK;
    MppTask task = NULL;
    vpu_api_dbg_func("enter\n");

    if (!init_ok)
        return VPU_API_ERR_VPU_CODEC_INIT;

    /* check input param */
    if (!aEncInStrm || !aEncOut) {
        mpp_err("invalid input %p and output %p\n", aEncInStrm, aEncOut);
        return VPU_API_ERR_UNKNOW;
    }

    if (NULL == aEncInStrm->buf || 0 == aEncInStrm->size) {
        mpp_err("invalid input buffer %p size %d\n", aEncInStrm->buf, aEncInStrm->size);
        return VPU_API_ERR_UNKNOW;
    }

    /* try import input buffer and output buffer */
    RK_S32 fd           = -1;
    RK_U32 width        = ctx->width;
    RK_U32 height       = ctx->height;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    MppFrame    frame   = NULL;
    MppPacket   packet  = NULL;
    MppBuffer   pic_buf = NULL;
    MppBuffer   str_buf = NULL;

    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
        mpp_err_f("mpp_frame_init failed\n");
        goto ENCODE_OUT;
    }

    mpp_frame_set_width(frame, width);
    mpp_frame_set_height(frame, height);
    mpp_frame_set_hor_stride(frame, hor_stride);
    mpp_frame_set_ver_stride(frame, ver_stride);

    fd = aEncInStrm->bufPhyAddr;
    if (fd_input < 0) {
        fd_input = is_valid_dma_fd(fd);
    }
    if (fd_input) {
        MppBufferInfo   inputCommit;

        memset(&inputCommit, 0, sizeof(inputCommit));
        inputCommit.type = MPP_BUFFER_TYPE_ION;
        inputCommit.size = aEncInStrm->size;
        inputCommit.fd = fd;

        ret = mpp_buffer_import(&pic_buf, &inputCommit);
        if (ret) {
            mpp_err_f("import input picture buffer failed\n");
            goto ENCODE_OUT;
        }
    } else {
        if (NULL == aEncInStrm->buf) {
            ret = MPP_ERR_NULL_PTR;
            goto ENCODE_OUT;
        }

        ret = mpp_buffer_get(memGroup, &pic_buf, aEncInStrm->size);
        if (ret) {
            mpp_err_f("allocate input picture buffer failed\n");
            goto ENCODE_OUT;
        }
        memcpy((RK_U8*) mpp_buffer_get_ptr(pic_buf), aEncInStrm->buf, aEncInStrm->size);
    }

    fd = (RK_S32)(aEncOut->timeUs & 0xffffffff);

    if (fd_output < 0) {
        fd_output = is_valid_dma_fd(fd);
    }
    if (fd_output) {
        RK_S32 *tmp = (RK_S32*)(&aEncOut->timeUs);
        MppBufferInfo outputCommit;

        memset(&outputCommit, 0, sizeof(outputCommit));
        /* in order to avoid interface change use space in output to transmit information */
        outputCommit.type = MPP_BUFFER_TYPE_ION;
        outputCommit.fd = fd;
        outputCommit.size = tmp[1];
        outputCommit.ptr = (void*)aEncOut->data;

        ret = mpp_buffer_import(&str_buf, &outputCommit);
        if (ret) {
            mpp_err_f("import output stream buffer failed\n");
            goto ENCODE_OUT;
        }
    } else {
        ret = mpp_buffer_get(memGroup, &str_buf, hor_stride * ver_stride);
        if (ret) {
            mpp_err_f("allocate output stream buffer failed\n");
            goto ENCODE_OUT;
        }
    }

    mpp_frame_set_buffer(frame, pic_buf);
    mpp_packet_init_with_buffer(&packet, str_buf);
    mpp_packet_set_length(packet, 0);

    vpu_api_dbg_func("mpp import input fd %d output fd %d",
                     mpp_buffer_get_fd(pic_buf), mpp_buffer_get_fd(str_buf));

    ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
    if (ret) {
        mpp_err("mpp input poll failed\n");
        goto ENCODE_OUT;
    }

    ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
    if (ret) {
        mpp_err("mpp task input dequeue failed\n");
        goto ENCODE_OUT;
    }
    if (task == NULL) {
        mpp_err("mpi dequeue from MPP_PORT_INPUT fail, task equal with NULL!");
        goto ENCODE_OUT;
    }

    mpp_task_meta_set_frame (task, KEY_INPUT_FRAME,  frame);
    mpp_task_meta_set_packet(task, KEY_OUTPUT_PACKET, packet);

    ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
    if (ret) {
        mpp_err("mpp task input enqueue failed\n");
        goto ENCODE_OUT;
    }
    task = NULL;

    ret = mpi->poll(mpp_ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
    if (ret) {
        mpp_err("mpp output poll failed\n");
        goto ENCODE_OUT;
    }

    ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task);
    if (ret) {
        mpp_err("ret %d mpp task output dequeue failed\n", ret);
        goto ENCODE_OUT;
    }

    mpp_assert(task);

    if (task) {
        MppFrame frame_out = NULL;
        MppFrame packet_out = NULL;

        mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, &packet_out);

        mpp_assert(packet_out == packet);
        vpu_api_dbg_func("encoded frame %d\n", frame_count);
        frame_count++;

        ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
        if (ret) {
            mpp_err("mpp task output enqueue failed\n");
            goto ENCODE_OUT;
        }
        task = NULL;

        ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
        if (ret) {
            mpp_err("mpp input poll failed\n");
            goto ENCODE_OUT;
        }

        // dequeue task from MPP_PORT_INPUT
        ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
        if (ret) {
            mpp_log_f("failed to dequeue from input port ret %d\n", ret);
            goto ENCODE_OUT;
        }
        mpp_assert(task);
        ret = mpp_task_meta_get_frame(task, KEY_INPUT_FRAME, &frame_out);
        mpp_assert(frame_out  == frame);
        ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
        if (ret) {
            mpp_err("mpp task output enqueue failed\n");
            goto ENCODE_OUT;
        }
        task = NULL;
    }

    // copy encoded stream into output buffer, and set output stream size
    if (packet) {
        RK_U32 eos = mpp_packet_get_eos(packet);
        RK_S64 pts = mpp_packet_get_pts(packet);
        size_t length = mpp_packet_get_length(packet);
        MppMeta meta = mpp_packet_get_meta(packet);
        RK_S32 is_intra = 0;

        if (!fd_output) {
            RK_U8 *src = (RK_U8 *)mpp_packet_get_data(packet);
            size_t buffer = MPP_ALIGN(length, SZ_4K);

            aEncOut->data = mpp_malloc(RK_U8, buffer);

            if (ctx->videoCoding == OMX_RK_VIDEO_CodingAVC) {
                // remove first 00 00 00 01
                length -= 4;
                memcpy(aEncOut->data, src + 4, length);
            } else {
                memcpy(aEncOut->data, src, length);
            }
        }

        mpp_meta_get_s32(meta, KEY_OUTPUT_INTRA, &is_intra);

        aEncOut->size = (RK_S32)length;
        aEncOut->timeUs = pts;
        aEncOut->keyFrame = is_intra;

        vpu_api_dbg_output("get packet %p size %d pts %lld keyframe %d eos %d\n",
                           packet, length, pts, aEncOut->keyFrame, eos);

        mpp_packet_deinit(&packet);
    } else {
        mpp_log("outputPacket is NULL!");
    }

ENCODE_OUT:
    if (pic_buf) {
        mpp_buffer_put(pic_buf);
        pic_buf = NULL;
    }

    if (str_buf) {
        mpp_buffer_put(str_buf);
        str_buf = NULL;
    }

    if (frame)
        mpp_frame_deinit(&frame);

    if (packet)
        mpp_packet_deinit(&packet);

    vpu_api_dbg_func("leave ret %d\n", ret);

    return ret;
}

RK_S32 VpuApiLegacy::encoder_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm)
{
    RK_S32 ret = 0;
    vpu_api_dbg_func("enter\n");

    RK_U32 width        = ctx->width;
    RK_U32 height       = ctx->height;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 8);
    RK_S64 pts          = aEncInStrm->timeUs;
    RK_S32 fd           = aEncInStrm->bufPhyAddr;
    RK_U32 size         = aEncInStrm->size;

    /* try import input buffer and output buffer */
    MppFrame frame = NULL;

    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
        mpp_err_f("mpp_frame_init failed\n");
        goto FUNC_RET;
    }

    mpp_frame_set_width(frame, width);
    mpp_frame_set_height(frame, height);
    mpp_frame_set_hor_stride(frame, hor_stride);
    mpp_frame_set_ver_stride(frame, ver_stride);
    mpp_frame_set_pts(frame, pts);

    if (aEncInStrm->nFlags) {
        mpp_log_f("found eos true\n");
        mpp_frame_set_eos(frame, 1);
    }

    if (size <= 0) {
        mpp_frame_set_buffer(frame, NULL);
        if (!aEncInStrm->nFlags)
            mpp_err_f("found empty frame without eos flag set!\n");
        goto PUT_FRAME;
    }

    if (fd_input < 0) {
        fd_input = is_valid_dma_fd(fd);
    }
    if (fd_input) {
        MppBufferInfo   inputCommit;

        memset(&inputCommit, 0, sizeof(inputCommit));
        inputCommit.type = MPP_BUFFER_TYPE_ION;
        inputCommit.size = size;
        inputCommit.fd = fd;
        if (size > 0) {
            MppBuffer buffer = NULL;
            ret = mpp_buffer_import(&buffer, &inputCommit);
            if (ret) {
                mpp_err_f("import input picture buffer failed\n");
                goto FUNC_RET;
            }
            mpp_frame_set_buffer(frame, buffer);
            mpp_buffer_put(buffer);
            buffer = NULL;
        }
    } else {
        RK_U32 align_size   = 0;

        if (NULL == aEncInStrm->buf) {
            ret = MPP_ERR_NULL_PTR;
            goto FUNC_RET;
        }
        if (format >= MPP_FMT_YUV420SP && format < MPP_FMT_YUV_BUTT) {
            align_size = hor_stride * MPP_ALIGN(ver_stride, 16) * 3 / 2;
        } else if (format >= MPP_FMT_RGB565 && format < MPP_FMT_BGR888) {
            align_size = hor_stride * MPP_ALIGN(ver_stride, 16) * 3;
        } else if (format >= MPP_FMT_RGB101010 && format < MPP_FMT_RGB_BUTT) {
            align_size = hor_stride * MPP_ALIGN(ver_stride, 16) * 4;
        } else {
            mpp_err_f("unsupport input format:%d\n", format);
            ret = MPP_NOK;
            goto FUNC_RET;
        }
        if (align_size > 0) {
            MppBuffer buffer = NULL;
            ret = mpp_buffer_get(memGroup, &buffer, align_size);
            if (ret) {
                mpp_err_f("allocate input picture buffer failed\n");
                goto FUNC_RET;
            }
            copy_align_raw_buffer_to_dest((RK_U8 *)mpp_buffer_get_ptr(buffer),
                                          aEncInStrm->buf, width, height, format);
            mpp_frame_set_buffer(frame, buffer);
            mpp_buffer_put(buffer);
            buffer = NULL;
        }
    }

PUT_FRAME:

    vpu_api_dbg_input("w %d h %d input fd %d size %d pts %lld, flag %d \n",
                      width, height, fd, size, aEncInStrm->timeUs, aEncInStrm->nFlags);
    if (mlvec) {
        MppMeta meta = mpp_frame_get_meta(frame);

        vpu_api_mlvec_set_dy_cfg(mlvec, &mlvec_dy_cfg, meta);
    }

    ret = mpi->encode_put_frame(mpp_ctx, frame);
    if (ret)
        mpp_err_f("encode_put_frame ret %d\n", ret);
    else
        aEncInStrm->size = 0;

FUNC_RET:
    if (frame)
        mpp_frame_deinit(&frame);

    vpu_api_dbg_func("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::encoder_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut)
{
    RK_S32 ret = 0;
    MppPacket packet = NULL;
    vpu_api_dbg_func("enter\n");

    ret = mpi->encode_get_packet(mpp_ctx, &packet);
    if (ret) {
        mpp_err_f("encode_get_packet failed ret %d\n", ret);
        goto FUNC_RET;
    }
    if (packet) {
        RK_U8 *src = (RK_U8 *)mpp_packet_get_data(packet);
        RK_U32 eos = mpp_packet_get_eos(packet);
        RK_S64 pts = mpp_packet_get_pts(packet);
        size_t length = mpp_packet_get_length(packet);
        MppMeta meta = mpp_packet_get_meta(packet);
        RK_S32 is_intra = 0;

        RK_U32 offset = 0;
        if (ctx->videoCoding == OMX_RK_VIDEO_CodingAVC) {
            offset = 4;
            length = (length > offset) ? (length - offset) : 0;
        }
        aEncOut->data = NULL;
        if (length > 0) {
            aEncOut->data = mpp_calloc(RK_U8, MPP_ALIGN(length + 16, SZ_4K));
            if (aEncOut->data)
                memcpy(aEncOut->data, src + offset, length);
        }

        mpp_meta_get_s32(meta, KEY_OUTPUT_INTRA, &is_intra);

        aEncOut->size = (RK_S32)length;
        aEncOut->timeUs = pts;
        aEncOut->keyFrame = is_intra;

        vpu_api_dbg_output("get packet %p size %d pts %lld keyframe %d eos %d\n",
                           packet, length, pts, aEncOut->keyFrame, eos);

        mEosSet = eos;
        mpp_packet_deinit(&packet);
    } else {
        aEncOut->size = 0;
        vpu_api_dbg_output("get NULL packet, eos %d\n", mEosSet);
        if (mEosSet)
            ret = -1;
    }

FUNC_RET:
    vpu_api_dbg_func("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::perform(PerformCmd cmd, RK_S32 *data)
{
    vpu_api_dbg_func("enter\n");
    switch (cmd) {
    case INPUT_FORMAT_MAP : {
        EncInputPictureType vpu_frame_fmt = *(EncInputPictureType *)data;
        MppFrameFormat mpp_frame_fmt = vpu_pic_type_remap_to_mpp(vpu_frame_fmt);
        *(MppFrameFormat *)data = mpp_frame_fmt;
    } break;
    default:
        mpp_err("cmd can not match with any option!");
        break;
    }
    vpu_api_dbg_func("leave\n");
    return 0;
}

RK_S32 VpuApiLegacy::control(VpuCodecContext *ctx, VPU_API_CMD cmd, void *param)
{
    vpu_api_dbg_func("enter cmd 0x%x param %p\n", cmd, param);

    if (mpi == NULL && !init_ok) {
        return 0;
    }

    MpiCmd mpicmd = MPI_CMD_BUTT;
    switch (cmd) {
    case VPU_API_ENABLE_DEINTERLACE : {
        mpicmd = MPP_DEC_SET_ENABLE_DEINTERLACE;
    } break;
    case VPU_API_ENC_SETCFG : {
        MppCodingType coding = (MppCodingType)ctx->videoCoding;

        memcpy(&enc_param, param, sizeof(enc_param));
        return vpu_api_set_enc_cfg(mpp_ctx, mpi, enc_cfg, coding, format, &enc_param);
    } break;
    case VPU_API_ENC_GETCFG : {
        memcpy(param, &enc_param, sizeof(enc_param));
        return 0;
    } break;
    case VPU_API_ENC_SETFORMAT : {
        EncInputPictureType type = *((EncInputPictureType *)param);
        format = vpu_pic_type_remap_to_mpp(type);
        return 0;
    } break;
    case VPU_API_ENC_SETIDRFRAME : {
        mpicmd = MPP_ENC_SET_IDR_FRAME;
    } break;
    case VPU_API_SET_VPUMEM_CONTEXT: {
        mpicmd = MPP_DEC_SET_EXT_BUF_GROUP;
    } break;
    case VPU_API_USE_PRESENT_TIME_ORDER: {
        mpicmd = MPP_DEC_SET_PRESENT_TIME_ORDER;
    } break;
    case VPU_API_SET_INFO_CHANGE: {
        mpicmd = MPP_DEC_SET_INFO_CHANGE_READY;
    } break;
    case VPU_API_USE_FAST_MODE: {
        mpicmd = MPP_DEC_SET_PARSER_FAST_MODE;
    } break;
    case VPU_API_DEC_GET_STREAM_COUNT: {
        mpicmd = MPP_DEC_GET_STREAM_COUNT;
    } break;
    case VPU_API_GET_VPUMEM_USED_COUNT: {
        mpicmd = MPP_DEC_GET_VPUMEM_USED_COUNT;
    } break;
    case VPU_API_SET_OUTPUT_BLOCK: {
        mpicmd = MPP_SET_OUTPUT_TIMEOUT;
        if (param) {
            RK_S32 timeout = *((RK_S32*)param);

            if (timeout) {
                if (timeout < 0)
                    mpp_log("set output mode to block\n");
                else
                    mpp_log("set output timeout %d ms\n", timeout);
            } else {
                mpp_log("set output mode to non-block\n");
            }
        }
    } break;
    case VPU_API_GET_EOS_STATUS: {
        *((RK_S32 *)param) = mEosSet;
        mpicmd = MPI_CMD_BUTT;
    } break;
    case VPU_API_GET_FRAME_INFO: {
        *((VPU_GENERIC *)param) = vpug;
        mpicmd = MPI_CMD_BUTT;
    } break;
    case VPU_API_SET_OUTPUT_MODE: {
        mpicmd = MPP_DEC_SET_OUTPUT_FORMAT;
    } break;
    case VPU_API_SET_IMMEDIATE_OUT: {
        mpicmd = MPP_DEC_SET_IMMEDIATE_OUT;
    } break;
    case VPU_API_ENC_SET_VEPU22_CFG: {
        mpicmd = MPP_ENC_SET_CODEC_CFG;
    } break;
    case VPU_API_ENC_GET_VEPU22_CFG: {
        mpicmd = MPP_ENC_GET_CODEC_CFG;
    } break;
    case VPU_API_ENC_SET_VEPU22_CTU_QP: {
        mpicmd = MPP_ENC_SET_CTU_QP;
    } break;
    case VPU_API_ENC_SET_VEPU22_ROI: {
        mpicmd = MPP_ENC_SET_ROI_CFG;
    } break;
    case VPU_API_ENC_SET_MAX_TID: {
        RK_S32 max_tid = *(RK_S32 *)param;

        vpu_api_dbg_ctrl("VPU_API_ENC_SET_MAX_TID %d\n", max_tid);
        mlvec_dy_cfg.max_tid = max_tid;
        mlvec_dy_cfg.updated |= VPU_API_ENC_MAX_TID_UPDATED;

        if (mlvec)
            vpu_api_mlvec_set_dy_max_tid(mlvec, max_tid);

        return 0;
    } break;
    case VPU_API_ENC_SET_MARK_LTR: {
        RK_S32 mark_ltr = *(RK_S32 *)param;

        vpu_api_dbg_ctrl("VPU_API_ENC_SET_MARK_LTR %d\n", mark_ltr);

        mlvec_dy_cfg.mark_ltr = mark_ltr;
        if (mark_ltr >= 0)
            mlvec_dy_cfg.updated |= VPU_API_ENC_MARK_LTR_UPDATED;

        return 0;
    } break;
    case VPU_API_ENC_SET_USE_LTR: {
        RK_S32 use_ltr = *(RK_S32 *)param;

        vpu_api_dbg_ctrl("VPU_API_ENC_SET_USE_LTR %d\n", use_ltr);

        mlvec_dy_cfg.use_ltr = use_ltr;
        mlvec_dy_cfg.updated |= VPU_API_ENC_USE_LTR_UPDATED;

        return 0;
    } break;
    case VPU_API_ENC_SET_FRAME_QP: {
        RK_S32 frame_qp = *(RK_S32 *)param;

        vpu_api_dbg_ctrl("VPU_API_ENC_SET_FRAME_QP %d\n", frame_qp);

        mlvec_dy_cfg.frame_qp = frame_qp;
        mlvec_dy_cfg.updated |= VPU_API_ENC_FRAME_QP_UPDATED;

        return 0;
    } break;
    case VPU_API_ENC_SET_BASE_LAYER_PID: {
        RK_S32 base_layer_pid = *(RK_S32 *)param;

        vpu_api_dbg_ctrl("VPU_API_ENC_SET_BASE_LAYER_PID %d\n", base_layer_pid);

        mlvec_dy_cfg.base_layer_pid = base_layer_pid;
        mlvec_dy_cfg.updated |= VPU_API_ENC_BASE_PID_UPDATED;

        return 0;
    } break;
    case VPU_API_GET_EXTRA_INFO: {
        EncoderOut_t *out = (EncoderOut_t *)param;

        vpu_api_dbg_ctrl("VPU_API_GET_EXTRA_INFO\n");

        if (NULL == enc_hdr_pkt) {
            if (NULL == enc_hdr_buf) {
                enc_hdr_buf_size = SZ_1K;
                enc_hdr_buf = mpp_calloc_size(RK_U8, enc_hdr_buf_size);
            }

            if (enc_hdr_buf)
                mpp_packet_init(&enc_hdr_pkt, enc_hdr_buf, enc_hdr_buf_size);
        }

        mpp_assert(enc_hdr_pkt);
        if (enc_hdr_pkt) {
            mpi->control(mpp_ctx, MPP_ENC_GET_HDR_SYNC, enc_hdr_pkt);

            RK_S32 length = mpp_packet_get_length(enc_hdr_pkt);
            void *src = mpp_packet_get_data(enc_hdr_pkt);

            memcpy(out->data, src, length);
            out->size = length;
        }
        return 0;
    } break;
    case VPU_API_SET_PARSER_SPLIT_MODE: {
        mpicmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
    } break;
    default: {
    } break;
    }

    RK_S32 ret = -1;
    if (mpicmd < MPI_CMD_BUTT)
        ret = mpi->control(mpp_ctx, mpicmd, (MppParam)param);

    vpu_api_dbg_func("leave\n");
    return ret;
}

