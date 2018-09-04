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

#define MODULE_TAG "hal_vp8d_vdpu1"
#include <string.h>

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_mem.h"

#include "mpp_device.h"
#include "hal_vp8d_vdpu1.h"
#include "hal_vp8d_vdpu1_reg.h"

static RK_U32 vp8h_debug;

#define CLIP3(l, h, v) ((v) < (l) ? (l) : ((v) > (h) ? (h) : (v)))

static const RK_U32 mcFilter[8][6] = {
    { 0,  0,  128,    0,   0,  0 },
    { 0, -6,  123,   12,  -1,  0 },
    { 2, -11, 108,   36,  -8,  1 },
    { 0, -9,   93,   50,  -6,  0 },
    { 3, -16,  77,   77, -16,  3 },
    { 0, -6,   50,   93,  -9,  0 },
    { 1, -8,   36,  108, -11,  2 },
    { 0, -1,   12,  123,  -6,  0 }
};

MPP_RET hal_vp8d_vdpu1_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    VP8DHalContext_t *ctx = (VP8DHalContext_t *)hal;

    FUN_T("FUN_IN");
    //configure
    ctx->packet_slots = cfg->packet_slots;
    ctx->frame_slots = cfg->frame_slots;

    mpp_env_get_u32("vp8h_debug", &vp8h_debug, 0);

    //get vpu socket
    MppDevCfg dev_cfg = {
        .type = MPP_CTX_DEC,              /* type */
        .coding = MPP_VIDEO_CodingVP8,    /* coding */
        .platform = 0,                    /* platform */
        .pp_enable = 0,                   /* pp_enable */
    };

    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err("mpp_device_init failed, ret: %d\n", ret);
        FUN_T("FUN_OUT");
        return ret;
    }

    if (NULL == ctx->regs) {
        ctx->regs = mpp_calloc_size(void, sizeof(VP8DRegSet_t));
        if (NULL == ctx->regs) {
            mpp_err("hal_vp8 reg alloc failed\n");
            FUN_T("FUN_OUT");
            return MPP_ERR_NOMEM;
        }
    }

    if (NULL == ctx->group) {
        ret = mpp_buffer_group_get_internal(&ctx->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("hal_vp8 mpp_buffer_group_get failed\n");
            FUN_T("FUN_OUT");
            return ret;
        }
    }

    ret = mpp_buffer_get(ctx->group, &ctx->probe_table, VP8D_PROB_TABLE_SIZE);
    if (ret) {
        mpp_err("hal_vp8 probe_table get buffer failed\n");
        FUN_T("FUN_OUT");
        return ret;
    }

    ret = mpp_buffer_get(ctx->group, &ctx->seg_map, VP8D_MAX_SEGMAP_SIZE);

    if (ret) {
        mpp_err("hal_vp8 seg_map get buffer failed\n");
        FUN_T("FUN_OUT");
        return ret;
    }

    return ret;
}

MPP_RET hal_vp8d_vdpu1_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    VP8DHalContext_t *ctx = (VP8DHalContext_t *)hal;

    FUN_T("FUN_IN");
    if (ctx->dev_ctx) {
        ret = mpp_device_deinit(ctx->dev_ctx);
        if (ret) {
            mpp_err("mpp_device_deinit failed. ret: %d\n", ret);
        }
    }

    if (ctx->probe_table) {
        ret = mpp_buffer_put(ctx->probe_table);
        if (ret) {
            mpp_err("hal_vp8 probe table put buffer failed\n");
        }
    }

    if (ctx->seg_map) {
        ret = mpp_buffer_put(ctx->seg_map);
        if (ret) {
            mpp_err("hal_vp8 seg map put buffer failed\n");
        }
    }

    if (ctx->group) {
        ret = mpp_buffer_group_put(ctx->group);
        if (ret) {
            mpp_err("hal_vp8 group free buffer failed\n");
        }
    }

    if (ctx->regs) {
        mpp_free(ctx->regs);
        ctx->regs = NULL;
    }

    FUN_T("FUN_OUT");
    return ret;
}

