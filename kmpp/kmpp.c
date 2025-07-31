/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_impl.h"
#include "mpp_2str.h"
#include "mpp_debug.h"

#include "kmpp.h"
#include "kmpp_obj.h"
#include "mpp_soc.h"
#include "kmpp_frame.h"
#include "mpp_buffer_impl.h"
#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"
#include "kmpp_packet.h"
#include "mpp_vcodec_client.h"
#include "mpp_enc_cfg_impl.h"

typedef struct KmppFrameInfos_t {
    RK_U32  width;
    RK_U32  height;
    RK_U32  hor_stride;
    RK_U32  ver_stride;
    RK_U32  hor_stride_pixel;
    RK_U32  offset_x;
    RK_U32  offset_y;
    RK_U32  fmt;
    RK_U32  fd;
    RK_U64  pts;
    RK_S32  jpeg_chan_id;
    void    *osd_buf;
    RK_S32  mpi_buf_id;
    void    *jpg_combo_osd_buf;
    RK_U32  is_gray;
    RK_U32  is_full;
    RK_U32  phy_addr;
    RK_U64  dts;
    void    *pp_info;
    RK_U32  pskip_num;
    union {
        RK_U32 val;
        struct {
            RK_U32  eos : 1;
            RK_U32  pskip : 1;
            RK_U32  isr_request : 1;
        };
    };
} KmppFrameInfos;

static void kmpp_release_venc_packet(void *ctx, void *arg)
{
    KmppPacket pkt = (KmppPacket)arg;

    if (!ctx || !pkt) {
        mpp_err_f("invalid input ctx %p pkt %p\n", ctx, pkt);
        return;
    }

    kmpp_packet_put(pkt);
}

static MPP_RET init(Kmpp *ctx, MppCtxType type, MppCodingType coding)
{
    MPP_RET ret = MPP_NOK;
    RK_U32 chan_id;
    void *hnd;
    RK_U32 size;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (mpp_check_support_format(type, coding)) {
        mpp_err("unable to create %s %s for mpp unsupported\n",
                strof_ctx_type(type), strof_coding_type(coding));
        return MPP_NOK;
    }

    if (ctx->mClientFd < 0) {
        ctx->mClientFd = mpp_vcodec_open();
        if (ctx->mClientFd < 0) {
            mpp_err("mpp_vcodec dev open failed\n");
            return MPP_NOK;
        }
    }

    hnd = kmpp_obj_to_shm(ctx->mVencInitKcfg);
    size = kmpp_obj_to_shm_size(ctx->mVencInitKcfg);
    kmpp_obj_get_u32(ctx->mVencInitKcfg, "chan_dup", &ctx->mChanDup);
    kmpp_obj_set_s32(ctx->mVencInitKcfg, "chan_fd", ctx->mClientFd);

    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_CREATE, 0, size, hnd);
    if (ret) {
        mpp_err("chan %d VCODEC_CHAN_CREATE failed\n", ctx->mChanId);
        return ret;
    }

    if (!ctx->mChanDup) {
        ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_START, 0, 0, 0);
        if (ret) {
            mpp_err("chan %d VCODEC_CHAN_START failed\n", ctx->mChanId);
            return ret;
        }
    }

    if (ctx->mPacketGroup == NULL)
        mpp_buffer_group_get_internal(&ctx->mPacketGroup, MPP_BUFFER_TYPE_ION);

    kmpp_obj_get_u32(ctx->mVencInitKcfg, "chan_id", &chan_id);
    mpp_log("client %d open chan_id %d ok", ctx->mClientFd, chan_id);
    ctx->mInitDone = 1;
    ctx->mChanId = chan_id;
    ctx->mType = type;
    ctx->mOutputTimeout = MPP_POLL_BLOCK;
    ctx->mTimeout.tv_sec  = 2;
    ctx->mTimeout.tv_usec = 100000;

    return ret;
}

static MPP_RET open_client(Kmpp *ctx)
{
    if (!ctx)
        return MPP_ERR_VALUE;

    ctx->mClientFd = mpp_vcodec_open();
    if (ctx->mClientFd < 0) {
        mpp_err("mpp_vcodec dev open failed\n");
        return MPP_NOK;
    }
    return MPP_OK;
}

