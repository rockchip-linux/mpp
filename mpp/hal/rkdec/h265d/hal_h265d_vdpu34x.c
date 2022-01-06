/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_h265d_vdpu34x"

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
#include "hal_h265d_vdpu34x.h"
#include "vdpu34x_h265d.h"

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
    /* 400    420      422       444 */
    {{0, 0}, {9, 31}, {12, 39}, {12, 39}}, //ctu 16
    {{0, 0}, {9, 25}, {12, 33}, {12, 33}}, //ctu 32
    {{0, 0}, {9, 21}, {12, 29}, {12, 29}}  //ctu 64
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

static MPP_RET hal_h265d_vdpu34x_init(void *hal, MppHalCfg *cfg)
{
    RK_S32 ret = 0;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;

    mpp_slots_set_prop(reg_cxt->slots, SLOTS_HOR_ALIGN, hevc_hor_align);
    mpp_slots_set_prop(reg_cxt->slots, SLOTS_VER_ALIGN, hevc_ver_align);

    reg_cxt->scaling_qm = mpp_calloc(DXVA_Qmatrix_HEVC, 1);
    if (reg_cxt->scaling_qm == NULL) {
        mpp_err("scaling_org alloc fail");
        return MPP_ERR_MALLOC;
    }

    reg_cxt->scaling_rk = mpp_calloc(scalingFactor_t, 1);
    reg_cxt->pps_buf = mpp_calloc(RK_U64, 15);
    reg_cxt->sw_rps_buf = mpp_calloc(RK_U64, 400);

    if (reg_cxt->scaling_rk == NULL) {
        mpp_err("scaling_rk alloc fail");
        return MPP_ERR_MALLOC;
    }

    if (reg_cxt->group == NULL) {
        ret = mpp_buffer_group_get_internal(&reg_cxt->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    {
        RK_U32 i = 0;
        RK_U32 max_cnt = reg_cxt->fast_mode ? MAX_GEN_REG : 1;

        //!< malloc buffers
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->bufs, ALL_BUFFER_SIZE(max_cnt));
        if (ret) {
            mpp_err("h265d mpp_buffer_get failed\n");
            return ret;
        }

        reg_cxt->bufs_fd = mpp_buffer_get_fd(reg_cxt->bufs);
        reg_cxt->offset_cabac = CABAC_TAB_OFFSET;
        for (i = 0; i < max_cnt; i++) {
            reg_cxt->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu34xH265dRegSet));
            reg_cxt->offset_spspps[i] = SPSPPS_OFFSET(i);
            reg_cxt->offset_rps[i] = RPS_OFFSET(i);
            reg_cxt->offset_sclst[i] = SCALIST_OFFSET(i);
        }
    }

    if (!reg_cxt->fast_mode) {
        reg_cxt->hw_regs = reg_cxt->g_buf[0].hw_regs;
        reg_cxt->spspps_offset = reg_cxt->offset_spspps[0];
        reg_cxt->rps_offset = reg_cxt->offset_rps[0];
        reg_cxt->sclst_offset = reg_cxt->offset_sclst[0];
    }

    ret = mpp_buffer_write(reg_cxt->bufs, 0, (void*)cabac_table, sizeof(cabac_table));
    if (ret) {
        mpp_err("h265d write cabac_table data failed\n");
        return ret;
    }

    {
        // report hw_info to parser
        const MppSocInfo *info = mpp_get_soc_info();
        const void *hw_info = NULL;
        RK_U32 i;

        for (i = 0; i < MPP_ARRAY_ELEMS(info->dec_caps); i++) {
            if (info->dec_caps[i] && info->dec_caps[i]->type == VPU_CLIENT_RKVDEC) {
                hw_info = info->dec_caps[i];
                break;
            }
        }

        mpp_assert(hw_info);
        cfg->hw_info = hw_info;
    }

#ifdef dump
    fp = fopen("/data/hal.bin", "wb");
#endif
    (void) cfg;
    return MPP_OK;
}

static MPP_RET hal_h265d_vdpu34x_deinit(void *hal)
{
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;
    RK_U32 loop = reg_cxt->fast_mode ? MPP_ARRAY_ELEMS(reg_cxt->g_buf) : 1;
    RK_U32 i;

    if (reg_cxt->bufs) {
        mpp_buffer_put(reg_cxt->bufs);
        reg_cxt->bufs = NULL;
    }

    loop = reg_cxt->fast_mode ? MPP_ARRAY_ELEMS(reg_cxt->rcb_buf) : 1;
    for (i = 0; i < loop; i++) {
        if (reg_cxt->rcb_buf[i]) {
            mpp_buffer_put(reg_cxt->rcb_buf[i]);
            reg_cxt->rcb_buf[i] = NULL;
        }
    }

    if (reg_cxt->group) {
        mpp_buffer_group_put(reg_cxt->group);
        reg_cxt->group = NULL;
    }

    for (i = 0; i < loop; i++)
        MPP_FREE(reg_cxt->g_buf[i].hw_regs);

    MPP_FREE(reg_cxt->scaling_qm);
    MPP_FREE(reg_cxt->scaling_rk);
    MPP_FREE(reg_cxt->pps_buf);
    MPP_FREE(reg_cxt->sw_rps_buf);

    if (reg_cxt->cmv_bufs) {
        hal_bufs_deinit(reg_cxt->cmv_bufs);
        reg_cxt->cmv_bufs = NULL;
    }

    return MPP_OK;
}

