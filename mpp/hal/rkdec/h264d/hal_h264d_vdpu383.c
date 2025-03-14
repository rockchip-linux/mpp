/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_h264d_vdpu383"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "hal_h264d_global.h"
#include "hal_h264d_vdpu383.h"
#include "vdpu383_h264d.h"
#include "mpp_dec_cb_param.h"

/* Number registers for the decoder */
#define DEC_VDPU383_REGISTERS       276

#define VDPU383_CABAC_TAB_SIZE      (928*4 + 128)       /* bytes */
#define VDPU383_SPSPPS_SIZE         (168 + 128)     /* bytes */
#define VDPU383_RPS_SIZE            (128 + 128 + 128)   /* bytes */
#define VDPU383_SCALING_LIST_SIZE   (6*16+2*64 + 128)   /* bytes */
#define VDPU383_ERROR_INFO_SIZE     (256*144*4)         /* bytes */
#define H264_CTU_SIZE               16

#define VDPU383_CABAC_TAB_ALIGNED_SIZE      (MPP_ALIGN(VDPU383_CABAC_TAB_SIZE, SZ_4K))
#define VDPU383_ERROR_INFO_ALIGNED_SIZE     (0)
#define VDPU383_SPSPPS_ALIGNED_SIZE         (MPP_ALIGN(VDPU383_SPSPPS_SIZE, SZ_4K))
#define VDPU383_RPS_ALIGNED_SIZE            (MPP_ALIGN(VDPU383_RPS_SIZE, SZ_4K))
#define VDPU383_SCALING_LIST_ALIGNED_SIZE   (MPP_ALIGN(VDPU383_SCALING_LIST_SIZE, SZ_4K))
#define VDPU383_STREAM_INFO_SET_SIZE        (VDPU383_SPSPPS_ALIGNED_SIZE + \
                                             VDPU383_RPS_ALIGNED_SIZE + \
                                             VDPU383_SCALING_LIST_ALIGNED_SIZE)

#define VDPU383_CABAC_TAB_OFFSET            (0)
#define VDPU383_ERROR_INFO_OFFSET           (VDPU383_CABAC_TAB_OFFSET + VDPU383_CABAC_TAB_ALIGNED_SIZE)
#define VDPU383_STREAM_INFO_OFFSET_BASE     (VDPU383_ERROR_INFO_OFFSET + VDPU383_ERROR_INFO_ALIGNED_SIZE)
#define VDPU383_SPSPPS_OFFSET(pos)          (VDPU383_STREAM_INFO_OFFSET_BASE + (VDPU383_STREAM_INFO_SET_SIZE * pos))
#define VDPU383_RPS_OFFSET(pos)             (VDPU383_SPSPPS_OFFSET(pos) + VDPU383_SPSPPS_ALIGNED_SIZE)
#define VDPU383_SCALING_LIST_OFFSET(pos)    (VDPU383_RPS_OFFSET(pos) + VDPU383_RPS_ALIGNED_SIZE)
#define VDPU383_INFO_BUFFER_SIZE(cnt)       (VDPU383_STREAM_INFO_OFFSET_BASE + (VDPU383_STREAM_INFO_SET_SIZE * cnt))

#define VDPU383_SPS_PPS_LEN            (MPP_ALIGN(1338, 128) / 8) // byte, 1338 bit

#define SET_REF_INFO(regs, index, field, value)\
    do{ \
        switch(index){\
        case 0: regs.reg99.ref0_##field = value; break;\
        case 1: regs.reg99.ref1_##field = value; break;\
        case 2: regs.reg99.ref2_##field = value; break;\
        case 3: regs.reg99.ref3_##field = value; break;\
        case 4: regs.reg100.ref4_##field = value; break;\
        case 5: regs.reg100.ref5_##field = value; break;\
        case 6: regs.reg100.ref6_##field = value; break;\
        case 7: regs.reg100.ref7_##field = value; break;\
        case 8: regs.reg101.ref8_##field = value; break;\
        case 9: regs.reg101.ref9_##field = value; break;\
        case 10: regs.reg101.ref10_##field = value; break;\
        case 11: regs.reg101.ref11_##field = value; break;\
        case 12: regs.reg102.ref12_##field = value; break;\
        case 13: regs.reg102.ref13_##field = value; break;\
        case 14: regs.reg102.ref14_##field = value; break;\
        case 15: regs.reg102.ref15_##field = value; break;\
        default: break;}\
    }while(0)

#define VDPU383_FAST_REG_SET_CNT    3

typedef struct h264d_rkv_buf_t {
    RK_U32              valid;
    Vdpu383H264dRegSet  *regs;
} H264dRkvBuf_t;

