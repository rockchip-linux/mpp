/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_h265d_vdpu382"

#include <stdio.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_bitput.h"

#include "h265d_syntax.h"
#include "hal_h265d_debug.h"
#include "hal_h265d_ctx.h"
#include "hal_h265d_com.h"
#include "hal_h265d_vdpu382.h"
#include "vdpu382_h265d.h"

/* #define dump */
#ifdef dump
static FILE *fp = NULL;
#endif

#define HW_RPS
#define PPS_SIZE                (112 * 64)//(96x64)

#define SET_REF_VALID(regs, index, value)\
    do{ \
        switch(index){\
        case 0: regs.reg99.hevc_ref_valid_0 = value; break;\
        case 1: regs.reg99.hevc_ref_valid_1 = value; break;\
        case 2: regs.reg99.hevc_ref_valid_2 = value; break;\
        case 3: regs.reg99.hevc_ref_valid_3 = value; break;\
        case 4: regs.reg99.hevc_ref_valid_4 = value; break;\
        case 5: regs.reg99.hevc_ref_valid_5 = value; break;\
        case 6: regs.reg99.hevc_ref_valid_6 = value; break;\
        case 7: regs.reg99.hevc_ref_valid_7 = value; break;\
        case 8: regs.reg99.hevc_ref_valid_8 = value; break;\
        case 9: regs.reg99.hevc_ref_valid_9 = value; break;\
        case 10: regs.reg99.hevc_ref_valid_10 = value; break;\
        case 11: regs.reg99.hevc_ref_valid_11 = value; break;\
        case 12: regs.reg99.hevc_ref_valid_12 = value; break;\
        case 13: regs.reg99.hevc_ref_valid_13 = value; break;\
        case 14: regs.reg99.hevc_ref_valid_14 = value; break;\
        default: break;}\
    }while(0)

#define FMT 4
#define CTU 3

typedef struct {
    RK_U32 a;
    RK_U32 b;
} FilterdColBufRatio;

static const FilterdColBufRatio filterd_fbc_on[CTU][FMT] = {
    /* 400    420      422       444 */
    {{0, 0}, {27, 15}, {36, 15}, {52, 15}}, //ctu 16
    {{0, 0}, {27, 8},  {36, 8},  {52, 8}}, //ctu 32
    {{0, 0}, {27, 5},  {36, 5},  {52, 5}}  //ctu 64
};

static const FilterdColBufRatio filterd_fbc_off[CTU][FMT] = {
    /* 400     420       422       444 */
    {{0, 0}, {15, 5},  {20, 5},  {20, 5}},  //ctu 16
    {{0, 0}, {15, 9},  {20, 9},  {20, 9}},  //ctu 32
    {{0, 0}, {15, 16}, {20, 16}, {20, 16}}  //ctu 64
};

#define CABAC_TAB_ALIGEND_SIZE          (MPP_ALIGN(27456, SZ_4K))
#define SPSPPS_ALIGNED_SIZE             (MPP_ALIGN(112 * 64, SZ_4K))
#define RPS_ALIGEND_SIZE                (MPP_ALIGN(400 * 8, SZ_4K))
#define SCALIST_ALIGNED_SIZE            (MPP_ALIGN(81 * 1360, SZ_4K))
#define INFO_BUFFER_SIZE                (SPSPPS_ALIGNED_SIZE + RPS_ALIGEND_SIZE + SCALIST_ALIGNED_SIZE)
#define ALL_BUFFER_SIZE(cnt)            (CABAC_TAB_ALIGEND_SIZE + INFO_BUFFER_SIZE *cnt)

#define CABAC_TAB_OFFSET                (0)
#define SPSPPS_OFFSET(pos)              (CABAC_TAB_OFFSET + CABAC_TAB_ALIGEND_SIZE + (INFO_BUFFER_SIZE * pos))
#define RPS_OFFSET(pos)                 (SPSPPS_OFFSET(pos) + SPSPPS_ALIGNED_SIZE)
#define SCALIST_OFFSET(pos)             (RPS_OFFSET(pos) + RPS_ALIGEND_SIZE)

static MPP_RET hal_h265d_vdpu382_init(void *hal, MppHalCfg *cfg)
{
    RK_S32 ret = 0;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;

    mpp_slots_set_prop(reg_ctx->slots, SLOTS_HOR_ALIGN, hevc_hor_align);
    mpp_slots_set_prop(reg_ctx->slots, SLOTS_VER_ALIGN, hevc_ver_align);

    reg_ctx->scaling_qm = mpp_calloc(DXVA_Qmatrix_HEVC, 1);
    if (reg_ctx->scaling_qm == NULL) {
        mpp_err("scaling_org alloc fail");
        return MPP_ERR_MALLOC;
    }

    reg_ctx->scaling_rk = mpp_calloc(scalingFactor_t, 1);
    reg_ctx->pps_buf = mpp_calloc(RK_U64, 15);
    reg_ctx->sw_rps_buf = mpp_calloc(RK_U64, 400);

    if (reg_ctx->scaling_rk == NULL) {
        mpp_err("scaling_rk alloc fail");
        return MPP_ERR_MALLOC;
    }

    if (reg_ctx->group == NULL) {
        ret = mpp_buffer_group_get_internal(&reg_ctx->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    {
        RK_U32 i = 0;
        RK_U32 max_cnt = reg_ctx->fast_mode ? MAX_GEN_REG : 1;

        //!< malloc buffers
        ret = mpp_buffer_get(reg_ctx->group, &reg_ctx->bufs, ALL_BUFFER_SIZE(max_cnt));
        if (ret) {
            mpp_err("h265d mpp_buffer_get failed\n");
            return ret;
        }

        reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
        reg_ctx->offset_cabac = CABAC_TAB_OFFSET;
        for (i = 0; i < max_cnt; i++) {
            reg_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu382H265dRegSet));
            reg_ctx->offset_spspps[i] = SPSPPS_OFFSET(i);
            reg_ctx->offset_rps[i] = RPS_OFFSET(i);
            reg_ctx->offset_sclst[i] = SCALIST_OFFSET(i);
        }
    }

    if (!reg_ctx->fast_mode) {
        reg_ctx->hw_regs = reg_ctx->g_buf[0].hw_regs;
        reg_ctx->spspps_offset = reg_ctx->offset_spspps[0];
        reg_ctx->rps_offset = reg_ctx->offset_rps[0];
        reg_ctx->sclst_offset = reg_ctx->offset_sclst[0];
    }

    ret = mpp_buffer_write(reg_ctx->bufs, 0, (void*)cabac_table, sizeof(cabac_table));
    if (ret) {
        mpp_err("h265d write cabac_table data failed\n");
        return ret;
    }

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu382_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 16;
    }