static RK_S32 hal_h265d_v345_output_pps_packet(void *hal, void *dxva)
{
    RK_S32 fifo_len = 14;//12
    RK_S32 i, j;
    RK_U32 addr;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height;
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;
    Vdpu34xH265dRegSet *hw_reg = (Vdpu34xH265dRegSet*)(reg_cxt->hw_regs);
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    BitputCtx_t bp;

    if (NULL == reg_cxt || dxva_cxt == NULL) {
        mpp_err("%s:%s:%d reg_cxt or dxva_cxt is NULL",
                __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }
    void *pps_ptr = mpp_buffer_get_ptr(reg_cxt->bufs) + reg_cxt->spspps_offset;
    if (dxva_cxt->pp.ps_update_flag) {
        RK_U64 *pps_packet = reg_cxt->pps_buf;
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
        MppDevRegOffsetCfg trans_cfg;
        RK_U8 *ptr_scaling = (RK_U8 *)mpp_buffer_get_ptr(reg_cxt->bufs) + reg_cxt->sclst_offset;

        if (dxva_cxt->pp.scaling_list_data_present_flag) {
            addr = (dxva_cxt->pp.pps_id + 16) * 1360;
        } else if (dxva_cxt->pp.scaling_list_enabled_flag) {
            addr = dxva_cxt->pp.sps_id * 1360;
        } else {
            addr = 80 * 1360;
        }

        hal_h265d_output_scalinglist_packet(hal, ptr_scaling + addr, dxva);

        hw_reg->h265d_addr.reg180_scanlist_addr = reg_cxt->bufs_fd;
        hw_reg->common.reg012.scanlist_addr_valid_en = 1;

        /* need to config addr */
        trans_cfg.reg_idx = 180;
        trans_cfg.offset = addr + reg_cxt->sclst_offset;
        mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    for (i = 0; i < 64; i++)
        memcpy(pps_ptr + i * 112, reg_cxt->pps_buf, 112);
#ifdef dump
    fwrite(pps_ptr, 1, 80 * 64, fp);
    RK_U32 *tmp = (RK_U32 *)pps_ptr;
    for (i = 0; i < 112 / 4; i++) {
        mpp_log("pps[%3d] = 0x%08x\n", i, tmp[i]);
    }
#endif
    return 0;
}

static RK_S32 hal_h265d_output_pps_packet(void *hal, void *dxva)
{
    RK_S32 fifo_len = 10;
    RK_S32 i, j;
    RK_U32 addr;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height;
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    BitputCtx_t bp;

    if (NULL == reg_cxt || dxva_cxt == NULL) {
        mpp_err("%s:%s:%d reg_cxt or dxva_cxt is NULL",
                __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

    void *pps_ptr = mpp_buffer_get_ptr(reg_cxt->bufs) + reg_cxt->spspps_offset;

    if (dxva_cxt->pp.ps_update_flag || dxva_cxt->pp.scaling_list_enabled_flag) {
        RK_U64 *pps_packet = reg_cxt->pps_buf;

        if (NULL == pps_ptr) {
            mpp_err("pps_data get ptr error");
            return MPP_ERR_NOMEM;
        }

        for (i = 0; i < 10; i++) pps_packet[i] = 0;

        mpp_set_bitput_ctx(&bp, pps_packet, fifo_len);

        // SPS
        mpp_put_bits(&bp, dxva_cxt->pp.vps_id                            , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.sps_id                            , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.chroma_format_idc                 , 2);

        log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;
        width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
        height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

        mpp_put_bits(&bp, width                                          , 16);//yandong
        mpp_put_bits(&bp, height                                         , 16);//yandong
        mpp_put_bits(&bp, dxva_cxt->pp.bit_depth_luma_minus8 + 8         , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.bit_depth_chroma_minus8 + 8       , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_max_pic_order_cnt_lsb_minus4 + 4      , 5);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size   , 2); //log2_maxa_coding_block_depth
        mpp_put_bits(&bp, dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3 , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_min_transform_block_size_minus2 + 2   , 3);
        ///<-zrh comment ^  57 bit above
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

        mpp_put_bits(&bp, 0                                                    , 7 );
        mpp_put_align(&bp                                                      , 32, 0xf);

        // PPS
        mpp_put_bits(&bp, dxva_cxt->pp.pps_id                                    , 6 );
        mpp_put_bits(&bp, dxva_cxt->pp.sps_id                                    , 4 );
        mpp_put_bits(&bp, dxva_cxt->pp.dependent_slice_segments_enabled_flag     , 1 );
        mpp_put_bits(&bp, dxva_cxt->pp.output_flag_present_flag                  , 1 );
        mpp_put_bits(&bp, dxva_cxt->pp.num_extra_slice_header_bits               , 13);
        mpp_put_bits(&bp, dxva_cxt->pp.sign_data_hiding_enabled_flag , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.cabac_init_present_flag                   , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.num_ref_idx_l0_default_active_minus1 + 1  , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.num_ref_idx_l1_default_active_minus1 + 1  , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.init_qp_minus26                           , 7);
        mpp_put_bits(&bp, dxva_cxt->pp.constrained_intra_pred_flag               , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.transform_skip_enabled_flag               , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.cu_qp_delta_enabled_flag                  , 1);

        mpp_put_bits(&bp, log2_min_cb_size +
                     dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size -
                     dxva_cxt->pp.diff_cu_qp_delta_depth                           , 3);

        h265h_dbg(H265H_DBG_PPS, "log2_min_cb_size %d %d %d \n", log2_min_cb_size,
                  dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size, dxva_cxt->pp.diff_cu_qp_delta_depth );

        mpp_put_bits(&bp, dxva_cxt->pp.pps_cb_qp_offset                            , 5);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_cr_qp_offset                            , 5);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_slice_chroma_qp_offsets_present_flag    , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.weighted_pred_flag                          , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.weighted_bipred_flag                        , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.transquant_bypass_enabled_flag              , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.tiles_enabled_flag                          , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.entropy_coding_sync_enabled_flag            , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_loop_filter_across_slices_enabled_flag  , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.loop_filter_across_tiles_enabled_flag       , 1);

        mpp_put_bits(&bp, dxva_cxt->pp.deblocking_filter_override_enabled_flag     , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_deblocking_filter_disabled_flag         , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_beta_offset_div2                        , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.pps_tc_offset_div2                          , 4);
        mpp_put_bits(&bp, dxva_cxt->pp.lists_modification_present_flag             , 1);
        mpp_put_bits(&bp, dxva_cxt->pp.log2_parallel_merge_level_minus2 + 2        , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.slice_segment_header_extension_present_flag , 1);
        mpp_put_bits(&bp, 0                                                        , 3);
        mpp_put_bits(&bp, dxva_cxt->pp.num_tile_columns_minus1 + 1, 5);
        mpp_put_bits(&bp, dxva_cxt->pp.num_tile_rows_minus1 + 1 , 5 );
        mpp_put_bits(&bp, 3, 2); //mSps_Pps[i]->mMode
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

                    RK_S32 pic_in_cts_width = (width +
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
                mpp_put_bits(&bp, column_width[j], 12);// yandong 8bit -> 12bit
            }

            for (j = 0; j < 22; j++) {
                if (row_height[j] > 0)
                    row_height[j]--;
                mpp_put_bits(&bp, row_height[j], 12);// yandong 8bit -> 12bit
            }
        }

        {
            RK_U8 *ptr_scaling = (RK_U8 *)mpp_buffer_get_ptr(reg_cxt->scaling_list_data);
            if (dxva_cxt->pp.scaling_list_data_present_flag) {
                addr = (dxva_cxt->pp.pps_id + 16) * 1360;
            } else if (dxva_cxt->pp.scaling_list_enabled_flag) {
                addr = dxva_cxt->pp.sps_id * 1360;
            } else {
                addr = 80 * 1360;
            }

            hal_h265d_output_scalinglist_packet(hal, ptr_scaling + addr, dxva);

            RK_U32 fd = mpp_buffer_get_fd(reg_cxt->scaling_list_data);
            /* need to config addr */
            addr = fd | (addr << 10);

            mpp_put_bits(&bp, addr, 32);
            mpp_put_align(&bp, 64, 0xf);
        }
        for (i = 0; i < 64; i++)
            memcpy(pps_ptr + i * 80, reg_cxt->pps_buf, 80);
    } else if (reg_cxt->fast_mode) {
        for (i = 0; i < 64; i++)
            memcpy(pps_ptr + i * 80, reg_cxt->pps_buf, 80);
    }

#ifdef dump
    fwrite(pps_ptr, 1, 80 * 64, fp);
    fflush(fp);
#endif
    return 0;
}

