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

#define MODULE_TAG "h264e_slice"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_bitwrite.h"

#include "h264e_debug.h"
#include "h264e_slice.h"
#include "h264e_dpb.h"

#define FP_FIFO_IS_FULL

void h264e_slice_init(H264eSlice *slice, H264eReorderInfo *reorder,
                      H264eMarkingInfo *marking)
{
    memset(slice, 0, sizeof(*slice));

    slice->num_ref_idx_active = 1;

    slice->reorder = reorder;
    slice->marking = marking;
}

RK_S32 h264e_slice_update(H264eSlice *slice, MppEncCfgSet *cfg,
                          H264eSps *sps, H264ePps *pps, H264eDpbFrm *frm)
{
    MppEncH264Cfg *h264 = &cfg->h264;
    RK_S32 is_idr = frm->status.is_idr;

    slice->mb_w = sps->pic_width_in_mbs;
    slice->mb_h = sps->pic_height_in_mbs;
    slice->max_num_ref_frames = sps->num_ref_frames;
    slice->log2_max_frame_num = sps->log2_max_frame_num_minus4 + 4;
    slice->log2_max_poc_lsb = sps->log2_max_poc_lsb_minus4 + 4;
    slice->entropy_coding_mode = h264->entropy_coding_mode;
    slice->pic_order_cnt_type = sps->pic_order_cnt_type;
    slice->qp_init = pps->pic_init_qp;

    slice->nal_reference_idc = (frm->status.is_non_ref) ? (H264_NALU_PRIORITY_DISPOSABLE) :
                               (is_idr) ? (H264_NALU_PRIORITY_HIGHEST) :
                               (H264_NALU_PRIORITY_HIGH);
    slice->nalu_type = (is_idr) ? (H264_NALU_TYPE_IDR) : (H264_NALU_TYPE_SLICE);

    slice->first_mb_in_slice = 0;
    slice->slice_type = (is_idr) ? (H264_I_SLICE) : (H264_P_SLICE);
    slice->pic_parameter_set_id = 0;
    slice->frame_num = frm->frame_num;
    slice->num_ref_idx_override = 0;
    slice->qp_delta = 0;
    slice->cabac_init_idc = h264->entropy_coding_mode ? h264->cabac_init_idc : -1;
    slice->disable_deblocking_filter_idc = h264->deblock_disable;
    slice->slice_alpha_c0_offset_div2 = h264->deblock_offset_alpha;
    slice->slice_beta_offset_div2 = h264->deblock_offset_beta;

    slice->idr_flag = is_idr;

    if (slice->idr_flag) {
        slice->idr_pic_id = slice->next_idr_pic_id;
        slice->next_idr_pic_id++;
        if (slice->next_idr_pic_id >= 16)
            slice->next_idr_pic_id = 0;
    }

    slice->pic_order_cnt_lsb = frm->poc & ((1 << slice->log2_max_poc_lsb) - 1);
    slice->num_ref_idx_active = 1;
    slice->no_output_of_prior_pics = 0;
    if (slice->idr_flag)
        slice->long_term_reference_flag = frm->status.is_lt_ref;
    else
        slice->long_term_reference_flag = 0;

    return MPP_OK;
}

MPP_RET h264e_reorder_init(H264eReorderInfo *reorder)
{
    reorder->size = H264E_MAX_REFS_CNT;
    reorder->rd_cnt = 0;
    reorder->wr_cnt = 0;

    return MPP_OK;
}

MPP_RET h264e_reorder_wr_rewind(H264eReorderInfo *info)
{
    info->wr_cnt = 0;
    return MPP_OK;
}

MPP_RET h264e_reorder_rd_rewind(H264eReorderInfo *info)
{
    info->rd_cnt = 0;
    return MPP_OK;
}

MPP_RET h264e_reorder_wr_op(H264eReorderInfo *info, H264eRplmo *op)
{
    if (info->wr_cnt >= info->size) {
        mpp_err_f("write too many reorder op %d vs %d\n",
                  info->wr_cnt, info->size);
        return MPP_NOK;
    }

    info->ops[info->wr_cnt++] = *op;
    return MPP_OK;
}

MPP_RET h264e_reorder_rd_op(H264eReorderInfo *info, H264eRplmo *op)
{
    if (info->rd_cnt >= info->wr_cnt)
        return MPP_NOK;

    *op = info->ops[info->rd_cnt++];
    return MPP_OK;
}

MPP_RET h264e_marking_init(H264eMarkingInfo *marking)
{
    marking->idr_flag = 0;
    marking->no_output_of_prior_pics = 0;
    marking->long_term_reference_flag = 0;
    marking->adaptive_ref_pic_buffering = 0;
    marking->size = MAX_H264E_MMCO_CNT;
    marking->wr_cnt = 0;
    marking->rd_cnt = 0;

    return MPP_OK;
}

RK_S32 h264e_marking_is_empty(H264eMarkingInfo *info)
{
    return info->rd_cnt >= info->wr_cnt;
}

MPP_RET h264e_marking_wr_rewind(H264eMarkingInfo *marking)
{
    marking->wr_cnt = 0;
    return MPP_OK;
}

MPP_RET h264e_marking_rd_rewind(H264eMarkingInfo *marking)
{
    marking->rd_cnt = 0;
    return MPP_OK;
}

MPP_RET h264e_marking_wr_op(H264eMarkingInfo *info, H264eMmco *op)
{
    if (info->wr_cnt >= info->size) {
        mpp_err_f("write too many mmco op %d vs %d\n",
                  info->wr_cnt, info->size);
        return MPP_NOK;
    }

    info->ops[info->wr_cnt++] = *op;
    return MPP_OK;
}

MPP_RET h264e_marking_rd_op(H264eMarkingInfo *info, H264eMmco *op)
{
    if (h264e_marking_is_empty(info))
        return MPP_NOK;

    *op = info->ops[info->rd_cnt++];
    return MPP_OK;
}