typedef struct Vdpu383H264dRegCtx_t {
    RK_U8               spspps[VDPU383_SPS_PPS_LEN];
    RK_U8               rps[VDPU383_RPS_SIZE];
    RK_U8               sclst[VDPU383_SCALING_LIST_SIZE];

    MppBuffer           bufs;
    RK_S32              bufs_fd;
    void                *bufs_ptr;
    RK_U32              offset_cabac;
    RK_U32              offset_errinfo;
    RK_U32              offset_spspps[VDPU383_FAST_REG_SET_CNT];
    RK_U32              offset_rps[VDPU383_FAST_REG_SET_CNT];
    RK_U32              offset_sclst[VDPU383_FAST_REG_SET_CNT];

    H264dRkvBuf_t       reg_buf[VDPU383_FAST_REG_SET_CNT];

    RK_U32              spspps_offset;
    RK_U32              rps_offset;
    RK_U32              sclst_offset;

    RK_S32              width;
    RK_S32              height;
    /* rcb buffers info */
    RK_U32              bit_depth;
    RK_U32              mbaff;
    RK_U32              chroma_format_idc;

    RK_S32              rcb_buf_size;
    Vdpu383RcbInfo      rcb_info[RCB_BUF_COUNT];
    MppBuffer           rcb_buf[VDPU383_FAST_REG_SET_CNT];

    Vdpu383H264dRegSet  *regs;
    HalBufs             origin_bufs;
} Vdpu383H264dRegCtx;

MPP_RET vdpu383_h264d_deinit(void *hal);
static RK_U32 rkv_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static RK_U32 rkv_len_align(RK_U32 val)
{
    return (2 * MPP_ALIGN(val, 16));
}

static RK_U32 rkv_len_align_422(RK_U32 val)
{
    return ((5 * MPP_ALIGN(val, 16)) / 2);
}

static MPP_RET vdpu383_setup_scale_origin_bufs(H264dHalCtx_t *p_hal, MppFrame mframe)
{
    Vdpu383H264dRegCtx *ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;
    /* for 8K FrameBuf scale mode */
    size_t origin_buf_size = 0;

    origin_buf_size = mpp_frame_get_buf_size(mframe);

    if (!origin_buf_size) {
        mpp_err_f("origin_bufs get buf size failed\n");
        return MPP_NOK;
    }
    if (ctx->origin_bufs) {
        hal_bufs_deinit(ctx->origin_bufs);
        ctx->origin_bufs = NULL;
    }
    hal_bufs_init(&ctx->origin_bufs);
    if (!ctx->origin_bufs) {
        mpp_err_f("origin_bufs init fail\n");
        return MPP_ERR_NOMEM;
    }
    hal_bufs_setup(ctx->origin_bufs, 16, 1, &origin_buf_size);

    return MPP_OK;
}

