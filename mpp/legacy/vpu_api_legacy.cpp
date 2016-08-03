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

#include "string.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"

#include "vpu_api_legacy.h"
#include "mpp_packet_impl.h"

#define MAX_WRITE_HEIGHT        (480)
#define MAX_WRITE_WIDTH         (960)

RK_U32 vpu_api_debug = 0;

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

static RK_U32 hevc_ver_align_64(RK_U32 val)
{
    return MPP_ALIGN(val, 64);
}

VpuApiLegacy::VpuApiLegacy() :
    mpp_ctx(NULL),
    mpi(NULL),
    init_ok(0),
    frame_count(0),
    set_eos(0),
    fp(NULL),
    fp_buf(NULL),
    memGroup(NULL),
    pictureMem(NULL),
    outbufMem(NULL),
    outData(NULL),
    enc_in_fmt(ENC_INPUT_YUV420_PLANAR),
    mEosSet(0)
{
    mpp_env_get_u32("vpu_api_debug", &vpu_api_debug, 0);

    vpu_api_dbg_func("enter\n");

    mpp_create(&mpp_ctx, &mpi);

    if (vpu_api_debug & VPU_API_DBG_DUMP_YUV) {
        fp = fopen("/sdcard/rk_mpp_dump.yuv", "wb");
        fp_buf = mpp_malloc(RK_U8, (MAX_WRITE_HEIGHT * MAX_WRITE_WIDTH * 2));
    }

    vpu_api_dbg_func("leave\n");
}

VpuApiLegacy::~VpuApiLegacy()
{
    vpu_api_dbg_func("enter\n");
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
    if (fp_buf) {
        mpp_free(fp_buf);
        fp_buf = NULL;
    }
    if (outData) {
        mpp_free(outData);
        outData = NULL;
    }
    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    mpp_destroy(mpp_ctx);

    vpu_api_dbg_func("leave\n");
}

RK_S32 VpuApiLegacy::init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size)
{
    vpu_api_dbg_func("enter\n");

    MPP_RET ret = MPP_OK;
    MppCtxType type;
    MppPacket pkt = NULL;

    if (CODEC_DECODER == ctx->codecType) {
        type = MPP_CTX_DEC;
    } else if (CODEC_ENCODER == ctx->codecType) {
        if (memGroup == NULL) {
            ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_ION);
            if (MPP_OK != ret) {
                mpp_err("memGroup mpp_buffer_group_get failed\n");
                return ret;
            }
        }
        // TODO set control cmd
        MppParam param = NULL;
        RK_U32 block = 1;
        param = &block;
        ret = mpi->control(mpp_ctx, MPP_SET_INPUT_BLOCK, param);
        if (MPP_OK != ret) {
            mpp_err("mpi->control MPP_SET_INPUT_BLOCK failed\n");
        }
        block = 0;
        ret = mpi->control(mpp_ctx, MPP_SET_OUTPUT_BLOCK, param);
        if (MPP_OK != ret) {
            mpp_err("mpi->control MPP_SET_OUTPUT_BLOCK failed\n");
        }

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

    if (MPP_CTX_ENC == type) {
        EncParameter_t *param = (EncParameter_t*)ctx->private_data;
        MppEncConfig mpp_cfg;

        memset(&mpp_cfg, 0, sizeof(mpp_cfg));

        mpp_log("setup encoder rate control config:\n");
        mpp_log("width %4d height %4d format %d\n", param->width, param->height, param->format);
        mpp_log("rc_mode %s qp %d bps %d\n", (param->rc_mode) ? ("VBR") : ("CQP"), param->qp, param->bitRate);
        mpp_log("fps in %d fps out %d gop %d\n", param->framerate, param->framerateout, param->intraPicRate);
        mpp_log("setup encoder stream feature config:\n");
        mpp_log("profile %d level %d cabac %d\n", param->profileIdc, param->levelIdc, param->enableCabac);

        mpp_assert(param->width);
        mpp_assert(param->height);

        outData = mpp_malloc(RK_U8, (param->width * param->height));

        mpp_cfg.width       = param->width;
        mpp_cfg.height      = param->height;
        mpp_cfg.format      = param->format;
        mpp_cfg.rc_mode     = param->rc_mode;
        mpp_cfg.skip_cnt    = 0;
        mpp_cfg.bps         = param->bitRate;
        mpp_cfg.fps_in      = param->framerate;
        if (param->framerateout)
            mpp_cfg.fps_out = param->framerateout;
        else
            mpp_cfg.fps_out = param->framerate;
        mpp_cfg.qp          = (param->qp) ? (param->qp) : (26);
        mpp_cfg.gop         = param->intraPicRate;

        mpp_cfg.profile     = param->profileIdc;
        mpp_cfg.level       = param->levelIdc;
        mpp_cfg.cabac_en    = param->enableCabac;

        mpi->control(mpp_ctx, MPP_ENC_SET_CFG, &mpp_cfg);

        mpi->control(mpp_ctx, MPP_ENC_GET_EXTRA_INFO, &pkt);

        if (pkt) {
            ctx->extradata_size = mpp_packet_get_length(pkt);
            ctx->extradata      = mpp_packet_get_data(pkt);
        }
        pkt = NULL;
    }

    VPU_GENERIC vpug;
    vpug.CodecType  = ctx->codecType;
    vpug.ImgWidth   = ctx->width;
    vpug.ImgHeight  = ctx->height;
    if (MPP_CTX_ENC != type)
        control(ctx, VPU_API_SET_DEFAULT_WIDTH_HEIGH, &vpug);
    if (extraData != NULL) {
        mpp_packet_init(&pkt, extraData, extra_size);
        mpp_packet_set_extra_data(pkt);
        mpi->decode_put_packet(mpp_ctx, pkt);
        mpp_packet_deinit(&pkt);
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
    }
    vpu_api_dbg_func("leave\n");
    return 0;
}