void write_marking(MppWriteCtx *s, H264eMarkingInfo *marking)
{
    if (marking->idr_flag) {
        /* no_output_of_prior_pics_flag */
        mpp_writer_put_bits(s, marking->no_output_of_prior_pics, 1);
        h264e_dbg_slice("used bit %2d no_output_of_prior_pics_flag %d\n",
                        mpp_writer_bits(s), marking->no_output_of_prior_pics);

        /* long_term_reference_flag */
        mpp_writer_put_bits(s, marking->long_term_reference_flag, 1);
        h264e_dbg_slice("used bit %2d long_term_reference_flag %d\n",
                        mpp_writer_bits(s), marking->long_term_reference_flag);
    } else {
        h264e_dbg_mmco("mmco count %d\n", marking->wr_cnt);

        h264e_marking_rd_rewind(marking);

        if (!h264e_marking_is_empty(marking)) {
            H264eMmco mmco;

            /* adaptive_ref_pic_marking_mode_flag */
            mpp_writer_put_bits(s, 1, 1);
            h264e_dbg_slice("used bit %2d adaptive_ref_pic_marking_mode_flag 1\n",
                            mpp_writer_bits(s));

            while (MPP_OK == h264e_marking_rd_op(marking, &mmco)) {
                /* memory_management_control_operation */
                mpp_writer_put_ue(s, mmco.mmco);
                h264e_dbg_slice("used bit %2d memory_management_control_operation %d\n",
                                mpp_writer_bits(s), mmco.mmco);

                switch (mmco.mmco) {
                case 1 : {
                    /* difference_of_pic_nums_minus1 */
                    mpp_writer_put_ue(s, mmco.difference_of_pic_nums_minus1);
                    h264e_dbg_slice("used bit %2d difference_of_pic_nums_minus1 %d\n",
                                    mpp_writer_bits(s), mmco.difference_of_pic_nums_minus1);
                } break;
                case 2 : {
                    /* long_term_pic_num */
                    mpp_writer_put_ue(s, mmco.long_term_pic_num );
                    h264e_dbg_slice("used bit %2d long_term_pic_num %d\n",
                                    mpp_writer_bits(s), mmco.long_term_pic_num);
                } break;
                case 3 : {
                    /* difference_of_pic_nums_minus1 */
                    mpp_writer_put_ue(s, mmco.difference_of_pic_nums_minus1);
                    h264e_dbg_slice("used bit %2d difference_of_pic_nums_minus1 %d\n",
                                    mpp_writer_bits(s), mmco.difference_of_pic_nums_minus1);

                    /* long_term_frame_idx */
                    mpp_writer_put_ue(s, mmco.long_term_frame_idx );
                    h264e_dbg_slice("used bit %2d long_term_frame_idx %d\n",
                                    mpp_writer_bits(s), mmco.long_term_frame_idx);
                } break;
                case 4 : {
                    /* max_long_term_frame_idx_plus1 */
                    mpp_writer_put_ue(s, mmco.max_long_term_frame_idx_plus1);
                    h264e_dbg_slice("used bit %2d max_long_term_frame_idx_plus1 %d\n",
                                    mpp_writer_bits(s), mmco.max_long_term_frame_idx_plus1);
                } break;
                case 5 : {
                } break;
                case 6 : {
                    /* long_term_frame_idx */
                    mpp_writer_put_ue(s, mmco.long_term_frame_idx);
                    h264e_dbg_slice("used bit %2d long_term_frame_idx %d\n",
                                    mpp_writer_bits(s), mmco.long_term_frame_idx);
                } break;
                default : {
                    mpp_err_f("invalid mmco %d\n", mmco.mmco);
                } break;
                }
            }

            /* memory_management_control_operation */
            mpp_writer_put_ue(s, 0);
            h264e_dbg_slice("used bit %2d memory_management_control_operation 0\n",
                            mpp_writer_bits(s));
        } else {
            /* adaptive_ref_pic_marking_mode_flag */
            mpp_writer_put_bits(s, 0, 1);
            h264e_dbg_slice("used bit %2d adaptive_ref_pic_marking_mode_flag 0\n",
                            mpp_writer_bits(s));
        }
    }
}