static MPP_RET prepare_spspps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0;
    RK_S32 is_long_term = 0, voidx = 0;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    RK_U32 tmp = 0;
    BitputCtx_t bp;

    mpp_set_bitput_ctx(&bp, data, len);

    if (!p_hal->fast_mode && !pp->spspps_update) {
        bp.index = 2;
        bp.bitpos = 24;
        bp.bvalue = bp.pbuf[bp.index] & 0xFFFFFF;
    } else {
        RK_U32 pic_width, pic_height;

        //!< sps syntax
        pic_width   = 16 * (pp->wFrameWidthInMbsMinus1 + 1);
        pic_height  = 16 * (pp->wFrameHeightInMbsMinus1 + 1);
        pic_height *= (2 - pp->frame_mbs_only_flag);
        pic_height /= (1 + pp->field_pic_flag);
        mpp_put_bits(&bp, pp->seq_parameter_set_id, 4);
        mpp_put_bits(&bp, pp->profile_idc, 8);
        mpp_put_bits(&bp, pp->constraint_set3_flag, 1);
        mpp_put_bits(&bp, pp->chroma_format_idc, 2);
        mpp_put_bits(&bp, pp->bit_depth_luma_minus8, 3);
        mpp_put_bits(&bp, pp->bit_depth_chroma_minus8, 3);
        mpp_put_bits(&bp, 0, 1); // set 0
        mpp_put_bits(&bp, pp->log2_max_frame_num_minus4, 4);
        mpp_put_bits(&bp, pp->num_ref_frames, 5);
        mpp_put_bits(&bp, pp->pic_order_cnt_type, 2);
        mpp_put_bits(&bp, pp->log2_max_pic_order_cnt_lsb_minus4, 4);
        mpp_put_bits(&bp, pp->delta_pic_order_always_zero_flag, 1);
        mpp_put_bits(&bp, pic_width, 16);
        mpp_put_bits(&bp, pic_height, 16);
        mpp_put_bits(&bp, pp->frame_mbs_only_flag, 1);
        mpp_put_bits(&bp, pp->MbaffFrameFlag, 1);
        mpp_put_bits(&bp, pp->direct_8x8_inference_flag, 1);
        /* multi-view */
        mpp_put_bits(&bp, pp->mvc_extension_enable, 1);
        if (pp->mvc_extension_enable) {
            mpp_put_bits(&bp, (pp->num_views_minus1 + 1), 2);
            mpp_put_bits(&bp, pp->view_id[0], 10);
            mpp_put_bits(&bp, pp->view_id[1], 10);
        } else {
            mpp_put_bits(&bp, 0, 22);
        }
        // hw_fifo_align_bits(&bp, 128);
        //!< pps syntax
        mpp_put_bits(&bp, pp->pps_pic_parameter_set_id, 8);
        mpp_put_bits(&bp, pp->pps_seq_parameter_set_id, 5);
        mpp_put_bits(&bp, pp->entropy_coding_mode_flag, 1);
        mpp_put_bits(&bp, pp->pic_order_present_flag, 1);

        mpp_put_bits(&bp, pp->num_ref_idx_l0_active_minus1, 5);
        mpp_put_bits(&bp, pp->num_ref_idx_l1_active_minus1, 5);
        mpp_put_bits(&bp, pp->weighted_pred_flag, 1);
        mpp_put_bits(&bp, pp->weighted_bipred_idc, 2);
        mpp_put_bits(&bp, pp->pic_init_qp_minus26, 7);
        mpp_put_bits(&bp, pp->pic_init_qs_minus26, 6);
        mpp_put_bits(&bp, pp->chroma_qp_index_offset, 5);
        mpp_put_bits(&bp, pp->deblocking_filter_control_present_flag, 1);
        mpp_put_bits(&bp, pp->constrained_intra_pred_flag, 1);
        mpp_put_bits(&bp, pp->redundant_pic_cnt_present_flag, 1);
        mpp_put_bits(&bp, pp->transform_8x8_mode_flag, 1);
        mpp_put_bits(&bp, pp->second_chroma_qp_index_offset, 5);
        mpp_put_bits(&bp, pp->scaleing_list_enable_flag, 1);
    }

    //!< set dpb
    for (i = 0; i < 16; i++) {
        is_long_term = (pp->RefFrameList[i].bPicEntry != 0xff) ? pp->RefFrameList[i].AssociatedFlag : 0;
        tmp |= (RK_U32)(is_long_term & 0x1) << i;
    }
    for (i = 0; i < 16; i++) {
        voidx = (pp->RefFrameList[i].bPicEntry != 0xff) ? pp->RefPicLayerIdList[i] : 0;
        tmp |= (RK_U32)(voidx & 0x1) << (i + 16);
    }
    mpp_put_bits(&bp, tmp, 32);
    /* set current frame */
    mpp_put_bits(&bp, pp->field_pic_flag, 1);
    mpp_put_bits(&bp, (pp->field_pic_flag && pp->CurrPic.AssociatedFlag), 1);

    mpp_put_bits(&bp, pp->CurrFieldOrderCnt[0], 32);
    mpp_put_bits(&bp, pp->CurrFieldOrderCnt[1], 32);

    /* refer poc */
    for (i = 0; i < 16; i++) {
        mpp_put_bits(&bp, pp->FieldOrderCntList[i][0], 32);
        mpp_put_bits(&bp, pp->FieldOrderCntList[i][1], 32);
    }

    tmp = 0;
    for (i = 0; i < 16; i++) {
        RK_U32 field_flag = (pp->RefPicFiledFlags >> i) & 0x01;

        tmp |= field_flag << i;
    }
    for (i = 0; i < 16; i++) {
        RK_U32 top_used = (pp->UsedForReferenceFlags >> (2 * i + 0)) & 0x01;

        tmp |= top_used << (i + 16);
    }
    mpp_put_bits(&bp, tmp, 32);

    tmp = 0;
    for (i = 0; i < 16; i++) {
        RK_U32 bot_used = (pp->UsedForReferenceFlags >> (2 * i + 1)) & 0x01;

        tmp |= bot_used << i;
    }
    for (i = 0; i < 16; i++) {
        RK_U32 ref_colmv_used = (pp->RefPicColmvUsedFlags >> i) & 0x01;

        tmp |= ref_colmv_used << (i + 16);
    }
    mpp_put_bits(&bp, tmp, 32);
    mpp_put_align(&bp, 64, 0);//128

#ifdef DUMP_VDPU383_DATAS
    {
        char *cur_fname = "global_cfg.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)bp.pbuf, 64 * bp.index + bp.bitpos, 64, 0);
    }
#endif

    return MPP_OK;
}

