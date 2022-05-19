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

#define MODULE_TAG "hal_h265d_rkv"

#include <stdio.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_bitput.h"

#include "hal_h265d_debug.h"
#include "hal_h265d_ctx.h"
#include "hal_h265d_com.h"
#include "hal_h265d_rkv.h"
#include "hal_h265d_rkv_reg.h"
#include "h265d_syntax.h"

/* #define dump */
#ifdef dump
static FILE *fp = NULL;
#endif
#define HW_RPS

#define PPS_SIZE                (96 * 64)

static MPP_RET hal_h265d_alloc_res(void *hal)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;
    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            reg_cxt->g_buf[i].hw_regs =
                mpp_calloc_size(void, sizeof(H265d_REGS_t));
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].scaling_list_data,
                                 SCALING_LIST_SIZE);
            if (ret) {
                mpp_err("h265d scaling_list_data get buffer failed\n");
                return ret;
            }

            ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->g_buf[i].pps_data,
                                 PPS_SIZE);
            if (ret) {
                mpp_err("h265d pps_data get buffer failed\n");
                return ret;
            }

            ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->g_buf[i].rps_data,
                                 RPS_SIZE);
            if (ret) {
                mpp_err("h265d rps_data get buffer failed\n");
                return ret;
            }
        }
    } else {
        reg_cxt->hw_regs = mpp_calloc_size(void, sizeof(H265d_REGS_t));
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->scaling_list_data,
                             SCALING_LIST_SIZE);
        if (ret) {
            mpp_err("h265d scaling_list_data get buffer failed\n");
            return ret;
        }

        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->pps_data, PPS_SIZE);
        if (ret) {
            mpp_err("h265d pps_data get buffer failed\n");
            return ret;
        }

        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->rps_data, RPS_SIZE);
        if (ret) {
            mpp_err("h265d rps_data get buffer failed\n");
            return ret;
        }

    }
    return MPP_OK;
}