#ifdef dump
    fp = fopen("/data/hal.bin", "wb");
#endif
    (void) cfg;
    return MPP_OK;
}

static MPP_RET hal_h265d_vdpu382_deinit(void *hal)
{
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_U32 loop = reg_ctx->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->g_buf) : 1;
    RK_U32 i;

    if (reg_ctx->bufs) {
        mpp_buffer_put(reg_ctx->bufs);
        reg_ctx->bufs = NULL;
    }

    loop = reg_ctx->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->rcb_buf) : 1;
    for (i = 0; i < loop; i++) {
        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }
    }

    if (reg_ctx->group) {
        mpp_buffer_group_put(reg_ctx->group);
        reg_ctx->group = NULL;
    }

    for (i = 0; i < loop; i++)
        MPP_FREE(reg_ctx->g_buf[i].hw_regs);

    MPP_FREE(reg_ctx->scaling_qm);
    MPP_FREE(reg_ctx->scaling_rk);
    MPP_FREE(reg_ctx->pps_buf);
    MPP_FREE(reg_ctx->sw_rps_buf);

    if (reg_ctx->cmv_bufs) {
        hal_bufs_deinit(reg_ctx->cmv_bufs);
        reg_ctx->cmv_bufs = NULL;
    }

    return MPP_OK;
}