RK_S32 VpuApiLegacy::decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut)
{
    RK_S32 ret = 0;
    vpu_api_dbg_func("enter\n");

    (void)ctx;
    (void)pkt;
    (void)aDecOut;

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

    do {
        if (mpi->decode_put_packet(mpp_ctx, mpkt) == MPP_OK) {
            pkt->size = 0;
            break;
        }
    } while (pkt->nFlags & OMX_BUFFERFLAG_SYNC);

    mpp_packet_deinit(&mpkt);

    vpu_api_dbg_func("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy:: decode_getoutframe(DecoderOut_t *aDecOut)
{
    RK_S32 ret = 0;
    VPU_FRAME *vframe = (VPU_FRAME *)aDecOut->data;
    MppFrame  mframe = NULL;
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
        MppBufferInfo info;
        RK_U64 pts  = mpp_frame_get_pts(mframe);
        RK_U32 mode = mpp_frame_get_mode(mframe);
        RK_S32 fd   = -1;

        if (buf)
            mpp_buffer_inc_ref(buf);

        aDecOut->size = sizeof(VPU_FRAME);
        vframe->DisplayWidth = mpp_frame_get_width(mframe);
        vframe->DisplayHeight = mpp_frame_get_height(mframe);
        vframe->FrameWidth = mpp_frame_get_hor_stride(mframe);
        vframe->FrameHeight = mpp_frame_get_ver_stride(mframe);
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
        aDecOut->timeUs = pts;
        vframe->ShowTime.TimeHigh = (RK_U32)(pts >> 32);
        vframe->ShowTime.TimeLow = (RK_U32)pts;
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
            vframe->OutputWidth = 0x10;
            break;
        }
        case MPP_FMT_YUV422SP_10BIT: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV422;
            vframe->ColorType |= VPU_OUTPUT_FORMAT_BIT_10;
            vframe->OutputWidth = 0x23;
            break;
        }
        default:
            break;
        }
        if (buf) {
            void* ptr = mpp_buffer_get_ptr(buf);

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
                        psrc += step * vframe->FrameWidth * ((mpp_frame_get_fmt(mframe) > MPP_FMT_YUV420SP_10BIT) ? 2 : 1);
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
            memset(&info, 0, sizeof(MppBufferInfo));
            mpp_buffer_info_get(buf, &info);
            vframe->vpumem.size = vframe->FrameWidth * vframe->FrameHeight * 3 / 2;
            vframe->vpumem.size |= ((info.index << 27) & 0xf8000000);
            vframe->vpumem.offset = (RK_U32*)buf;
        }
        if (vpu_api_debug & VPU_API_DBG_OUTPUT) {
            mpp_log("get one frame pts %lld, fd 0x%x, poc %d, errinfo %x, discard %d, eos %d, verr %d",
                    aDecOut->timeUs, fd, mpp_frame_get_poc(mframe),
                    mpp_frame_get_errinfo(mframe),
                    mpp_frame_get_discard(mframe),
                    mpp_frame_get_eos(mframe), vframe->ErrorInfo);
        }
        if (mpp_frame_get_eos(mframe)) {
            set_eos = 1;
            if (buf == NULL) {
                aDecOut->size = 0;
            }
        }

        /*
         * IMPORTANT: mframe is malloced from mpi->decode_get_frame
         * So we need to deinit mframe here. But the buffer in the frame should not be free with mframe.
         * Because buffer need to be set to vframe->vpumem.offset and send to display.
         * The we have to clear the buffer pointer in mframe then release mframe.
         */
        mpp_frame_deinit(&mframe);
    }

    return ret;
}