static MPP_RET hal_vp8_init_hwcfg(VP8DHalContext_t *ctx)
{

    VP8DRegSet_t *reg = (VP8DRegSet_t *)ctx->regs;

    FUN_T("FUN_IN");
    memset(reg, 0, sizeof(VP8DRegSet_t));

    reg->reg1_interrupt.sw_dec_e = 1;

    reg->reg2_dec_ctrl.sw_dec_out_tiled_e = 0;
    reg->reg2_dec_ctrl.sw_dec_scmd_dis    = 0;
    reg->reg2_dec_ctrl.sw_dec_adv_pre_dis = 0;
    reg->reg2_dec_ctrl.sw_dec_latency     = 0;

    reg->reg2_dec_ctrl.sw_dec_in_endian     = 1;
    reg->reg2_dec_ctrl.sw_dec_out_endian    = 1;
    reg->reg2_dec_ctrl.sw_dec_inswap32_e    = 1;
    reg->reg2_dec_ctrl.sw_dec_outswap32_e   = 1;
    reg->reg2_dec_ctrl.sw_dec_strswap32_e   = 1;
    reg->reg2_dec_ctrl.sw_dec_strendian_e   = 1;
    reg->reg2_dec_ctrl.sw_dec_axi_rn_id     = 0;

    reg->reg2_dec_ctrl.sw_dec_data_disc_e   = 0;
    reg->reg2_dec_ctrl.sw_dec_max_burst     = 16;
    reg->reg2_dec_ctrl.sw_dec_timeout_e  = 1;
    reg->reg2_dec_ctrl.sw_dec_clk_gate_e = 1;

    reg->reg3.sw_dec_out_dis = 0;
    reg->reg3.sw_dec_axi_wr_id = 0;
    reg->reg3.sw_dec_mode = DEC_MODE_VP8;

    reg->reg1_interrupt.sw_dec_irq = 0;

    reg->reg10_segment_map_base = mpp_buffer_get_fd(ctx->seg_map);
    reg->reg40_qtable_base = mpp_buffer_get_fd(ctx->probe_table);

    FUN_T("FUN_OUT");
    return MPP_OK;
}

static MPP_RET hal_vp8d_pre_filter_tap_set(VP8DHalContext_t *ctx)
{
    VP8DRegSet_t *regs = (VP8DRegSet_t *)ctx->regs;

    FUN_T("FUN_IN");
    regs->reg49.sw_pred_bc_tap_0_0 = mcFilter[0][1];
    regs->reg49.sw_pred_bc_tap_0_1 = mcFilter[0][2];
    regs->reg49.sw_pred_bc_tap_0_2 = mcFilter[0][3];
    regs->reg34.sw_pred_bc_tap_0_3 = mcFilter[0][4];
    regs->reg34.sw_pred_bc_tap_1_0 = mcFilter[1][1];
    regs->reg34.sw_pred_bc_tap_1_1 = mcFilter[1][2];
    regs->reg35.sw_pred_bc_tap_1_2 = mcFilter[1][3];
    regs->reg35.sw_pred_bc_tap_1_3 = mcFilter[1][4];
    regs->reg35.sw_pred_bc_tap_2_0 = mcFilter[2][1];
    regs->reg36.sw_pred_bc_tap_2_1 = mcFilter[2][2];
    regs->reg36.sw_pred_bc_tap_2_2 = mcFilter[2][3];
    regs->reg36.sw_pred_bc_tap_2_3 = mcFilter[2][4];

    regs->reg37.sw_pred_bc_tap_3_0 = mcFilter[3][1];
    regs->reg37.sw_pred_bc_tap_3_1 = mcFilter[3][2];
    regs->reg37.sw_pred_bc_tap_3_2 = mcFilter[3][3];
    regs->reg38.sw_pred_bc_tap_3_3 = mcFilter[3][4];
    regs->reg38.sw_pred_bc_tap_4_0 = mcFilter[4][1];
    regs->reg38.sw_pred_bc_tap_4_1 = mcFilter[4][2];
    regs->reg39.sw_pred_bc_tap_4_2 = mcFilter[4][3];
    regs->reg39.sw_pred_bc_tap_4_3 = mcFilter[4][4];
    regs->reg39.sw_pred_bc_tap_5_0 = mcFilter[5][1];

    regs->reg42.sw_pred_bc_tap_5_1 = mcFilter[5][2];
    regs->reg42.sw_pred_bc_tap_5_2 = mcFilter[5][3];
    regs->reg42.sw_pred_bc_tap_5_3 = mcFilter[5][4];

    regs->reg43.sw_pred_bc_tap_6_0 = mcFilter[6][1];
    regs->reg43.sw_pred_bc_tap_6_1 = mcFilter[6][2];
    regs->reg43.sw_pred_bc_tap_6_2 = mcFilter[6][3];

    regs->reg44.sw_pred_bc_tap_6_3 = mcFilter[6][4];
    regs->reg44.sw_pred_bc_tap_7_0 = mcFilter[7][1];
    regs->reg44.sw_pred_bc_tap_7_1 = mcFilter[7][2];

    regs->reg45.sw_pred_bc_tap_7_2 = mcFilter[7][3];
    regs->reg45.sw_pred_bc_tap_7_3 = mcFilter[7][4];

    regs->reg45.sw_pred_tap_2_M1 = mcFilter[2][0];
    regs->reg45.sw_pred_tap_2_4  = mcFilter[2][5];
    regs->reg45.sw_pred_tap_4_M1 = mcFilter[4][0];
    regs->reg45.sw_pred_tap_4_4  = mcFilter[4][5];
    regs->reg45.sw_pred_tap_6_M1 = mcFilter[6][0];
    regs->reg45.sw_pred_tap_6_4  = mcFilter[6][5];

    FUN_T("FUN_OUT");
    return MPP_OK;
}