static void h265d_refine_rcb_size(Vdpu34xRcbInfo *rcb_info,
                                  Vdpu34xH265dRegSet *hw_regs,
                                  RK_S32 width, RK_S32 height, void *dxva)
{
    RK_U32 rcb_bits = 0;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    DXVA_PicParams_HEVC *pp = &dxva_cxt->pp;
    RK_U32 chroma_fmt_idc = pp->chroma_format_idc;//0 400,1 4202 ,422,3 444
    RK_U8 bit_depth = MPP_MAX(pp->bit_depth_luma_minus8, pp->bit_depth_chroma_minus8) + 8;
    RK_U8 ctu_size = 1 << (pp->log2_diff_max_min_luma_coding_block_size + pp->log2_min_luma_coding_block_size_minus3 + 3);
    RK_U32 num_tiles = pp->num_tile_rows_minus1 + 1;
    RK_U32 ext_align_size = num_tiles * 64 * 8;

    width = MPP_ALIGN(width, ctu_size);
    height = MPP_ALIGN(height, ctu_size);
    /* RCB_STRMD_ROW */
    if (width > 8192) {
        RK_U32 factor = ctu_size / 16;
        rcb_bits = (MPP_ALIGN(width, ctu_size) + factor - 1) * factor * 24 + ext_align_size;
    } else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_TRANSD_ROW */
    if (width > 8192)
        rcb_bits = (MPP_ALIGN(width - 8192, 4) << 1) + ext_align_size;
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_TRANSD_COL */
    if (height > 8192)
        rcb_bits = (MPP_ALIGN(height - 8192, 4) << 1) + ext_align_size;
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_COL].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTER_ROW */
    rcb_bits = width * 22 + ext_align_size;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTER_COL */
    rcb_bits = height * 22 + ext_align_size;
    rcb_info[RCB_INTER_COL].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTRA_ROW */
    rcb_bits = width * 48 + ext_align_size;
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
    rcb_bits += (num_tiles * (bit_depth == 8 ? 256 : 192)) + ext_align_size;
    rcb_info[RCB_DBLK_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_SAO_ROW */
    if (chroma_fmt_idc == 1 || chroma_fmt_idc == 2) {
        rcb_bits = width * (128 / ctu_size + 2 * bit_depth);
    } else {
        rcb_bits = width * (128 / ctu_size + 3 * bit_depth);
    }
    rcb_bits += (num_tiles * (bit_depth == 8 ? 160 : 128)) + ext_align_size;
    rcb_info[RCB_SAO_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_FBC_ROW */
    if (hw_regs->common.reg012.fbc_e) {
        rcb_bits = width * (chroma_fmt_idc - 1) * 2 * bit_depth;
        rcb_bits += (num_tiles * (bit_depth == 8 ? 128 : 64)) + ext_align_size;
    } else
        rcb_bits = 0;
    rcb_info[RCB_FBC_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_FILT_COL */
    if (hw_regs->common.reg012.fbc_e) {
        RK_U32 ctu_idx = ctu_size >> 5;
        RK_U32 a = filterd_fbc_on[chroma_fmt_idc][ctu_idx].a;
        RK_U32 b = filterd_fbc_on[chroma_fmt_idc][ctu_idx].b;

        rcb_bits = height * (a * bit_depth + b);
    } else {
        RK_U32 ctu_idx = ctu_size >> 5;
        RK_U32 a = filterd_fbc_off[chroma_fmt_idc][ctu_idx].a;
        RK_U32 b = filterd_fbc_off[chroma_fmt_idc][ctu_idx].b;

        rcb_bits = height * (a * bit_depth + b + (bit_depth == 10 ? 192 * ctu_size >> 4 : 0));
    }
    rcb_bits += ext_align_size;
    rcb_info[RCB_FILT_COL].size = MPP_RCB_BYTES(rcb_bits);
}

static void hal_h265d_rcb_info_update(void *hal,  void *dxva,
                                      Vdpu34xH265dRegSet *hw_regs,
                                      RK_S32 width, RK_S32 height)
{
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    DXVA_PicParams_HEVC *pp = &dxva_cxt->pp;
    RK_U32 chroma_fmt_idc = pp->chroma_format_idc;//0 400,1 4202 ,422,3 444
    RK_U8 bit_depth = MPP_MAX(pp->bit_depth_luma_minus8, pp->bit_depth_chroma_minus8) + 8;
    RK_U8 ctu_size = 1 << (pp->log2_diff_max_min_luma_coding_block_size + pp->log2_min_luma_coding_block_size_minus3 + 3);
    RK_U32 num_tiles = pp->num_tile_rows_minus1 + 1;

    if (reg_cxt->num_row_tiles != num_tiles ||
        reg_cxt->bit_depth != bit_depth ||
        reg_cxt->chroma_fmt_idc != chroma_fmt_idc ||
        reg_cxt->ctu_size !=  ctu_size ||
        reg_cxt->width != width ||
        reg_cxt->height != height) {
        RK_U32 i = 0;
        RK_U32 loop = reg_cxt->fast_mode ? MPP_ARRAY_ELEMS(reg_cxt->g_buf) : 1;

        reg_cxt->rcb_buf_size = get_rcb_buf_size(reg_cxt->rcb_info, width, height);
        h265d_refine_rcb_size(reg_cxt->rcb_info, hw_regs, width, height, dxva_cxt);

        for (i = 0; i < loop; i++) {
            MppBuffer rcb_buf;

            if (reg_cxt->rcb_buf[i]) {
                mpp_buffer_put(reg_cxt->rcb_buf[i]);
                reg_cxt->rcb_buf[i] = NULL;
            }
            mpp_buffer_get(reg_cxt->group, &rcb_buf, reg_cxt->rcb_buf_size);
            reg_cxt->rcb_buf[i] = rcb_buf;
        }

        reg_cxt->num_row_tiles  = num_tiles;
        reg_cxt->bit_depth      = bit_depth;
        reg_cxt->chroma_fmt_idc = chroma_fmt_idc;
        reg_cxt->ctu_size       = ctu_size;
        reg_cxt->width          = width;
        reg_cxt->height         = height;
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

static MPP_RET hal_h265d_vdpu34x_gen_regs(void *hal,  HalTaskInfo *syn)
{
    RK_S32 i = 0;
    RK_S32 log2_min_cb_size;
    RK_S32 width, height;
    RK_S32 stride_y, stride_uv, virstrid_y;
    Vdpu34xH265dRegSet *hw_regs;
    RK_S32 ret = MPP_SUCCESS;
    MppBuffer streambuf = NULL;
    RK_S32 aglin_offset = 0;
    RK_S32 valid_ref = -1;
    MppBuffer framebuf = NULL;
    RK_U32  sw_ref_valid = 0;
    HalBuf *mv_buf = NULL;
    RK_S32 fd = -1;
    RK_U32 mv_size = 0;
    RK_U32 fbc_flag = 0;

    if (syn->dec.flags.parse_err ||
        syn->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    h265d_dxva2_picture_context_t *dxva_cxt =
        (h265d_dxva2_picture_context_t *)syn->dec.syntax.data;
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;
    RK_S32 max_poc =  dxva_cxt->pp.current_poc;
    RK_S32 min_poc =  0;

    void *rps_ptr = NULL;
    if (reg_cxt ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_cxt->g_buf[i].use_flag) {
                syn->dec.reg_index = i;

                reg_cxt->spspps_offset = reg_cxt->offset_spspps[i];
                reg_cxt->rps_offset = reg_cxt->offset_rps[i];
                reg_cxt->sclst_offset = reg_cxt->offset_sclst[i];

                reg_cxt->hw_regs = reg_cxt->g_buf[i].hw_regs;
                reg_cxt->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("hevc rps buf all used");
            return MPP_ERR_NOMEM;
        }
    }
    rps_ptr = mpp_buffer_get_ptr(reg_cxt->bufs) + reg_cxt->rps_offset;
    if (NULL == rps_ptr) {

        mpp_err("rps_data get ptr error");
        return MPP_ERR_NOMEM;
    }


    if (syn->dec.syntax.data == NULL) {
        mpp_err("%s:%s:%d dxva is NULL", __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

    /* output pps */
    hw_regs = (Vdpu34xH265dRegSet*)reg_cxt->hw_regs;
    memset(hw_regs, 0, sizeof(Vdpu34xH265dRegSet));

    if (reg_cxt->is_v34x) {
        hal_h265d_v345_output_pps_packet(hal, syn->dec.syntax.data);
    } else {
        hal_h265d_output_pps_packet(hal, syn->dec.syntax.data);
    }

    if (NULL == reg_cxt->hw_regs) {
        return MPP_ERR_NULL_PTR;
    }


    log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;

    width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);
    mv_size = (MPP_ALIGN(width, 64) * MPP_ALIGN(height, 64)) >> 3;
    if (reg_cxt->cmv_bufs == NULL || reg_cxt->mv_size < mv_size) {
        size_t size = mv_size;

        if (reg_cxt->cmv_bufs) {
            hal_bufs_deinit(reg_cxt->cmv_bufs);
            reg_cxt->cmv_bufs = NULL;
        }

        hal_bufs_init(&reg_cxt->cmv_bufs);
        if (reg_cxt->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            return MPP_ERR_NULL_PTR;
        }

        reg_cxt->mv_size = mv_size;
        reg_cxt->mv_count = mpp_buf_slot_get_count(reg_cxt->slots);
        hal_bufs_setup(reg_cxt->cmv_bufs, reg_cxt->mv_count, 1, &size);
    }

    {
        MppFrame mframe = NULL;
        RK_U32 ver_virstride;

        mpp_buf_slot_get_prop(reg_cxt->slots, dxva_cxt->pp.CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        stride_y = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        stride_uv = stride_y;
        virstrid_y = ver_virstride * stride_y;
        hw_regs->common.reg013.h26x_error_mode = 1;
        hw_regs->common.reg013.h26x_streamd_error_mode = 1;
        hw_regs->common.reg013.colmv_error_mode = 1;
        hw_regs->common.reg021.error_deb_en = 1;
        hw_regs->common.reg021.inter_error_prc_mode = 0;
        hw_regs->common.reg021.error_intra_mode = 1;

        hw_regs->common.reg017.slice_num = dxva_cxt->slice_count;
        hw_regs->h265d_param.reg64.h26x_rps_mode = 0;
        hw_regs->h265d_param.reg64.h26x_frame_orslice = 0;
        hw_regs->h265d_param.reg64.h26x_stream_mode = 0;

        if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
            fbc_flag = 1;
            RK_U32 pixel_width = MPP_ALIGN(mpp_frame_get_width(mframe), 64);
            RK_U32 fbd_offset = MPP_ALIGN(pixel_width * (MPP_ALIGN(ver_virstride, 64) + 16) / 16,
                                          SZ_4K);

            hw_regs->common.reg012.fbc_e = 1;
            hw_regs->common.reg018.y_hor_virstride = pixel_width >> 4;
            hw_regs->common.reg019.uv_hor_virstride = pixel_width >> 4;
            hw_regs->common.reg020_fbc_payload_off.payload_st_offset = fbd_offset >> 4;
        } else {
            hw_regs->common.reg012.fbc_e = 0;
            hw_regs->common.reg018.y_hor_virstride = stride_y >> 4;
            hw_regs->common.reg019.uv_hor_virstride = stride_uv >> 4;
            hw_regs->common.reg020_y_virstride.y_virstride = virstrid_y >> 4;
        }
    }
    mpp_buf_slot_get_prop(reg_cxt->slots, dxva_cxt->pp.CurrPic.Index7Bits,
                          SLOT_BUFFER, &framebuf);
    hw_regs->common_addr.reg130_decout_base  = mpp_buffer_get_fd(framebuf); //just index need map
    /*if out_base is equal to zero it means this frame may error
    we return directly add by csy*/

    if (hw_regs->common_addr.reg130_decout_base == 0) {
        return 0;
    }
    fd =  mpp_buffer_get_fd(framebuf);
    hw_regs->common_addr.reg130_decout_base = fd;
    mv_buf = hal_bufs_get_buf(reg_cxt->cmv_bufs, dxva_cxt->pp.CurrPic.Index7Bits);
    hw_regs->common_addr.reg131_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);

    hw_regs->h265d_param.reg65.cur_top_poc = dxva_cxt->pp.CurrPicOrderCntVal;

    mpp_buf_slot_get_prop(reg_cxt->packet_slots, syn->dec.input, SLOT_BUFFER,
                          &streambuf);
    if ( dxva_cxt->bitstream == NULL) {
        dxva_cxt->bitstream = mpp_buffer_get_ptr(streambuf);
    }
    if (reg_cxt->is_v34x) {
#ifdef HW_RPS
        hw_regs->common.reg012.wait_reset_en = 1;
        hw_regs->h265d_param.reg103.ref_pic_layer_same_with_cur = 0xffff;
        hal_h265d_slice_hw_rps(syn->dec.syntax.data, rps_ptr, reg_cxt->sw_rps_buf, reg_cxt->fast_mode);
#else
        hw_regs->sw_sysctrl.sw_h26x_rps_mode = 1;
        hal_h265d_slice_output_rps(syn->dec.syntax.data, rps_ptr);
#endif
    } else {
        hal_h265d_slice_output_rps(syn->dec.syntax.data, rps_ptr);
    }

    MppDevRegOffsetCfg trans_cfg;
    /* cabac table */
    hw_regs->h265d_addr.reg197_cabactbl_base    = reg_cxt->bufs_fd;
    /* pps */
    hw_regs->h265d_addr.reg161_pps_base         = reg_cxt->bufs_fd;
    hw_regs->h265d_addr.reg163_rps_base         = reg_cxt->bufs_fd;

    hw_regs->common_addr.reg128_rlc_base        = mpp_buffer_get_fd(streambuf);
    hw_regs->common_addr.reg129_rlcwrite_base   = mpp_buffer_get_fd(streambuf);
    hw_regs->common.reg016_str_len              = ((dxva_cxt->bitstream_size + 15)
                                                   & (~15)) + 64;
    aglin_offset =  hw_regs->common.reg016_str_len - dxva_cxt->bitstream_size;
    if (aglin_offset > 0) {
        memset((void *)(dxva_cxt->bitstream + dxva_cxt->bitstream_size), 0,
               aglin_offset);
    }
    hw_regs->common.reg010.dec_e                = 1;
    hw_regs->common.reg011.dec_timeout_e        = 1;
    hw_regs->common.reg012.wr_ddr_align_en      = dxva_cxt->pp.tiles_enabled_flag
                                                  ? 0 : 1;
    hw_regs->common.reg012.colmv_compress_en    = 1;

    hw_regs->common.reg024.cabac_err_en_lowbits = 0xffffdfff;
    hw_regs->common.reg025.cabac_err_en_highbits = 0x3ffbf9ff;

    hw_regs->common.reg011.dec_clkgate_e    = 1;
    hw_regs->common.reg011.dec_e_strmd_clkgate_dis = 0;
    hw_regs->common.reg026.swreg_block_gating_e =
        (mpp_get_soc_type() == ROCKCHIP_SOC_RK3588) ? 0xfffef : 0xfffff;
    hw_regs->common.reg026.reg_cfg_gating_en = 1;
    hw_regs->common.reg032_timeout_threshold = 0x3ffff;

    valid_ref = hw_regs->common_addr.reg130_decout_base;
    reg_cxt->error_index = dxva_cxt->pp.CurrPic.Index7Bits;
    hw_regs->common_addr.reg132_error_ref_base = valid_ref;
    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_cxt->pp.RefPicList); i++) {
        if (dxva_cxt->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_cxt->pp.RefPicList[i].bPicEntry != 0x7f) {

            MppFrame mframe = NULL;
            hw_regs->h265d_param.reg67_82_ref_poc[i] = dxva_cxt->pp.PicOrderCntValList[i];
            mpp_buf_slot_get_prop(reg_cxt->slots,
                                  dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);
            mpp_buf_slot_get_prop(reg_cxt->slots, dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_FRAME_PTR, &mframe);
            if (framebuf != NULL) {
                hw_regs->h265d_addr.reg164_179_ref_base[i] = mpp_buffer_get_fd(framebuf);
                valid_ref = hw_regs->h265d_addr.reg164_179_ref_base[i];
                if ((dxva_cxt->pp.PicOrderCntValList[i] < max_poc) &&
                    (dxva_cxt->pp.PicOrderCntValList[i] >= min_poc)
                    && (!mpp_frame_get_errinfo(mframe))) {

                    min_poc = dxva_cxt->pp.PicOrderCntValList[i];
                    hw_regs->common_addr.reg132_error_ref_base = hw_regs->h265d_addr.reg164_179_ref_base[i];
                    reg_cxt->error_index = dxva_cxt->pp.RefPicList[i].Index7Bits;
                    hw_regs->common.reg021.error_intra_mode = 0;

                }
            } else {
                hw_regs->h265d_addr.reg164_179_ref_base[i] = valid_ref;
            }

            mv_buf = hal_bufs_get_buf(reg_cxt->cmv_bufs, dxva_cxt->pp.RefPicList[i].Index7Bits);
            hw_regs->h265d_addr.reg181_196_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);

            sw_ref_valid          |=   (1 << i);
            SET_REF_VALID(hw_regs->h265d_param, i, 1);
        }
    }

    if ((reg_cxt->error_index == dxva_cxt->pp.CurrPic.Index7Bits) &&
        !dxva_cxt->pp.IntraPicFlag && fbc_flag) {
        syn->dec.flags.ref_err = 1;
        return MPP_OK;
    }

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_cxt->pp.RefPicList); i++) {

        if (dxva_cxt->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_cxt->pp.RefPicList[i].bPicEntry != 0x7f) {
            MppFrame mframe = NULL;

            mpp_buf_slot_get_prop(reg_cxt->slots,
                                  dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);

            mpp_buf_slot_get_prop(reg_cxt->slots, dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_FRAME_PTR, &mframe);

            if (framebuf == NULL || mpp_frame_get_errinfo(mframe)) {
                mv_buf = hal_bufs_get_buf(reg_cxt->cmv_bufs, reg_cxt->error_index);
                hw_regs->h265d_addr.reg164_179_ref_base[i] = hw_regs->common_addr.reg132_error_ref_base;
                hw_regs->h265d_addr.reg181_196_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
            }
        } else {
            mv_buf = hal_bufs_get_buf(reg_cxt->cmv_bufs, reg_cxt->error_index);
            hw_regs->h265d_addr.reg164_179_ref_base[i] = hw_regs->common_addr.reg132_error_ref_base;
            hw_regs->h265d_addr.reg181_196_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
            /* mark 3 to differ from current frame */
            if (reg_cxt->error_index == dxva_cxt->pp.CurrPic.Index7Bits)
                SET_POC_HIGNBIT_INFO(hw_regs->highpoc, i, poc_highbit, 3);
        }
    }

    trans_cfg.reg_idx = 161;
    trans_cfg.offset = reg_cxt->spspps_offset;
    mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    /* rps */
    trans_cfg.reg_idx = 163;
    trans_cfg.offset = reg_cxt->rps_offset;
    mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_OFFSET, &trans_cfg);

    hw_regs->common.reg013.timeout_mode = 1;
    hw_regs->common.reg013.cur_pic_is_idr = dxva_cxt->pp.IdrPicFlag;//p_hal->slice_long->idr_flag;

    hw_regs->common.reg011.buf_empty_en = 1;

    hal_h265d_rcb_info_update(hal, dxva_cxt, hw_regs, width, height);
    vdpu34x_setup_rcb(&hw_regs->common_addr, reg_cxt->dev, reg_cxt->fast_mode ?
                      reg_cxt->rcb_buf[syn->dec.reg_index] : reg_cxt->rcb_buf[0],
                      reg_cxt->rcb_info);
    vdpu34x_setup_statistic(&hw_regs->common, &hw_regs->statistic);

    return ret;
}

