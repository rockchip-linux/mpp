/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 *
 * kmpp_venc_obj.c - kmpp_obj path operations via /dev/kmpp_objs + /dev/kmpp_ioctl
 */

#define MODULE_TAG "kmpp_venc_obj"

#include <string.h>

#include "rk_mpi.h"

#include "mpp_2str.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"

#include "kmpp.h"
#include "kmpp_obj.h"
#include "kmpp_frame.h"
#include "kmpp_packet.h"
#include "kmpp_meta_impl.h"

#include "kmpp_venc.h"

/* helper: verify param is a kobj, return NOK if not */
#define VENC_KOBJ_CHECK(param, name) \
    do { \
        if (!kmpp_obj_is_kobj((KmppObj)(param))) { \
            mpp_loge_f(name " needs a kobj, got %p\n", (param)); \
            return rk_nok; \
        } \
    } while(0)

static void kmpp_clear_frame_meta(KmppMeta meta)
{
    RK_S32 val;
    void *ptr = NULL;

    kmpp_meta_get_s32(meta, KEY_INPUT_IDR_REQ, &val);
    kmpp_meta_get_s32(meta, KEY_INPUT_PSKIP, &val);
    kmpp_meta_get_s32(meta, KEY_INPUT_PSKIP_NON_REF, &val);
    kmpp_meta_get_s32(meta, KEY_INPUT_PSKIP_NUM, &val);
    kmpp_meta_get_s32(meta, KEY_ENC_MARK_LTR, &val);
    kmpp_meta_get_s32(meta, KEY_ENC_USE_LTR, &val);
    kmpp_meta_get_s32(meta, KEY_ENC_FRAME_QP, &val);
    kmpp_meta_get_s32(meta, KEY_ENC_BASE_LAYER_PID, &val);
    kmpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &val);
    kmpp_meta_get_ptr(meta, KEY_ROI_DATA, &ptr);
    kmpp_meta_get_ptr(meta, KEY_OSD_DATA4, &ptr);
    kmpp_meta_get_ptr(meta, KEY_USER_DATA, &ptr);
    kmpp_meta_get_ptr(meta, KEY_USER_DATAS, &ptr);
    kmpp_meta_get_ptr(meta, KEY_JPEG_ROI_DATA, &ptr);
}

static MPP_RET kmpp_convert_frame_meta(KmppMeta dst, MppMeta src)
{
    MppEncROICfg *roi = NULL;
    MppEncOSDData3 *osd = NULL;
    MppEncUserData *ud = NULL;
    MppEncUserDataSet *uds = NULL;
    MppJpegROICfg *jpeg_roi = NULL;
    RK_S32 val;
    MPP_RET ret;

#define CONVERT_META_SCALAR(key) \
    do { \
        if (mpp_meta_get_s32(src, key, &val) == MPP_OK) { \
            if (kmpp_meta_set_s32(dst, key, val)) \
                mpp_loge("frame scalar key %#08x not converted, skipped\n", key); \
        } \
    } while (0)

    CONVERT_META_SCALAR(KEY_INPUT_IDR_REQ);
    CONVERT_META_SCALAR(KEY_INPUT_PSKIP);
    CONVERT_META_SCALAR(KEY_INPUT_PSKIP_NON_REF);
    CONVERT_META_SCALAR(KEY_INPUT_PSKIP_NUM);
    CONVERT_META_SCALAR(KEY_ENC_MARK_LTR);
    CONVERT_META_SCALAR(KEY_ENC_USE_LTR);
    CONVERT_META_SCALAR(KEY_ENC_FRAME_QP);
    CONVERT_META_SCALAR(KEY_ENC_BASE_LAYER_PID);
    CONVERT_META_SCALAR(KEY_TEMPORAL_ID);

#undef CONVERT_META_SCALAR

    mpp_meta_get_ptr(src, KEY_ROI_DATA, (void **)&roi);
    if (roi) {
        MppEncROICfgLegacy legacy = { .change = 1 };

        if (roi->number > MPP_ARRAY_ELEMS(legacy.regions) || (roi->number && !roi->regions))
            return MPP_ERR_VALUE;

        legacy.number = roi->number;
        if (roi->number)
            memcpy(legacy.regions, roi->regions, roi->number * sizeof(*roi->regions));

        ret = kmpp_meta_set_ptr(dst, KEY_ROI_DATA, &legacy);
        if (ret)
            return ret;
    }

    mpp_meta_get_ptr(src, KEY_OSD_DATA3, (void **)&osd);
    if (osd) {
        ret = kmpp_meta_set_osd(dst, osd);
        if (ret)
            return ret;
    }

    mpp_meta_get_ptr(src, KEY_USER_DATA, (void **)&ud);
    if (ud) {
        MppEncUserDataShm shm = { .len = ud->len };

        shm.data.uptr = ud->pdata;
        ret = kmpp_meta_set_ptr(dst, KEY_USER_DATA, &shm);
        if (ret)
            return ret;
    }

    mpp_meta_get_ptr(src, KEY_USER_DATAS, (void **)&uds);
    if (uds) {
        MppEncUserDataSetShm *shm;
        size_t size;
        RK_U32 i;

        if (uds->count && !uds->datas)
            return MPP_ERR_VALUE;

        size = sizeof(*shm) + uds->count * sizeof(MppEncUserDataFullShm);
        shm = mpp_calloc_size(MppEncUserDataSetShm, size);
        if (!shm)
            return MPP_ERR_MALLOC;

        shm->count = uds->count;
        for (i = 0; i < uds->count; i++) {
            shm->data[i].len = uds->datas[i].len;
            shm->data[i].uuid.uptr = uds->datas[i].uuid;
            shm->data[i].data.uptr = uds->datas[i].pdata;
        }

        ret = kmpp_meta_set_ptr(dst, KEY_USER_DATAS, shm);
        mpp_free(shm);
        if (ret)
            return ret;
    }

    mpp_meta_get_ptr(src, KEY_JPEG_ROI_DATA, (void **)&jpeg_roi);
    if (jpeg_roi) {
        ret = kmpp_meta_set_ptr(dst, KEY_JPEG_ROI_DATA, jpeg_roi);
        if (ret)
            return ret;
    }

    return MPP_OK;
}