RK_S32 VpuApiLegacy::encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut)
{
    MPP_RET ret = MPP_OK;
    MppTask task = NULL;
    vpu_api_dbg_func("enter\n");

    if (!init_ok)
        return VPU_API_ERR_VPU_CODEC_INIT;

    /* try import input buffer and output buffer */
    MppBufferInfo   inputCommit;
    MppBufferInfo   outputCommit;
    RK_U32          use_fd_flag = 0;

    memset(&inputCommit, 0, sizeof(inputCommit));
    memset(&outputCommit, 0, sizeof(outputCommit));

    RK_U32 width        = ctx->width;
    RK_U32 height       = ctx->height;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    MppFrame    frame;
    MppPacket   packet;

    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
        mpp_err("mpp_frame_init failed\n");
        goto ENCODE_FAIL;
    }

    mpp_frame_set_width(frame, width);
    mpp_frame_set_height(frame, height);
    mpp_frame_set_hor_stride(frame, hor_stride);
    mpp_frame_set_ver_stride(frame, ver_stride);

    if (aEncOut->timeUs != 0)
        use_fd_flag = 1;

    if (!use_fd_flag) {
        RK_U32 outputBufferSize = hor_stride * ver_stride;
        ret = mpp_buffer_get(memGroup, &pictureMem, aEncInStrm->size);
        if (ret != MPP_OK) {
            mpp_err( "Failed to allocate pictureMem buffer!\n");
            pictureMem = NULL;
            return ret;
        }
        memcpy((RK_U8*) mpp_buffer_get_ptr(pictureMem), aEncInStrm->buf, aEncInStrm->size);
        ret = mpp_buffer_get(memGroup, &outbufMem, outputBufferSize);
        if (ret != MPP_OK) {
            mpp_err( "Failed to allocate output buffer!\n");
            outbufMem = NULL;
            return 1;
        }
    } else {
        inputCommit.type = MPP_BUFFER_TYPE_ION;
        inputCommit.size = aEncInStrm->size;
        inputCommit.fd = aEncInStrm->bufPhyAddr;

        outputCommit.type = MPP_BUFFER_TYPE_ION;
        RK_S32 *tmp = (RK_S32*)(&aEncOut->timeUs);
        memcpy(&outputCommit.fd, tmp, sizeof(RK_S32));
        memcpy(&outputCommit.size, (tmp + 1), sizeof(RK_S32));
        outputCommit.ptr = (void*)aEncOut->data;

        ret = mpp_buffer_import(&pictureMem, &inputCommit);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_commit failed\n");
        }

        ret = mpp_buffer_import(&outbufMem, &outputCommit);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_commit failed\n");
        }
        vpu_api_dbg_func("mpp import input fd %d output fd %d",
                         mpp_buffer_get_fd(pictureMem), mpp_buffer_get_fd(outbufMem));
    }

    mpp_frame_set_buffer(frame, pictureMem);
    mpp_packet_init_with_buffer(&packet, outbufMem);

    do {
        ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
        if (ret) {
            mpp_err("mpp task input dequeue failed\n");
            goto ENCODE_FAIL;
        }
        if (task == NULL) {
            mpp_log("mpi dequeue from MPP_PORT_INPUT fail, task equal with NULL!");
            usleep(3000);
        } else
            break;
    } while (1);

    mpp_task_meta_set_frame (task, MPP_META_KEY_INPUT_FRM,  frame);
    mpp_task_meta_set_packet(task, MPP_META_KEY_OUTPUT_PKT, packet);

    if (mpi != NULL) {
        ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
        if (ret) {
            mpp_err("mpp task input enqueue failed\n");
            goto ENCODE_FAIL;
        }
        task = NULL;

        do {
            ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task);
            if (ret) {
                mpp_err("ret %d mpp task output dequeue failed\n", ret);
                goto ENCODE_FAIL;
            }

            if (task) {
                MppFrame frame_out = NULL;
                MppFrame packet_out = NULL;

                mpp_task_meta_get_packet(task, MPP_META_KEY_OUTPUT_PKT, &packet_out);

                mpp_assert(packet_out == packet);
                vpu_api_dbg_func("encoded frame %d\n", frame_count);
                frame_count++;

                ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
                if (ret) {
                    mpp_err("mpp task output enqueue failed\n");
                    goto ENCODE_FAIL;
                }
                task = NULL;

                // dequeue task from MPP_PORT_INPUT
                ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
                if (ret) {
                    mpp_log_f("failed to dequeue from input port ret %d\n", ret);
                    break;
                }
                mpp_assert(task);
                ret = mpp_task_meta_get_frame(task, MPP_META_KEY_INPUT_FRM, &frame_out);
                mpp_assert(frame_out  == frame);
                ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
                if (ret) {
                    mpp_err("mpp task output enqueue failed\n");
                    goto ENCODE_FAIL;
                }
                task = NULL;

                break;
            }
            usleep(3000);
        } while (1);
    } else {
        mpp_err("mpi pointer is NULL, failed!");
    }

    // copy encoded stream into output buffer, and set outpub stream size
    if (packet != NULL) {
        RK_S64 pts = mpp_packet_get_pts(packet);
        RK_U32 flag = mpp_packet_get_flag(packet);
        size_t length = mpp_packet_get_length(packet);
        aEncOut->size = (RK_S32)length;
        aEncOut->timeUs = pts;
        aEncOut->keyFrame = (flag & MPP_PACKET_FLAG_INTRA) ? (1) : (0);
        if (!use_fd_flag) {
            mpp_assert(aEncOut->data);
            memcpy(aEncOut->data, (RK_U8*) mpp_buffer_get_ptr(outbufMem), aEncOut->size);
        }
        mpp_buffer_put(outbufMem);
        mpp_packet_deinit(&packet);
    } else {
        mpp_log("outputPacket is NULL!");
    }

    if (pictureMem)
        mpp_buffer_put(pictureMem);
    if (frame) {
        mpp_frame_deinit(&frame);
        frame = NULL;
    }

    vpu_api_dbg_func("leave ret %d\n", ret);
    return ret;