static MPP_RET
hal_vp8d_dct_partition_cfg(VP8DHalContext_t *ctx, HalTaskInfo *task)
{
    RK_U32 i = 0, len = 0, len1 = 0;
    RK_U32 extraBytesPacked = 0;
    RK_U32 addr = 0, byte_offset = 0;
    RK_U32 fd = 0;
    MppBuffer streambuf = NULL;
    VP8DRegSet_t *regs = (VP8DRegSet_t *)ctx->regs;
    DXVA_PicParams_VP8 *pic_param = (DXVA_PicParams_VP8 *)task->dec.syntax.data;


    FUN_T("FUN_IN");

    mpp_buf_slot_get_prop(ctx->packet_slots, task->dec.input,
                          SLOT_BUFFER, &streambuf);
    fd =  mpp_buffer_get_fd(streambuf);
    regs->reg27_bitpl_ctrl_base = fd;
    regs->reg27_bitpl_ctrl_base |= (pic_param->stream_start_offset << 10);
    regs->reg5.sw_strm1_start_bit = pic_param->stream_start_bit;

    /* calculate dct partition length here instead */
    if (pic_param->decMode == VP8HWD_VP8 && !pic_param->frame_type)
        extraBytesPacked += 7;

    len = pic_param->streamEndPos + pic_param->frameTagSize
          - pic_param->dctPartitionOffsets[0];
    len += ((1 << pic_param->log2_nbr_of_dct_partitions) - 1) * 3;
    len1 = extraBytesPacked + pic_param->dctPartitionOffsets[0];
    len += (len1 & 0x7);
    regs->reg6.sw_stream_len = len;

    len = pic_param->offsetToDctParts + pic_param->frameTagSize -
          (pic_param->stream_start_offset - extraBytesPacked);
    len++;

    regs->reg9.sw_stream1_len = len;
    regs->reg9.sw_coeffs_part_am = (1 << pic_param->log2_nbr_of_dct_partitions);
    regs->reg9.sw_coeffs_part_am--;
    for (i = 0; i < (RK_U32)(1 << pic_param->log2_nbr_of_dct_partitions); i++) {
        addr = extraBytesPacked + pic_param->dctPartitionOffsets[i];
        byte_offset = addr & 0x7;
        addr = addr & 0xFFFFFFF8;

        if (i == 0) {
            regs->reg12_input_stream_base = fd | (addr << 10);
        } else if (i <= 5) {
            regs->reg_dct_strm0_base[i - 1] = fd | (addr << 10);
        } else {
            regs->reg_dct_strm1_base[i - 6] = fd | (addr << 10);
        }

        switch (i) {
        case 0:
            regs->reg5.sw_strm0_start_bit = byte_offset * 8;
            break;
        case 1:
            regs->reg7.sw_dct1_start_bit = byte_offset * 8;
            break;
        case 2:
            regs->reg7.sw_dct2_start_bit = byte_offset * 8;
            break;
        case 3:
            regs->reg11.sw_dct_start_bit_3 = byte_offset * 8;
            break;
        case 4:
            regs->reg11.sw_dct_start_bit_4 = byte_offset * 8;
            break;
        case 5:
            regs->reg11.sw_dct_start_bit_5 = byte_offset * 8;
            break;
        case 6:
            regs->reg11.sw_dct_start_bit_6 = byte_offset * 8;
            break;
        case 7:
            regs->reg11.sw_dct_start_bit_6 = byte_offset * 8;
            break;
        default:
            break;
        }
    }

    FUN_T("FUN_OUT");
    return MPP_OK;
}