static KmppFrame kmpp_convert_frame(Kmpp *ctx, MppFrame frame, RK_U32 *is_kframe)
{
    if (!__check_is_mpp_frame(frame)) {
        MppFrameImpl *impl = (MppFrameImpl *)frame;
        KmppMeta kMeta = NULL;

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
            KmppShmPtr *buf_shm = kmpp_obj_to_shm(impl->buffer);

            if (buf_shm) {
                kmpp_frame_set_buffer(ctx->mKframe, buf_shm);
            } else {
                kmpp_frame_set_buf_fd(ctx->mKframe, mpp_buffer_get_fd(impl->buffer));
            }
        }

        kmpp_frame_get_meta_obj(ctx->mKframe, &kMeta);
        if (kMeta)
            kmpp_clear_frame_meta(kMeta);

        if (mpp_frame_has_meta(frame)) {
            MppMeta meta = mpp_frame_get_meta(frame);
            MppPacket packet = NULL;

            /* Convert frame-scoped Meta into the reused KmppFrame object. */
            if (kMeta && kmpp_convert_frame_meta(kMeta, meta)) {
                mpp_loge_f("convert frame meta failed\n");
                return NULL;
            }

            mpp_meta_get_packet(meta, KEY_OUTPUT_PACKET, &packet);
            ctx->mPacket = packet;
        }

        *is_kframe = 0;
        return ctx->mKframe;
    }

    *is_kframe = 1;
    return (KmppFrame)frame;
}