static void clear(Kmpp *ctx)
{
    if (!ctx)
        return;

    if (!ctx->mChanDup) {
        MPP_RET ret;

        ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_DESTROY, 0, 0, 0);
        if (ret)
            mpp_err("chan %d VCODEC_CHAN_DESTROY failed\n", ctx->mChanId);
    }

    if (ctx->mClientFd >= 0)
        mpp_vcodec_close(ctx->mClientFd);
    ctx->mClientFd = -1;

    if (ctx->mPacketGroup) {
        mpp_buffer_group_put(ctx->mPacketGroup);
        ctx->mPacketGroup = NULL;
    }

    if (ctx->mKframe) {
        kmpp_frame_put(ctx->mKframe);
        ctx->mKframe = NULL;
    }
}

static MPP_RET start(Kmpp *ctx)
{
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (ctx->mChanDup)
        return MPP_OK;

    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_START, 0, 0, 0);
    if (ret)
        mpp_err("chan %d VCODEC_CHAN_START failed\n", ctx->mChanId);

    return ret;
}

static MPP_RET stop(Kmpp *ctx)
{
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (ctx->mChanDup)
        return MPP_OK;

    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_START, 0, 0, 0);
    if (ret)
        mpp_err("chan %d VCODEC_CHAN_START failed\n", ctx->mChanId);

    return ret;
}

static MPP_RET mpp_pause(Kmpp *ctx)
{
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (ctx->mChanDup)
        return MPP_OK;

    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_PAUSE, 0, 0, 0);
    if (ret)
        mpp_err("chan %d VCODEC_CHAN_PAUSE failed\n", ctx->mChanId);

    return ret;
}

static MPP_RET resume(Kmpp *ctx)
{
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (ctx->mChanDup)
        return MPP_OK;

    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_RESUME, 0, 0, 0);
    if (ret)
        mpp_err("chan %d VCODEC_CHAN_RESUME failed\n", ctx->mChanId);

    return ret;
}

static MPP_RET put_packet(Kmpp *ctx, MppPacket packet)
{
    (void) packet;
    MPP_RET ret = MPP_OK;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    return ret;
}

static MPP_RET get_frame(Kmpp *ctx, MppFrame *frame)
{
    (void)frame;
    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    return MPP_OK;
}

static MPP_RET put_frame(Kmpp *ctx, MppFrame frame)
{
    MPP_RET ret = MPP_OK;
    KmppShmPtr *ptr = NULL;
    rk_s32 size;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    if (!__check_is_mpp_frame(frame)) {
        MppFrameImpl *impl = (MppFrameImpl *)frame;

        if (ctx->mKframe == NULL)
            kmpp_frame_get(&ctx->mKframe);

        kmpp_frame_set_width(ctx->mKframe, impl->width);
        kmpp_frame_set_height(ctx->mKframe, impl->height);
        kmpp_frame_set_hor_stride(ctx->mKframe, impl->hor_stride);
        kmpp_frame_set_ver_stride(ctx->mKframe, impl->ver_stride);
        kmpp_frame_set_fmt(ctx->mKframe, impl->fmt);
        kmpp_frame_set_eos(ctx->mKframe, impl->eos);
        kmpp_frame_set_pts(ctx->mKframe, impl->pts);
        kmpp_frame_set_dts(ctx->mKframe, impl->dts);
        kmpp_frame_set_offset_x(ctx->mKframe, impl->offset_x);
        kmpp_frame_set_offset_y(ctx->mKframe, impl->offset_y);
        kmpp_frame_set_hor_stride_pixel(ctx->mKframe, impl->hor_stride_pixel);

        if (impl->buffer) {
            kmpp_frame_set_buf_fd(ctx->mKframe, mpp_buffer_get_fd(impl->buffer));
        } else {
            mpp_loge_f("kmpp put_frame buf is NULL\n");
            return MPP_NOK;
        }

        if (mpp_frame_has_meta(frame)) {
            MppMeta meta = mpp_frame_get_meta(frame);
            MppPacket packet = NULL;

            mpp_meta_get_packet(meta, KEY_OUTPUT_PACKET, &packet);
            ctx->mPacket = packet;

            /* set roi */
            {
                MppEncROICfg *roi_data = NULL;
                MppEncROICfgLegacy roi_data0;

                mpp_meta_get_ptr(meta, KEY_ROI_DATA, (void**)&roi_data);
                if (roi_data) {
                    roi_data0.change = 1;
                    roi_data0.number = roi_data->number;
                    memcpy(roi_data0.regions, roi_data->regions, roi_data->number * sizeof(MppEncROIRegion));
                    ctx->mApi->control(ctx, MPP_ENC_SET_ROI_CFG, &roi_data0);
                }
            }
        }

        ptr = kmpp_obj_to_shm(ctx->mKframe);
        size = kmpp_obj_to_shm_size(ctx->mKframe);
    } else {
        ptr = kmpp_obj_to_shm(frame);
        size = kmpp_obj_to_shm_size(frame);
    }

    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_IN_FRM_RDY, 0, size, ptr);
    if (ret)
        mpp_err("chan %d VCODEC_CHAN_IN_FRM_RDY failed\n", ctx->mChanId);

    return ret;
}