/* vepu read slice */
RK_S32 h264e_slice_read(H264eSlice *slice, void *p, RK_S32 size)
{
    BitReadCtx_t bit;
    RK_S32 ret = 0;
    RK_S32 val = 0;
    RK_S32 bit_cnt = 0;

    mpp_set_bitread_ctx(&bit, p, size);
    /* enable remove 03 */
    mpp_set_bitread_pseudo_code_type(&bit, PSEUDO_CODE_H264_H265);

    /* start_code */
    ret |= mpp_read_longbits(&bit, 32, (RK_U32 *)&val);
    h264e_dbg_slice("used bit %2d start_code %x\n",
                    bit.used_bits, val);

    /* forbidden_zero_bit */
    ret |= mpp_read_bits(&bit, 1, &val);
    h264e_dbg_slice("used bit %2d forbidden_zero_bit %x\n",
                    bit.used_bits, val);

    /* nal_ref_idc */
    ret |= mpp_read_bits(&bit, 2, &slice->nal_reference_idc);
    h264e_dbg_slice("used bit %2d nal_reference_idc %d\n",
                    bit.used_bits, slice->nal_reference_idc);

    /* nal_unit_type */
    ret |= mpp_read_bits(&bit, 5, &slice->nalu_type);
    h264e_dbg_slice("used bit %2d nal_unit_type %d\n",
                    bit.used_bits, slice->nalu_type);

    /* first_mb_nr */
    ret = mpp_read_ue(&bit, &slice->first_mb_in_slice);
    h264e_dbg_slice("used bit %2d first_mb_in_slice %d\n",
                    bit.used_bits, slice->first_mb_in_slice);

    /* slice_type */
    ret |= mpp_read_ue(&bit, &slice->slice_type);
    h264e_dbg_slice("used bit %2d slice_type %d\n",
                    bit.used_bits, slice->slice_type);

    /* pic_parameter_set_id */
    ret |= mpp_read_ue(&bit, &slice->pic_parameter_set_id);
    h264e_dbg_slice("used bit %2d pic_parameter_set_id %d\n",
                    bit.used_bits, slice->pic_parameter_set_id);

    /* frame_num */
    /* NOTE: vpu hardware fix 16 bit frame_num */
    ret |= mpp_read_bits(&bit, slice->log2_max_frame_num, &slice->frame_num);
    h264e_dbg_slice("used bit %2d frame_num %d\n",
                    bit.used_bits, slice->frame_num);

    slice->idr_flag = (slice->nalu_type == 5);
    if (slice->idr_flag) {
        /* idr_pic_id */
        ret |= mpp_read_ue(&bit, &slice->idr_pic_id);
        h264e_dbg_slice("used bit %2d idr_pic_id %d\n",
                        bit.used_bits, slice->idr_pic_id);
    }

    /* pic_order_cnt_type */
    if (slice->pic_order_cnt_type == 0) {
        /* pic_order_cnt_lsb */
        ret |= mpp_read_bits(&bit, slice->log2_max_poc_lsb,
                             (RK_S32 *)&slice->pic_order_cnt_lsb);
        h264e_dbg_slice("used bit %2d pic_order_cnt_lsb %d\n",
                        bit.used_bits, slice->pic_order_cnt_lsb);
    }

    // NOTE: Only P slice has num_ref_idx_override flag and ref_pic_list_modification flag
    if (slice->slice_type == H264_P_SLICE) {
        /* num_ref_idx_override */
        ret |= mpp_read_bits(&bit, 1, &slice->num_ref_idx_override);
        h264e_dbg_slice("used bit %2d num_ref_idx_override %d\n",
                        bit.used_bits, slice->num_ref_idx_override);

        mpp_assert(slice->num_ref_idx_override == 0);

        // NOTE: vpu hardware is always zero
        /* ref_pic_list_modification_flag */
        ret |= mpp_read_bits(&bit, 1, &slice->ref_pic_list_modification_flag);
        h264e_dbg_slice("used bit %2d ref_pic_list_modification_flag %d\n",
                        bit.used_bits, slice->ref_pic_list_modification_flag);

        if (slice->ref_pic_list_modification_flag) {
            RK_U32 modification_of_pic_nums_idc = 0;
            H264eRplmo ops;

            h264e_reorder_wr_rewind(slice->reorder);

            do {
                /* modification_of_pic_nums_idc */
                ret |= mpp_read_ue(&bit, &modification_of_pic_nums_idc);
                ops.modification_of_pic_nums_idc = modification_of_pic_nums_idc;
                h264e_dbg_slice("used bit %2d modification_of_pic_nums_idc %d\n",
                                bit.used_bits, modification_of_pic_nums_idc);

                switch (modification_of_pic_nums_idc) {
                case 0 :
                case 1 : {
                    /* abs_diff_pic_num_minus1 */
                    RK_U32 abs_diff_pic_num_minus1 = 0;

                    ret |= mpp_read_ue(&bit, &abs_diff_pic_num_minus1);
                    ops.abs_diff_pic_num_minus1 = abs_diff_pic_num_minus1;
                    h264e_dbg_slice("used bit %2d abs_diff_pic_num_minus1 %d\n",
                                    bit.used_bits, abs_diff_pic_num_minus1);
                } break;
                case 2 : {
                    /* long_term_pic_idx */
                    RK_U32 long_term_pic_idx = 0;

                    ret |= mpp_read_ue(&bit, &long_term_pic_idx);
                    ops.long_term_pic_idx = long_term_pic_idx;
                    h264e_dbg_slice("used bit %2d long_term_pic_idx %d\n",
                                    bit.used_bits, long_term_pic_idx);
                } break;
                case 3 : {
                } break;
                default : {
                    mpp_err_f("invalid modification_of_pic_nums_idc %d\n",
                              modification_of_pic_nums_idc);
                } break;
                }

                h264e_reorder_wr_op(slice->reorder, &ops);
                memset(&ops, 0, sizeof(ops));
            } while (modification_of_pic_nums_idc != 3);
        }
    }

    if (slice->nal_reference_idc) {
        if (slice->idr_flag) {
            /* no_output_of_prior_pics */
            ret |= mpp_read_bits(&bit, 1, &slice->no_output_of_prior_pics);
            slice->marking->no_output_of_prior_pics = slice->no_output_of_prior_pics;
            h264e_dbg_slice("used bit %2d no_output_of_prior_pics %d\n",
                            bit.used_bits, slice->no_output_of_prior_pics);

            /* long_term_reference_flag */
            ret |= mpp_read_bits(&bit, 1, &slice->long_term_reference_flag);
            slice->marking->long_term_reference_flag = slice->long_term_reference_flag;
            h264e_dbg_slice("used bit %2d long_term_reference_flag %d\n",
                            bit.used_bits, slice->long_term_reference_flag);
        } else {
            /* adaptive_ref_pic_buffering */
            ret |= mpp_read_bits(&bit, 1, &slice->adaptive_ref_pic_buffering);
            slice->marking->adaptive_ref_pic_buffering = slice->adaptive_ref_pic_buffering;
            h264e_dbg_slice("used bit %2d adaptive_ref_pic_buffering %d\n",
                            bit.used_bits, slice->adaptive_ref_pic_buffering);

            if (slice->adaptive_ref_pic_buffering) {
                RK_U32 mmco;
                H264eMmco opt;

                h264e_marking_wr_rewind(slice->marking);

                do {
                    ret |= mpp_read_ue(&bit, &mmco);
                    opt.mmco = mmco;
                    h264e_dbg_slice("used bit %2d memory_management_control_operation %d\n",
                                    bit.used_bits, mmco);

                    // avoidance of writing extra struct H264eMmco
                    if (mmco == 0)
                        break;

                    if (mmco == 1 || mmco == 3) {
                        RK_U32 difference_of_pic_nums_minus1;

                        ret |= mpp_read_ue(&bit, &difference_of_pic_nums_minus1);
                        opt.difference_of_pic_nums_minus1 = difference_of_pic_nums_minus1;
                        h264e_dbg_slice("used bit %2d difference_of_pic_nums_minus1 %d\n",
                                        bit.used_bits, difference_of_pic_nums_minus1);
                    }

                    if (mmco == 2) {
                        RK_U32 long_term_pic_num;

                        ret |= mpp_read_ue(&bit, &long_term_pic_num);
                        opt.long_term_pic_num = long_term_pic_num;
                        h264e_dbg_slice("used bit %2d long_term_pic_num %d\n",
                                        bit.used_bits, long_term_pic_num);
                    }

                    if (mmco == 3 || mmco == 6) {
                        RK_U32 long_term_frame_idx;

                        ret |= mpp_read_ue(&bit, &long_term_frame_idx);
                        opt.long_term_frame_idx = long_term_frame_idx;
                        h264e_dbg_slice("used bit %2d long_term_frame_idx %d\n",
                                        bit.used_bits, long_term_frame_idx);
                    }

                    if (mmco == 4) {
                        RK_U32 max_long_term_frame_idx_plus1;

                        ret |= mpp_read_ue(&bit, &max_long_term_frame_idx_plus1);
                        opt.max_long_term_frame_idx_plus1 = max_long_term_frame_idx_plus1;
                        h264e_dbg_slice("used bit %2d max_long_term_frame_idx_plus1 %d\n",
                                        bit.used_bits, max_long_term_frame_idx_plus1);
                    }

                    h264e_marking_wr_op(slice->marking, &opt);
                    memset(&opt, 0, sizeof(opt));
                } while (mmco);
            }
        }
    }

    if (slice->entropy_coding_mode && slice->slice_type != H264_I_SLICE) {
        /* cabac_init_idc */
        ret |= mpp_read_ue(&bit, &slice->cabac_init_idc);
        h264e_dbg_slice("used bit %2d cabac_init_idc %d\n",
                        bit.used_bits, slice->cabac_init_idc);
    }

    /* qp_delta */
    ret |= mpp_read_se(&bit, &slice->qp_delta);
    h264e_dbg_slice("used bit %2d qp_delta %d\n",
                    bit.used_bits, slice->qp_delta);

    /* disable_deblocking_filter_idc */
    ret |= mpp_read_ue(&bit, &slice->disable_deblocking_filter_idc);
    h264e_dbg_slice("used bit %2d disable_deblocking_filter_idc %d\n",
                    bit.used_bits, slice->disable_deblocking_filter_idc);

    /* slice_alpha_c0_offset_div2 */
    ret |= mpp_read_se(&bit, &slice->slice_alpha_c0_offset_div2);
    h264e_dbg_slice("used bit %2d slice_alpha_c0_offset_div2 %d\n",
                    bit.used_bits, slice->slice_alpha_c0_offset_div2);

    /* slice_beta_offset_div2 */
    ret |= mpp_read_se(&bit, &slice->slice_beta_offset_div2);
    h264e_dbg_slice("used bit %2d slice_beta_offset_div2 %d\n",
                    bit.used_bits, slice->slice_beta_offset_div2);

    h264e_dbg_slice("used bit %2d non-aligned length\n", bit.used_bits);

    if (slice->entropy_coding_mode) {
        if (bit.num_remaining_bits_in_curr_byte_) {
            RK_U32 tmp = bit.num_remaining_bits_in_curr_byte_;

            /* cabac_aligned_bit */
            ret |= mpp_read_bits(&bit, tmp, &val);
            h264e_dbg_slice("used bit %2d cabac_aligned_bit %x\n",
                            bit.used_bits, val);
        }
    }
    bit_cnt = bit.used_bits;

    h264e_dbg_slice("used bit %2d total aligned length, read result %d\n", bit.used_bits, ret);

    if (h264e_debug & H264E_DBG_SLICE) {
        RK_S32 pos = 0;
        RK_S32 i;
        char log[256];
        RK_U8 *tmp = (RK_U8 *)p;

        pos = sprintf(log + pos, "hw stream: ");
        for (i = 0; i < 16; i++) {
            pos += sprintf(log + pos, "%02x ", tmp[i]);
        }
        pos += sprintf(log + pos, "\n");
        h264e_dbg_slice(log);
    }

    return bit_cnt;
}