ENCODE_FAIL:

    if (frame != NULL)
        mpp_frame_deinit(&frame);

    if (packet != NULL)
        mpp_packet_deinit(&packet);

    return ret;
}

RK_S32 VpuApiLegacy::encoder_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm)
{
    RK_S32 ret = 0;
    vpu_api_dbg_func("enter\n");

    RK_U32 width        = ctx->width;
    RK_U32 height       = ctx->height;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    RK_S32 pts          = aEncInStrm->timeUs;
    RK_S32 import_fd    = -1;
    RK_U32 import_size  = 0;

    /* try import input buffer and output buffer */
    MppFrame frame = NULL;
    MppBuffer buffer = NULL;
    MppBufferInfo info;
    memset(&info, 0, sizeof(info));

    info.type = MPP_BUFFER_TYPE_ION;
    info.size = aEncInStrm->size;
    info.fd   = aEncInStrm->bufPhyAddr;

    vpu_api_dbg_input("input fd %d size %d flag %d pts %lld\n",
                      info.fd, info.size, aEncInStrm->timeUs, aEncInStrm->nFlags);

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

    if (aEncInStrm->size) {
        ret = mpp_buffer_import(&buffer, &info);
        if (MPP_OK != ret) {
            mpp_err_f("mpp_buffer_commit fd %d size %d failed\n",
                      aEncInStrm->bufPhyAddr, aEncInStrm->size);
            goto FUNC_RET;
        }
        import_fd   = mpp_buffer_get_fd(buffer);
        import_size = mpp_buffer_get_size(buffer);

        mpp_frame_set_buffer(frame, buffer);

        mpp_buffer_put(buffer);
        buffer = NULL;
    }

    if (aEncInStrm->nFlags || aEncInStrm->size == 0) {
        mpp_log_f("found eos true");
        mpp_frame_set_eos(frame, 1);
    }

    vpu_api_dbg_input("w %d h %d fd %d size %d\n", width, height, import_fd, import_size);

    ret = mpi->encode_put_frame(mpp_ctx, frame);
    if (ret)
        mpp_err_f("encode_put_frame ret %d\n", ret);

    aEncInStrm->size = 0;
FUNC_RET:

    vpu_api_dbg_func("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::getDecoderFormat(VpuCodecContext *ctx, DecoderFormat_t *decoder_format)
{
    int32_t ret = 0;

    memset(decoder_format, 0, sizeof(DecoderFormat_t));

    decoder_format->width = ctx->width;
    decoder_format->height = ctx->height;
    decoder_format->stride = ctx->width;
    decoder_format->format = VPU_VIDEO_PIXEL_FMT_NV12;
    decoder_format->frame_size = (decoder_format->width * decoder_format->height * 3) >> 1;

    switch (ctx->videoCoding) {
    case OMX_RK_VIDEO_CodingMPEG2:      /**< AKA: H.262 */
    case OMX_RK_VIDEO_CodingMPEG4:      /**< MPEG-4 */
    case OMX_RK_VIDEO_CodingAVC:        /**< H.264/AVC */
    case OMX_RK_VIDEO_CodingVP8:                     /**< VP8 */
    case OMX_RK_VIDEO_CodingH263:
        decoder_format->aligned_width = (decoder_format->width + 15) & (~15);
        //printf("decoder_format->aligned_width %d\n", decoder_format->aligned_width);
        decoder_format->aligned_height = (decoder_format->height + 15) & (~15);
        decoder_format->aligned_stride = decoder_format->aligned_width;
        decoder_format->aligned_frame_size = (decoder_format->aligned_width * decoder_format->aligned_height * 3) >> 1;
        break;
    case OMX_RK_VIDEO_CodingHEVC:        /**< H.265/HEVC */
        //decoder_format->aligned_width = ((decoder_format->width + 255) & (~255)) | 256;
        decoder_format->aligned_width = (decoder_format->width + 63) & (~63);
        decoder_format->aligned_height = (decoder_format->height + 7) & (~7);
        decoder_format->aligned_stride = decoder_format->aligned_width;
        decoder_format->aligned_frame_size = (decoder_format->aligned_width * decoder_format->aligned_height * 3) >> 1;
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

RK_S32 VpuApiLegacy::encoder_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut)
{
    RK_S32 ret = 0;
    MppPacket packet = NULL;
    vpu_api_dbg_func("enter\n");
    (void) ctx;

    ret = mpi->encode_get_packet(mpp_ctx, &packet);
    if (ret) {
        mpp_err_f("encode_get_packet failed ret %d\n", ret);
        goto FUNC_RET;
    }

    if (packet) {
        RK_U8 *src = (RK_U8 *)mpp_packet_get_data(packet);
        RK_U32 eos = mpp_packet_get_eos(packet);
        RK_S64 pts = mpp_packet_get_pts(packet);
        RK_U32 flag = mpp_packet_get_flag(packet);
        size_t length = mpp_packet_get_length(packet);

        mpp_assert(length);
        // remove first 00 00 00 01
        length -= 4;
        aEncOut->data = outData;
        aEncOut->size = (RK_S32)length;
        aEncOut->timeUs = pts;
        aEncOut->keyFrame = (flag & MPP_PACKET_FLAG_INTRA) ? (1) : (0);
        memcpy(outData, src + 4, length);
        vpu_api_dbg_output("get packet %p size %d pts %lld keyframe %d eos %d\n",
                           packet, length, pts, aEncOut->keyFrame, eos);

        mEosSet = eos;
        mpp_packet_deinit(&packet);
    } else {
        aEncOut->size = 0;
        vpu_api_dbg_output("encode_get_packet get NULL packet\n");
        if (mEosSet)
            ret = -1;
    }

FUNC_RET:
    vpu_api_dbg_func("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::perform(RK_U32 cmd, RK_U32 *data)
{
    vpu_api_dbg_func("enter\n");
    (void)cmd;
    (void)data;
    vpu_api_dbg_func("leave\n");
    return 0;
}

RK_S32 VpuApiLegacy::control(VpuCodecContext *ctx, VPU_API_CMD cmd, void *param)
{
    vpu_api_dbg_func("enter cmd %x param %p\n", cmd, param);

    if (mpi == NULL && !init_ok) {
        return 0;
    }

    MpiCmd mpicmd = MPI_CMD_BUTT;
    switch (cmd) {
    case VPU_API_ENC_SETCFG : {
        /* input EncParameter_t need to be transform to MppEncConfig */
        EncParameter_t *cfg = (EncParameter_t *)param;
        MppEncConfig mpp_cfg;

        mpp_cfg.size        = sizeof(mpp_cfg);
        mpp_cfg.version     = 0;
        mpp_cfg.width       = cfg->width;
        mpp_cfg.height      = cfg->height;
        mpp_cfg.hor_stride  = MPP_ALIGN(cfg->width, 16);
        mpp_cfg.ver_stride  = MPP_ALIGN(cfg->height, 16);
        mpp_cfg.format      = (RK_S32)enc_in_fmt;
        mpp_cfg.rc_mode     = cfg->rc_mode;
        mpp_cfg.skip_cnt    = 0;
        mpp_cfg.bps         = cfg->bitRate;
        mpp_cfg.fps_in      = cfg->framerate;
        mpp_cfg.fps_out     = cfg->framerateout;
        mpp_cfg.qp          = cfg->qp;
        mpp_cfg.gop         = cfg->intraPicRate;
        mpp_cfg.profile     = cfg->profileIdc;
        mpp_cfg.level       = cfg->levelIdc;
        mpp_cfg.cabac_en    = cfg->enableCabac;

        return mpi->control(mpp_ctx, MPP_ENC_SET_CFG, (MppParam)&mpp_cfg);
    } break;
    case VPU_API_ENC_GETCFG : {
        /* input EncParameter_t need to be transform to MppEncConfig */
        EncParameter_t *cfg = (EncParameter_t *)param;
        MppEncConfig mpp_cfg;

        MPP_RET ret = mpi->control(mpp_ctx, MPP_ENC_GET_CFG, (MppParam)&mpp_cfg);

        cfg->width          = mpp_cfg.width;
        cfg->height         = mpp_cfg.height;
        cfg->rc_mode        = mpp_cfg.rc_mode;
        cfg->bitRate        = mpp_cfg.bps;
        cfg->framerate      = mpp_cfg.fps_in;
        cfg->framerateout   = mpp_cfg.fps_out;
        cfg->qp             = mpp_cfg.qp;
        cfg->intraPicRate   = mpp_cfg.gop;
        cfg->profileIdc     = mpp_cfg.profile;
        cfg->levelIdc       = mpp_cfg.level;
        cfg->enableCabac    = mpp_cfg.cabac_en;
        cfg->cabacInitIdc   = 0;

        return ret;
    } break;
    case VPU_API_ENC_SETFORMAT : {
        enc_in_fmt = *((EncInputPictureType *)param);
        return 0;
    } break;
    case VPU_API_SET_VPUMEM_CONTEXT: {
        mpicmd = MPP_DEC_SET_EXT_BUF_GROUP;
        break;
    }
    case VPU_API_USE_PRESENT_TIME_ORDER: {
        mpicmd = MPP_DEC_SET_INTERNAL_PTS_ENABLE;
        break;
    }
    case VPU_API_SET_DEFAULT_WIDTH_HEIGH: {
        RK_U32 ImgWidth = 0;
        VPU_GENERIC *p = (VPU_GENERIC *)param;
        mpicmd = MPP_DEC_SET_FRAME_INFO;
        /**hightest of p->ImgWidth bit show current dec bitdepth
          * 0 - 8bit
          * 1 - 10bit
          **/
        if (p->ImgWidth & 0x80000000) {

            ImgWidth = ((p->ImgWidth & 0xFFFF) * 10) >> 3;
            p->CodecType = (p->ImgWidth & 0x40000000) ? MPP_FMT_YUV422SP_10BIT : MPP_FMT_YUV420SP_10BIT;
        } else {
            ImgWidth = (p->ImgWidth & 0xFFFF);
            p->CodecType = (p->ImgWidth & 0x40000000) ? MPP_FMT_YUV422SP : MPP_FMT_YUV420SP;
        }
        p->ImgWidth = (p->ImgWidth & 0xFFFF);
        if (ctx->videoCoding == OMX_RK_VIDEO_CodingHEVC) {
#ifdef SOFIA_3GR_LINUX
            p->ImgHorStride = hevc_ver_align_64(ImgWidth);
            p->ImgVerStride = hevc_ver_align_8(p->ImgHeight);
#else
            p->ImgHorStride = hevc_ver_align_256_odd(ImgWidth);
            p->ImgVerStride = hevc_ver_align_8(p->ImgHeight);
#endif
        } else {
            p->ImgHorStride = default_align_16(ImgWidth);
            p->ImgVerStride = default_align_16(p->ImgHeight);
        }

        break;
    }
    case VPU_API_SET_INFO_CHANGE: {
        mpicmd = MPP_DEC_SET_INFO_CHANGE_READY;
        break;
    }
    case VPU_API_USE_FAST_MODE: {
        mpicmd = MPP_DEC_SET_PARSER_FAST_MODE;
        break;
    }
    case VPU_API_DEC_GET_STREAM_COUNT: {
        mpicmd = MPP_DEC_GET_STREAM_COUNT;
        break;
    }
    case VPU_API_GET_VPUMEM_USED_COUNT: {
        mpicmd = MPP_DEC_GET_VPUMEM_USED_COUNT;
        break;
    }
    case VPU_API_DEC_GETFORMAT: {
        mpicmd = MPI_CMD_BUTT;
        getDecoderFormat(ctx, (DecoderFormat_t *)param);
        break;
    }
    case VPU_API_SET_OUTPUT_BLOCK: {
        mpicmd = MPP_SET_OUTPUT_BLOCK;
        break;
    }
	case VPU_API_DEC_GET_EOS_STATUS: {
        *(RK_S32 *)param = mEosSet;
        mpicmd = MPI_CMD_BUTT;
        break;
    }
    default: {
        break;
    }
    }

    RK_S32 ret = -1;
    if (mpicmd < MPI_CMD_BUTT)
        ret = mpi->control(mpp_ctx, mpicmd, (MppParam)param);

    vpu_api_dbg_func("leave\n");
    return ret;
}