static void hal_vp8hw_asic_probe_update(DXVA_PicParams_VP8 *p, RK_U8 *probTbl)
{
    RK_U8   *dst;
    RK_U32  i, j, k;

    FUN_T("FUN_IN");
    /* first probs */
    dst = probTbl;

    dst[0] = p->probe_skip_false;
    dst[1] = p->prob_intra;
    dst[2] = p->prob_last;
    dst[3] = p->prob_golden;
    dst[4] = p->stVP8Segments.mb_segment_tree_probs[0];
    dst[5] = p->stVP8Segments.mb_segment_tree_probs[1];
    dst[6] = p->stVP8Segments.mb_segment_tree_probs[2];
    dst[7] = 0; /*unused*/

    dst += 8;
    dst[0] = p->intra_16x16_prob[0];
    dst[1] = p->intra_16x16_prob[1];
    dst[2] = p->intra_16x16_prob[2];
    dst[3] = p->intra_16x16_prob[3];
    dst[4] = p->intra_chroma_prob[0];
    dst[5] = p->intra_chroma_prob[1];
    dst[6] = p->intra_chroma_prob[2];
    dst[7] = 0; /*unused*/

    /* mv probs */
    dst += 8;
    dst[0] = p->vp8_mv_update_probs[0][0]; /* is short */
    dst[1] = p->vp8_mv_update_probs[1][0];
    dst[2] = p->vp8_mv_update_probs[0][1]; /* sign */
    dst[3] = p->vp8_mv_update_probs[1][1];
    dst[4] = p->vp8_mv_update_probs[0][8 + 9];
    dst[5] = p->vp8_mv_update_probs[0][9 + 9];
    dst[6] = p->vp8_mv_update_probs[1][8 + 9];
    dst[7] = p->vp8_mv_update_probs[1][9 + 9];
    dst += 8;
    for ( i = 0 ; i < 2 ; ++i ) {
        for ( j = 0 ; j < 8 ; j += 4 ) {
            dst[0] = p->vp8_mv_update_probs[i][j + 9 + 0];
            dst[1] = p->vp8_mv_update_probs[i][j + 9 + 1];
            dst[2] = p->vp8_mv_update_probs[i][j + 9 + 2];
            dst[3] = p->vp8_mv_update_probs[i][j + 9 + 3];
            dst += 4;
        }
    }
    for ( i = 0 ; i < 2 ; ++i ) {
        dst[0] =  p->vp8_mv_update_probs[i][0 + 2];
        dst[1] =  p->vp8_mv_update_probs[i][1 + 2];
        dst[2] =  p->vp8_mv_update_probs[i][2 + 2];
        dst[3] =  p->vp8_mv_update_probs[i][3 + 2];
        dst[4] =  p->vp8_mv_update_probs[i][4 + 2];
        dst[5] =  p->vp8_mv_update_probs[i][5 + 2];
        dst[6] =  p->vp8_mv_update_probs[i][6 + 2];
        dst[7] = 0; /*unused*/
        dst += 8;
    }

    /* coeff probs (header part) */
    dst = (RK_U8*)probTbl;
    dst += (8 * 7);
    for ( i = 0 ; i < 4 ; ++i ) {
        for ( j = 0 ; j < 8 ; ++j ) {
            for ( k = 0 ; k < 3 ; ++k ) {
                dst[0] = p->vp8_coef_update_probs[i][j][k][0];
                dst[1] = p->vp8_coef_update_probs[i][j][k][1];
                dst[2] = p->vp8_coef_update_probs[i][j][k][2];
                dst[3] = p->vp8_coef_update_probs[i][j][k][3];
                dst += 4;
            }
        }
    }

    /* coeff probs (footer part) */
    dst = (RK_U8*)probTbl;
    dst += (8 * 55);
    for ( i = 0 ; i < 4 ; ++i ) {
        for ( j = 0 ; j < 8 ; ++j ) {
            for ( k = 0 ; k < 3 ; ++k ) {
                dst[0] = p->vp8_coef_update_probs[i][j][k][4];
                dst[1] = p->vp8_coef_update_probs[i][j][k][5];
                dst[2] = p->vp8_coef_update_probs[i][j][k][6];
                dst[3] = p->vp8_coef_update_probs[i][j][k][7];
                dst[4] = p->vp8_coef_update_probs[i][j][k][8];
                dst[5] = p->vp8_coef_update_probs[i][j][k][9];
                dst[6] = p->vp8_coef_update_probs[i][j][k][10];
                dst[7] = 0; /*unused*/
                dst += 8;
            }
        }
    }
    FUN_T("FUN_OUT");
    return ;
}