static MPP_RET prepare_framerps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0, j = 0;
    RK_S32 dpb_idx = 0, voidx = 0;
    RK_S32 dpb_valid = 0, bottom_flag = 0;
    RK_U32 max_frame_num = 0;
    RK_U16 frame_num_wrap = 0;
    RK_U32 tmp = 0;

    BitputCtx_t bp;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;

    mpp_set_bitput_ctx(&bp, data, len);

    max_frame_num = 1 << (pp->log2_max_frame_num_minus4 + 4);
    for (i = 0; i < 16; i++) {
        if ((pp->NonExistingFrameFlags >> i) & 0x01) {
            frame_num_wrap = 0;
        } else {
            if (pp->RefFrameList[i].AssociatedFlag) {
                frame_num_wrap = pp->FrameNumList[i];
            } else {
                frame_num_wrap = (pp->FrameNumList[i] > pp->frame_num) ?
                                 (pp->FrameNumList[i] - max_frame_num) : pp->FrameNumList[i];
            }
        }
        mpp_put_bits(&bp, frame_num_wrap, 16);
    }

    tmp = 0;
    for (i = 0; i < 16; i++) {
        tmp |= (RK_U32)pp->RefPicLayerIdList[i] << (i + 16);
    }
    mpp_put_bits(&bp, tmp, 32);

    for (i = 0; i < 32; i++) {
        tmp = 0;
        dpb_valid = (p_hal->slice_long[0].RefPicList[0][i].bPicEntry == 0xff) ? 0 : 1;
        dpb_idx = dpb_valid ? p_hal->slice_long[0].RefPicList[0][i].Index7Bits : 0;
        bottom_flag = dpb_valid ? p_hal->slice_long[0].RefPicList[0][i].AssociatedFlag : 0;
        voidx = dpb_valid ? pp->RefPicLayerIdList[dpb_idx] : 0;

        tmp |= (RK_U32)(dpb_idx | (dpb_valid << 4)) & 0x1f;
        tmp |= (RK_U32)(bottom_flag & 0x1) << 5;
        if (dpb_valid)
            tmp |= (RK_U32)(voidx & 0x1) << 6;
        mpp_put_bits(&bp, tmp, 7);
    }

    for (j = 1; j < 3; j++) {
        for (i = 0; i < 32; i++) {
            tmp = 0;
            dpb_valid = (p_hal->slice_long[0].RefPicList[j][i].bPicEntry == 0xff) ? 0 : 1;
            dpb_idx = dpb_valid ? p_hal->slice_long[0].RefPicList[j][i].Index7Bits : 0;
            bottom_flag = dpb_valid ? p_hal->slice_long[0].RefPicList[j][i].AssociatedFlag : 0;
            voidx = dpb_valid ? pp->RefPicLayerIdList[dpb_idx] : 0;
            tmp |= (RK_U32)(dpb_idx | (dpb_valid << 4)) & 0x1f;
            tmp |= (RK_U32)(bottom_flag & 0x1) << 5;
            if (dpb_valid)
                tmp |= (RK_U32)(voidx & 0x1) << 6;
            mpp_put_bits(&bp, tmp, 7);
        }
    }

    mpp_put_align(&bp, 128, 0);

#ifdef DUMP_VDPU383_DATAS
    {
        char *cur_fname = "rps.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)bp.pbuf, 64 * bp.index + bp.bitpos, 64, 0);
    }
#endif

    return MPP_OK;
}

static MPP_RET prepare_scanlist(H264dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len)
{
    RK_U32 i = 0, j = 0, n = 0;

    if (!p_hal->pp->scaleing_list_enable_flag)
        return MPP_OK;

    for (i = 0; i < 6; i++) { //4x4, 6 lists
        /* dump by block4x4, vectial direction */
        for (j = 0; j < 4; j++) {
            data[n++] = p_hal->qm->bScalingLists4x4[i][j * 4 + 0];
            data[n++] = p_hal->qm->bScalingLists4x4[i][j * 4 + 1];
            data[n++] = p_hal->qm->bScalingLists4x4[i][j * 4 + 2];
            data[n++] = p_hal->qm->bScalingLists4x4[i][j * 4 + 3];
        }
    }

    for (i = 0; i < 2; i++) { //8x8, 2 lists
        RK_U32 blk4_x = 0, blk4_y = 0;

        /* dump by block4x4, vectial direction */
        for (blk4_y = 0; blk4_y < 8; blk4_y += 4) {
            for (blk4_x = 0; blk4_x < 8; blk4_x += 4) {
                RK_U32 pos = blk4_y * 8 + blk4_x;

                for (j = 0; j < 4; j++) {
                    data[n++] = p_hal->qm->bScalingLists8x8[i][pos + j * 8 + 0];
                    data[n++] = p_hal->qm->bScalingLists8x8[i][pos + j * 8 + 1];
                    data[n++] = p_hal->qm->bScalingLists8x8[i][pos + j * 8 + 2];
                    data[n++] = p_hal->qm->bScalingLists8x8[i][pos + j * 8 + 3];
                }
            }
        }
    }

    mpp_assert(n <= len);

#ifdef DUMP_VDPU383_DATAS
    {
        char *cur_fname = "scanlist.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)data, 8 * n, 128, 0);
    }
#endif

    return MPP_OK;
}