static RK_S32 hal_h265d_v382_output_pps_packet(void *hal, void *dxva)
{
    RK_S32 fifo_len = 14;//12
    RK_S32 i, j;
    RK_U32 addr;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height;
    HalH265dCtx *reg_ctx = ( HalH265dCtx *)hal;
    Vdpu382H265dRegSet *hw_reg = (Vdpu382H265dRegSet*)(reg_ctx->hw_regs);
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    BitputCtx_t bp;

    if (NULL == reg_ctx || dxva_cxt == NULL) {
        mpp_err("%s:%s:%d reg_ctx or dxva_cxt is NULL",
                __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }
    void *pps_ptr = mpp_buffer_get_ptr(reg_ctx->bufs) + reg_ctx->spspps_offset;
    if (dxva_cxt->pp.ps_update_flag) {
        RK_U64 *pps_packet = reg_ctx->pps_buf;
        if (NULL == pps_ptr) {
            mpp_err("pps_data get ptr error");
            return MPP_ERR_NOMEM;
        }

        for (i = 0; i < 14; i++) pps_packet[i] = 0;

        mpp_set_bitput_ctx(&bp, pps_packet, fifo_len);

        // SPS
        mpp_put_bits(&bp, dxva_cxt->pp.vps_id                            , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.sps_id                            , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.chroma_format_idc                 , 2);

        log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;
        width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
        height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

        mpp_put_bits(&bp, width                                          , 16);
        mpp_put_bits(&bp, height                                         , 16);
        mpp_put_bits(&bp, dxva_cxt->pp.bit_depth_luma_minus8 + 8         , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.bit_depth_chroma_minus8 + 8       , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_max_pic_order_cnt_lsb_minus4 + 4      , 5);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size   , 2); //log2_maxa_coding_block_depth
        mpp_put_bits(&bp, dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3 , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_min_transform_block_size_minus2 + 2   , 3);
        ///<-zrh comment ^  63 bit above
        mpp_put_bits(&bp, dxva_cxt->pp.log2_diff_max_min_transform_block_size     , 2);
        mpp_put_bits(&bp, dxva_cxt->pp.max_transform_hierarchy_depth_inter        , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.max_transform_hierarchy_depth_intra        , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.scaling_list_enabled_flag                  , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.amp_enabled_flag                           , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.sample_adaptive_offset_enabled_flag        , 1);
        ///<-zrh comment ^  68 bit above
        mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag                           , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag ? (dxva_cxt->pp.pcm_sample_bit_depth_luma_minus1 + 1) : 0  , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag ? (dxva_cxt->pp.pcm_sample_bit_depth_chroma_minus1 + 1) : 0 , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.pcm_loop_filter_disabled_flag                                               , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_diff_max_min_pcm_luma_coding_block_size                                , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag ? (dxva_cxt->pp.log2_min_pcm_luma_coding_block_size_minus3 + 3) : 0, 3);

        mpp_put_bits(&bp, dxva_cxt->pp.num_short_term_ref_pic_sets             , 7);
        mpp_put_bits(&bp, dxva_cxt->pp.long_term_ref_pics_present_flag         , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.num_long_term_ref_pics_sps              , 6);
        mpp_put_bits(&bp, dxva_cxt->pp.sps_temporal_mvp_enabled_flag           , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.strong_intra_smoothing_enabled_flag     , 1);
        ///<-zrh comment ^ 100 bit above

        mpp_put_bits(&bp, 0                                                    , 7 ); //49bits
        //yandong change
        mpp_put_bits(&bp, dxva_cxt->pp.sps_max_dec_pic_buffering_minus1,       4);
        mpp_put_bits(&bp, 0, 3);
        mpp_put_align(&bp                                                        , 32, 0xf); //128
        // PPS
        mpp_put_bits(&bp, dxva_cxt->pp.pps_id                                    , 6 );
        mpp_put_bits(&bp, dxva_cxt->pp.sps_id                                    , 4 );
        mpp_put_bits(&bp, dxva_cxt->pp.dependent_slice_segments_enabled_flag     , 1 );
        mpp_put_bits(&bp, dxva_cxt->pp.output_flag_present_flag                  , 1 );
        mpp_put_bits(&bp, dxva_cxt->pp.num_extra_slice_header_bits               , 13);
        mpp_put_bits(&bp, dxva_cxt->pp.sign_data_hiding_enabled_flag , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.cabac_init_present_flag                   , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.num_ref_idx_l0_default_active_minus1 + 1  , 4);//31 bits
        mpp_put_bits(&bp, dxva_cxt->pp.num_ref_idx_l1_default_active_minus1 + 1  , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.init_qp_minus26                           , 7);
        mpp_put_bits(&bp, dxva_cxt->pp.constrained_intra_pred_flag               , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.transform_skip_enabled_flag               , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.cu_qp_delta_enabled_flag                  , 1); //164
        mpp_put_bits(&bp, log2_min_cb_size +
                     dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size -
                     dxva_cxt->pp.diff_cu_qp_delta_depth                             , 3);

        h265h_dbg(H265H_DBG_PPS, "log2_min_cb_size %d %d %d \n", log2_min_cb_size,
                  dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size, dxva_cxt->pp.diff_cu_qp_delta_depth );

        mpp_put_bits(&bp, dxva_cxt->pp.pps_cb_qp_offset                            , 5);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_cr_qp_offset                            , 5);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_slice_chroma_qp_offsets_present_flag    , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.weighted_pred_flag                          , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.weighted_bipred_flag                        , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.transquant_bypass_enabled_flag              , 1 );
        mpp_put_bits(&bp, dxva_cxt->pp.tiles_enabled_flag                          , 1 );
        mpp_put_bits(&bp, dxva_cxt->pp.entropy_coding_sync_enabled_flag            , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_loop_filter_across_slices_enabled_flag  , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.loop_filter_across_tiles_enabled_flag       , 1); //185
        mpp_put_bits(&bp, dxva_cxt->pp.deblocking_filter_override_enabled_flag     , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_deblocking_filter_disabled_flag         , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_beta_offset_div2                        , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_tc_offset_div2                          , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.lists_modification_present_flag             , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_parallel_merge_level_minus2 + 2        , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.slice_segment_header_extension_present_flag , 1);
        mpp_put_bits(&bp, 0                                                        , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.tiles_enabled_flag ? dxva_cxt->pp.num_tile_columns_minus1 + 1 : 0, 5);
        mpp_put_bits(&bp, dxva_cxt->pp.tiles_enabled_flag ? dxva_cxt->pp.num_tile_rows_minus1 + 1 : 0 , 5 );
        mpp_put_bits(&bp, 0, 4);//2 //mSps_Pps[i]->mMode
        mpp_put_align(&bp, 64, 0xf);
        {
            /// tiles info begin
            RK_U16 column_width[20];
            RK_U16 row_height[22];

            memset(column_width, 0, sizeof(column_width));
            memset(row_height, 0, sizeof(row_height));

            if (dxva_cxt->pp.tiles_enabled_flag) {

                if (dxva_cxt->pp.uniform_spacing_flag == 0) {
                    RK_S32 maxcuwidth = dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size + log2_min_cb_size;
                    RK_S32 ctu_width_in_pic = (width +
                                               (1 << maxcuwidth) - 1) / (1 << maxcuwidth) ;
                    RK_S32 ctu_height_in_pic = (height +
                                                (1 << maxcuwidth) - 1) / (1 << maxcuwidth) ;
                    RK_S32 sum = 0;
                    for (i = 0; i < dxva_cxt->pp.num_tile_columns_minus1; i++) {
                        column_width[i] = dxva_cxt->pp.column_width_minus1[i] + 1;
                        sum += column_width[i]  ;
                    }
                    column_width[i] = ctu_width_in_pic - sum;

                    sum = 0;
                    for (i = 0; i < dxva_cxt->pp.num_tile_rows_minus1; i++) {
                        row_height[i] = dxva_cxt->pp.row_height_minus1[i] + 1;
                        sum += row_height[i];
                    }
                    row_height[i] = ctu_height_in_pic - sum;
                } // end of (pps->uniform_spacing_flag == 0)
                else {

                    RK_S32    pic_in_cts_width = (width +
                                                  (1 << (log2_min_cb_size +
                                                         dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size)) - 1)
                                                 / (1 << (log2_min_cb_size +
                                                          dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size));
                    RK_S32 pic_in_cts_height = (height +
                                                (1 << (log2_min_cb_size +
                                                       dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size)) - 1)
                                               / (1 << (log2_min_cb_size +
                                                        dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size));

                    for (i = 0; i < dxva_cxt->pp.num_tile_columns_minus1 + 1; i++)
                        column_width[i] = ((i + 1) * pic_in_cts_width) / (dxva_cxt->pp.num_tile_columns_minus1 + 1) -
                                          (i * pic_in_cts_width) / (dxva_cxt->pp.num_tile_columns_minus1 + 1);

                    for (i = 0; i < dxva_cxt->pp.num_tile_rows_minus1 + 1; i++)
                        row_height[i] = ((i + 1) * pic_in_cts_height) / (dxva_cxt->pp.num_tile_rows_minus1 + 1) -
                                        (i * pic_in_cts_height) / (dxva_cxt->pp.num_tile_rows_minus1 + 1);
                }
            } // pps->tiles_enabled_flag
            else {
                RK_S32 MaxCUWidth = (1 << (dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size + log2_min_cb_size));
                column_width[0] = (width  + MaxCUWidth - 1) / MaxCUWidth;
                row_height[0]   = (height + MaxCUWidth - 1) / MaxCUWidth;
            }

            for (j = 0; j < 20; j++) {
                if (column_width[j] > 0)
                    column_width[j]--;
                mpp_put_bits(&bp, column_width[j], 12);
            }

            for (j = 0; j < 22; j++) {
                if (row_height[j] > 0)
                    row_height[j]--;
                mpp_put_bits(&bp, row_height[j], 12);
            }
        }

        mpp_put_bits(&bp, 0, 32);
        mpp_put_bits(&bp, 0, 70);
        mpp_put_align(&bp, 64, 0xf);//128
    }

    if (dxva_cxt->pp.scaling_list_enabled_flag) {
        RK_U8 *ptr_scaling = (RK_U8 *)mpp_buffer_get_ptr(reg_ctx->bufs) + reg_ctx->sclst_offset;

        if (dxva_cxt->pp.scaling_list_data_present_flag) {
            addr = (dxva_cxt->pp.pps_id + 16) * 1360;
        } else if (dxva_cxt->pp.scaling_list_enabled_flag) {
            addr = dxva_cxt->pp.sps_id * 1360;
        } else {
            addr = 80 * 1360;
        }

        hal_h265d_output_scalinglist_packet(hal, ptr_scaling + addr, dxva);

        hw_reg->h265d_addr.reg180_scanlist_addr = reg_ctx->bufs_fd;
        hw_reg->common.reg012.scanlist_addr_valid_en = 1;

        /* need to config addr */
        mpp_dev_set_reg_offset(reg_ctx->dev, 180, addr + reg_ctx->sclst_offset);
    }

    for (i = 0; i < 64; i++)
        memcpy(pps_ptr + i * 112, reg_ctx->pps_buf, 112);