void h264e_slice_write_header(H264eSlice *slice, MppWriteCtx *s)
{
    H264eMarkingInfo *marking = slice->marking;
    H264eRplmo rplmo;
    MPP_RET ret = MPP_OK;

    /* nal header */
    /* start_code_prefix 00 00 00 01 */
    mpp_writer_put_raw_bits(s, 0, 24);
    mpp_writer_put_raw_bits(s, 1, 8);
    h264e_dbg_slice("used bit %2d start_code_prefix\n",
                    mpp_writer_bits(s));

    /* forbidden_zero_bit */
    mpp_writer_put_raw_bits(s, 0, 1);
    h264e_dbg_slice("used bit %2d forbidden_zero_bit\n",
                    mpp_writer_bits(s));

    /* nal_reference_idc */
    mpp_writer_put_raw_bits(s, slice->nal_reference_idc, 2);
    h264e_dbg_slice("used bit %2d nal_reference_idc %d\n",
                    mpp_writer_bits(s), slice->nal_reference_idc);
    /* nalu_type */
    mpp_writer_put_raw_bits(s, slice->nalu_type, 5);
    h264e_dbg_slice("used bit %2d nalu_type %d\n",
                    mpp_writer_bits(s), slice->nalu_type);

    /* slice header */
    /* start_mb_nr */
    mpp_writer_put_ue(s, slice->first_mb_in_slice);
    h264e_dbg_slice("used bit %2d first_mb_in_slice %d\n",
                    mpp_writer_bits(s), slice->first_mb_in_slice);

    /* slice_type */
    mpp_writer_put_ue(s, slice->slice_type);
    h264e_dbg_slice("used bit %2d slice_type %d\n",
                    mpp_writer_bits(s), slice->slice_type);

    /* pic_parameter_set_id */
    mpp_writer_put_ue(s, slice->pic_parameter_set_id);
    h264e_dbg_slice("used bit %2d pic_parameter_set_id %d\n",
                    mpp_writer_bits(s), slice->pic_parameter_set_id);

    /* frame_num */
    mpp_writer_put_bits(s, slice->frame_num, slice->log2_max_frame_num);
    h264e_dbg_slice("used bit %2d frame_num %d\n",
                    mpp_writer_bits(s), slice->frame_num);

    if (slice->nalu_type == 5) {
        /* idr_pic_id */
        mpp_writer_put_ue(s, slice->idr_pic_id);
        h264e_dbg_slice("used bit %2d idr_pic_id %d\n",
                        mpp_writer_bits(s), slice->idr_pic_id);
        marking->idr_flag = 1;
    } else
        marking->idr_flag = 0;

    // Force to use poc type 0 here
    if (slice->pic_order_cnt_type == 0) {
        RK_S32 pic_order_cnt_lsb = slice->pic_order_cnt_lsb;
        RK_S32 max_poc_lsb = (1 << slice->log2_max_poc_lsb);

        if (pic_order_cnt_lsb >= max_poc_lsb)
            pic_order_cnt_lsb &= (max_poc_lsb - 1);

        /* pic_order_cnt_lsb */
        mpp_writer_put_bits(s, pic_order_cnt_lsb, slice->log2_max_poc_lsb);
        h264e_dbg_slice("used bit %2d pic_order_cnt_lsb %d\n",
                        mpp_writer_bits(s), pic_order_cnt_lsb);
    } else {
        mpp_assert(slice->pic_order_cnt_type == 2);
    }

    /* num_ref_idx_override */
    slice->ref_pic_list_modification_flag = 0;

    h264e_reorder_rd_rewind(slice->reorder);

    if (slice->slice_type == H264_P_SLICE) {
        mpp_assert(slice->num_ref_idx_override == 0);

        mpp_writer_put_bits(s, slice->num_ref_idx_override, 1);
        h264e_dbg_slice("used bit %2d num_ref_idx_override %d\n",
                        mpp_writer_bits(s), slice->num_ref_idx_override);


        /* read reorder and check */
        ret = h264e_reorder_rd_op(slice->reorder, &rplmo);

        /* ref_pic_list_modification_flag */
        slice->ref_pic_list_modification_flag = (ret == MPP_OK);
        mpp_writer_put_bits(s, slice->ref_pic_list_modification_flag, 1);
        h264e_dbg_slice("used bit %2d ref_pic_list_modification_flag %d\n",
                        mpp_writer_bits(s), slice->ref_pic_list_modification_flag);

        if (slice->ref_pic_list_modification_flag) {
            /* modification_of_pic_nums_idc */
            mpp_writer_put_ue(s, rplmo.modification_of_pic_nums_idc);
            h264e_dbg_slice("used bit %2d modification_of_pic_nums_idc %d\n",
                            mpp_writer_bits(s),
                            rplmo.modification_of_pic_nums_idc);

            switch (rplmo.modification_of_pic_nums_idc) {
            case 0 :
            case 1 : {
                /* abs_diff_pic_num_minus1 */
                mpp_writer_put_ue(s, rplmo.abs_diff_pic_num_minus1);
                h264e_dbg_slice("used bit %2d abs_diff_pic_num_minus1 %d\n",
                                mpp_writer_bits(s),
                                rplmo.abs_diff_pic_num_minus1);
            } break;
            case 2 : {
                /* long_term_pic_idx */
                mpp_writer_put_ue(s, rplmo.long_term_pic_idx);
                h264e_dbg_slice("used bit %2d long_term_pic_idx %d\n",
                                mpp_writer_bits(s),
                                rplmo.long_term_pic_idx);
            } break;
            default : {
                mpp_err_f("invalid modification_of_pic_nums_idc %d\n",
                          rplmo.modification_of_pic_nums_idc);
            } break;
            }

            /* modification_of_pic_nums_idc */
            mpp_writer_put_ue(s, 3);
            h264e_dbg_slice("used bit %2d modification_of_pic_nums_idc 3\n",
                            mpp_writer_bits(s));

        }
    }

    // NOTE: ignore nal ref idc here
    h264e_dbg_mmco("nal_reference_idc %d idr_flag %d\n",
                   slice->nal_reference_idc, slice->idr_flag);

    if (slice->nal_reference_idc) {
        /* In VEPU1/2, lt_ref_flag is fixed to 0, so it must be replaced by data in slice. */
        if (slice->long_term_reference_flag != marking->long_term_reference_flag) {
            marking->long_term_reference_flag = slice->long_term_reference_flag;
            h264e_dbg_slice("warning: update lt_ref_flag from %d to %d\n",
                            marking->long_term_reference_flag, slice->long_term_reference_flag);
        }

        h264e_dbg_slice("get marking %p\n", marking);
        write_marking(s, marking);
    }

    if (slice->entropy_coding_mode && slice->slice_type != H264_I_SLICE) {
        /* cabac_init_idc */
        mpp_writer_put_ue(s, slice->cabac_init_idc);
        h264e_dbg_slice("used bit %2d cabac_init_idc %d\n",
                        mpp_writer_bits(s), slice->cabac_init_idc);
    }

    /* qp_delta */
    mpp_writer_put_se(s, slice->qp_delta);
    h264e_dbg_slice("used bit %2d qp_delta %d\n",
                    mpp_writer_bits(s), slice->qp_delta);

    /* disable_deblocking_filter_idc */
    mpp_writer_put_ue(s, slice->disable_deblocking_filter_idc);
    h264e_dbg_slice("used bit %2d disable_deblocking_filter_idc %d\n",
                    mpp_writer_bits(s), slice->disable_deblocking_filter_idc);

    /* slice_alpha_c0_offset_div2 */
    mpp_writer_put_se(s, slice->slice_alpha_c0_offset_div2);
    h264e_dbg_slice("used bit %2d slice_alpha_c0_offset_div2 %d\n",
                    mpp_writer_bits(s), slice->slice_alpha_c0_offset_div2);

    /* slice_beta_offset_div2 */
    mpp_writer_put_se(s, slice->slice_beta_offset_div2);
    h264e_dbg_slice("used bit %2d slice_beta_offset_div2 %d\n",
                    mpp_writer_bits(s), slice->slice_beta_offset_div2);

    /* cabac_alignment_one_bit */
    if (slice->entropy_coding_mode) {
        mpp_writer_align_one(s);
        h264e_dbg_slice("used bit %2d align_bit 1\n",
                        mpp_writer_bits(s));
    }

    mpp_writer_flush(s);
}