MPP_RET hal_vp8d_vdpu1_gen_regs(void* hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 mb_width = 0, mb_height = 0;
    MppBuffer framebuf = NULL;
    RK_U8 *segmap_ptr = NULL;
    RK_U8 *probe_ptr = NULL;
    VP8DHalContext_t *ctx = (VP8DHalContext_t *)hal;
    VP8DRegSet_t *regs = (VP8DRegSet_t *)ctx->regs;
    DXVA_PicParams_VP8 *pic_param = (DXVA_PicParams_VP8 *)task->dec.syntax.data;

    FUN_T("FUN_IN");

    hal_vp8_init_hwcfg(ctx);
    mb_width = (pic_param->width + 15) >> 4;
    mb_height = (pic_param->height + 15) >> 4;

    regs->reg4.sw_pic_mb_width = mb_width & 0x1FF;
    regs->reg4.sw_pic_mb_hight_p =  mb_height & 0xFF;
    regs->reg4.sw_pic_mb_w_ext = mb_width >> 9;
    regs->reg4.sw_pic_mb_h_ext = mb_height >> 8;

    if (!pic_param->frame_type) {
        segmap_ptr = mpp_buffer_get_ptr(ctx->seg_map);
        if (NULL != segmap_ptr) {
            memset(segmap_ptr, 0, VP8D_MAX_SEGMAP_SIZE);
        }
    }

    probe_ptr = mpp_buffer_get_ptr(ctx->probe_table);
    if (NULL != probe_ptr) {
        hal_vp8hw_asic_probe_update(pic_param, probe_ptr);
    }
    mpp_buf_slot_get_prop(ctx->frame_slots, pic_param->CurrPic.Index7Bits, SLOT_BUFFER, &framebuf);
    regs->reg13_cur_pic_base = mpp_buffer_get_fd(framebuf);
    if (!pic_param->frame_type) { //key frame
        if ((mb_width * mb_height) << 8 > 0x400000) {
            mpp_log("mb_width*mb_height is big then 0x400000,iommu err");
        }
        regs->reg14_ref0_base = regs->reg13_cur_pic_base | ((mb_width * mb_height) << 18);
    } else if (pic_param->lst_fb_idx.Index7Bits < 0x7f) { //config ref0 base
        mpp_buf_slot_get_prop(ctx->frame_slots, pic_param->lst_fb_idx.Index7Bits, SLOT_BUFFER, &framebuf);
        regs->reg14_ref0_base = mpp_buffer_get_fd(framebuf);
    } else {
        regs->reg14_ref0_base = regs->reg13_cur_pic_base;
    }

    /* golden reference */
    if (pic_param->gld_fb_idx.Index7Bits < 0x7f) {
        mpp_buf_slot_get_prop(ctx->frame_slots, pic_param->gld_fb_idx.Index7Bits, SLOT_BUFFER, &framebuf);
        regs->reg18_golden_ref_base = mpp_buffer_get_fd(framebuf);
    } else {
        regs->reg18_golden_ref_base = regs->reg13_cur_pic_base;
    }

    regs->reg18_golden_ref_base = regs->reg18_golden_ref_base | (pic_param->ref_frame_sign_bias_golden << 10);

    /* alternate reference */
    if (pic_param->alt_fb_idx.Index7Bits < 0x7f) {
        mpp_buf_slot_get_prop(ctx->frame_slots, pic_param->alt_fb_idx.Index7Bits, SLOT_BUFFER, &framebuf);
        regs->reg19.alternate_ref_base = mpp_buffer_get_fd(framebuf);
    } else {
        regs->reg19.alternate_ref_base = regs->reg13_cur_pic_base;
    }

    regs->reg19.alternate_ref_base = regs->reg19.alternate_ref_base | (pic_param->ref_frame_sign_bias_altref << 10);

    regs->reg10_segment_map_base = regs->reg10_segment_map_base |
                                   ((pic_param->stVP8Segments.segmentation_enabled
                                     + (pic_param->stVP8Segments.update_mb_segmentation_map << 1)) << 10);

    regs->reg3.sw_pic_inter_e = pic_param->frame_type;
    regs->reg3.sw_skip_mode = !pic_param->mb_no_coeff_skip;

    if (!pic_param->stVP8Segments.segmentation_enabled) {
        regs->reg32.sw_filt_level_0 = pic_param->filter_level;
    } else if (pic_param->stVP8Segments.update_mb_segmentation_data) {
        regs->reg32.sw_filt_level_0 =
            pic_param->stVP8Segments.segment_feature_data[1][0];
        regs->reg32.sw_filt_level_1 =
            pic_param->stVP8Segments.segment_feature_data[1][1];
        regs->reg32.sw_filt_level_2 =
            pic_param->stVP8Segments.segment_feature_data[1][2];
        regs->reg32.sw_filt_level_3 =
            pic_param->stVP8Segments.segment_feature_data[1][3];
    } else {
        regs->reg32.sw_filt_level_0 = CLIP3(0, 63,
                                            (RK_S32)pic_param->filter_level
                                            + pic_param->stVP8Segments.segment_feature_data[1][0]);
        regs->reg32.sw_filt_level_1 = CLIP3(0, 63,
                                            (RK_S32)pic_param->filter_level
                                            + pic_param->stVP8Segments.segment_feature_data[1][1]);
        regs->reg32.sw_filt_level_2 = CLIP3(0, 63,
                                            (RK_S32)pic_param->filter_level
                                            + pic_param->stVP8Segments.segment_feature_data[1][2]);
        regs->reg32.sw_filt_level_3 = CLIP3(0, 63,
                                            (RK_S32)pic_param->filter_level
                                            + pic_param->stVP8Segments.segment_feature_data[1][3]);
    }

    regs->reg30.sw_filt_type = pic_param->filter_type;
    regs->reg30.sw_filt_sharpness = pic_param->sharpness;

    if (pic_param->filter_level == 0)
        regs->reg3.sw_filtering_dis = 1;

    if (pic_param->version != 3)
        regs->reg7.sw_ch_mv_res = 1;

    if (pic_param->decMode == VP8HWD_VP8 && (pic_param->version & 0x3))
        regs->reg7.sw_bilin_mc_e = 1;

    regs->reg5.sw_boolean_value = pic_param->bool_value;
    regs->reg5.sw_boolean_range = pic_param->bool_range;

    {
        if (!pic_param->stVP8Segments.segmentation_enabled)
            regs->reg33.sw_quant_0 = pic_param->y1ac_delta_q;
        else if (pic_param->stVP8Segments.update_mb_segmentation_data) { /* absolute mode */
            regs->reg33.sw_quant_0 =
                pic_param->stVP8Segments.segment_feature_data[0][0];
            regs->reg33.sw_quant_1 =
                pic_param->stVP8Segments.segment_feature_data[0][1];
            regs->reg46.sw_quant_2 = pic_param->stVP8Segments.segment_feature_data[0][2];
            regs->reg46.sw_quant_3 = pic_param->stVP8Segments.segment_feature_data[0][3];
        } else { /* delta mode */
            regs->reg33.sw_quant_0 = CLIP3(0, 127,
                                           pic_param->y1ac_delta_q
                                           + pic_param->stVP8Segments.segment_feature_data[0][0]);
            regs->reg33.sw_quant_1 = CLIP3(0, 127,
                                           pic_param->y1ac_delta_q
                                           + pic_param->stVP8Segments.segment_feature_data[0][1]);
            regs->reg46.sw_quant_2 = CLIP3(0, 127,
                                           pic_param->y1ac_delta_q
                                           + pic_param->stVP8Segments.segment_feature_data[0][2]);
            regs->reg46.sw_quant_3 = CLIP3(0, 127,
                                           pic_param->y1ac_delta_q
                                           + pic_param->stVP8Segments.segment_feature_data[0][3]);
        }

        regs->reg33.sw_quant_delta_0 = pic_param->y1dc_delta_q;
        regs->reg33.sw_quant_delta_1 = pic_param->y2dc_delta_q;
        regs->reg46.sw_quant_delta_2 = pic_param->y2ac_delta_q;
        regs->reg46.sw_quant_delta_3 = pic_param->uvdc_delta_q;
        regs->reg47.sw_quant_delta_4 = pic_param->uvac_delta_q;

        if (pic_param->mode_ref_lf_delta_enabled) {
            regs->reg31.sw_filt_ref_adj_0 = pic_param->ref_lf_deltas[0];
            regs->reg31.sw_filt_ref_adj_1 = pic_param->ref_lf_deltas[1];
            regs->reg31.sw_filt_ref_adj_2 = pic_param->ref_lf_deltas[2];
            regs->reg31.sw_filt_ref_adj_3 = pic_param->ref_lf_deltas[3];
            regs->reg30.sw_filt_mb_adj_0  = pic_param->mode_lf_deltas[0];
            regs->reg30.sw_filt_mb_adj_1  = pic_param->mode_lf_deltas[1];
            regs->reg30.sw_filt_mb_adj_2  = pic_param->mode_lf_deltas[2];
            regs->reg30.sw_filt_mb_adj_3  = pic_param->mode_lf_deltas[3];
        }
    }

    if ((pic_param->version & 0x3) == 0)
        hal_vp8d_pre_filter_tap_set(ctx);

    hal_vp8d_dct_partition_cfg(ctx, task);

    FUN_T("FUN_OUT");
    return ret;
}