#ifdef dump
    fwrite(pps_ptr, 1, 80 * 64, fp);
    RK_U32 *tmp = (RK_U32 *)pps_ptr;
    for (i = 0; i < 112 / 4; i++) {
        mpp_log("pps[%3d] = 0x%08x\n", i, tmp[i]);
    }
#endif
    return 0;
}

static void h265d_refine_rcb_size(Vdpu382RcbInfo *rcb_info,
                                  Vdpu382H265dRegSet *hw_regs,
                                  RK_S32 width, RK_S32 height, void *dxva)
{
    RK_U32 rcb_bits = 0;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    DXVA_PicParams_HEVC *pp = &dxva_cxt->pp;
    RK_U32 chroma_fmt_idc = pp->chroma_format_idc;//0 400,1 420 ,2 422,3 444
    RK_U8 bit_depth = MPP_MAX(pp->bit_depth_luma_minus8, pp->bit_depth_chroma_minus8) + 8;
    RK_U8 ctu_size = 1 << (pp->log2_diff_max_min_luma_coding_block_size + pp->log2_min_luma_coding_block_size_minus3 + 3);
    RK_U32 tile_col_cut_num = pp->num_tile_columns_minus1;
    RK_U32 ext_align_size = tile_col_cut_num * 64 * 8;

    width = MPP_ALIGN(width, ctu_size);
    height = MPP_ALIGN(height, ctu_size);

    /* RCB_STRMD_ROW */
    if (width >= 8192) {
        RK_U32 factor = 64 / ctu_size;

        rcb_bits = (MPP_ALIGN(width, ctu_size) + factor - 1) / factor * 24 + ext_align_size;
    } else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_TRANSD_ROW */
    if (width >= 8192)
        rcb_bits = (MPP_ALIGN(width - 8192, 4) << 1) + ext_align_size;
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_TRANSD_COL */
    if (height >= 8192 && tile_col_cut_num)
        rcb_bits = tile_col_cut_num ? (MPP_ALIGN(height - 8192, 4) << 1) : 0;
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_COL].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_INTER_ROW */
    rcb_bits = width * 22 + ext_align_size;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_INTER_COL */
    rcb_bits = tile_col_cut_num ? (height * 22) : 0;
    rcb_info[RCB_INTER_COL].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_INTRA_ROW */
    rcb_bits = width * ((chroma_fmt_idc ? 1 : 0) + 1) * 11 + ext_align_size;
    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_DBLK_ROW */
    if (chroma_fmt_idc == 1 || chroma_fmt_idc == 2) {
        if (ctu_size == 32)
            rcb_bits = width * ( 4 + 6 * bit_depth);
        else
            rcb_bits = width * ( 2 + 6 * bit_depth);
    } else {
        if (ctu_size == 32)
            rcb_bits = width * ( 4 + 8 * bit_depth);
        else
            rcb_bits = width * ( 2 + 8 * bit_depth);
    }
    rcb_bits += (tile_col_cut_num * (bit_depth == 8 ? 256 : 192)) + ext_align_size;
    rcb_info[RCB_DBLK_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_SAO_ROW */
    if (chroma_fmt_idc == 1 || chroma_fmt_idc == 2) {
        rcb_bits = width * (128 / ctu_size + 2 * bit_depth);
    } else {
        rcb_bits = width * (128 / ctu_size + 3 * bit_depth);
    }
    rcb_bits += (tile_col_cut_num * (bit_depth == 8 ? 160 : 128)) + ext_align_size;
    rcb_info[RCB_SAO_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_FBC_ROW */
    if (hw_regs->common.reg012.fbc_e) {
        rcb_bits = width * (chroma_fmt_idc - 1) * 2 * bit_depth;
        rcb_bits += (tile_col_cut_num * (bit_depth == 8 ? 128 : 64)) + ext_align_size;
    } else
        rcb_bits = 0;
    rcb_info[RCB_FBC_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_FILT_COL */
    if (tile_col_cut_num) {
        if (hw_regs->common.reg012.fbc_e) {
            RK_U32 ctu_idx = ctu_size >> 5;
            RK_U32 a = filterd_fbc_on[ctu_idx][chroma_fmt_idc].a;
            RK_U32 b = filterd_fbc_on[ctu_idx][chroma_fmt_idc].b;

            rcb_bits = height * (a * bit_depth + b);
        } else {
            RK_U32 ctu_idx = ctu_size >> 5;
            RK_U32 a = filterd_fbc_off[ctu_idx][chroma_fmt_idc].a;
            RK_U32 b = filterd_fbc_off[ctu_idx][chroma_fmt_idc].b;

            rcb_bits = height * (a * bit_depth + b);
        }
    } else
        rcb_bits = 0;
    rcb_info[RCB_FILT_COL].size = MPP_RCB_BYTES(rcb_bits);
}

static void hal_h265d_rcb_info_update(void *hal,  void *dxva,
                                      Vdpu382H265dRegSet *hw_regs,
                                      RK_S32 width, RK_S32 height)
{
    HalH265dCtx *reg_ctx = ( HalH265dCtx *)hal;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    DXVA_PicParams_HEVC *pp = &dxva_cxt->pp;
    RK_U32 chroma_fmt_idc = pp->chroma_format_idc;//0 400,1 4202 ,422,3 444
    RK_U8 bit_depth = MPP_MAX(pp->bit_depth_luma_minus8, pp->bit_depth_chroma_minus8) + 8;
    RK_U8 ctu_size = 1 << (pp->log2_diff_max_min_luma_coding_block_size + pp->log2_min_luma_coding_block_size_minus3 + 3);
    RK_U32 num_tiles = pp->num_tile_rows_minus1 + 1;

    if (reg_ctx->num_row_tiles != num_tiles ||
        reg_ctx->bit_depth != bit_depth ||
        reg_ctx->chroma_fmt_idc != chroma_fmt_idc ||
        reg_ctx->ctu_size !=  ctu_size ||
        reg_ctx->width != width ||
        reg_ctx->height != height) {
        RK_U32 i = 0;
        RK_U32 loop = reg_ctx->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->g_buf) : 1;

        reg_ctx->rcb_buf_size = vdpu382_get_rcb_buf_size((Vdpu382RcbInfo*)reg_ctx->rcb_info, width, height);
        h265d_refine_rcb_size((Vdpu382RcbInfo*)reg_ctx->rcb_info, hw_regs, width, height, dxva_cxt);

        for (i = 0; i < loop; i++) {
            MppBuffer rcb_buf;

            if (reg_ctx->rcb_buf[i]) {
                mpp_buffer_put(reg_ctx->rcb_buf[i]);
                reg_ctx->rcb_buf[i] = NULL;
            }
            mpp_buffer_get(reg_ctx->group, &rcb_buf, reg_ctx->rcb_buf_size);
            reg_ctx->rcb_buf[i] = rcb_buf;
        }

        reg_ctx->num_row_tiles  = num_tiles;
        reg_ctx->bit_depth      = bit_depth;
        reg_ctx->chroma_fmt_idc = chroma_fmt_idc;
        reg_ctx->ctu_size       = ctu_size;
        reg_ctx->width          = width;
        reg_ctx->height         = height;
    }
}

#define SET_POC_HIGNBIT_INFO(regs, index, field, value)\
    do{ \
        switch(index){\
        case 0: regs.reg200.ref0_##field = value; break;\
        case 1: regs.reg200.ref1_##field = value; break;\
        case 2: regs.reg200.ref2_##field = value; break;\
        case 3: regs.reg200.ref3_##field = value; break;\
        case 4: regs.reg200.ref4_##field = value; break;\
        case 5: regs.reg200.ref5_##field = value; break;\
        case 6: regs.reg200.ref6_##field = value; break;\
        case 7: regs.reg200.ref7_##field = value; break;\
        case 8: regs.reg201.ref8_##field = value; break;\
        case 9: regs.reg201.ref9_##field = value; break;\
        case 10: regs.reg201.ref10_##field = value; break;\
        case 11: regs.reg201.ref11_##field = value; break;\
        case 12: regs.reg201.ref12_##field = value; break;\
        case 13: regs.reg201.ref13_##field = value; break;\
        case 14: regs.reg201.ref14_##field = value; break;\
        case 15: regs.reg201.ref15_##field = value; break;\
        default: break;}\
    }while(0)

#define pocdistance(a, b) (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))

static MPP_RET hal_h265d_vdpu382_setup_colmv_buf(void *hal, HalTaskInfo *syn)
{
    HalH265dCtx *reg_ctx = ( HalH265dCtx *)hal;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t *)syn->dec.syntax.data;
    DXVA_PicParams_HEVC *pp = &dxva_cxt->pp;
    RK_U8 ctu_size = 1 << (pp->log2_diff_max_min_luma_coding_block_size +
                           pp->log2_min_luma_coding_block_size_minus3 + 3);
    RK_U32 log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;

    RK_U32 width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
    RK_U32 height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);
    RK_U32 mv_size = 0, colmv_size = 16, colmv_byte = 16;
    RK_U32 compress = reg_ctx->hw_info ? reg_ctx->hw_info->cap_colmv_compress : 1;


    mv_size = vdpu382_get_colmv_size(width, height, ctu_size, colmv_byte, colmv_size, compress);

    if (reg_ctx->cmv_bufs == NULL || reg_ctx->mv_size < mv_size) {
        size_t size = mv_size;

        if (reg_ctx->cmv_bufs) {
            hal_bufs_deinit(reg_ctx->cmv_bufs);
            reg_ctx->cmv_bufs = NULL;
        }

        hal_bufs_init(&reg_ctx->cmv_bufs);
        if (reg_ctx->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            return MPP_ERR_NOMEM;;
        }

        reg_ctx->mv_size = mv_size;
        reg_ctx->mv_count = mpp_buf_slot_get_count(reg_ctx->slots);
        hal_bufs_setup(reg_ctx->cmv_bufs, reg_ctx->mv_count, 1, &size);
    }

    return MPP_OK;
}