static MPP_RET get_packet(Kmpp *ctx, MppPacket *packet)
{
    RK_S32 ret;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    struct timeval timeout;
    fd_set read_fds;

    FD_ZERO(&read_fds);
    FD_SET(ctx->mClientFd, &read_fds);

    memcpy(&timeout, &ctx->mTimeout, sizeof(timeout));
    ret = select(ctx->mClientFd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret <= 0)
        return MPP_NOK;

    if (FD_ISSET(ctx->mClientFd, &read_fds)) {
        KmppShmPtr sptr;
        MppPacket pkt = NULL;
        KmppPacket kmpp_pkt = NULL;
        RK_S32 len;
        RK_U32 flag;
        KmppShmPtr pos;
        RK_S64 dts;
        RK_S64 pts;

        ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_OUT_PKT_RDY,
                               0, sizeof(KmppShmPtr), &sptr);
        if (ret) {
            mpp_err("chan %d VCODEC_CHAN_OUT_PKT_RDY failed\n", ctx->mChanId);
            return MPP_NOK;
        }

        kmpp_obj_get_by_sptr_f(&kmpp_pkt, &sptr);
        kmpp_packet_get_flag(kmpp_pkt, &flag);
        kmpp_packet_get_length(kmpp_pkt, &len);
        kmpp_packet_get_pos(kmpp_pkt, &pos);
        kmpp_packet_get_dts(kmpp_pkt, &dts);
        kmpp_packet_get_pts(kmpp_pkt, &pts);

        if (ctx->mPacket) {
            void *dst;

            pkt = ctx->mPacket;
            ctx->mPacket = NULL;
            if (pos.uptr) {
                dst = mpp_packet_get_pos(pkt);
                memcpy(dst, pos.uptr, len);
            }

            kmpp_packet_put(kmpp_pkt);
            mpp_packet_set_length(pkt, len);
        } else {
            mpp_packet_init(&pkt, pos.uptr, len);
            mpp_packet_set_release(pkt, kmpp_release_venc_packet, ctx, kmpp_pkt);
        }

        mpp_packet_set_dts(pkt, dts);
        mpp_packet_set_pts(pkt, pts);
        mpp_packet_set_flag(pkt, flag);
        if (flag & MPP_PACKET_FLAG_INTRA) {
            MppMeta meta = mpp_packet_get_meta(pkt);

            mpp_meta_set_s32(meta, KEY_OUTPUT_INTRA, 1);
        }
        *packet = pkt;
    }

    return MPP_OK;
}

static MPP_RET release_packet(Kmpp *ctx, MppPacket *packet)
{
    KmppPacket pkt  = (KmppPacket) * packet;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    if (*packet == NULL)
        return MPP_NOK;

    if (ctx->mClientFd < 0)
        return MPP_NOK;

    kmpp_packet_put(pkt);

    return MPP_OK;
}

static MPP_RET poll(Kmpp *ctx, MppPortType type, MppPollType timeout)
{
    MPP_RET ret = MPP_NOK;
    (void)type;
    (void)timeout;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    return ret;
}

static MPP_RET dequeue(Kmpp *ctx, MppPortType type, MppTask *task)
{
    MPP_RET ret = MPP_NOK;
    (void)type;
    (void)task;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    return ret;
}

static MPP_RET enqueue(Kmpp *ctx, MppPortType type, MppTask task)
{
    MPP_RET ret = MPP_NOK;
    (void)type;
    (void)task;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    return ret;
}