static MPP_RET hal_h265d_release_res(void *hal)
{
    RK_S32 ret = 0;
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;
    RK_S32 i = 0;
    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (reg_cxt->g_buf[i].scaling_list_data) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].scaling_list_data);
                if (ret) {
                    mpp_err("h265d scaling_list_data free buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].pps_data) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].pps_data);
                if (ret) {
                    mpp_err("h265d pps_data free buffer failed\n");
                    return ret;
                }
            }

            if (reg_cxt->g_buf[i].rps_data) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].rps_data);
                if (ret) {
                    mpp_err("h265d rps_data free buffer failed\n");
                    return ret;
                }
            }

            if (reg_cxt->g_buf[i].hw_regs) {
                mpp_free(reg_cxt->g_buf[i].hw_regs);
                reg_cxt->g_buf[i].hw_regs = NULL;
            }
        }
    } else {
        if (reg_cxt->scaling_list_data) {
            ret = mpp_buffer_put(reg_cxt->scaling_list_data);
            if (ret) {
                mpp_err("h265d scaling_list_data free buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->pps_data) {
            ret = mpp_buffer_put(reg_cxt->pps_data);
            if (ret) {
                mpp_err("h265d pps_data free buffer failed\n");
                return ret;
            }
        }

        if (reg_cxt->rps_data) {
            ret = mpp_buffer_put(reg_cxt->rps_data);
            if (ret) {
                mpp_err("h265d rps_data free buffer failed\n");
                return ret;
            }
        }

        if (reg_cxt->hw_regs) {
            mpp_free(reg_cxt->hw_regs);
            reg_cxt->hw_regs = NULL;
        }
    }
    return MPP_OK;
}

MPP_RET hal_h265d_rkv_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_NOK;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;

    mpp_slots_set_prop(reg_cxt->slots, SLOTS_HOR_ALIGN, hevc_hor_align);
    mpp_slots_set_prop(reg_cxt->slots, SLOTS_VER_ALIGN, hevc_ver_align);

    reg_cxt->scaling_qm = mpp_calloc(DXVA_Qmatrix_HEVC, 1);
    reg_cxt->sw_rps_buf = mpp_calloc(RK_U64, 400);

    if (reg_cxt->scaling_qm == NULL) {
        mpp_err("scaling_org alloc fail");
        return MPP_ERR_MALLOC;
    }

    reg_cxt->scaling_rk = mpp_calloc(scalingFactor_t, 1);
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

    ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->cabac_table_data, sizeof(cabac_table));
    if (ret) {
        mpp_err("h265d cabac_table get buffer failed\n");
        return ret;
    }

    ret = mpp_buffer_write(reg_cxt->cabac_table_data, 0, (void*)cabac_table, sizeof(cabac_table));
    if (ret) {
        mpp_err("h265d write cabac_table data failed\n");
        return ret;
    }

    ret = hal_h265d_alloc_res(hal);
    if (ret) {
        mpp_err("hal_h265d_alloc_res failed\n");
        return ret;
    }


    {
        // report hw_info to parser
        const MppSocInfo *info = mpp_get_soc_info();
        const void *hw_info = NULL;
        RK_U32 i = 0;

        for (i = 0; i < MPP_ARRAY_ELEMS(info->dec_caps); i++) {
            if (info->dec_caps[i] && ( info->dec_caps[i]->type == VPU_CLIENT_RKVDEC ||
                                       info->dec_caps[i]->type == VPU_CLIENT_HEVC_DEC)) {
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

MPP_RET hal_h265d_rkv_deinit(void *hal)
{
    RK_S32 ret = 0;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;

    ret = mpp_buffer_put(reg_cxt->cabac_table_data);
    if (ret) {
        mpp_err("h265d cabac_table free buffer failed\n");
        return ret;
    }

    if (reg_cxt->scaling_qm) {
        mpp_free(reg_cxt->scaling_qm);
    }

    if (reg_cxt->sw_rps_buf) {
        mpp_free(reg_cxt->sw_rps_buf);
    }

    if (reg_cxt->scaling_rk) {
        mpp_free(reg_cxt->scaling_rk);
    }

    hal_h265d_release_res(hal);

    if (reg_cxt->group) {
        ret = mpp_buffer_group_put(reg_cxt->group);
        if (ret) {
            mpp_err("h265d group free buffer failed\n");
            return ret;
        }
    }
    return MPP_OK;
}

static RK_S32 hal_h265d_v345_output_pps_packet(void *hal, void *dxva)
{
    RK_S32 fifo_len = 12;
    RK_S32 i, j;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height;
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    BitputCtx_t bp;
    RK_U64 *pps_packet = mpp_calloc(RK_U64, fifo_len + 1);

    if (NULL == reg_cxt || dxva_cxt == NULL) {
        mpp_err("%s:%s:%d reg_cxt or dxva_cxt is NULL",
                __FILE__, __FUNCTION__, __LINE__);
        MPP_FREE(pps_packet);
        return MPP_ERR_NULL_PTR;
    }

    void *pps_ptr = mpp_buffer_get_ptr(reg_cxt->pps_data);
    if (NULL == pps_ptr) {
        mpp_err("pps_data get ptr error");
        return MPP_ERR_NOMEM;
    }
    memset(pps_ptr, 0, 96 * 64);
    // pps_packet = (RK_U64 *)(pps_ptr + dxva_cxt->pp.pps_id * 80);

    for (i = 0; i < 12; i++) pps_packet[i] = 0;

    mpp_set_bitput_ctx(&bp, pps_packet, fifo_len);

    // SPS
    mpp_put_bits(&bp, dxva_cxt->pp.vps_id                            , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.sps_id                            , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.chroma_format_idc                 , 2);

    log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;
    width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

    mpp_put_bits(&bp, width                                          , 13);
    mpp_put_bits(&bp, height                                         , 13);
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
    mpp_put_bits(&bp, dxva_cxt->pp.num_ref_idx_l0_default_active_minus1 + 1  , 4);
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
    mpp_put_bits(&bp, 0, 4); //mSps_Pps[i]->mMode
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
            mpp_put_bits(&bp, column_width[j], 8);
        }

        for (j = 0; j < 22; j++) {
            if (row_height[j] > 0)
                row_height[j]--;
            mpp_put_bits(&bp, row_height[j], 8);
        }
    }

    {
        RK_U8 *ptr_scaling = (RK_U8 *)mpp_buffer_get_ptr(reg_cxt->scaling_list_data);
        RK_U32 fd = mpp_buffer_get_fd(reg_cxt->scaling_list_data);

        hal_h265d_output_scalinglist_packet(hal, ptr_scaling, dxva);
        mpp_put_bits(&bp, fd, 32);
        mpp_put_bits(&bp, 0, 60);
        mpp_put_align(&bp, 128, 0xf);
    }

    for (i = 0; i < 64; i++)
        memcpy(pps_ptr + i * 96, pps_packet, 96);

#ifdef dump
    fwrite(pps_ptr, 1, 80 * 64, fp);
    RK_U32 *tmp = (RK_U32 *)pps_ptr;
    for (i = 0; i < 96 / 4; i++) {
        h265h_dbg(H265H_DBG_PPS, "pps[%3d] = 0x%08x\n", i, tmp[i]);
    }
#endif
    MPP_FREE(pps_packet);
    return 0;
}

static RK_S32 hal_h265d_output_pps_packet(void *hal, void *dxva)
{
    RK_S32 fifo_len = 10;
    RK_S32 i, j;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height;
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    BitputCtx_t bp;
    RK_U64 *pps_packet = mpp_calloc(RK_U64, fifo_len + 1);

    if (NULL == reg_cxt || dxva_cxt == NULL) {
        mpp_err("%s:%s:%d reg_cxt or dxva_cxt is NULL",
                __FILE__, __FUNCTION__, __LINE__);
        MPP_FREE(pps_packet);
        return MPP_ERR_NULL_PTR;
    }

    void *pps_ptr = mpp_buffer_get_ptr(reg_cxt->pps_data);
    if (NULL == pps_ptr) {
        mpp_err("pps_data get ptr error");
        return MPP_ERR_NOMEM;
    }
    memset(pps_ptr, 0, 80 * 64);
    // pps_packet = (RK_U64 *)(pps_ptr + dxva_cxt->pp.pps_id * 80);

    for (i = 0; i < 10; i++) pps_packet[i] = 0;

    mpp_set_bitput_ctx(&bp, pps_packet, fifo_len);

    // SPS
    mpp_put_bits(&bp, dxva_cxt->pp.vps_id                            , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.sps_id                            , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.chroma_format_idc                 , 2);

    log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;
    width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

    mpp_put_bits(&bp, width                                          , 13);
    mpp_put_bits(&bp, height                                         , 13);
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
    mpp_put_align(&bp                                                         , 32, 0xf);

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
    mpp_put_bits(&bp, dxva_cxt->pp.loop_filter_across_tiles_enabled_flag       , 1);

    mpp_put_bits(&bp, dxva_cxt->pp.deblocking_filter_override_enabled_flag     , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_deblocking_filter_disabled_flag         , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_beta_offset_div2                        , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_tc_offset_div2                          , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.lists_modification_present_flag             , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.log2_parallel_merge_level_minus2 + 2        , 3);
    /*slice_segment_header_extension_present_flag need set 0 */
    mpp_put_bits(&bp, 0                                                        , 1);
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
            mpp_put_bits(&bp, column_width[j], 8);
        }

        for (j = 0; j < 22; j++) {
            if (row_height[j] > 0)
                row_height[j]--;
            mpp_put_bits(&bp, row_height[j], 8);
        }
    }

    {
        RK_U8 *ptr_scaling = (RK_U8 *)mpp_buffer_get_ptr(reg_cxt->scaling_list_data);
        RK_U32 fd = mpp_buffer_get_fd(reg_cxt->scaling_list_data);

        hal_h265d_output_scalinglist_packet(hal, ptr_scaling, dxva);
        mpp_put_bits(&bp, fd, 32);
        mpp_put_align(&bp, 64, 0xf);
    }

    for (i = 0; i < 64; i++)
        memcpy(pps_ptr + i * 80, pps_packet, 80);