static MPP_RET set_registers(H264dHalCtx_t *p_hal, Vdpu383H264dRegSet *regs, HalTaskInfo *task)
{
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    HalBuf *mv_buf = NULL;
    HalBuf *origin_buf = NULL;
    Vdpu383H264dRegCtx *ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;

    // memset(regs, 0, sizeof(Vdpu383H264dRegSet));
    regs->h264d_paras.reg66_stream_len = p_hal->strm_len;

    //!< caculate the yuv_frame_size
    {
        MppFrame mframe = NULL;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;
        uv_virstride = hor_virstride * ver_virstride / 2;

        if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset;

            fbd_offset = fbc_hdr_stride * MPP_ALIGN(ver_virstride, 64) / 16;

            regs->ctrl_regs.reg9.fbc_e = 1;
            regs->h264d_paras.reg68_hor_virstride = fbc_hdr_stride / 64;
            regs->h264d_addrs.reg193_fbc_payload_offset = fbd_offset;
        } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
            regs->ctrl_regs.reg9.tile_e = 1;
            regs->h264d_paras.reg68_hor_virstride = hor_virstride * 6 / 16;
            regs->h264d_paras.reg70_y_virstride = (y_virstride + uv_virstride) / 16;
        } else {
            regs->ctrl_regs.reg9.fbc_e = 0;
            regs->h264d_paras.reg68_hor_virstride = hor_virstride / 16;
            regs->h264d_paras.reg69_raster_uv_hor_virstride = hor_virstride / 16;
            regs->h264d_paras.reg70_y_virstride = y_virstride / 16;
        }
    }
    //!< set current
    {
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        fd = mpp_buffer_get_fd(mbuffer);
        regs->h264d_addrs.reg168_decout_base = fd;
        regs->h264d_addrs.reg192_payload_st_cur_base = fd;

        //colmv_cur_base
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, pp->CurrPic.Index7Bits);
        regs->h264d_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
        regs->h264d_addrs.reg169_error_ref_base = fd;
    }
    //!< set reference
    {
        RK_S32 i = 0;
        RK_S32 fd = -1;
        RK_S32 ref_index = -1;
        RK_S32 near_index = -1;
        MppBuffer mbuffer = NULL;
        RK_U32 min_frame_num  = 0;
        MppFrame mframe = NULL;

        for (i = 0; i < 15; i++) {
            if (pp->RefFrameList[i].bPicEntry != 0xff) {
                ref_index = pp->RefFrameList[i].Index7Bits;
                near_index = pp->RefFrameList[i].Index7Bits;
            } else {
                ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
            }
            /* mark 3 to differ from current frame */
            mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
            mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_FRAME_PTR, &mframe);
            if (ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                origin_buf = hal_bufs_get_buf(ctx->origin_bufs, ref_index);
                mbuffer = origin_buf->buf[0];
            }

            if (pp->FrameNumList[i] < pp->frame_num &&
                pp->FrameNumList[i] > min_frame_num &&
                (!mpp_frame_get_errinfo(mframe))) {
                min_frame_num = pp->FrameNumList[i];
                regs->h264d_addrs.reg169_error_ref_base = mpp_buffer_get_fd(mbuffer);
            }

            fd = mpp_buffer_get_fd(mbuffer);
            regs->h264d_addrs.reg170_185_ref_base[i] = fd;
            regs->h264d_addrs.reg195_210_payload_st_ref_base[i] = fd;
            mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, ref_index);
            regs->h264d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }

        if (pp->RefFrameList[15].bPicEntry != 0xff) {
            ref_index = pp->RefFrameList[15].Index7Bits;
        } else {
            ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
        }

        mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
        fd = mpp_buffer_get_fd(mbuffer);
        if (mpp_frame_get_thumbnail_en(mframe) == 2) {
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, ref_index);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
        }
        regs->h264d_addrs.reg170_185_ref_base[15] = fd;
        regs->h264d_addrs.reg195_210_payload_st_ref_base[15] = fd;
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, ref_index);
        regs->h264d_addrs.reg217_232_colmv_ref_base[15] = mpp_buffer_get_fd(mv_buf->buf[0]);
    }
    {
        MppBuffer mbuffer = NULL;
        Vdpu383H264dRegCtx *reg_ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;

        mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        regs->common_addr.reg128_strm_base = mpp_buffer_get_fd(mbuffer);
        // regs->h264d_paras.reg65_strm_start_bit = 2 * 8;
#ifdef DUMP_VDPU383_DATAS
        {
            char *cur_fname = "stream_in.dat";
            memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
            sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
            dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer),
                              8 * p_hal->strm_len, 128, 0);
        }
#endif

        regs->common_addr.reg130_cabactbl_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 130, reg_ctx->offset_cabac);
    }

    {
        //scale down config
        MppFrame mframe = NULL;
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        fd = mpp_buffer_get_fd(mbuffer);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            regs->common_addr.reg133_scale_down_base = fd;
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, pp->CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            regs->h264d_addrs.reg168_decout_base = fd;
            regs->h264d_addrs.reg192_payload_st_cur_base = fd;
            regs->h264d_addrs.reg169_error_ref_base = fd;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs, (void*)&regs->h264d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            regs->common_addr.reg133_scale_down_base = fd;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs, (void*)&regs->h264d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

    return MPP_OK;
}

static MPP_RET init_ctrl_regs(Vdpu383H264dRegSet *regs)
{
    Vdpu383CtrlReg *ctrl_regs = &regs->ctrl_regs;

    ctrl_regs->reg8_dec_mode = 1;  //!< h264
    ctrl_regs->reg9.buf_empty_en = 0;

    ctrl_regs->reg10.strmd_auto_gating_e      = 1;
    ctrl_regs->reg10.inter_auto_gating_e      = 1;
    ctrl_regs->reg10.intra_auto_gating_e      = 1;
    ctrl_regs->reg10.transd_auto_gating_e     = 1;
    ctrl_regs->reg10.recon_auto_gating_e      = 1;
    ctrl_regs->reg10.filterd_auto_gating_e    = 1;
    ctrl_regs->reg10.bus_auto_gating_e        = 1;
    ctrl_regs->reg10.ctrl_auto_gating_e       = 1;
    ctrl_regs->reg10.rcb_auto_gating_e        = 1;
    ctrl_regs->reg10.err_prc_auto_gating_e    = 1;

    ctrl_regs->reg13_core_timeout_threshold = 0xffffff;

    ctrl_regs->reg16.error_proc_disable = 1;
    ctrl_regs->reg16.error_spread_disable = 0;
    ctrl_regs->reg16.roi_error_ctu_cal_en = 0;

    ctrl_regs->reg20_cabac_error_en_lowbits = 0xfffedfff;
    ctrl_regs->reg21_cabac_error_en_highbits = 0x0ffbf9ff;

    /* performance */
    ctrl_regs->reg28.axi_perf_work_e = 1;
    ctrl_regs->reg28.axi_cnt_type = 1;
    ctrl_regs->reg28.rd_latency_id = 11;

    ctrl_regs->reg29.addr_align_type = 2;
    ctrl_regs->reg29.ar_cnt_id_type = 0;
    ctrl_regs->reg29.aw_cnt_id_type = 0;
    ctrl_regs->reg29.ar_count_id = 0xa;
    ctrl_regs->reg29.aw_count_id = 0;
    ctrl_regs->reg29.rd_band_width_mode = 0;

    return MPP_OK;
}