static MPP_RET hal_h265d_vdpu382_gen_regs(void *hal,  HalTaskInfo *syn)
{
    RK_S32 i = 0;
    RK_S32 log2_min_cb_size;
    RK_S32 width, height;
    RK_S32 stride_y, stride_uv, virstrid_y;
    Vdpu382H265dRegSet *hw_regs;
    RK_S32 ret = MPP_SUCCESS;
    MppBuffer streambuf = NULL;
    RK_S32 aglin_offset = 0;
    RK_S32 valid_ref = -1;
    MppBuffer framebuf = NULL;
    HalBuf *mv_buf = NULL;
    RK_S32 fd = -1;
    RK_S32 distance = INT_MAX;
    h265d_dxva2_picture_context_t *dxva_cxt =
        (h265d_dxva2_picture_context_t *)syn->dec.syntax.data;
    HalH265dCtx *reg_ctx = ( HalH265dCtx *)hal;
    void *rps_ptr = NULL;
    RK_U32 stream_buf_size = 0;

    if (syn->dec.flags.parse_err ||
        (syn->dec.flags.ref_err && !reg_ctx->cfg->base.disable_error)) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    if (reg_ctx ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_ctx->g_buf[i].use_flag) {
                syn->dec.reg_index = i;

                reg_ctx->spspps_offset = reg_ctx->offset_spspps[i];
                reg_ctx->rps_offset = reg_ctx->offset_rps[i];
                reg_ctx->sclst_offset = reg_ctx->offset_sclst[i];

                reg_ctx->hw_regs = reg_ctx->g_buf[i].hw_regs;
                reg_ctx->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("hevc rps buf all used");
            return MPP_ERR_NOMEM;
        }
    } else {
        syn->dec.reg_index = 0;
    }
    rps_ptr = mpp_buffer_get_ptr(reg_ctx->bufs) + reg_ctx->rps_offset;
    if (NULL == rps_ptr) {

        mpp_err("rps_data get ptr error");
        return MPP_ERR_NOMEM;
    }


    if (syn->dec.syntax.data == NULL) {
        mpp_err("%s:%s:%d dxva is NULL", __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

    /* output pps */
    hw_regs = (Vdpu382H265dRegSet*)reg_ctx->hw_regs;
    memset(hw_regs, 0, sizeof(Vdpu382H265dRegSet));

    if (NULL == reg_ctx->hw_regs) {
        return MPP_ERR_NULL_PTR;
    }


    log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;

    width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);
    ret = hal_h265d_vdpu382_setup_colmv_buf(hal, syn);
    if (ret)
        return MPP_ERR_NOMEM;

    {
        MppFrame mframe = NULL;
        RK_U32 ver_virstride;

        mpp_buf_slot_get_prop(reg_ctx->slots, dxva_cxt->pp.CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        stride_y = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        stride_uv = stride_y;
        virstrid_y = ver_virstride * stride_y;
        hw_regs->common.reg013.h26x_error_mode = 1;
        hw_regs->common.reg021.error_deb_en = 1;
        hw_regs->common.reg021.inter_error_prc_mode = 0;
        hw_regs->common.reg021.error_intra_mode = 1;

        hw_regs->common.reg017.slice_num = dxva_cxt->slice_count;
        hw_regs->h265d_param.reg64.h26x_rps_mode = 0;
        hw_regs->h265d_param.reg64.h26x_frame_orslice = 0;
        hw_regs->h265d_param.reg64.h26x_stream_mode = 0;

        if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset = MPP_ALIGN(fbc_hdr_stride * (ver_virstride + 64) / 16, SZ_4K);

            hw_regs->common.reg012.fbc_e = 1;
            hw_regs->common.reg018.y_hor_virstride = fbc_hdr_stride >> 4;
            hw_regs->common.reg019.uv_hor_virstride = fbc_hdr_stride >> 4;
            hw_regs->common.reg020_fbc_payload_off.payload_st_offset = fbd_offset >> 4;
        } else {
            hw_regs->common.reg012.fbc_e = 0;
            hw_regs->common.reg018.y_hor_virstride = stride_y >> 4;
            hw_regs->common.reg019.uv_hor_virstride = stride_uv >> 4;
            hw_regs->common.reg020_y_virstride.y_virstride = virstrid_y >> 4;
        }
    }
    mpp_buf_slot_get_prop(reg_ctx->slots, dxva_cxt->pp.CurrPic.Index7Bits,
                          SLOT_BUFFER, &framebuf);
    hw_regs->common_addr.reg130_decout_base  = mpp_buffer_get_fd(framebuf); //just index need map
    /*if out_base is equal to zero it means this frame may error
    we return directly add by csy*/

    if (hw_regs->common_addr.reg130_decout_base == 0) {
        return 0;
    }
    fd =  mpp_buffer_get_fd(framebuf);
    hw_regs->common_addr.reg130_decout_base = fd;
    mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, dxva_cxt->pp.CurrPic.Index7Bits);
    hw_regs->common_addr.reg131_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);

    hw_regs->h265d_param.reg65.cur_top_poc = dxva_cxt->pp.CurrPicOrderCntVal;

    mpp_buf_slot_get_prop(reg_ctx->packet_slots, syn->dec.input, SLOT_BUFFER,
                          &streambuf);
    if ( dxva_cxt->bitstream == NULL) {
        dxva_cxt->bitstream = mpp_buffer_get_ptr(streambuf);
    }