RK_S32 h264e_slice_write(H264eSlice *slice, void *p, RK_U32 size)
{
    MppWriteCtx stream;
    MppWriteCtx *s = &stream;
    RK_S32 bitCnt = 0;

    mpp_writer_init(s, p, size);

    h264e_slice_write_header(slice, s);

    bitCnt = s->buffered_bits + s->byte_cnt * 8;

    // update on cabac mode
    if (slice->entropy_coding_mode)
        bitCnt = s->buffered_bits + s->byte_cnt * 8;

    if (h264e_debug & H264E_DBG_SLICE) {
        RK_S32 i;
        RK_U8 *tmp = p;
        RK_S32 pos = 0;
        char log[256];

        pos = sprintf(log + pos, "sw stream: ");
        for (i = 0; i < 16; i ++) {
            pos += sprintf(log + pos, "%02x ", tmp[i]);
        }
        pos += sprintf(log + pos, "\n");
        h264e_dbg_slice(log);
    }

    return bitCnt;
}

/*
 * For software force skip stream writing
 * 3 contexts state for skip flag
 * end_of_slice flag is equal to write bypass 1
 */
typedef struct H264eCabac_t {
    RK_S32      state;

    RK_S32      low;
    RK_S32      range;
    RK_S32      queue;
    RK_S32      bytes_outstanding;

    MppWriteCtx *s;
} H264eCabac;