MPP_RET vdpu383_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    (void) cfg;

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Vdpu383H264dRegCtx)));
    Vdpu383H264dRegCtx *reg_ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU383_FAST_REG_SET_CNT : 1;
    RK_U32 i = 0;

    //!< malloc buffers
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs,
                                   VDPU383_INFO_BUFFER_SIZE(max_cnt)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    reg_ctx->offset_cabac = VDPU383_CABAC_TAB_OFFSET;
    reg_ctx->offset_errinfo = VDPU383_ERROR_INFO_OFFSET;
    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu383H264dRegSet, 1);
        init_ctrl_regs(reg_ctx->reg_buf[i].regs);
        reg_ctx->offset_spspps[i] = VDPU383_SPSPPS_OFFSET(i);
        reg_ctx->offset_rps[i] = VDPU383_RPS_OFFSET(i);
        reg_ctx->offset_sclst[i] = VDPU383_SCALING_LIST_OFFSET(i);
    }

    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->spspps_offset = reg_ctx->offset_spspps[0];
        reg_ctx->rps_offset = reg_ctx->offset_rps[0];
        reg_ctx->sclst_offset = reg_ctx->offset_sclst[0];
    }

    //!< copy cabac table bytes
    memcpy((char *)reg_ctx->bufs_ptr + reg_ctx->offset_cabac,
           (void *)h264_cabac_table, sizeof(h264_cabac_table));

    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, rkv_ver_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, rkv_len_align);

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu383_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 16;
    }

__RETURN:
    return MPP_OK;
__FAILED:
    vdpu383_h264d_deinit(hal);

    return ret;
}

MPP_RET vdpu383_h264d_deinit(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    Vdpu383H264dRegCtx *reg_ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;

    RK_U32 i = 0;
    RK_U32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;

    if (reg_ctx->bufs) {
        mpp_buffer_put(reg_ctx->bufs);
        reg_ctx->bufs = NULL;
    }

    for (i = 0; i < loop; i++)
        MPP_FREE(reg_ctx->reg_buf[i].regs);

    loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->rcb_buf) : 1;
    for (i = 0; i < loop; i++) {
        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }
    }

    if (p_hal->cmv_bufs) {
        hal_bufs_deinit(p_hal->cmv_bufs);
        p_hal->cmv_bufs = NULL;
    }

    if (reg_ctx->origin_bufs) {
        hal_bufs_deinit(reg_ctx->origin_bufs);
        reg_ctx->origin_bufs = NULL;
    }

    MPP_FREE(p_hal->reg_ctx);

    return MPP_OK;
}

static void h264d_refine_rcb_size(H264dHalCtx_t *p_hal, Vdpu383RcbInfo *rcb_info,
                                  RK_S32 width, RK_S32 height)
{
    RK_U32 rcb_bits = 0;
    RK_U32 mbaff = p_hal->pp->MbaffFrameFlag;
    RK_U32 bit_depth = p_hal->pp->bit_depth_luma_minus8 + 8;
    RK_U32 chroma_format_idc = p_hal->pp->chroma_format_idc;
    RK_U32 row_uv_para = 1; // for yuv420/yuv422
    RK_U32 filterd_row_append = 8192;

    // vdpu383 h264d support yuv400/yuv420/yuv422
    if (chroma_format_idc == 0)
        row_uv_para = 0;

    width = MPP_ALIGN(width, H264_CTU_SIZE);
    height = MPP_ALIGN(height, H264_CTU_SIZE);
    /* RCB_STRMD_ROW && RCB_STRMD_TILE_ROW*/
    if (width > 4096)
        rcb_bits = ((width + 15) / 16) * 154 * (mbaff ? 2 : 1);
    else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_STRMD_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTER_ROW && RCB_INTER_TILE_ROW*/
    rcb_bits = ((width + 3) / 4) * 92 * (mbaff ? 2 : 1);
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTER_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTRA_ROW && RCB_INTRA_TILE_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2) * (mbaff ? 2 : 1);
    if (chroma_format_idc == 1 || chroma_format_idc == 2)
        rcb_bits = rcb_bits * 5 / 2; //TODO:

    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTRA_TILE_ROW].size = 0;
    /* RCB_FILTERD_ROW && RCB_FILTERD_PROTECT_ROW*/
    // save space mode : half for RCB_FILTERD_ROW, half for RCB_FILTERD_PROTECT_ROW
    rcb_bits = width * 17 * ((6 + 3 * row_uv_para) * (mbaff ? 2 : 1) + 2 * row_uv_para + 1.5);
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_info[RCB_FILTERD_ROW].size = MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FILTERD_PROTECT_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);

    rcb_info[RCB_FILTERD_TILE_ROW].size = 0;
    /* RCB_FILTERD_TILE_COL */
    rcb_info[RCB_FILTERD_TILE_COL].size = 0;

}