#ifdef dump
    fwrite(pps_ptr, 1, 80 * 64, fp);
    fflush(fp);
#endif

    MPP_FREE(pps_packet);
    return 0;
}

static void update_stream_buffer(MppBuffer streambuf, HalTaskInfo *syn)
{
    h265d_dxva2_picture_context_t *dxva_cxt =
        (h265d_dxva2_picture_context_t *)syn->dec.syntax.data;
    RK_U8 *ptr = (RK_U8*)mpp_buffer_get_ptr(streambuf);
    RK_U8 bit_left = 0;
    RK_U16 start_byte, end_byte, i = 0;
    RK_U32 stream_size = dxva_cxt->bitstream_size;
    RK_U8 *buf = NULL;
    RK_U8 *temp = NULL;
    RK_U32 cut_byte = 0, cut_byte_acc = 0;

    for (i = 0; i < dxva_cxt->slice_count; i++) {
        if (dxva_cxt->slice_cut_param[i].is_enable) {

            bit_left = 8 - (dxva_cxt->slice_cut_param[i].start_bit & 0x7);
            start_byte = dxva_cxt->slice_cut_param[i].start_bit >> 3;
            end_byte = (dxva_cxt->slice_cut_param[i].end_bit + 7) >> 3;
            buf = ptr + (dxva_cxt->slice_short[i].BSNALunitDataLocation - cut_byte_acc);
            temp = buf + start_byte;

            h265h_dbg(H265H_DBG_FUNCTION, "start bit %d start byte[%d] 0x%x end bit %d end byte[%d] 0x%x\n",
                      dxva_cxt->slice_cut_param[i].start_bit, start_byte, buf[start_byte],
                      dxva_cxt->slice_cut_param[i].end_bit, end_byte, buf[end_byte]);
            if (bit_left < 8) {
                *temp = (*temp >> bit_left) << bit_left;
                *temp |= 1 << (bit_left - 1);
            } else {
                *temp = 0x80;
            }
            if ((dxva_cxt->slice_cut_param[i].end_bit & 0x7) == 0 && buf[end_byte] == 0x80)
                end_byte += 1;

            h265h_dbg(H265H_DBG_FUNCTION, "i %d location %d count %d SliceBytesInBuffer %d bitstream_size %d\n",
                      i, dxva_cxt->slice_short[i].BSNALunitDataLocation, dxva_cxt->slice_count,
                      dxva_cxt->slice_short[i].SliceBytesInBuffer, dxva_cxt->bitstream_size);

            memcpy(buf + start_byte + 1, buf + end_byte,
                   stream_size - dxva_cxt->slice_short[i].BSNALunitDataLocation - end_byte);

            cut_byte = end_byte - start_byte - 1;
            dxva_cxt->slice_short[i].SliceBytesInBuffer -= cut_byte;
            dxva_cxt->bitstream_size -= cut_byte;
            cut_byte_acc += cut_byte;
        }
    }
}

