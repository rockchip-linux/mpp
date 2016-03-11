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

#define MODULE_TAG "vpu_api_legacy"

#include "mpp_log.h"
#include "mpp_frame.h"
#include "vpu_api_legacy.h"
#include "mpp_mem.h"
#include "string.h"
#include "mpp_common.h"

VpuApi::VpuApi()
{
    mpp_log_f("in\n");
    mpp_ctx = NULL;
    mpi = NULL;
#ifdef DUMP_YUV
    fp = fopen("data/hevcdump.yuv", "wb");
#endif
    mpp_create(&mpp_ctx, &mpi);
    frame_count  = 0;
    set_eos = 0;
    mpp_log_f("ok\n");

}

VpuApi::~VpuApi()
{
    mpp_log_f("in\n");
    mpp_destroy(mpp_ctx);
    mpp_log_f("ok\n");
}

RK_S32 VpuApi::init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size)
{
    mpp_log_f("in\n");
    MPP_RET ret = MPP_OK;
    MppCtxType type;
    MppPacket pkt = NULL;

    if (CODEC_DECODER == ctx->codecType) {
        type = MPP_CTX_DEC;
    } else if (CODEC_ENCODER == ctx->codecType) {
        type = MPP_CTX_ENC;
    } else {
        return MPP_ERR_VPU_CODEC_INIT;
    }

    if (mpp_ctx == NULL || mpi == NULL) {
        mpp_err("found invalid context input");
        return MPP_ERR_NULL_PTR;
    }
    ret = mpp_init(mpp_ctx, type, (MppCodingType)ctx->videoCoding);

    VPU_GENERIC vpug;
    vpug.CodecType  = ctx->codecType;
    vpug.ImgWidth   = ctx->width;
    vpug.ImgHeight  = ctx->height;
    control(ctx, VPU_API_SET_DEFAULT_WIDTH_HEIGH, &vpug);
    if (extraData != NULL) {
        mpp_packet_init(&pkt, extraData, extra_size);
        mpp_packet_set_extra_data(pkt);
        mpi->decode_put_packet(mpp_ctx, pkt);
        mpp_packet_deinit(&pkt);
    }

    mpp_log_f("ok\n");
    return ret;
}

RK_S32 VpuApi::flush(VpuCodecContext *ctx)
{
    (void)ctx;
    mpp_log_f("in\n");
    if (mpi && mpi->reset) {
        mpi->reset(mpp_ctx);
    }
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)pkt;
    (void)aDecOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::decode_sendstream(VideoPacket_t *pkt)
{
    //mpp_log_f("in\n");
    MppPacket mpkt = NULL;
    mpp_packet_init(&mpkt, pkt->data, pkt->size);
    mpp_packet_set_pts(mpkt, pkt->pts);
    if (pkt->nFlags & OMX_BUFFERFLAG_EOS) {
        mpp_err("decode_sendstream set eos");
        mpp_packet_set_eos(mpkt);
    }
    if (mpi->decode_put_packet(mpp_ctx, mpkt) == MPP_OK) {
        pkt->size = 0;
    }
    mpp_packet_deinit(&mpkt);
    // mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi:: decode_getoutframe(DecoderOut_t *aDecOut)
{
    // mpp_log_f("in\n");
    VPU_FRAME *vframe = (VPU_FRAME *)aDecOut->data;
    memset(vframe, 0, sizeof(VPU_FRAME));
    MppFrame  mframe = NULL;
    if (NULL == mpi) {
        aDecOut->size = 0;
        return 0;
    }
    if (set_eos) {
        aDecOut->size = 0;
        return VPU_API_EOS_STREAM_REACHED;
    }
    if (MPP_OK == mpi->decode_get_frame(mpp_ctx, &mframe)) {
        MppBuffer buf;
        RK_U64 pts;
        RK_U32 fd;
        void* ptr;
        aDecOut->size = sizeof(VPU_FRAME);
        vframe->DisplayWidth = mpp_frame_get_width(mframe);
        vframe->DisplayHeight = mpp_frame_get_height(mframe);
        vframe->FrameWidth = mpp_frame_get_hor_stride(mframe);
        vframe->FrameHeight = mpp_frame_get_ver_stride(mframe);
        vframe->ErrorInfo = mpp_frame_get_errinfo(mframe) | mpp_frame_get_discard(mframe);
        //mpp_err("vframe->ErrorInfo = %08x \n", vframe->ErrorInfo);
        pts = mpp_frame_get_pts(mframe);
        aDecOut->timeUs = pts;
        //mpp_err("get one frame timeUs %lld, poc=%d, errinfo=%d, discard=%d",aDecOut->timeUs,
		//	mpp_frame_get_poc(mframe), mpp_frame_get_errinfo(mframe), mpp_frame_get_discard(mframe));
        vframe->ShowTime.TimeHigh = (RK_U32)(pts >> 32);
        vframe->ShowTime.TimeLow = (RK_U32)pts;
        buf = mpp_frame_get_buffer(mframe);
        switch (mpp_frame_get_fmt(mframe)) {
        case MPP_FMT_YUV420SP: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV420_SEMIPLANAR;
            vframe->OutputWidth = 0x20;
            break;
        }
        case MPP_FMT_YUV420SP_10BIT: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV420_SEMIPLANAR;
            vframe->ColorType |= VPU_OUTPUT_FORMAT_BIT_10;
            vframe->OutputWidth = 0x22;
            break;
        }
        case MPP_FMT_YUV422SP: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV422;
            break;
        }
        case MPP_FMT_YUV422SP_10BIT: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV422;
            vframe->ColorType |= VPU_OUTPUT_FORMAT_BIT_10;
            break;
        }
        default:
            break;
        }
        if (buf) {
            ptr = mpp_buffer_get_ptr(buf);
            fd = mpp_buffer_get_fd(buf);
            vframe->FrameBusAddr[0] = fd;
            vframe->FrameBusAddr[1] = fd;
            vframe->vpumem.vir_addr = (RK_U32*)ptr;
            frame_count++;
#ifdef DUMP_YUV
            if (frame_count > 350) {
                fwrite(ptr, 1, vframe->FrameWidth * vframe->FrameHeight * 3 / 2, fp);
                fflush(fp);
            }
#endif
            vframe->vpumem.phy_addr = fd;
            vframe->vpumem.size = vframe->FrameWidth * vframe->FrameHeight * 3 / 2;
            vframe->vpumem.offset = (RK_U32*)buf;
        }
        if (mpp_frame_get_eos(mframe)) {
            set_eos = 1;
            if (buf == NULL) {
                aDecOut->size = 0;
            }
        }
        mpp_free(mframe);
    } else {
        aDecOut->size = 0;
    }

    // mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)aEncInStrm;
    (void)aEncOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::encoder_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)aEncInStrm;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::encoder_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut)
{
    mpp_log_f("in\n");
    (void)ctx;
    (void)aEncOut;
    mpp_log_f("ok\n");
    return 0;
}

