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

VpuApi::VpuApi()
{
    mpp_log_f("in\n");
    mpp_ctx = NULL;
    mpi = NULL;
#ifdef DUMP_YUV
    fp = fopen("data/hevcdump.yuv", "wb");
#endif
    frame_count  = 0;
    mpp_log_f("ok\n");

}

VpuApi::~VpuApi()
{
    mpp_log_f("in\n");
    mpp_deinit(mpp_ctx);
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

    ret = mpp_init(&mpp_ctx, &mpi, type, (MppCodingType)ctx->videoCoding);

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
    if (mpi && mpi->flush) {
        mpi->flush(mpp_ctx);
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
		vframe->ErrorInfo = mpp_frame_get_errinfo(mframe);
		//mpp_err("vframe->ErrorInfo = %08x \n", vframe->ErrorInfo);
        pts = mpp_frame_get_pts(mframe);
        aDecOut->timeUs = pts;
        //mpp_err("get one frame timeUs %lld, errinfo=%08x",aDecOut->timeUs, vframe->ErrorInfo);
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
            aDecOut->nFlags = VPU_API_EOS_STREAM_REACHED;
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

RK_S32 VpuApi::control(VpuCodecContext *ctx, VPU_API_CMD cmd, void *param)
{
    mpp_log_f("in\n");
    MpiCmd mpicmd;
    if (mpi == NULL) {
        return 0;
    }
    (void)ctx;
    switch (cmd) {
    case VPU_API_SET_VPUMEM_CONTEXT: {
        mpicmd = MPP_DEC_SET_EXT_BUF_GROUP;
        break;
    }
    case VPU_API_SET_DEFAULT_WIDTH_HEIGH: {
        mpicmd = MPP_CODEC_SET_FRAME_INFO;
        break;
    }
    case VPU_API_SET_INFO_CHANGE: {
        mpicmd = MPP_CODEC_SET_INFO_CHANGE_READY;
        break;
    }
    default: {
        break;
    }
    }
    return mpi->control(mpp_ctx, (MpiCmd)mpicmd, (MppParam)param);
    mpp_log_f("ok\n");
    return 0;
}