#ifdef HW_RPS
    hw_regs->h265d_param.reg103.ref_pic_layer_same_with_cur = 0xffff;
    hal_h265d_slice_hw_rps(syn->dec.syntax.data, rps_ptr, reg_ctx->sw_rps_buf, reg_ctx->fast_mode);
#else
    hw_regs->sw_sysctrl.sw_h26x_rps_mode = 1;
    hal_h265d_slice_output_rps(syn->dec.syntax.data, rps_ptr);
#endif

    /* cabac table */
    hw_regs->h265d_addr.reg197_cabactbl_base    = reg_ctx->bufs_fd;
    /* pps */
    hw_regs->h265d_addr.reg161_pps_base         = reg_ctx->bufs_fd;
    hw_regs->h265d_addr.reg163_rps_base         = reg_ctx->bufs_fd;

    hw_regs->common_addr.reg128_rlc_base        = mpp_buffer_get_fd(streambuf);
    hw_regs->common_addr.reg129_rlcwrite_base   = mpp_buffer_get_fd(streambuf);
    stream_buf_size                             = mpp_buffer_get_size(streambuf);
    hw_regs->common.reg016_str_len              = ((dxva_cxt->bitstream_size + 15)
                                                   & (~15)) + 64;
    hw_regs->common.reg016_str_len = stream_buf_size > hw_regs->common.reg016_str_len ?
                                     hw_regs->common.reg016_str_len : stream_buf_size;

    aglin_offset =  hw_regs->common.reg016_str_len - dxva_cxt->bitstream_size;
    if (aglin_offset > 0) {
        memset((void *)(dxva_cxt->bitstream + dxva_cxt->bitstream_size), 0,
               aglin_offset);
    }
    hw_regs->common.reg010.dec_e                = 1;
    hw_regs->common.reg012.colmv_compress_en = reg_ctx->hw_info ?
                                               reg_ctx->hw_info->cap_colmv_compress : 0;

    hw_regs->common.reg024.cabac_err_en_lowbits = 0xffffdfff;
    hw_regs->common.reg025.cabac_err_en_highbits = 0x3ffbf9ff;

    hw_regs->common.reg011.dec_clkgate_e    = 1;
    hw_regs->common.reg011.err_head_fill_e  = 1;
    hw_regs->common.reg011.err_colmv_fill_e = 1;

    hw_regs->common.reg026.inter_auto_gating_e = 1;
    hw_regs->common.reg026.filterd_auto_gating_e = 1;
    hw_regs->common.reg026.strmd_auto_gating_e = 1;
    hw_regs->common.reg026.mcp_auto_gating_e = 1;
    hw_regs->common.reg026.busifd_auto_gating_e = 1;
    hw_regs->common.reg026.dec_ctrl_auto_gating_e = 1;
    hw_regs->common.reg026.intra_auto_gating_e = 1;
    hw_regs->common.reg026.mc_auto_gating_e = 1;
    hw_regs->common.reg026.transd_auto_gating_e = 1;
    hw_regs->common.reg026.sram_auto_gating_e = 1;
    hw_regs->common.reg026.cru_auto_gating_e = 1;
    hw_regs->common.reg026.reg_cfg_gating_en = 1;
    hw_regs->common.reg032_timeout_threshold = 0x3ffff;

    valid_ref = hw_regs->common_addr.reg130_decout_base;
    reg_ctx->error_index[syn->dec.reg_index] = dxva_cxt->pp.CurrPic.Index7Bits;
    hw_regs->common_addr.reg132_error_ref_base = valid_ref;

    memset(&hw_regs->highpoc.reg205, 0, sizeof(RK_U32));

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_cxt->pp.RefPicList); i++) {
        if (dxva_cxt->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_cxt->pp.RefPicList[i].bPicEntry != 0x7f) {

            MppFrame mframe = NULL;
            hw_regs->h265d_param.reg67_82_ref_poc[i] = dxva_cxt->pp.PicOrderCntValList[i];
            mpp_buf_slot_get_prop(reg_ctx->slots,
                                  dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);
            mpp_buf_slot_get_prop(reg_ctx->slots, dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_FRAME_PTR, &mframe);
            if (framebuf != NULL) {
                hw_regs->h265d_addr.reg164_179_ref_base[i] = mpp_buffer_get_fd(framebuf);
                valid_ref = hw_regs->h265d_addr.reg164_179_ref_base[i];
                // mpp_log("cur poc %d, ref poc %d", dxva_cxt->pp.current_poc, dxva_cxt->pp.PicOrderCntValList[i]);
                if ((pocdistance(dxva_cxt->pp.PicOrderCntValList[i], dxva_cxt->pp.current_poc) < distance)
                    && (!mpp_frame_get_errinfo(mframe))) {
                    distance = pocdistance(dxva_cxt->pp.PicOrderCntValList[i], dxva_cxt->pp.current_poc);
                    hw_regs->common_addr.reg132_error_ref_base = hw_regs->h265d_addr.reg164_179_ref_base[i];
                    reg_ctx->error_index[syn->dec.reg_index] = dxva_cxt->pp.RefPicList[i].Index7Bits;
                    hw_regs->common.reg021.error_intra_mode = 0;

                }
            } else {
                hw_regs->h265d_addr.reg164_179_ref_base[i] = valid_ref;
            }

            mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, dxva_cxt->pp.RefPicList[i].Index7Bits);
            hw_regs->h265d_addr.reg181_196_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);

            SET_REF_VALID(hw_regs->h265d_param, i, 1);
        }
    }

    if ((reg_ctx->error_index[syn->dec.reg_index] == dxva_cxt->pp.CurrPic.Index7Bits) &&
        !dxva_cxt->pp.IntraPicFlag) {
        h265h_dbg(H265H_DBG_TASK_ERR, "current frm may be err, should skip process");
        syn->dec.flags.ref_err = 1;
        return MPP_OK;
    }

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_cxt->pp.RefPicList); i++) {

        if (dxva_cxt->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_cxt->pp.RefPicList[i].bPicEntry != 0x7f) {
            MppFrame mframe = NULL;

            mpp_buf_slot_get_prop(reg_ctx->slots,
                                  dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);

            mpp_buf_slot_get_prop(reg_ctx->slots, dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_FRAME_PTR, &mframe);

            if (framebuf == NULL || mpp_frame_get_errinfo(mframe)) {
                mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, reg_ctx->error_index[syn->dec.reg_index]);
                hw_regs->h265d_addr.reg164_179_ref_base[i] = hw_regs->common_addr.reg132_error_ref_base;
                hw_regs->h265d_addr.reg181_196_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
            }
        } else {
            mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, reg_ctx->error_index[syn->dec.reg_index]);
            hw_regs->h265d_addr.reg164_179_ref_base[i] = hw_regs->common_addr.reg132_error_ref_base;
            hw_regs->h265d_addr.reg181_196_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
            /* mark 3 to differ from current frame */
            if (reg_ctx->error_index[syn->dec.reg_index] == dxva_cxt->pp.CurrPic.Index7Bits)
                SET_POC_HIGNBIT_INFO(hw_regs->highpoc, i, poc_highbit, 3);
        }
    }
    hal_h265d_v382_output_pps_packet(hal, syn->dec.syntax.data);

    mpp_dev_set_reg_offset(reg_ctx->dev, 161, reg_ctx->spspps_offset);
    /* rps */
    mpp_dev_set_reg_offset(reg_ctx->dev, 163, reg_ctx->rps_offset);

    hw_regs->common.reg013.cur_pic_is_idr = dxva_cxt->pp.IdrPicFlag;//p_hal->slice_long->idr_flag;

    hw_regs->common.reg011.buf_empty_en = 1;

    hal_h265d_rcb_info_update(hal, dxva_cxt, hw_regs, width, height);
    vdpu382_setup_rcb(&hw_regs->common_addr, reg_ctx->dev, reg_ctx->fast_mode ?
                      reg_ctx->rcb_buf[syn->dec.reg_index] : reg_ctx->rcb_buf[0],
                      (Vdpu382RcbInfo*)reg_ctx->rcb_info);
    {
        MppFrame mframe = NULL;

        mpp_buf_slot_get_prop(reg_ctx->slots, dxva_cxt->pp.CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);

        if (mpp_frame_get_thumbnail_en(mframe)) {
            hw_regs->h265d_addr.reg198_scale_down_luma_base =
                hw_regs->common_addr.reg130_decout_base;
            hw_regs->h265d_addr.reg199_scale_down_chorme_base =
                hw_regs->common_addr.reg130_decout_base;
            vdpu382_setup_down_scale(mframe, reg_ctx->dev, &hw_regs->common);
        } else {
            hw_regs->h265d_addr.reg198_scale_down_luma_base = 0;
            hw_regs->h265d_addr.reg199_scale_down_chorme_base = 0;
            hw_regs->common.reg012.scale_down_en = 0;
        }
    }
    vdpu382_setup_statistic(&hw_regs->common, &hw_regs->statistic);
    mpp_buffer_sync_end(reg_ctx->bufs);

    return ret;
}