MPP_RET hal_h265d_rkv_gen_regs(void *hal,  HalTaskInfo *syn)
{
    RK_S32 i = 0;
    RK_S32 log2_min_cb_size;
    RK_S32 width, height;
    RK_S32 stride_y, stride_uv, virstrid_y, virstrid_yuv;
    H265d_REGS_t *hw_regs;
    RK_S32 ret = MPP_SUCCESS;
    MppBuffer streambuf = NULL;
    RK_S32 aglin_offset = 0;
    RK_S32 valid_ref = -1;
    MppBuffer framebuf = NULL;
    RK_U32 sw_ref_valid = 0;

    if (syn->dec.flags.parse_err ||
        syn->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    h265d_dxva2_picture_context_t *dxva_cxt =
        (h265d_dxva2_picture_context_t *)syn->dec.syntax.data;
    HalH265dCtx *reg_cxt = ( HalH265dCtx *)hal;

    void *rps_ptr = NULL;
    if (reg_cxt ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_cxt->g_buf[i].use_flag) {
                syn->dec.reg_index = i;
                reg_cxt->rps_data = reg_cxt->g_buf[i].rps_data;
                reg_cxt->scaling_list_data =
                    reg_cxt->g_buf[i].scaling_list_data;
                reg_cxt->pps_data = reg_cxt->g_buf[i].pps_data;
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
    rps_ptr = mpp_buffer_get_ptr(reg_cxt->rps_data);
    if (NULL == rps_ptr) {

        mpp_err("rps_data get ptr error");
        return MPP_ERR_NOMEM;
    }


    if (syn->dec.syntax.data == NULL) {
        mpp_err("%s:%s:%d dxva is NULL", __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

    /* output pps */
    if (reg_cxt->is_v345) {
        hal_h265d_v345_output_pps_packet(hal, syn->dec.syntax.data);
    } else {
        hal_h265d_output_pps_packet(hal, syn->dec.syntax.data);
    }

    if (NULL == reg_cxt->hw_regs) {
        return MPP_ERR_NULL_PTR;
    }

    hw_regs = (H265d_REGS_t*)reg_cxt->hw_regs;
    memset(hw_regs, 0, sizeof(H265d_REGS_t));

    log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;

    width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

    stride_y = ((MPP_ALIGN(width, 64)
                 * (dxva_cxt->pp.bit_depth_luma_minus8 + 8)) >> 3);
    stride_uv = ((MPP_ALIGN(width, 64)
                  * (dxva_cxt->pp.bit_depth_chroma_minus8 + 8)) >> 3);

    stride_y = hevc_hor_align(stride_y);
    stride_uv = hevc_hor_align(stride_uv);
    virstrid_y = hevc_ver_align(height) * stride_y;
    virstrid_yuv  = virstrid_y + stride_uv * hevc_ver_align(height) / 2;

    hw_regs->sw_picparameter.sw_slice_num = dxva_cxt->slice_count;
    hw_regs->sw_picparameter.sw_y_hor_virstride = stride_y >> 4;
    hw_regs->sw_picparameter.sw_uv_hor_virstride = stride_uv >> 4;
    hw_regs->sw_y_virstride = virstrid_y >> 4;
    hw_regs->sw_yuv_virstride = virstrid_yuv >> 4;
    hw_regs->sw_sysctrl.sw_h26x_rps_mode = 0;
    mpp_buf_slot_get_prop(reg_cxt->slots, dxva_cxt->pp.CurrPic.Index7Bits,
                          SLOT_BUFFER, &framebuf);
    hw_regs->sw_decout_base  = mpp_buffer_get_fd(framebuf); //just index need map

    /*if out_base is equal to zero it means this frame may error
    we return directly add by csy*/

    if (hw_regs->sw_decout_base == 0) {
        return 0;
    }

    hw_regs->sw_cur_poc = dxva_cxt->pp.CurrPicOrderCntVal;

    mpp_buf_slot_get_prop(reg_cxt->packet_slots, syn->dec.input, SLOT_BUFFER,
                          &streambuf);

    if ( dxva_cxt->bitstream == NULL) {
        dxva_cxt->bitstream = mpp_buffer_get_ptr(streambuf);
    }
    if (reg_cxt->is_v345) {
#ifdef HW_RPS
        hw_regs->sw_sysctrl.sw_wait_reset_en = 1;
        hw_regs->v345_reg_ends.reg064_mvc0.refp_layer_same_with_cur = 0xffff;
        hal_h265d_slice_hw_rps(syn->dec.syntax.data, rps_ptr, reg_cxt->sw_rps_buf, reg_cxt->fast_mode);
#else
        hw_regs->sw_sysctrl.sw_h26x_rps_mode = 1;
        hal_h265d_slice_output_rps(syn->dec.syntax.data, rps_ptr);
#endif
    } else {
        hal_h265d_slice_output_rps(syn->dec.syntax.data, rps_ptr);
    }

    if (dxva_cxt->pp.slice_segment_header_extension_present_flag && !reg_cxt->is_v345) {
        update_stream_buffer(streambuf, syn);
    }

    hw_regs->sw_cabactbl_base   =  mpp_buffer_get_fd(reg_cxt->cabac_table_data);
    hw_regs->sw_pps_base        =  mpp_buffer_get_fd(reg_cxt->pps_data);
    hw_regs->sw_rps_base        =  mpp_buffer_get_fd(reg_cxt->rps_data);
    hw_regs->sw_strm_rlc_base   =  mpp_buffer_get_fd(streambuf);
    hw_regs->sw_stream_len      = ((dxva_cxt->bitstream_size + 15)
                                   & (~15)) + 64;
    aglin_offset =  hw_regs->sw_stream_len - dxva_cxt->bitstream_size;
    if (aglin_offset > 0) {
        memset((void *)(dxva_cxt->bitstream + dxva_cxt->bitstream_size), 0,
               aglin_offset);
    }
    hw_regs->sw_interrupt.sw_dec_e         = 1;
    hw_regs->sw_interrupt.sw_dec_timeout_e = 1;
    hw_regs->sw_interrupt.sw_wr_ddr_align_en = dxva_cxt->pp.tiles_enabled_flag
                                               ? 0 : 1;

    ///find s->rps_model[i] position, and set register
    hw_regs->cabac_error_en = 0xfdfffffd;
    hw_regs->rkv_reg_ends.extern_error_en = 0x30000000;

    valid_ref = hw_regs->sw_decout_base;
    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_cxt->pp.RefPicList); i++) {
        if (dxva_cxt->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_cxt->pp.RefPicList[i].bPicEntry != 0x7f) {
            hw_regs->sw_refer_poc[i] = dxva_cxt->pp.PicOrderCntValList[i];
            mpp_buf_slot_get_prop(reg_cxt->slots,
                                  dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);
            if (framebuf != NULL) {
                hw_regs->sw_refer_base[i] = mpp_buffer_get_fd(framebuf);
                valid_ref = hw_regs->sw_refer_base[i];
            } else {
                hw_regs->sw_refer_base[i] = valid_ref;
            }
            sw_ref_valid          |=   (1 << i);
        } else {
            hw_regs->sw_refer_base[i] = hw_regs->sw_decout_base;
        }
    }

    if (sw_ref_valid) {
        mpp_dev_set_reg_offset(reg_cxt->dev, 10, sw_ref_valid & 0xf);
        mpp_dev_set_reg_offset(reg_cxt->dev, 11, ((sw_ref_valid >> 4) & 0xf));
        mpp_dev_set_reg_offset(reg_cxt->dev, 12, ((sw_ref_valid >> 8) & 0xf));
        mpp_dev_set_reg_offset(reg_cxt->dev, 13, ((sw_ref_valid >> 12) & 0xf));
    }

    return ret;
}

MPP_RET hal_h265d_rkv_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    H265d_REGS_t *hw_regs = NULL;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;
    RK_S32 index =  task->dec.reg_index;

    RK_U32 i;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    if (reg_cxt->fast_mode) {
        hw_regs = ( H265d_REGS_t *)reg_cxt->g_buf[index].hw_regs;
    } else {
        hw_regs = ( H265d_REGS_t *)reg_cxt->hw_regs;
    }

    if (hw_regs == NULL) {
        mpp_err("hal_h265d_start hw_regs is NULL");
        return MPP_ERR_NULL_PTR;
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = (reg_cxt->is_v345) ? V345_HEVC_REGISTERS :
                          (reg_cxt->client_type == VPU_CLIENT_RKVDEC) ?
                          RKVDEC_V1_REGISTERS : RKVDEC_HEVC_REGISTERS;

        reg_size *= sizeof(RK_U32);

        wr_cfg.reg = hw_regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        if (hal_h265d_debug & H265H_DBG_REG) {
            for (i = 0; i < reg_size / sizeof(RK_U32); i++)
                h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n", i, ((RK_U32 *)hw_regs)[i]);
        }

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = hw_regs;
        rd_cfg.size = reg_size;
        rd_cfg.offset = 0;

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    return ret;
}

MPP_RET hal_h265d_rkv_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_S32 index =  task->dec.reg_index;
    HalH265dCtx *reg_cxt = (HalH265dCtx *)hal;
    H265d_REGS_t *hw_regs = NULL;
    RK_S32 i;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        goto ERR_PROC;
    }

    if (reg_cxt->fast_mode) {
        hw_regs = ( H265d_REGS_t *)reg_cxt->g_buf[index].hw_regs;
    } else {
        hw_regs = ( H265d_REGS_t *)reg_cxt->hw_regs;
    }

    ret = mpp_dev_ioctl(reg_cxt->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

ERR_PROC:
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        hw_regs->sw_interrupt.sw_dec_error_sta ||
        hw_regs->sw_interrupt.sw_dec_timeout_sta ||
        hw_regs->sw_interrupt.sw_dec_empty_sta) {
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

    if (hal_h265d_debug & H265H_DBG_REG) {
        h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[1]=0x%08X, regs[45]=0x%08x\n", ((RK_U32 *)hw_regs)[1], ((RK_U32 *)hw_regs)[45]);
    }

    if (reg_cxt->fast_mode) {
        reg_cxt->g_buf[index].use_flag = 0;
    }

    return ret;
}

MPP_RET hal_h265d_rkv_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalH265dCtx *p_hal = (HalH265dCtx *)hal;

    p_hal->fast_mode_err_found = 0;

    return ret;
}

const MppHalApi hal_h265d_rkv = {
    .name = "h265d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(HalH265dCtx),
    .flag = 0,
    .init = hal_h265d_rkv_init,
    .deinit = hal_h265d_rkv_deinit,
    .reg_gen = hal_h265d_rkv_gen_regs,
    .start = hal_h265d_rkv_start,
    .wait = hal_h265d_rkv_wait,
    .reset = hal_h265d_rkv_reset,
    .flush = NULL,
    .control = NULL,
};