static MPP_RET init(Kmpp *ctx, MppCtxType type, MppCodingType coding)
{
    KmppVenc venc = NULL;
    MppVencKcfg kcfg = NULL;
    rk_s32 ret;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (mpp_check_support_format(type, coding)) {
        mpp_loge("unable to create %s %s for mpp unsupported\n",
                 strof_ctx_type(type), strof_coding_type(coding));
        return MPP_NOK;
    }

    /* get KmppVenc kernel object */
    ret = kmpp_venc_get(&venc);
    if (ret || !venc) {
        mpp_loge("kmpp_venc_get failed ret %d\n", ret);
        return MPP_NOK;
    }

    ctx->mVenc = venc;

    /* create init config and copy from mVencInitKcfg */
    mpp_venc_kcfg_init(&kcfg, MPP_VENC_KCFG_TYPE_INIT);

    ret = kmpp_obj_copy(kcfg, ctx->mVencInitKcfg);
    if (ret) {
        mpp_loge("kmpp_obj_copy failed ret %d\n", ret);
        goto done;
    }

    kmpp_obj_get_u32(ctx->mVencInitKcfg, "chan_dup", &ctx->mChanDup);

    /* init encoder */
    ret = kmpp_venc_init(venc, kcfg);
    if (ret) {
        mpp_loge("kmpp_venc_init failed ret %d\n", ret);
        goto done;
    }

    if (!ctx->mChanDup) {
        ret = kmpp_venc_start(venc);
        if (ret) {
            mpp_loge("kmpp_venc_start failed ret %d\n", ret);
            goto done;
        }
    }

    if (ctx->mPacketGroup == NULL)
        mpp_buffer_group_get_internal(&ctx->mPacketGroup, MPP_BUFFER_TYPE_ION);

done:
    if (ret) {
        if (kcfg)
            mpp_venc_kcfg_deinit(kcfg);
        if (venc)
            kmpp_venc_put(venc);

        ctx->mVenc = NULL;
    } else {
        ctx->mInitDone = 1;
        ctx->mType = type;
        ctx->mOutputTimeout = MPP_POLL_BLOCK;
    }

    return (MPP_RET)ret;
}

static void clear(Kmpp *ctx)
{
    if (!ctx)
        return;

    if (ctx->mVenc) {
        kmpp_venc_stop(ctx->mVenc);
        kmpp_venc_deinit(ctx->mVenc);
        kmpp_venc_put(ctx->mVenc);
        ctx->mVenc = NULL;
    }

    if (ctx->mPacketGroup) {
        mpp_buffer_group_put(ctx->mPacketGroup);
        ctx->mPacketGroup = NULL;
    }

    if (ctx->mKframe) {
        kmpp_frame_put(ctx->mKframe);
        ctx->mKframe = NULL;
    }
}

static MPP_RET put_frame(Kmpp *ctx, MppFrame frame)
{
    MPP_RET ret = MPP_OK;
    RK_U32 is_kframe = 0;
    KmppFrame kframe;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    kframe = kmpp_convert_frame(ctx, frame, &is_kframe);
    if (!kframe) {
        mpp_loge("kmpp_convert_frame failed\n");
        return MPP_NOK;
    }

    /* check buffer before sending */
    if (!is_kframe) {
        MppFrameImpl *impl = (MppFrameImpl *)frame;
        if (!impl->buffer) {
            mpp_loge_f("kmpp put_frame buf is NULL\n");
            return MPP_NOK;
        }
    }

    ret = kmpp_venc_put_frm(ctx->mVenc, kframe);
    if (ret)
        mpp_loge("kmpp_venc_put_frm failed ret %d\n", ret);

    return ret;
}

static MPP_RET get_packet(Kmpp *ctx, MppPacket *packet)
{
    KmppPacket kmpp_pkt = NULL;
    MppPacket pkt = NULL;
    RK_S32 len;
    RK_U32 flag;
    KmppShmPtr pos;
    RK_S64 dts;
    RK_S64 pts;
    rk_s32 ret;

    if (!ctx)
        return MPP_ERR_VALUE;

    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    /* blocking get - kmpp_venc_get_pkt will block until packet is ready */
    ret = kmpp_venc_get_pkt(ctx->mVenc, &kmpp_pkt);
    if (ret || !kmpp_pkt) {
        mpp_loge("kmpp_venc_get_pkt failed ret %d\n", ret);
        return MPP_NOK;
    }

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

        mpp_packet_set_length(pkt, len);
    } else {
        mpp_packet_init(&pkt, pos.uptr, len);
    }

    mpp_packet_set_release(pkt, kmpp_release_venc_packet, ctx, kmpp_pkt);
    mpp_packet_set_dts(pkt, dts);
    mpp_packet_set_pts(pkt, pts);
    mpp_packet_set_flag(pkt, flag);
    if (flag & MPP_PACKET_FLAG_INTRA) {
        MppMeta meta = mpp_packet_get_meta(pkt);

        mpp_meta_set_s32(meta, KEY_OUTPUT_INTRA, 1);
    }
    *packet = pkt;

    return MPP_OK;
}