static void hal_h264d_rcb_info_update(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t*)hal;
    RK_U32 mbaff = p_hal->pp->MbaffFrameFlag;
    RK_U32 bit_depth = p_hal->pp->bit_depth_luma_minus8 + 8;
    RK_U32 chroma_format_idc = p_hal->pp->chroma_format_idc;
    Vdpu383H264dRegCtx *ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;
    RK_S32 width = MPP_ALIGN((p_hal->pp->wFrameWidthInMbsMinus1 + 1) << 4, 64);
    RK_S32 height = MPP_ALIGN((p_hal->pp->wFrameHeightInMbsMinus1 + 1) << 4, 64);

    if ( ctx->bit_depth != bit_depth ||
         ctx->chroma_format_idc != chroma_format_idc ||
         ctx->mbaff != mbaff ||
         ctx->width != width ||
         ctx->height != height) {
        RK_U32 i;
        RK_U32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(ctx->reg_buf) : 1;

        ctx->rcb_buf_size = vdpu383_get_rcb_buf_size(ctx->rcb_info, width, height);
        h264d_refine_rcb_size(hal, ctx->rcb_info, width, height);
        for (i = 0; i < loop; i++) {
            MppBuffer rcb_buf = ctx->rcb_buf[i];

            if (rcb_buf) {
                mpp_buffer_put(rcb_buf);
                ctx->rcb_buf[i] = NULL;
            }
            mpp_buffer_get(p_hal->buf_group, &rcb_buf, ctx->rcb_buf_size);
            ctx->rcb_buf[i] = rcb_buf;
        }
        ctx->bit_depth      = bit_depth;
        ctx->width          = width;
        ctx->height         = height;
        ctx->mbaff          = mbaff;
        ctx->chroma_format_idc = chroma_format_idc;
    }
}

MPP_RET vdpu383_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    RK_S32 width = MPP_ALIGN((p_hal->pp->wFrameWidthInMbsMinus1 + 1) << 4, 64);
    RK_S32 height = MPP_ALIGN((p_hal->pp->wFrameHeightInMbsMinus1 + 1) << 4, 64);
    Vdpu383H264dRegCtx *ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;
    Vdpu383H264dRegSet *regs = ctx->regs;
    MppFrame mframe;
    RK_S32 mv_size = MPP_ALIGN(width, 64) * MPP_ALIGN(height, 16); // 16 byte unit

    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __RETURN;
    }

    /* if is field mode is enabled enlarge colmv buffer and disable colmv compression */
    if (!p_hal->pp->frame_mbs_only_flag)
        mv_size *= 2;

    if (p_hal->cmv_bufs == NULL || p_hal->mv_size < mv_size) {
        size_t size = mv_size;

        if (p_hal->cmv_bufs) {
            hal_bufs_deinit(p_hal->cmv_bufs);
            p_hal->cmv_bufs = NULL;
        }

        hal_bufs_init(&p_hal->cmv_bufs);
        if (p_hal->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            goto __RETURN;
        }
        p_hal->mv_size = mv_size;
        p_hal->mv_count = mpp_buf_slot_get_count(p_hal->frame_slots);
        hal_bufs_setup(p_hal->cmv_bufs, p_hal->mv_count, 1, &size);
    }

    mpp_buf_slot_get_prop(p_hal->frame_slots, p_hal->pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
        ctx->origin_bufs == NULL) {
        vdpu383_setup_scale_origin_bufs(p_hal, mframe);
    }

    if (p_hal->fast_mode) {
        RK_U32 i = 0;
        for (i = 0; i <  MPP_ARRAY_ELEMS(ctx->reg_buf); i++) {
            if (!ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                regs = ctx->reg_buf[i].regs;

                ctx->spspps_offset = ctx->offset_spspps[i];
                ctx->rps_offset = ctx->offset_rps[i];
                ctx->sclst_offset = ctx->offset_sclst[i];
                ctx->reg_buf[i].valid = 1;
                break;
            }
        }
    }

