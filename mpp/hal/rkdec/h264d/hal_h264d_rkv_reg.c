/*
*
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

#define MODULE_TAG "hal_h264d_rkv_reg"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_bitput.h"

#include "hal_h264d_global.h"
#include "hal_h264d_rkv_reg.h"
#include "mpp_dec_cb_param.h"
#include "hal_h264d_com.h"

/* Number registers for the decoder */
#define DEC_RKV_REGISTERS          78

#define RKV_CABAC_TAB_SIZE        (928*4 + 128)       /* bytes */
#define RKV_SPSPPS_SIZE           (256*32 + 128)      /* bytes */
#define RKV_RPS_SIZE              (128 + 128)         /* bytes */
#define RKV_SCALING_LIST_SIZE     (6*16+2*64 + 128)   /* bytes */
#define RKV_ERROR_INFO_SIZE       (256*144*4)         /* bytes */

typedef struct h264d_rkv_buf_t {
    RK_U32 valid;
    MppBuffer spspps;
    MppBuffer rps;
    MppBuffer sclst;
    H264dRkvRegs_t *regs;
} H264dRkvBuf_t;

typedef struct h264d_rkv_reg_ctx_t {
    RK_U8 spspps[32];
    RK_U8 rps[RKV_RPS_SIZE];
    RK_U8 sclst[RKV_SCALING_LIST_SIZE];

    MppBuffer cabac_buf;
    MppBuffer errinfo_buf;
    H264dRkvBuf_t reg_buf[3];

    MppBuffer spspps_buf;
    MppBuffer rps_buf;
    MppBuffer sclst_buf;
    H264dRkvRegs_t *regs;
} H264dRkvRegCtx_t;

MPP_RET rkv_h264d_deinit(void *hal);