static rk_s32 kmpp_venc_ctrl_exec(KmppVenc venc, KmppObj ctrl, MpiCmd cmd, MppParam param)
{
    rk_s32 ret;

    kmpp_obj_set_s32(ctrl, "cmd", cmd);
    kmpp_obj_set_u32(ctrl, "flags", KMPP_CTRL_FLAG_NONE);

    switch (cmd) {
    case MPP_ENC_SET_IDR_FRAME: {
        kmpp_obj_set_u64(ctrl, "val", 0);
    } break;
    case MPP_ENC_SET_HEADER_MODE:
    case MPP_ENC_SET_SEI_CFG: {
        RK_U32 val = param ? *(RK_U32 *)param : 0;

        kmpp_obj_set_u64(ctrl, "val", (RK_U64)val);
    } break;
    case MPP_SET_INPUT_TIMEOUT:
    case MPP_SET_OUTPUT_TIMEOUT: {
        RK_S64 val = param ? *(RK_S64 *)param : 0;

        kmpp_obj_set_u64(ctrl, "val", (rk_u64)val);
    } break;
    case MPP_ENC_SET_ROI_CFG: {
        MppEncROICfg *roi = (MppEncROICfg *)param;
        MppEncROICfgLegacy legacy;

        legacy.change = 1;
        legacy.number = roi->number > 8 ? 8 : roi->number;
        memcpy(legacy.regions, roi->regions, legacy.number * sizeof(MppEncROIRegion));
        if (mpp_venc_ctrl_set_flex(ctrl, &legacy, sizeof(MppEncROICfgLegacy)))
            return rk_nok;
    } break;
    case MPP_ENC_GET_ROI_CFG: {
        kmpp_obj_set_u32(ctrl, "flags", KMPP_CTRL_FLAG_FLEX);
        kmpp_obj_set_u32(ctrl, "size", sizeof(MppEncROICfgLegacy));
        if (kmpp_obj_resize_f(ctrl, sizeof(MppEncROICfgLegacy)))
            return rk_nok;
    } break;
    case MPP_ENC_SET_OSD_DATA_CFG: {
        if (mpp_venc_ctrl_set_flex(ctrl, param, sizeof(MppEncOSDData3)))
            return rk_nok;
    } break;
    case MPP_ENC_SET_USERDATA: {
        MppEncUserDataShm u = { .len = 0, .data = {.kaddr = 0, .uaddr = 0} };

        if (param) {
            MppEncUserData *ud = (MppEncUserData *)param;

            u.len = ud->len && ud->pdata ? ud->len : 0;
            u.data.uaddr = ud->len && ud->pdata ? (rk_u64)(intptr_t)ud->pdata : 0;
        }

        kmpp_obj_set_st(ctrl, "arg", &u);
    } break;
    case MPP_ENC_GET_HDR_SYNC: {
    } break;
    case MPP_ENC_SET_REF_CFG: {
        if (mpp_venc_ctrl_set_flex(ctrl, param, sizeof(MppEncRefParam)))
            return rk_nok;
    } break;
    default: {
        return rk_ok;
    } break;
    }

    ret = kmpp_venc_control(venc, ctrl);
    if (ret)
        mpp_loge_f("kmpp_venc_control cmd %d failed ret %d\n", cmd, ret);

    /* GET: read back kernel-written output */
    if (!ret) {
        switch (cmd) {
        case MPP_ENC_GET_ROI_CFG: {
            MppEncROICfgLegacy *legacy = (MppEncROICfgLegacy *)mpp_venc_ctrl_flex_base(ctrl);
            MppEncROICfg *roi = (MppEncROICfg *)param;

            if (param && roi) {
                roi->number = legacy->number;
                if (legacy->number)
                    memcpy(roi->regions, legacy->regions,
                           legacy->number * sizeof(MppEncROIRegion));
            } else {
                mpp_loge_f("invalid roi ptr %p -> %p\n", legacy, roi);
                return rk_nok;
            }
        } break;
        case MPP_ENC_GET_HDR_SYNC: {
            KmppShmPtr ret_sptr;
            KmppPacket hdr_pkt = NULL;

            kmpp_obj_get_st(ctrl, "ret", &ret_sptr);
            if (ret_sptr.uaddr)
                kmpp_obj_get_by_sptr_f((KmppObj *)&hdr_pkt, &ret_sptr);
            if (hdr_pkt) {
                KmppShmPtr pos;
                RK_S32 hlen = 0;

                kmpp_packet_get_pos(hdr_pkt, &pos);
                kmpp_packet_get_length(hdr_pkt, &hlen);
                if (pos.uptr && hlen > 0) {
                    memcpy(mpp_packet_get_pos((MppPacket)param), pos.uptr, hlen);
                    mpp_packet_set_length((MppPacket)param, hlen);
                }
                kmpp_obj_impl_put_f(hdr_pkt);
            }
        } break;
        default: {
        } break;
        }
    }

    return ret;
}