#ifdef DUMP_VDPU383_DATAS
    {
        memset(dump_cur_dir, 0, sizeof(dump_cur_dir));
        sprintf(dump_cur_dir, "avc/Frame%04d", dump_cur_frame);
        if (access(dump_cur_dir, 0)) {
            if (mkdir(dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", dump_cur_dir);
        }
        dump_cur_frame++;
    }
#endif

    prepare_spspps(p_hal, (RK_U64 *)&ctx->spspps, sizeof(ctx->spspps) / 8);
    prepare_framerps(p_hal, (RK_U64 *)&ctx->rps, sizeof(ctx->rps) / 8);
    prepare_scanlist(p_hal, ctx->sclst, sizeof(ctx->sclst));
    set_registers(p_hal, regs, task);

    //!< copy spspps datas
    memcpy((char *)ctx->bufs_ptr + ctx->spspps_offset, (char *)ctx->spspps, sizeof(ctx->spspps));

    regs->common_addr.reg131_gbl_base = ctx->bufs_fd;
    regs->h264d_paras.reg67_global_len = VDPU383_SPS_PPS_LEN / 16; // 128 bit as unit
    mpp_dev_set_reg_offset(p_hal->dev, 131, ctx->spspps_offset);

    memcpy((char *)ctx->bufs_ptr + ctx->rps_offset, (void *)ctx->rps, sizeof(ctx->rps));
    regs->common_addr.reg129_rps_base = ctx->bufs_fd;
    mpp_dev_set_reg_offset(p_hal->dev, 129, ctx->rps_offset);

    if (p_hal->pp->scaleing_list_enable_flag) {
        memcpy((char *)ctx->bufs_ptr + ctx->sclst_offset, (void *)ctx->sclst, sizeof(ctx->sclst));
        regs->common_addr.reg132_scanlist_addr = ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 132, ctx->sclst_offset);
    } else {
        regs->common_addr.reg132_scanlist_addr = 0;
    }

    hal_h264d_rcb_info_update(p_hal);
    vdpu383_setup_rcb(&regs->common_addr, p_hal->dev, p_hal->fast_mode ?
                      ctx->rcb_buf[task->dec.reg_index] : ctx->rcb_buf[0],
                      ctx->rcb_info);
    vdpu383_setup_statistic(&regs->ctrl_regs);
    mpp_buffer_sync_end(ctx->bufs);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __RETURN;
    }

    Vdpu383H264dRegCtx *reg_ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;
    Vdpu383H264dRegSet *regs = p_hal->fast_mode ?
                               reg_ctx->reg_buf[task->dec.reg_index].regs :
                               reg_ctx->regs;
    MppDev dev = p_hal->dev;

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &regs->ctrl_regs;
        wr_cfg.size = sizeof(regs->ctrl_regs);
        wr_cfg.offset = OFFSET_CTRL_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->common_addr;
        wr_cfg.size = sizeof(regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->h264d_paras;
        wr_cfg.size = sizeof(regs->h264d_paras);
        wr_cfg.offset = OFFSET_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->h264d_addrs;
        wr_cfg.size = sizeof(regs->h264d_addrs);
        wr_cfg.offset = OFFSET_CODEC_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(regs->ctrl_regs.reg15);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        /* rcb info for sram */
        vdpu383_set_rcbinfo(dev, (Vdpu383RcbInfo*)reg_ctx->rcb_info);

        /* send request to hardware */
        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    Vdpu383H264dRegCtx *reg_ctx = (Vdpu383H264dRegCtx *)p_hal->reg_ctx;
    Vdpu383H264dRegSet *p_regs = p_hal->fast_mode ?
                                 reg_ctx->reg_buf[task->dec.reg_index].regs :
                                 reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

__SKIP_HARD:
    if (p_hal->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)p_regs;

        if ((!p_regs->ctrl_regs.reg15.rkvdec_frame_rdy_sta) ||
            p_regs->ctrl_regs.reg15.rkvdec_strm_error_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_core_timeout_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_ip_timeout_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_bus_error_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_buffer_empty_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_colmv_ref_error_sta)
            param.hard_err = 1;
        else
            param.hard_err = 0;

        mpp_callback(p_hal->dec_cb, &param);
    }
    memset(&p_regs->ctrl_regs.reg19, 0, sizeof(RK_U32));
    if (p_hal->fast_mode) {
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;
    }

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_h264d_reset(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);


__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_h264d_flush(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_h264d_control(void *hal, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO: {
        MppFrameFormat fmt = mpp_frame_get_fmt((MppFrame)param);
        RK_U32 imgwidth = mpp_frame_get_width((MppFrame)param);
        RK_U32 imgheight = mpp_frame_get_height((MppFrame)param);

        mpp_log("control info: fmt %d, w %d, h %d\n", fmt, imgwidth, imgheight);
        if (fmt == MPP_FMT_YUV422SP) {
            mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, rkv_len_align_422);
        }
        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            vdpu383_afbc_align_calc(p_hal->frame_slots, (MppFrame)param, 16);
        } else if (imgwidth > 1920 || imgheight > 1088) {
            mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
        }
    } break;
    case MPP_DEC_GET_THUMBNAIL_FRAME_INFO: {
        vdpu383_update_thumbnail_frame_info((MppFrame)param);
    } break;
    case MPP_DEC_SET_OUTPUT_FORMAT: {
    } break;
    default : {
    } break;
    }

__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_h264d_vdpu383 = {
    .name     = "h264d_vdpu383",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(Vdpu383H264dRegCtx),
    .flag     = 0,
    .init     = vdpu383_h264d_init,
    .deinit   = vdpu383_h264d_deinit,
    .reg_gen  = vdpu383_h264d_gen_regs,
    .start    = vdpu383_h264d_start,
    .wait     = vdpu383_h264d_wait,
    .reset    = vdpu383_h264d_reset,
    .flush    = vdpu383_h264d_flush,
    .control  = vdpu383_h264d_control,
};