static const RK_S8 skip_init_state[3][2] = {
    //              skip_flag ctx 11
    /* model 0 */   { 23, 33 },
    /* model 1 */   { 22, 25 },
    /* model 2 */   { 29, 16 },
};

static void init_context(H264eCabac *ctx, RK_S32 qp, RK_S32 model, MppWriteCtx *s)
{
    const RK_S8 *init = &skip_init_state[model][0];
    RK_S32 state = MPP_CLIP3(1, 126, ((init[0] * qp) >> 4) + init[1]);

    ctx->state = (MPP_MIN(state, 127 - state) << 1) | (state >> 6);

    ctx->low   = 0;
    ctx->range = 0x01FE;
    ctx->queue = -9;
    ctx->bytes_outstanding = 0;
    ctx->s     = s;
}

static inline void h264e_cabac_putbyte(H264eCabac *ctx)
{
    if (ctx->queue >= 0) {
        RK_S32 out = ctx->low >> (ctx->queue + 10);

        ctx->low &= (0x400 << ctx->queue) - 1;
        ctx->queue -= 8;

        if ((out & 0xff) == 0xff) {
            ctx->bytes_outstanding++;
        } else {
            MppWriteCtx *s = ctx->s;
            RK_S32 carry = out >> 8;
            RK_S32 bytes_outstanding = ctx->bytes_outstanding;

            if (ctx->queue > 0)
                mpp_writer_put_bits(s, carry, ctx->queue & 0x7);

            while (bytes_outstanding > 0) {
                mpp_writer_put_bits(s, carry - 1, 8);
                bytes_outstanding--;
            }

            mpp_writer_put_bits(s, out, MPP_MIN(8, 8 - ctx->queue));
            ctx->bytes_outstanding = 0;
        }
    }
}