static MPP_RET control(Kmpp *ctx, MpiCmd cmd, MppParam param)
{
    if (!ctx || !ctx->mVenc)
        return MPP_ERR_VALUE;

    if (cmd == MPP_SET_SELECT_TIMEOUT)
        return MPP_OK;

    return kmpp_venc_ctrl(ctx->mVenc, cmd, param);
}

rk_s32 kmpp_venc_ctrl(KmppVenc venc, MpiCmd cmd, MppParam param)
{
    rk_s32 ret = rk_nok;

    if (!venc)
        return ret;

    switch (cmd) {
    case MPP_ENC_SET_CFG: {
        VENC_KOBJ_CHECK(param, "SET_CFG");
        ret = kmpp_venc_set_cfg(venc, param);
    } break;
    case MPP_ENC_GET_CFG: {
        VENC_KOBJ_CHECK(param, "GET_CFG");
        ret = kmpp_venc_get_cfg(venc, param);
    } break;
    case MPP_ENC_SET_REF_CFG: {
        MppVencKcfg def_ref = NULL;

        if (param) {
            if (!kmpp_obj_is_obj((KmppObj)(param))) {
                mpp_logi_f("SET_REF_CFG the param is not obj, apply as legacy MppEncRefParam\n");
                goto DEFAULT;
            }

            if (!kmpp_obj_is_kobj((KmppObj)(param))) {
                mpp_loge_f("SET_REF_CFG the param is not kobj\n");
                return rk_nok;
            }
        } else {
            /* NULL → apply default IPPP (1 st frame, forward ref, no lt) */
            mpp_venc_kcfg_init(&def_ref, MPP_VENC_KCFG_TYPE_REF_CFG);
            if (!def_ref) {
                mpp_loge("failed to create default ref_cfg\n");
                return MPP_NOK;
            }
            param = def_ref;
        }

        ret = kmpp_venc_set_ref_cfg(venc, param);

        if (def_ref)
            mpp_venc_kcfg_deinit(def_ref);
    } break;
    case MPP_ENC_GET_REF_CFG: {
        VENC_KOBJ_CHECK(param, "GET_REF_CFG");
        ret = kmpp_venc_get_ref_cfg(venc, param);
    } break;
    case MPP_SET_VENC_INIT_KCFG: {
        VENC_KOBJ_CHECK(param, "SET_INIT_CFG");
        ret = kmpp_venc_init(venc, param);
    } break;
    default: {
DEFAULT:
        {
            MppVencKcfg ctrl = NULL;

            mpp_venc_kcfg_init(&ctrl, MPP_VENC_KCFG_TYPE_CTRL_CFG);
            if (ctrl) {
                ret = kmpp_venc_ctrl_exec(venc, ctrl, cmd, param);
                mpp_venc_kcfg_deinit(ctrl);
            } else {
                mpp_loge_f("can not create valid ctrl_cfg object\n");
            }
        }
    } break;
    }

    return ret;
}

static MPP_RET reset(Kmpp *ctx)
{
    if (!ctx)
        return MPP_ERR_VALUE;
    if (ctx->mChanDup)
        return MPP_OK;
    if (!ctx->mInitDone)
        return MPP_ERR_INIT;

    return kmpp_venc_reset(ctx->mVenc);
}

KmppOps kmpp_venc_obj_ops = {
    .init           = init,
    .start          = NULL,
    .stop           = NULL,
    .pause          = NULL,
    .resume         = NULL,
    .put_frame      = put_frame,
    .get_packet     = get_packet,
    .control        = control,
    .reset          = reset,
    .clear          = clear,
};