static MPP_RET prepare_spspps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0;
    RK_S32 is_long_term = 0, voidx = 0;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;

    BitputCtx_t bp;

    mpp_set_bitput_ctx(&bp, data, len);
    //!< sps syntax
    {
        mpp_put_bits(&bp, -1, 4);   //!< seq_parameter_set_id
        mpp_put_bits(&bp, -1, 8);   //!< profile_idc
        mpp_put_bits(&bp, -1, 1);   //!< constraint_set3_flag
        mpp_put_bits(&bp, pp->chroma_format_idc, 2);
        mpp_put_bits(&bp, pp->bit_depth_luma_minus8, 3);
        mpp_put_bits(&bp, pp->bit_depth_chroma_minus8, 3);
        mpp_put_bits(&bp, 0, 1);   //!< qpprime_y_zero_transform_bypass_flag
        mpp_put_bits(&bp, pp->log2_max_frame_num_minus4, 4);
        mpp_put_bits(&bp, pp->num_ref_frames, 5);
        mpp_put_bits(&bp, pp->pic_order_cnt_type, 2);
        mpp_put_bits(&bp, pp->log2_max_pic_order_cnt_lsb_minus4, 4);
        mpp_put_bits(&bp, pp->delta_pic_order_always_zero_flag, 1);
        mpp_put_bits(&bp, (pp->wFrameWidthInMbsMinus1 + 1), 9);
        mpp_put_bits(&bp, (pp->wFrameHeightInMbsMinus1 + 1), 9);
        mpp_put_bits(&bp, pp->frame_mbs_only_flag, 1);
        mpp_put_bits(&bp, pp->MbaffFrameFlag, 1);
        mpp_put_bits(&bp, pp->direct_8x8_inference_flag, 1);

        mpp_put_bits(&bp, 1, 1);    //!< mvc_extension_enable
        mpp_put_bits(&bp, (pp->num_views_minus1 + 1), 2);
        mpp_put_bits(&bp, pp->view_id[0], 10);
        mpp_put_bits(&bp, pp->view_id[1], 10);
        mpp_put_bits(&bp, pp->num_anchor_refs_l0[0], 1);
        if (pp->num_anchor_refs_l0[0]) {
            mpp_put_bits(&bp, pp->anchor_ref_l0[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10);
        }
        mpp_put_bits(&bp, pp->num_anchor_refs_l1[0], 1);
        if (pp->num_anchor_refs_l1[0]) {
            mpp_put_bits(&bp, pp->anchor_ref_l1[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10); //!< anchor_ref_l1
        }
        mpp_put_bits(&bp, pp->num_non_anchor_refs_l0[0], 1);
        if (pp->num_non_anchor_refs_l0[0]) {
            mpp_put_bits(&bp, pp->non_anchor_ref_l0[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10); //!< non_anchor_ref_l0
        }
        mpp_put_bits(&bp, pp->num_non_anchor_refs_l1[0], 1);
        if (pp->num_non_anchor_refs_l1[0]) {
            mpp_put_bits(&bp, pp->non_anchor_ref_l1[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10);//!< non_anchor_ref_l1
        }
        mpp_put_align(&bp, 32, 0);
    }
    //!< pps syntax
    {
        H264dRkvRegCtx_t *reg_ctx = (H264dRkvRegCtx_t *)p_hal->reg_ctx;

        mpp_put_bits(&bp, -1, 8); //!< pps_pic_parameter_set_id
        mpp_put_bits(&bp, -1, 5); //!< pps_seq_parameter_set_id
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
        mpp_put_bits(&bp, mpp_buffer_get_fd(reg_ctx->sclst_buf), 32);
    }
    //!< set dpb
    for (i = 0; i < 16; i++) {
        is_long_term = (pp->RefFrameList[i].bPicEntry != 0xff) ? pp->RefFrameList[i].AssociatedFlag : 0;
        mpp_put_bits(&bp, is_long_term, 1);
    }
    for (i = 0; i < 16; i++) {
        voidx = (pp->RefFrameList[i].bPicEntry != 0xff) ? pp->RefPicLayerIdList[i] : 0;
        mpp_put_bits(&bp, voidx, 1);
    }
    mpp_put_align(&bp, 64, 0);

    return MPP_OK;
}

static MPP_RET prepare_framerps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0, j = 0;
    RK_S32 dpb_idx = 0, voidx = 0;
    RK_S32 dpb_valid = 0, bottom_flag = 0;
    RK_U32 max_frame_num = 0;
    RK_U16 frame_num_wrap = 0;

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
    for (i = 0; i < 16; i++) {
        mpp_put_bits(&bp, 0, 1);//!< NULL
    }
    for (i = 0; i < 16; i++) {
        mpp_put_bits(&bp, pp->RefPicLayerIdList[i], 1); //!< voidx
    }
    for (i = 0; i < 32; i++) {
        dpb_valid = (p_hal->slice_long[0].RefPicList[0][i].bPicEntry == 0xff) ? 0 : 1;
        dpb_idx = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[0][i].Index7Bits : 0;
        bottom_flag = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[0][i].AssociatedFlag : 0;
        voidx = (dpb_valid != 0) ? pp->RefPicLayerIdList[dpb_idx] : 0;
        mpp_put_bits(&bp, dpb_idx | (dpb_valid << 4), 5); //!< dpb_idx
        mpp_put_bits(&bp, bottom_flag, 1);
        mpp_put_bits(&bp, voidx, 1);
    }
    for (j = 1; j < 3; j++) {
        for (i = 0; i < 32; i++) {
            dpb_valid = (p_hal->slice_long[0].RefPicList[j][i].bPicEntry == 0xff) ? 0 : 1;
            dpb_idx = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[j][i].Index7Bits : 0;
            bottom_flag = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[j][i].AssociatedFlag : 0;
            voidx = (dpb_valid != 0) ? pp->RefPicLayerIdList[dpb_idx] : 0;
            mpp_put_bits(&bp, dpb_idx | (dpb_valid << 4), 5); //!< dpb_idx
            mpp_put_bits(&bp, bottom_flag, 1);
            mpp_put_bits(&bp, voidx, 1);
        }
    }
    mpp_put_align(&bp, 128, 0);

    return MPP_OK;
}

static MPP_RET prepare_scanlist(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0, j = 0;

    if (p_hal->pp->scaleing_list_enable_flag) {
        BitputCtx_t bp;
        mpp_set_bitput_ctx(&bp, data, len);

        for (i = 0; i < 6; i++) { //!< 4x4, 6 lists
            for (j = 0; j < 16; j++) {
                mpp_put_bits(&bp, p_hal->qm->bScalingLists4x4[i][j], 8);
            }

        }
        for (i = 0; i < 2; i++) { //!< 8x8, 2 lists
            for (j = 0; j < 64; j++) {
                mpp_put_bits(&bp, p_hal->qm->bScalingLists8x8[i][j], 8);
            }
        }
    }
    return MPP_OK;
}

static MPP_RET set_registers(H264dHalCtx_t *p_hal, H264dRkvRegs_t *p_regs, HalTaskInfo *task)
{
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    MppHalCfg *cfg = p_hal->cfg;

    memset(p_regs, 0, sizeof(H264dRkvRegs_t));
    //!< set dec_mode && rlc_mode && rps_mode && slice_num
    {
        p_regs->sw02.dec_mode = 1;  //!< h264
        if (p_regs->sw02.rlc_mode == 1) {
            p_regs->sw05.stream_len = 0;
        } else {
            p_regs->sw05.stream_len = p_hal->strm_len;
        }
        if (p_regs->sw02.rps_mode) { // rps_mode == 1
            p_regs->sw43.rps_base += 0x8;
        }
        p_regs->sw03.slice_num_lowbits = 0x7ff;
        p_regs->sw03.slice_num_highbit = 1;
    }
    //!< caculate the yuv_frame_size
    {
        MppFrame mframe = NULL;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 yuv_virstride = 0;

        mpp_buf_slot_get_prop(cfg->frame_slots, pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;

        if (pp->chroma_format_idc == 0) { //!< Y400
            yuv_virstride = y_virstride;
        } else if (pp->chroma_format_idc == 1) { //!< Y420
            yuv_virstride = y_virstride + y_virstride / 2;
        } else if (pp->chroma_format_idc == 2) { //!< Y422
            yuv_virstride = 2 * y_virstride;
        }
        p_regs->sw03.y_hor_virstride = hor_virstride / 16;
        p_regs->sw03.uv_hor_virstride = hor_virstride / 16;
        p_regs->sw08.y_virstride = y_virstride / 16;
        p_regs->sw09.yuv_virstride = yuv_virstride / 16;
    }
    //!< set current
    {
        MppBuffer mbuffer = NULL;
        p_regs->sw40.cur_poc = pp->CurrFieldOrderCnt[0];
        p_regs->sw74.cur_poc1 = pp->CurrFieldOrderCnt[1];
        mpp_buf_slot_get_prop(cfg->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        p_regs->sw07.decout_base = mpp_buffer_get_fd(mbuffer);
    }
    //!< set reference
    {
        RK_S32 i = 0;
        RK_S32 ref_index = -1;
        RK_S32 near_index = -1;
        RK_U32 sw10_24_offset = 0;
        RK_U32 sw48_offset = 0;
        MppBuffer mbuffer = NULL;

        for (i = 0; i < 15; i++) {
            p_regs->sw25_39[i].ref0_14_poc = ((i & 1) != 0)
                                             ? pp->FieldOrderCntList[i / 2][1] : pp->FieldOrderCntList[i / 2][0];
            p_regs->sw49_63[i].ref15_29_poc = ((i & 1) != 0)
                                              ? pp->FieldOrderCntList[(i + 15) / 2][0] : pp->FieldOrderCntList[(i + 15) / 2][1];
            sw10_24_offset = ((pp->RefPicFiledFlags >> i) & 0x01) |
                             ((pp->UsedForReferenceFlags >> (2 * i + 0)) & 0x01) << 0x01 |
                             ((pp->UsedForReferenceFlags >> (2 * i + 1)) & 0x01) << 0x02 |
                             ((pp->RefPicColmvUsedFlags >> i) & 0x01) << 0x03;
            mpp_dev_set_reg_offset(cfg->dev, 10 + i, sw10_24_offset);

            if (pp->RefFrameList[i].bPicEntry != 0xff) {
                ref_index = pp->RefFrameList[i].Index7Bits;
                near_index = pp->RefFrameList[i].Index7Bits;
            } else {
                ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
            }
            mpp_buf_slot_get_prop(cfg->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
            p_regs->sw10_24[i].ref0_14_base = mpp_buffer_get_fd(mbuffer);
        }
        p_regs->sw72.ref30_poc = pp->FieldOrderCntList[15][0];
        p_regs->sw73.ref31_poc = pp->FieldOrderCntList[15][1];
        sw48_offset = ((pp->RefPicFiledFlags >> 15) & 0x01) |
                      ((pp->UsedForReferenceFlags >> 30) & 0x01) << 0x01 |
                      ((pp->UsedForReferenceFlags >> 31) & 0x01) << 0x02 |
                      ((pp->RefPicColmvUsedFlags >> 15) & 0x01) << 0x03;
        mpp_dev_set_reg_offset(cfg->dev, 48, sw48_offset);

        if (pp->RefFrameList[15].bPicEntry != 0xff) {
            ref_index = pp->RefFrameList[15].Index7Bits;
        } else {
            ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
        }
        mpp_buf_slot_get_prop(cfg->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
        p_regs->sw48.ref15_base = mpp_buffer_get_fd(mbuffer);
    }
    {
        MppBuffer mbuffer = NULL;
        H264dRkvRegCtx_t *reg_ctx = (H264dRkvRegCtx_t *)p_hal->reg_ctx;
        mpp_buf_slot_get_prop(cfg->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        p_regs->sw04.strm_rlc_base = mpp_buffer_get_fd(mbuffer);
        p_regs->sw06.cabactbl_base = mpp_buffer_get_fd(reg_ctx->cabac_buf);
        p_regs->sw41.rlcwrite_base = p_regs->sw04.strm_rlc_base;
    }
    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET rkv_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    MppBufferGroup group = cfg->buf_group;

    mpp_env_get_u32("hal_h264d_debug", &hal_h264d_debug, 0);

    INP_CHECK(ret, NULL == p_hal);
    p_hal->cfg = cfg;

    cfg->support_fast_mode = 1;
    p_hal->fast_mode = cfg->cfg->base.fast_parse && cfg->support_fast_mode;

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(H264dRkvRegCtx_t)));
    H264dRkvRegCtx_t *reg_ctx = (H264dRkvRegCtx_t *)p_hal->reg_ctx;
    //!< malloc buffers
    FUN_CHECK(ret = mpp_buffer_get(group, &reg_ctx->cabac_buf, RKV_CABAC_TAB_SIZE));
    FUN_CHECK(ret = mpp_buffer_get(group, &reg_ctx->errinfo_buf, RKV_ERROR_INFO_SIZE));
    // malloc buffers
    RK_U32 i = 0;
    RK_U32 loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    for (i = 0; i < loop; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(H264dRkvRegs_t, 1);
        FUN_CHECK(ret = mpp_buffer_get(group, &reg_ctx->reg_buf[i].spspps, RKV_SPSPPS_SIZE));
        FUN_CHECK(ret = mpp_buffer_get(group, &reg_ctx->reg_buf[i].rps, RKV_RPS_SIZE));
        FUN_CHECK(ret = mpp_buffer_get(group, &reg_ctx->reg_buf[i].sclst, RKV_SCALING_LIST_SIZE));
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->spspps_buf = reg_ctx->reg_buf[0].spspps;
        reg_ctx->rps_buf = reg_ctx->reg_buf[0].rps;
        reg_ctx->sclst_buf = reg_ctx->reg_buf[0].sclst;
    }

    //!< copy cabac table bytes
    FUN_CHECK(ret = mpp_buffer_write(reg_ctx->cabac_buf, 0,
                                     (void *)rkv_cabac_table, sizeof(rkv_cabac_table)));
    mpp_buffer_sync_end(reg_ctx->cabac_buf);

    mpp_slots_set_prop(cfg->frame_slots, SLOTS_HOR_ALIGN, mpp_align_16);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_VER_ALIGN, mpp_align_16);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);

__RETURN:
    return MPP_OK;
__FAILED:
    rkv_h264d_deinit(hal);

    return ret;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET rkv_h264d_deinit(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    H264dRkvRegCtx_t *reg_ctx = (H264dRkvRegCtx_t *)p_hal->reg_ctx;

    RK_U32 i = 0;
    RK_U32 loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    for (i = 0; i < loop; i++) {
        MPP_FREE(reg_ctx->reg_buf[i].regs);
        mpp_buffer_put(reg_ctx->reg_buf[i].spspps);
        mpp_buffer_put(reg_ctx->reg_buf[i].rps);
        mpp_buffer_put(reg_ctx->reg_buf[i].sclst);
    }
    mpp_buffer_put(reg_ctx->cabac_buf);
    mpp_buffer_put(reg_ctx->errinfo_buf);
    MPP_FREE(p_hal->reg_ctx);

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET rkv_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->cfg->base.disable_error)) {
        goto __RETURN;
    }
    H264dRkvRegCtx_t *reg_ctx = (H264dRkvRegCtx_t *)p_hal->reg_ctx;
    if (p_hal->fast_mode) {
        RK_U32 i = 0;
        for (i = 0; i <  MPP_ARRAY_ELEMS(reg_ctx->reg_buf); i++) {
            if (!reg_ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                reg_ctx->spspps_buf = reg_ctx->reg_buf[i].spspps;
                reg_ctx->rps_buf = reg_ctx->reg_buf[i].rps;
                reg_ctx->sclst_buf = reg_ctx->reg_buf[i].sclst;
                reg_ctx->regs = reg_ctx->reg_buf[i].regs;
                reg_ctx->reg_buf[i].valid = 1;
                break;
            }
        }
    }

    prepare_spspps(p_hal, (RK_U64 *)reg_ctx->spspps, sizeof(reg_ctx->spspps));
    prepare_framerps(p_hal, (RK_U64 *)reg_ctx->rps, sizeof(reg_ctx->rps));
    prepare_scanlist(p_hal, (RK_U64 *)reg_ctx->sclst, sizeof(reg_ctx->sclst));
    set_registers(p_hal, reg_ctx->regs, task);

    //!< copy datas
    RK_U32 i = 0;
    for (i = 0; i < 256; i++) {
        mpp_buffer_write(reg_ctx->spspps_buf,
                         (sizeof(reg_ctx->spspps) * i), (void *)reg_ctx->spspps,
                         sizeof(reg_ctx->spspps));
    }
    reg_ctx->regs->sw42.pps_base = mpp_buffer_get_fd(reg_ctx->spspps_buf);

    mpp_buffer_write(reg_ctx->rps_buf, 0,
                     (void *)reg_ctx->rps, sizeof(reg_ctx->rps));
    reg_ctx->regs->sw43.rps_base = mpp_buffer_get_fd(reg_ctx->rps_buf);

    mpp_buffer_write(reg_ctx->sclst_buf, 0,
                     (void *)reg_ctx->sclst, sizeof(reg_ctx->sclst));
    reg_ctx->regs->sw75.errorinfo_base = mpp_buffer_get_fd(reg_ctx->errinfo_buf);
    mpp_buffer_sync_end(reg_ctx->spspps_buf);
    mpp_buffer_sync_end(reg_ctx->rps_buf);
    mpp_buffer_sync_end(reg_ctx->sclst_buf);

__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET rkv_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->cfg->base.disable_error)) {
        goto __RETURN;
    }

    H264dRkvRegCtx_t *reg_ctx = (H264dRkvRegCtx_t *)p_hal->reg_ctx;
    RK_U32 *p_regs = (p_hal->fast_mode != 0) ?
                     (RK_U32 *)reg_ctx->reg_buf[task->dec.reg_index].regs :
                     (RK_U32 *)reg_ctx->regs;

    p_regs[64] = 0;
    p_regs[65] = 0;
    p_regs[66] = 0;
    p_regs[67] = 0x000000ff;   // disable fpga reset
    p_regs[44] = 0xfffffff7;   // 0xffff_ffff, debug enable
    p_regs[77] = 0xffffffff;   // 0xffff_dfff, debug enable

    p_regs[1] |= 0x00000061;   // run hardware, enable buf_empty_en

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = DEC_RKV_REGISTERS * sizeof(RK_U32);
        MppDev dev = p_hal->cfg->dev;

        wr_cfg.reg = p_regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = p_regs;
        rd_cfg.size = reg_size;
        rd_cfg.offset = 0;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    (void)task;
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET rkv_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    H264dRkvRegCtx_t *reg_ctx = (H264dRkvRegCtx_t *)p_hal->reg_ctx;
    H264dRkvRegs_t *p_regs = (p_hal->fast_mode != 0) ?
                             reg_ctx->reg_buf[task->dec.reg_index].regs :
                             reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->cfg->base.disable_error)) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->cfg->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