static MPP_RET hal_h265d_vdpu382_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U8* p = NULL;
    Vdpu382H265dRegSet *hw_regs = NULL;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_S32 index =  task->dec.reg_index;

    RK_U32 i;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !reg_ctx->cfg->base.disable_error)) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    if (reg_ctx->fast_mode) {
        p = (RK_U8*)reg_ctx->g_buf[index].hw_regs;
        hw_regs = ( Vdpu382H265dRegSet *)reg_ctx->g_buf[index].hw_regs;
    } else {
        p = (RK_U8*)reg_ctx->hw_regs;
        hw_regs = ( Vdpu382H265dRegSet *)reg_ctx->hw_regs;
    }

    if (hw_regs == NULL) {
        mpp_err("hal_h265d_start hw_regs is NULL");
        return MPP_ERR_NULL_PTR;
    }
    for (i = 0; i < 68; i++) {
        h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                  i, *((RK_U32*)p));
        //mpp_log("RK_HEVC_DEC: regs[%02d]=%08X\n", i, *((RK_U32*)p));
        p += 4;
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &hw_regs->common;
        wr_cfg.size = sizeof(hw_regs->common);
        wr_cfg.offset = OFFSET_COMMON_REGS;

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->h265d_param;
        wr_cfg.size = sizeof(hw_regs->h265d_param);
        wr_cfg.offset = OFFSET_CODEC_PARAMS_REGS;

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->common_addr;
        wr_cfg.size = sizeof(hw_regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->h265d_addr;
        wr_cfg.size = sizeof(hw_regs->h265d_addr);
        wr_cfg.offset = OFFSET_CODEC_ADDR_REGS;

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->statistic;
        wr_cfg.size = sizeof(hw_regs->statistic);
        wr_cfg.offset = OFFSET_STATISTIC_REGS;

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->highpoc;
        wr_cfg.size = sizeof(hw_regs->highpoc);
        wr_cfg.offset = OFFSET_POC_HIGHBIT_REGS;

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &hw_regs->irq_status;
        rd_cfg.size = sizeof(hw_regs->irq_status);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }
        /* rcb info for sram */
        vdpu382_set_rcbinfo(reg_ctx->dev, (Vdpu382RcbInfo*)reg_ctx->rcb_info);

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    return ret;
}