RK_S32 VpuApi::perform(RK_U32 cmd, RK_U32 *data)
{
    mpp_log_f("in\n");
    (void)cmd;
    (void)data;
    mpp_log_f("ok\n");
    return 0;
}

static RK_U32 hevc_ver_align_8(RK_U32 val)
{
    return MPP_ALIGN(val, 8);
}

static RK_U32 hevc_ver_align_256_odd(RK_U32 val)
{
    return MPP_ALIGN(val, 256) | 256;
}

static RK_U32 default_align_16(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}
RK_S32 VpuApi::control(VpuCodecContext *ctx, VPU_API_CMD cmd, void *param)
{
    mpp_log_f("in\n");

    (void)ctx;
    if (mpi == NULL) {
        return 0;
    }

    MpiCmd mpicmd = MPI_CMD_BUTT;
    switch (cmd) {
    case VPU_API_SET_VPUMEM_CONTEXT: {
        mpicmd = MPP_DEC_SET_EXT_BUF_GROUP;
        break;
    }
    case VPU_API_SET_DEFAULT_WIDTH_HEIGH: {
        VPU_GENERIC *p = (VPU_GENERIC *)param;
        mpicmd = MPP_CODEC_SET_FRAME_INFO;
        if (ctx->videoCoding == OMX_RK_VIDEO_CodingHEVC) {
            p->ImgHorStride = hevc_ver_align_256_odd(p->ImgWidth);
            p->ImgVerStride = hevc_ver_align_8(p->ImgHeight);
        } else {
            p->ImgHorStride = default_align_16(p->ImgWidth);
            p->ImgVerStride = default_align_16(p->ImgHeight);
        }
        break;
    }
    case VPU_API_SET_INFO_CHANGE: {
        mpicmd = MPP_CODEC_SET_INFO_CHANGE_READY;
        break;
    }
    case VPU_API_USE_FAST_MODE: {
        mpicmd = MPP_DEC_USE_FAST_MODE;
        break;
    }
	case VPU_API_DEC_GET_STREAM_COUNT: {
		mpicmd = MPP_DEC_GET_STREAM_COUNT;
		break;
	}
    default: {
        break;
    }
    }

    RK_S32 ret = -1;
    if (mpicmd < MPI_CMD_BUTT)
        ret = mpi->control(mpp_ctx, (MpiCmd)mpicmd, (MppParam)param);

    mpp_log_f("ok\n");
    return ret;
}