MPP_RET hal_vp8d_vdpu1_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;
    VP8DHalContext_t *ctx = (VP8DHalContext_t *)hal;
    VP8DRegSet_t *regs = (VP8DRegSet_t *)ctx->regs;
    RK_U8 *p = ctx->regs;

    FUN_T("FUN_IN");

    for (i = 0; i < VP8D_REG_NUM; i++) {
        vp8h_dbg(VP8H_DBG_REG, "vp8d: regs[%02d]=%08X\n", i, *((RK_U32*)p));
        p += 4;
    }

    ret = mpp_device_send_reg(ctx->dev_ctx, (RK_U32 *)regs, VP8D_REG_NUM);
    if (ret) {
        mpp_err("mpp_device_send_reg Failed!!!\n");
        return MPP_ERR_VPUHW;
    }

    FUN_T("FUN_OUT");

    (void)task;
    return ret;
}

MPP_RET hal_vp8d_vdpu1_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    VP8DHalContext_t *ctx = (VP8DHalContext_t *)hal;
    VP8DRegSet_t reg_out;

    FUN_T("FUN_IN");
    memset(&reg_out, 0, sizeof(VP8DRegSet_t));

    ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)&reg_out, VP8D_REG_NUM);
    FUN_T("FUN_OUT");

    (void)task;
    return ret;
}

MPP_RET hal_vp8d_vdpu1_reset(void *hal)
{
    MPP_RET ret = MPP_OK;

    FUN_T("FUN_IN");
    (void)hal;
    FUN_T("FUN_OUT");
    return ret;
}

MPP_RET hal_vp8d_vdpu1_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    FUN_T("FUN_IN");
    (void)hal;
    FUN_T("FUN_OUT");
    return ret;
}

MPP_RET hal_vp8d_vdpu1_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    FUN_T("FUN_IN");
    (void)hal;
    (void)cmd_type;
    (void)param;
    FUN_T("FUN_OUT");
    return ret;
}