static MPP_RET hal_h265d_vdpu382_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_S32 index =  task->dec.reg_index;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_U8* p = NULL;
    Vdpu382H265dRegSet *hw_regs = NULL;
    RK_S32 i;

    if (reg_ctx->fast_mode) {
        hw_regs = ( Vdpu382H265dRegSet *)reg_ctx->g_buf[index].hw_regs;
    } else {
        hw_regs = ( Vdpu382H265dRegSet *)reg_ctx->hw_regs;
    }

    p = (RK_U8*)hw_regs;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !reg_ctx->cfg->base.disable_error)) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        goto ERR_PROC;
    }

    ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

ERR_PROC:
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        hw_regs->irq_status.reg224.dec_error_sta ||
        hw_regs->irq_status.reg224.buf_empty_sta ||
        hw_regs->irq_status.reg224.dec_bus_sta ||
        !hw_regs->irq_status.reg224.dec_rdy_sta) {
        if (!reg_ctx->fast_mode) {
            if (reg_ctx->dec_cb)
                mpp_callback(reg_ctx->dec_cb, &task->dec);
        } else {
            MppFrame mframe = NULL;
            mpp_buf_slot_get_prop(reg_ctx->slots, task->dec.output,
                                  SLOT_FRAME_PTR, &mframe);
            if (mframe) {
                reg_ctx->fast_mode_err_found = 1;
                mpp_frame_set_errinfo(mframe, 1);
            }
        }
    } else {
        if (reg_ctx->fast_mode && reg_ctx->fast_mode_err_found) {
            for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(task->dec.refer); i++) {
                if (task->dec.refer[i] >= 0) {
                    MppFrame frame_ref = NULL;

                    mpp_buf_slot_get_prop(reg_ctx->slots, task->dec.refer[i],
                                          SLOT_FRAME_PTR, &frame_ref);
                    h265h_dbg(H265H_DBG_FAST_ERR, "refer[%d] %d frame %p\n",
                              i, task->dec.refer[i], frame_ref);
                    if (frame_ref && mpp_frame_get_errinfo(frame_ref)) {
                        MppFrame frame_out = NULL;
                        mpp_buf_slot_get_prop(reg_ctx->slots, task->dec.output,
                                              SLOT_FRAME_PTR, &frame_out);
                        mpp_frame_set_errinfo(frame_out, 1);
                        break;
                    }
                }
            }
        }
    }

    for (i = 0; i < 68; i++) {
        if (i == 1) {
            h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                      i, *((RK_U32*)p));
        }

        if (i == 45) {
            h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                      i, *((RK_U32*)p));
        }
        p += 4;
    }

    if (reg_ctx->fast_mode) {
        reg_ctx->g_buf[index].use_flag = 0;
    }

    return ret;
}

static MPP_RET hal_h265d_vdpu382_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalH265dCtx *p_hal = (HalH265dCtx *)hal;
    p_hal->fast_mode_err_found = 0;
    (void)hal;
    return ret;
}

static MPP_RET hal_h265d_vdpu382_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

static MPP_RET hal_h265d_vdpu382_control(void *hal, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;
    HalH265dCtx *p_hal = (HalH265dCtx *)hal;

    (void)hal;
    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO: {
        MppFrame frame = (MppFrame)param;
        MppFrameFormat fmt = mpp_frame_get_fmt(frame);

        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            vdpu382_afbc_align_calc(p_hal->slots, frame, 16);
        }
        break;
    }
    case MPP_DEC_SET_OUTPUT_FORMAT: {
    } break;
    default:
        break;
    }
    return  ret;
}

const MppHalApi hal_h265d_vdpu382 = {
    .name = "h265d_vdpu382",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(HalH265dCtx),
    .flag = 0,
    .init = hal_h265d_vdpu382_init,
    .deinit = hal_h265d_vdpu382_deinit,
    .reg_gen = hal_h265d_vdpu382_gen_regs,
    .start = hal_h265d_vdpu382_start,
    .wait = hal_h265d_vdpu382_wait,
    .reset = hal_h265d_vdpu382_reset,
    .flush = hal_h265d_vdpu382_flush,
    .control = hal_h265d_vdpu382_control,
};