__SKIP_HARD:
    if (p_hal->cfg->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)p_regs;

        if (p_regs->sw01.dec_error_sta
            || (!p_regs->sw01.dec_rdy_sta)
            || p_regs->sw01.dec_empty_sta
            || p_regs->sw45.strmd_error_status
            || p_regs->sw45.colmv_error_ref_picidx
            || p_regs->sw76.strmd_detect_error_flag)
            param.hard_err = 1;
        else
            param.hard_err = 0;

        mpp_callback(p_hal->cfg->dec_cb, &param);
    }
    memset(&p_regs->sw01, 0, sizeof(RK_U32));
    if (p_hal->fast_mode) {
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;
    }

    (void)task;
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET rkv_h264d_control(void *hal, MpiCmd cmd_type, void *param)
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
            mpp_slots_set_prop(p_hal->cfg->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);
        }
        if (imgwidth > 1920 || imgheight > 1088) {
            mpp_slots_set_prop(p_hal->cfg->frame_slots, SLOTS_HOR_ALIGN, mpp_align_256_odd);
        }
        break;
    }

    default:
        break;
    }

__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_h264d_rkvdpu = {
    .name     = "h264d_rkvdpu",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(H264dHalCtx_t),
    .flag     = 0,
    .init     = rkv_h264d_init,
    .deinit   = rkv_h264d_deinit,
    .reg_gen  = rkv_h264d_gen_regs,
    .start    = rkv_h264d_start,
    .wait     = rkv_h264d_wait,
    .reset    = vdpu_h264d_reset,
    .flush    = vdpu_h264d_flush,
    .control  = rkv_h264d_control,
    .client   = VPU_CLIENT_RKVDEC,
    .soc_type = {
        ROCKCHIP_SOC_RK3128H,
        ROCKCHIP_SOC_RK3399,
        ROCKCHIP_SOC_RK3328,
        ROCKCHIP_SOC_RK3228,
        ROCKCHIP_SOC_RK3228H,
        ROCKCHIP_SOC_RK3229,
        ROCKCHIP_SOC_RV1108,
        ROCKCHIP_SOC_RV1109,
        ROCKCHIP_SOC_RV1126,
        ROCKCHIP_SOC_BUTT
    },
};

MPP_DEC_HAL_API_REGISTER(hal_h264d_rkvdpu)