static inline void h264e_cabac_renorm(H264eCabac *ctx)
{
    static const RK_U8 x264_cabac_renorm_shift[64] = {
        6, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    RK_S32 shift = x264_cabac_renorm_shift[ctx->range >> 3];

    ctx->range <<= shift;
    ctx->low   <<= shift;
    ctx->queue  += shift;

    h264e_cabac_putbyte(ctx);
}

static void h264e_cabac_write_skip_flag(H264eCabac *ctx)
{
    static const RK_U8 h264e_cabac_range_lps[64][4] = {
        {  2,   2,   2,   2}, {  6,   7,   8,   9}, {  6,   7,   9,  10}, {  6,   8,   9,  11},
        {  7,   8,  10,  11}, {  7,   9,  10,  12}, {  7,   9,  11,  12}, {  8,   9,  11,  13},
        {  8,  10,  12,  14}, {  9,  11,  12,  14}, {  9,  11,  13,  15}, { 10,  12,  14,  16},
        { 10,  12,  15,  17}, { 11,  13,  15,  18}, { 11,  14,  16,  19}, { 12,  14,  17,  20},
        { 12,  15,  18,  21}, { 13,  16,  19,  22}, { 14,  17,  20,  23}, { 14,  18,  21,  24},
        { 15,  19,  22,  25}, { 16,  20,  23,  27}, { 17,  21,  25,  28}, { 18,  22,  26,  30},
        { 19,  23,  27,  31}, { 20,  24,  29,  33}, { 21,  26,  30,  35}, { 22,  27,  32,  37},
        { 23,  28,  33,  39}, { 24,  30,  35,  41}, { 26,  31,  37,  43}, { 27,  33,  39,  45},
        { 29,  35,  41,  48}, { 30,  37,  43,  50}, { 32,  39,  46,  53}, { 33,  41,  48,  56},
        { 35,  43,  51,  59}, { 37,  45,  54,  62}, { 39,  48,  56,  65}, { 41,  50,  59,  69},
        { 43,  53,  63,  72}, { 46,  56,  66,  76}, { 48,  59,  69,  80}, { 51,  62,  73,  85},
        { 53,  65,  77,  89}, { 56,  69,  81,  94}, { 59,  72,  86,  99}, { 62,  76,  90, 104},
        { 66,  80,  95, 110}, { 69,  85, 100, 116}, { 73,  89, 105, 122}, { 77,  94, 111, 128},
        { 81,  99, 117, 135}, { 85, 104, 123, 142}, { 90, 110, 130, 150}, { 95, 116, 137, 158},
        {100, 122, 144, 166}, {105, 128, 152, 175}, {111, 135, 160, 185}, {116, 142, 169, 195},
        {123, 150, 178, 205}, {128, 158, 187, 216}, {128, 167, 197, 227}, {128, 176, 208, 240}
    };

    static const RK_U8 h264e_cabac_transition[128][2] = {
        {  0,   0}, {  1,   1}, {  2,  50}, { 51,   3}, {  2,  50}, { 51,   3}, {  4,  52}, { 53,   5},
        {  6,  52}, { 53,   7}, {  8,  52}, { 53,   9}, { 10,  54}, { 55,  11}, { 12,  54}, { 55,  13},
        { 14,  54}, { 55,  15}, { 16,  56}, { 57,  17}, { 18,  56}, { 57,  19}, { 20,  56}, { 57,  21},
        { 22,  58}, { 59,  23}, { 24,  58}, { 59,  25}, { 26,  60}, { 61,  27}, { 28,  60}, { 61,  29},
        { 30,  60}, { 61,  31}, { 32,  62}, { 63,  33}, { 34,  62}, { 63,  35}, { 36,  64}, { 65,  37},
        { 38,  66}, { 67,  39}, { 40,  66}, { 67,  41}, { 42,  66}, { 67,  43}, { 44,  68}, { 69,  45},
        { 46,  68}, { 69,  47}, { 48,  70}, { 71,  49}, { 50,  72}, { 73,  51}, { 52,  72}, { 73,  53},
        { 54,  74}, { 75,  55}, { 56,  74}, { 75,  57}, { 58,  76}, { 77,  59}, { 60,  78}, { 79,  61},
        { 62,  78}, { 79,  63}, { 64,  80}, { 81,  65}, { 66,  82}, { 83,  67}, { 68,  82}, { 83,  69},
        { 70,  84}, { 85,  71}, { 72,  84}, { 85,  73}, { 74,  88}, { 89,  75}, { 76,  88}, { 89,  77},
        { 78,  90}, { 91,  79}, { 80,  90}, { 91,  81}, { 82,  94}, { 95,  83}, { 84,  94}, { 95,  85},
        { 86,  96}, { 97,  87}, { 88,  96}, { 97,  89}, { 90, 100}, {101,  91}, { 92, 100}, {101,  93},
        { 94, 102}, {103,  95}, { 96, 104}, {105,  97}, { 98, 104}, {105,  99}, {100, 108}, {109, 101},
        {102, 108}, {109, 103}, {104, 110}, {111, 105}, {106, 112}, {113, 107}, {108, 114}, {115, 109},
        {110, 116}, {117, 111}, {112, 118}, {119, 113}, {114, 118}, {119, 115}, {116, 122}, {123, 117},
        {118, 122}, {123, 119}, {120, 124}, {125, 121}, {122, 126}, {127, 123}, {124, 127}, {126, 125}
    };

    RK_S32 skip = 1;
    RK_S32 state = ctx->state;
    RK_S32 range_lps = h264e_cabac_range_lps[state >> 1][(ctx->range >> 6) - 4];

    ctx->range -= range_lps;
    if (skip != (state & 1) ) {
        ctx->low += ctx->range;
        ctx->range = range_lps;
    }

    ctx->state = h264e_cabac_transition[state][skip];

    h264e_cabac_renorm(ctx);
}

static void h264e_cabac_terminal(H264eCabac *ctx)
{
    ctx->range -= 2;
    h264e_cabac_renorm(ctx);
}

static void h264e_cabac_flush(H264eCabac *ctx)
{
    /* end_of_slice_flag = 1 */
    ctx->low += ctx->range - 2;
    ctx->low |= 1;
    ctx->low <<= 9;
    ctx->queue += 9;

    h264e_cabac_putbyte( ctx );
    h264e_cabac_putbyte( ctx );

    ctx->low <<= -ctx->queue;
    ctx->low |= 1 << 10;
    ctx->queue = 0;

    h264e_cabac_putbyte( ctx );

    while (ctx->bytes_outstanding > 0) {
        mpp_writer_put_bits(ctx->s, 0xff, 8);
        ctx->bytes_outstanding--;
    }
}

RK_S32 h264e_slice_write_pskip(H264eSlice *slice, void *p, RK_U32 size)
{
    MppWriteCtx stream;
    MppWriteCtx *s = &stream;
    RK_S32 bitCnt = 0;

    mpp_writer_init(s, p, size);

    h264e_slice_write_header(slice, s);
    if (slice->entropy_coding_mode) {
        /* cabac */
        H264eCabac ctx;
        RK_S32 i;

        init_context(&ctx, slice->qp_init, slice->cabac_init_idc, s);

        for (i = 0; i < slice->mb_w * slice->mb_h; i++) {
            /* end_of_slice_flag = 0 */
            if (i)
                h264e_cabac_terminal(&ctx);

            /* skip flag */
            h264e_cabac_write_skip_flag(&ctx);
        }

        /* end_of_slice_flag = 1 and flush */
        h264e_cabac_flush(&ctx);
    } else {
        /* cavlc */
        /* mb skip run */
        mpp_writer_put_ue(s, slice->mb_w * slice->mb_h);
        h264e_dbg_slice("used bit %d mb_skip_run %d\n",
                        mpp_writer_bits(s), slice->mb_w * slice->mb_h);

        /* rbsp_stop_one_bit */
        mpp_writer_trailing(s);
        h264e_dbg_slice("used bit %d tailing %d\n", mpp_writer_bits(s));
    }
    mpp_writer_flush(s);

    bitCnt = s->buffered_bits + s->byte_cnt * 8;
    return bitCnt;
}

RK_S32 h264e_slice_move(RK_U8 *dst, RK_U8 *src, RK_S32 dst_bit, RK_S32 src_bit, RK_S32 src_size)
{
    RK_S32 dst_byte = dst_bit / 8;
    RK_S32 src_byte = src_bit / 8;
    RK_S32 dst_bit_r = dst_bit & 7;
    RK_S32 src_bit_r = src_bit & 7;
    RK_S32 src_len = src_size - src_byte;
    RK_S32 diff_len = 0;
    static RK_S32 frame_no = 0;

    if (src_bit_r == 0 && dst_bit_r == 0) {
        // direct copy
        if (h264e_debug & H264E_DBG_SLICE)
            mpp_log_f("direct copy %p -> %p %d\n", src, dst, src_len);

        h264e_dbg_slice("bit [%d %d] [%d %d] [%d %d] len %d\n",
                        src_bit, dst_bit, src_byte, dst_byte,
                        src_bit_r, dst_bit_r, src_len);

        memcpy(dst + dst_byte, src + src_byte, src_len);
        return diff_len;
    }

    RK_U8 *psrc = src + src_byte;
    RK_U8 *pdst = dst + dst_byte;

    RK_U16 tmp16a, tmp16b, tmp16c, last_tmp, dst_mask;
    RK_U8 tmp0, tmp1;
    RK_U32 loop = src_len + (src_bit_r > 0);
    RK_U32 i = 0;
    RK_U32 src_zero_cnt = 0;
    RK_U32 dst_zero_cnt = 0;
    RK_U32 dst_len = 0;

    last_tmp = (RK_U16)pdst[0];
    dst_mask = 0xFFFF << (8 - dst_bit_r);

    h264e_dbg_slice("bit [%d %d] [%d %d] [%d %d] loop %d mask %04x last %04x\n",
                    src_bit, dst_bit, src_byte, dst_byte,
                    src_bit_r, dst_bit_r, loop, dst_mask, last_tmp);

    for (i = 0; i < loop; i++) {
        if (psrc[0] == 0) {
            src_zero_cnt++;
        } else {
            src_zero_cnt = 0;
        }

        // tmp0 tmp1 is next two non-aligned bytes from src
        tmp0 = psrc[0];
        tmp1 = (i < loop - 1) ? psrc[1] : 0;

        if (src_zero_cnt >= 2 && tmp1 == 3) {
            if (h264e_debug & H264E_DBG_SLICE)
                mpp_log("found 03 at src pos %d %02x %02x %02x %02x %02x %02x %02x %02x\n",
                        i, psrc[-2], psrc[-1], psrc[0], psrc[1], psrc[2],
                        psrc[3], psrc[4], psrc[5]);

            psrc++;
            i++;
            tmp1 = psrc[1];
            src_zero_cnt = 0;
            diff_len--;
        }
        // get U16 data
        tmp16a = ((RK_U16)tmp0 << 8) | (RK_U16)tmp1;

        if (src_bit_r) {
            tmp16b = tmp16a << src_bit_r;
        } else {
            tmp16b = tmp16a;
        }

        if (dst_bit_r)
            tmp16c = tmp16b >> dst_bit_r | ((last_tmp << 8) & dst_mask);
        else
            tmp16c = tmp16b;

        pdst[0] = (tmp16c >> 8) & 0xFF;
        pdst[1] = tmp16c & 0xFF;

        if (h264e_debug & H264E_DBG_SLICE) {
            if (i < 10) {
                mpp_log("%03d src [%04x] -> [%04x] + last [%04x] -> %04x\n", i, tmp16a, tmp16b, last_tmp, tmp16c);
            }
            if (i >= loop - 10) {
                mpp_log("%03d src [%04x] -> [%04x] + last [%04x] -> %04x\n", i, tmp16a, tmp16b, last_tmp, tmp16c);
            }
        }

        if (dst_zero_cnt == 2 && pdst[0] <= 0x3) {
            if (h264e_debug & H264E_DBG_SLICE)
                mpp_log("found 03 at dst frame %d pos %d\n", frame_no, dst_len);

            pdst[2] = pdst[1];
            pdst[1] = pdst[0];
            pdst[0] = 0x3;
            pdst++;
            diff_len++;
            dst_len++;
            dst_zero_cnt = 0;
        }

        if (pdst[0] == 0)
            dst_zero_cnt++;
        else
            dst_zero_cnt = 0;

        last_tmp = tmp16c;

        psrc++;
        pdst++;
        dst_len++;
    }

    frame_no++;

    return diff_len;
}

RK_S32 h264e_slice_write_prefix_nal_unit_svc(H264ePrefixNal *prefix, void *p, RK_S32 size)
{
    MppWriteCtx stream;
    MppWriteCtx *s = &stream;
    RK_S32 bitCnt = 0;

    mpp_writer_init(s, p, size);

    /* nal header */
    /* start_code_prefix 00 00 00 01 */
    mpp_writer_put_raw_bits(s, 0, 24);
    mpp_writer_put_raw_bits(s, 1, 8);

    /* forbidden_zero_bit */
    mpp_writer_put_raw_bits(s, 0, 1);

    /* nal_reference_idc */
    mpp_writer_put_raw_bits(s, prefix->nal_ref_idc, 2);

    /* nalu_type */
    mpp_writer_put_raw_bits(s, 14, 5);

    /* svc_extension_flag */
    mpp_writer_put_raw_bits(s, 1, 1);

    /* nal_unit_header_svc_extension */
    /* idr_flag */
    mpp_writer_put_raw_bits(s, prefix->idr_flag, 1);

    /* priority_id */
    mpp_writer_put_raw_bits(s, prefix->priority_id , 6);

    /* no_inter_layer_pred_flag */
    mpp_writer_put_raw_bits(s, prefix->no_inter_layer_pred_flag , 1);

    /* dependency_id */
    mpp_writer_put_raw_bits(s, prefix->dependency_id, 3);

    /* quality_id */
    mpp_writer_put_raw_bits(s, prefix->quality_id, 4);

    /* temporal_id */
    mpp_writer_put_raw_bits(s, prefix->temporal_id, 3);

    /* use_ref_base_pic_flag */
    mpp_writer_put_raw_bits(s, prefix->use_ref_base_pic_flag, 1);

    /* discardable_flag */
    mpp_writer_put_raw_bits(s, prefix->discardable_flag, 1);

    /* output_flag */
    mpp_writer_put_raw_bits(s, prefix->output_flag, 1);

    /* reserved_three_2bits */
    mpp_writer_put_raw_bits(s, 3, 2);

    /* prefix_nal_unit_svc */
    if (prefix->nal_ref_idc) {
        /* store_ref_base_pic_flag */
        mpp_writer_put_raw_bits(s, 0, 1);

        /* additional_prefix_nal_unit_extension_flag */
        mpp_writer_put_raw_bits(s, 0, 1);

        /* rbsp_trailing_bits */
        mpp_writer_trailing(s);
    }

    mpp_writer_flush(s);

    bitCnt = s->buffered_bits + s->byte_cnt * 8;

    return bitCnt;
}