static MPP_RET hal_h265d_vdpu34x_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U8* p = NULL;
    Vdpu34xH265dRegSet *hw_regs = NULL;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;
    RK_S32 index =  task->dec.reg_index;

    RK_U32 i;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    if (reg_cxt->fast_mode) {
        p = (RK_U8*)reg_cxt->g_buf[index].hw_regs;
        hw_regs = ( Vdpu34xH265dRegSet *)reg_cxt->g_buf[index].hw_regs;
    } else {
        p = (RK_U8*)reg_cxt->hw_regs;
        hw_regs = ( Vdpu34xH265dRegSet *)reg_cxt->hw_regs;
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

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->h265d_param;
        wr_cfg.size = sizeof(hw_regs->h265d_param);
        wr_cfg.offset = OFFSET_CODEC_PARAMS_REGS;

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->common_addr;
        wr_cfg.size = sizeof(hw_regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->h265d_addr;
        wr_cfg.size = sizeof(hw_regs->h265d_addr);
        wr_cfg.offset = OFFSET_CODEC_ADDR_REGS;

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->statistic;
        wr_cfg.size = sizeof(hw_regs->statistic);
        wr_cfg.offset = OFFSET_STATISTIC_REGS;

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3588) {
            wr_cfg.reg = &hw_regs->highpoc;
            wr_cfg.size = sizeof(hw_regs->highpoc);
            wr_cfg.offset = OFFSET_POC_HIGHBIT_REGS;

            ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_WR, &wr_cfg);
            if (ret) {
                mpp_err_f("set register write failed %d\n", ret);
                break;
            }
        }

        rd_cfg.reg = &hw_regs->irq_status;
        rd_cfg.size = sizeof(hw_regs->irq_status);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }
        /* rcb info for sram */
        {
            MppDevRcbInfoCfg rcb_cfg;
            Vdpu34xRcbInfo  rcb_info[RCB_BUF_COUNT];

            memcpy(rcb_info, reg_cxt->rcb_info, sizeof(rcb_info));
            qsort(rcb_info, MPP_ARRAY_ELEMS(rcb_info),
                  sizeof(rcb_info[0]), vdpu34x_compare_rcb_size);

            for (i = 0; i < MPP_ARRAY_ELEMS(rcb_info); i++) {
                rcb_cfg.reg_idx = rcb_info[i].reg;
                rcb_cfg.size = rcb_info[i].size;
                if (rcb_cfg.size > 0)
                    mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_RCB_INFO, &rcb_cfg);
                else
                    break;
            }
        }
        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    return ret;
}