static MPP_RET control(Kmpp *ctx, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;
    RK_U32 size = 0;
    void *arg = param;

    if (!ctx)
        return MPP_ERR_VALUE;

    switch (cmd) {
    case MPP_ENC_SET_CFG :
    case MPP_ENC_GET_CFG : {
        MppEncCfgImpl *impl = (MppEncCfgImpl *)param;

        mpp_assert(impl->is_kobj);
        if (impl->is_kobj) {
            KmppObj obj = impl->obj;

            arg = kmpp_obj_to_shm(obj);
            size = kmpp_obj_to_shm_size(obj);
        } else {
            mpp_loge("can not set non-kobj %p to kmpp\n", impl);
            return MPP_NOK;
        }
    } break;
    case MPP_ENC_SET_HEADER_MODE :
    case MPP_ENC_SET_SEI_CFG : {
        size = sizeof(RK_U32);
    } break;
    case MPP_ENC_GET_REF_CFG :
    case MPP_ENC_SET_REF_CFG : {
        size = sizeof(MppEncRefParam);
    } break;
    case MPP_ENC_GET_ROI_CFG:
    case MPP_ENC_SET_ROI_CFG: {
        size = sizeof(MppEncROICfgLegacy);
    } break;
    // case MPP_ENC_SET_JPEG_ROI_CFG : {
    //     size = sizeof(MppJpegROICfg);
    // } break;
    case MPP_ENC_SET_OSD_DATA_CFG: {
        size = sizeof(MppEncOSDData3);
    } break;
    case MPP_ENC_SET_USERDATA: {
        size = sizeof(MppEncUserData);
    } break;
    case MPP_SET_VENC_INIT_KCFG: {
        KmppObj obj = param;

        arg = kmpp_obj_to_shm(obj);
        size = kmpp_obj_to_shm_size(obj);
    } break;
    case MPP_SET_SELECT_TIMEOUT: {
        struct timeval *p = (struct timeval *)param;
        ctx->mTimeout.tv_sec = p->tv_sec;
        ctx->mTimeout.tv_usec = p->tv_usec;
        return MPP_OK;
    } break;
    case MPP_ENC_SET_IDR_FRAME: {
        size = 0;
    } break;
    default : {
        size = 0;
        return MPP_OK;
    } break;
    }
    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_CONTROL, cmd, size, arg);
    if (ret)
        mpp_err("chan %d VCODEC_CHAN_CONTROL failed\n", ctx->mChanId);

    return ret;
}

static MPP_RET reset(Kmpp *ctx)
{
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;
    if (ctx->mChanDup)
        return MPP_OK;
    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    ret = mpp_vcodec_ioctl(ctx->mClientFd, VCODEC_CHAN_RESET, 0, 0, 0);
    if (ret)
        mpp_err("chan %d VCODEC_CHAN_RESET failed\n", ctx->mChanId);

    return ret;
}

static MPP_RET notify_flag(Kmpp *ctx, RK_U32 flag)
{
    (void)flag;
    (void)ctx;

    return MPP_NOK;
}

static MPP_RET notify(Kmpp *ctx, MppBufferGroup group)
{
    MPP_RET ret = MPP_OK;
    (void)group;

    switch (ctx->mType) {
    case MPP_CTX_DEC : {
    } break;
    default : {
    } break;
    }

    return ret;
}

static MPP_RET get_fd(Kmpp *ctx, RK_S32 *fd)
{
    MPP_RET ret = MPP_OK;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (ctx->mClientFd >= 0)
        *fd = fcntl(ctx->mClientFd, F_DUPFD_CLOEXEC, 3);
    else
        *fd = -1;

    if (*fd < 0)
        ret = MPP_NOK;

    return ret;
}

static MPP_RET close_fd(Kmpp *ctx, RK_S32 fd)
{
    if (fd >= 0)
        close(fd);
    (void)ctx;
    return MPP_OK;
}

static KmppOps ops = {
    .open_client    = open_client,
    .init           = init,
    .start          = start,
    .stop           = stop,
    .pause          = mpp_pause,
    .resume         = resume,
    .put_packet     = put_packet,
    .get_frame      = get_frame,
    .put_frame      = put_frame,
    .get_packet     = get_packet,
    .release_packet = release_packet,
    .poll           = poll,
    .dequeue        = dequeue,
    .enqueue        = enqueue,
    .reset          = reset,
    .control        = control,
    .notify_flag    = notify_flag,
    .notify         = notify,
    .get_fd         = get_fd,
    .close_fd       = close_fd,
    .clear          = clear,
};

void mpp_get_api(Kmpp *ctx)
{
    if (!ctx)
        return;

    ctx->mApi = &ops;
}
