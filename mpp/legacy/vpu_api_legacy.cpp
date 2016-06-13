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

#include "mpp_log.h"
#include "mpp_frame.h"
#include "vpu_api_legacy.h"
#include "mpp_mem.h"
#include "string.h"
#include "mpp_common.h"
#include "mpp_env.h"

#define VPU_API_DBG_OUTPUT    (0x00000001)
#define VPU_API_DBG_DUMP_YUV  (0x00000002)
#define VPU_API_DBG_DUMP_LOG  (0x00000004)
#define MAX_WRITE_HEIGHT      (480)
#define MAX_WRITE_WIDTH       (960)

VpuApi::VpuApi()
{
    mpp_log_f("in\n");
    mpp_ctx = NULL;
    mpi = NULL;

    mpp_create(&mpp_ctx, &mpi);
    frame_count  = 0;
    set_eos = 0;
    vpu_api_debug = 0;
    fp = NULL;
	fp_buf = NULL;
    mpp_env_get_u32("vpu_api_debug", &vpu_api_debug, 0);
    if (vpu_api_debug & VPU_API_DBG_DUMP_YUV) {
        fp = fopen("/sdcard/rk_mpp_dump.yuv", "wb");
		fp_buf = mpp_malloc(RK_U8, (MAX_WRITE_HEIGHT * MAX_WRITE_WIDTH * 2));
    }
    mpp_log_f("ok\n");
}

VpuApi::~VpuApi()
{
    mpp_log_f("in\n");
    if (fp) {
        fclose(fp);
		fp = NULL;
    }
	if (fp_buf) {
		mpp_free(fp_buf);
		fp_buf = NULL;
    }
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
	if (ret) {
		mpp_err_f(" init error. \n");
		return ret;
	}
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
        MppBuffer buf = NULL;
        RK_U64 pts = 0;
        RK_U32 fd = 0;
        void* ptr = NULL;
        aDecOut->size = sizeof(VPU_FRAME);
        vframe->DisplayWidth = mpp_frame_get_width(mframe);
        vframe->DisplayHeight = mpp_frame_get_height(mframe);
        vframe->FrameWidth = mpp_frame_get_hor_stride(mframe);
        vframe->FrameHeight = mpp_frame_get_ver_stride(mframe);
        vframe->ErrorInfo = mpp_frame_get_errinfo(mframe) | mpp_frame_get_discard(mframe);
        pts = mpp_frame_get_pts(mframe);
        aDecOut->timeUs = pts;
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
            //!< Dump yuv
			if (fp && !vframe->ErrorInfo) {
				if ((vframe->FrameWidth >= 1920) || (vframe->FrameHeight >= 1080)) {
					RK_U32 i = 0, j = 0, step = 0;
					RK_U32 img_w = 0, img_h = 0;
					RK_U8 *pdes = NULL, *psrc = NULL;
					step = MPP_MAX(vframe->FrameWidth / MAX_WRITE_WIDTH, vframe->FrameHeight / MAX_WRITE_HEIGHT);
					img_w = vframe->FrameWidth / step;
					img_h = vframe->FrameHeight / step;
					pdes = fp_buf;
					psrc = (RK_U8 *)ptr;
					for (i = 0; i < img_h; i++) {
						for (j = 0; j < img_w; j++) {
							pdes[j] = psrc[j * step];
						}
						pdes += img_w;
						psrc += step * vframe->FrameWidth;
					}
					pdes = fp_buf + img_w * img_h;
					psrc = (RK_U8 *)ptr + vframe->FrameWidth * vframe->FrameHeight;
					for (i = 0; i < (img_h / 2); i++) {
						for (j = 0; j < (img_w / 2); j++) {
							pdes[2 * j + 0] = psrc[2 * j * step + 0];
							pdes[2 * j + 1] = psrc[2 * j * step + 1];
						}
						pdes += img_w;
						psrc += step * vframe->FrameWidth;
					}
					fwrite(fp_buf, 1, img_w * img_h * 3 / 2, fp);
					if (vpu_api_debug & VPU_API_DBG_DUMP_LOG) {
						mpp_log("[write_out_yuv] timeUs=%lld, FrameWidth=%d, FrameHeight=%d", aDecOut->timeUs, img_w, img_h);
					}
				} else {
                	fwrite(ptr, 1, vframe->FrameWidth * vframe->FrameHeight * 3 / 2, fp);
					if (vpu_api_debug & VPU_API_DBG_DUMP_LOG) {
						mpp_log("[write_out_yuv] timeUs=%lld, FrameWidth=%d, FrameHeight=%d", aDecOut->timeUs, vframe->FrameWidth, vframe->FrameHeight);
					}
				}
                fflush(fp);
            }
            vframe->vpumem.phy_addr = fd;
            vframe->vpumem.size = vframe->FrameWidth * vframe->FrameHeight * 3 / 2;
            vframe->vpumem.offset = (RK_U32*)buf;
        }
		if (vpu_api_debug & VPU_API_DBG_OUTPUT) {
			mpp_log("get one frame timeUs %lld, fd=0x%x, poc=%d, errinfo=%d, discard=%d, eos=%d, verr=%d", aDecOut->timeUs, fd,
				mpp_frame_get_poc(mframe), mpp_frame_get_errinfo(mframe), mpp_frame_get_discard(mframe), mpp_frame_get_eos(mframe), vframe->ErrorInfo);
		}
        if (mpp_frame_get_eos(mframe)) {
            set_eos = 1;
            if (buf == NULL) {
                aDecOut->size = 0;
            }
        }

        /*
         * IMPORTANT: mframe is malloced frome mpi->decode_get_frame
         * So we need to deinit mframe here. But the buffer in the frame should not be free with mframe.
         * Because buffer need to be set to vframe->vpumem.offset and send to display.
         * The we have to clear the buffer pointer in mframe then release mframe.
         */
        mpp_frame_set_buffer(mframe, NULL);
		mpp_frame_deinit(&mframe);
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
		RK_U32 ImgWidth =  p->ImgWidth;
        mpicmd = MPP_CODEC_SET_FRAME_INFO;
		/**hightest of p->ImgWidth bit show current dec bitdepth
		  * 0 - 8bit
		  * 1 - 10bit
		  **/
        if(((p->ImgWidth&0x80000000)>>31)){
            p->ImgWidth =  (p->ImgWidth&0x7FFFFFFF);
            ImgWidth = (p->ImgWidth *10)>>3;
        }
        if (ctx->videoCoding == OMX_RK_VIDEO_CodingHEVC) {
            p->ImgHorStride = hevc_ver_align_256_odd(ImgWidth);
            p->ImgVerStride = hevc_ver_align_8(p->ImgHeight);
        } else {
            p->ImgHorStride = default_align_16(ImgWidth);
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
	case VPU_API_GET_VPUMEM_USED_COUNT:{
		mpicmd = MPP_CODEC_GET_VPUMEM_USED_COUNT;
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