static MPP_RET hal_h265d_vdpu34x_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_S32 index =  task->dec.reg_index;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;
    RK_U8* p = NULL;
    Vdpu34xH265dRegSet *hw_regs = NULL;
    RK_S32 i;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        goto ERR_PROC;
    }

    if (reg_cxt->fast_mode) {
        hw_regs = ( Vdpu34xH265dRegSet *)reg_cxt->g_buf[index].hw_regs;
    } else {
        hw_regs = ( Vdpu34xH265dRegSet *)reg_cxt->hw_regs;
    }

    p = (RK_U8*)hw_regs;

    ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

ERR_PROC:
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        hw_regs->irq_status.reg224.dec_error_sta ||
        hw_regs->irq_status.reg224.buf_empty_sta) {
        if (!reg_cxt->fast_mode) {
            if (reg_cxt->dec_cb)
                mpp_callback(reg_cxt->dec_cb, &task->dec);
        } else {
            MppFrame mframe = NULL;
            mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.output,
                                  SLOT_FRAME_PTR, &mframe);
            if (mframe) {
                reg_cxt->fast_mode_err_found = 1;
                mpp_frame_set_errinfo(mframe, 1);
            }
        }
    } else {
        if (reg_cxt->fast_mode && reg_cxt->fast_mode_err_found) {
            for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(task->dec.refer); i++) {
                if (task->dec.refer[i] >= 0) {
                    MppFrame frame_ref = NULL;

                    mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.refer[i],
                                          SLOT_FRAME_PTR, &frame_ref);
                    h265h_dbg(H265H_DBG_FAST_ERR, "refer[%d] %d frame %p\n",
                              i, task->dec.refer[i], frame_ref);
                    if (frame_ref && mpp_frame_get_errinfo(frame_ref)) {
                        MppFrame frame_out = NULL;
                        mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.output,
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

    if (reg_cxt->fast_mode) {
        reg_cxt->g_buf[index].use_flag = 0;
    }

    return ret;
}

static MPP_RET hal_h265d_vdpu34x_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalH265dCtx *p_hal = (HalH265dCtx *)hal;
    p_hal->fast_mode_err_found = 0;
    (void)hal;
    return ret;
}

static MPP_RET hal_h265d_vdpu34x_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

static MPP_RET hal_h265d_vdpu34x_control(void *hal, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    (void)param;
    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_OUTPUT_FORMAT: {
    } break;
    default:
        break;
    }
    return  ret;
}

const MppHalApi hal_h265d_vdpu34x = {
    .name = "h265d_vdpu34x",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(HalH265dCtx),
    .flag = 0,
    .init = hal_h265d_vdpu34x_init,
    .deinit = hal_h265d_vdpu34x_deinit,
    .reg_gen = hal_h265d_vdpu34x_gen_regs,
    .start = hal_h265d_vdpu34x_start,
    .wait = hal_h265d_vdpu34x_wait,
    .reset = hal_h265d_vdpu34x_reset,
    .flush = hal_h265d_vdpu34x_flush,
    .control = hal_h265d_vdpu34x_control,
};
