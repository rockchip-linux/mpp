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
                          SynH264eSps *sps, H264eDpbFrm *frm)
{
    MppEncH264Cfg *h264 = &cfg->codec.h264;

    slice->log2_max_frame_num = sps->log2_max_frame_num_minus4 + 4;
    slice->log2_max_poc_lsb = sps->log2_max_poc_lsb_minus4 + 4;
    slice->entropy_coding_mode = h264->entropy_coding_mode;

    slice->nal_reference_idc = H264_NALU_PRIORITY_HIGH;;
    slice->nalu_type = H264_NALU_TYPE_IDR;

    slice->first_mb_in_slice = 0;
    slice->slice_type = (frm->status.is_idr) ? (H264_I_SLICE) : (H264_P_SLICE);
    slice->pic_parameter_set_id = 0;
    slice->frame_num = frm->frame_num;
    slice->num_ref_idx_override = 0;
    slice->qp_delta = 0;
    slice->cabac_init_idc = h264->entropy_coding_mode ? h264->cabac_init_idc : -1;
    slice->disable_deblocking_filter_idc = h264->deblock_disable;
    slice->slice_alpha_c0_offset_div2 = h264->deblock_offset_alpha;
    slice->slice_beta_offset_div2 = h264->deblock_offset_beta;

    slice->idr_flag = frm->status.is_idr;

    if (slice->idr_flag) {
        slice->idr_pic_id = slice->next_idr_pic_id;
        slice->next_idr_pic_id++;
        if (slice->next_idr_pic_id >= 16)
            slice->next_idr_pic_id = 0;
    }

    slice->pic_order_cnt_lsb = frm->poc;
    slice->num_ref_idx_active = 1;

    return MPP_OK;
}

MPP_RET h264e_reorder_init(H264eReorderInfo *reorder)
{
    reorder->size = H264E_MAX_REFS_CNT;
    reorder->pos_wr = 0;
    reorder->pos_rd = reorder->size;

    return MPP_OK;
}

MPP_RET h264e_reorder_wr_op(H264eReorderInfo *info, H264eRplmo *op)
{
    if (info->pos_rd == info->pos_wr)
        return MPP_NOK;

    RK_S32 pos_wr = info->pos_wr;

    if (pos_wr >= info->size)
        pos_wr -= info->size;

    info->ops[pos_wr] = *op;

    info->pos_wr++;
    if (info->pos_wr >= info->size * 2)
        info->pos_wr = 0;

    return MPP_OK;
}

MPP_RET h264e_reorder_rd_op(H264eReorderInfo *info, H264eRplmo *op)
{
    if (info->pos_rd - info->pos_wr == info->size ||
        info->pos_wr - info->pos_rd == info->size)
        return MPP_NOK;

    RK_S32 pos_rd = info->pos_rd;

    if (pos_rd >= info->size)
        pos_rd -= info->size;

    *op = info->ops[pos_rd];

    info->pos_rd++;
    if (info->pos_rd >= info->size * 2)
        info->pos_rd = 0;

    return MPP_OK;
}

MPP_RET h264e_marking_init(H264eMarkingInfo *marking)
{
    marking->idr_flag = 0;
    marking->no_output_of_prior_pics = 1;
    marking->long_term_reference_flag = 0;
    marking->adaptive_ref_pic_buffering = 0;
    marking->size = MAX_H264E_MMCO_CNT;
    marking->pos_wr = 0;
    marking->pos_rd = marking->size;
    marking->count = 0;

    return MPP_OK;
}

RK_S32 h264e_marking_is_empty(H264eMarkingInfo *info)
{
    return (info->pos_rd - info->pos_wr == info->size ||
            info->pos_wr - info->pos_rd == info->size) ? (1) : (0);
}

RK_S32 h264e_marking_is_full(H264eMarkingInfo *info)
{
    return (info->pos_rd == info->pos_wr) ? (1) : (0);
}

MPP_RET h264e_marking_wr_op(H264eMarkingInfo *info, H264eMmco *op)
{
    if (h264e_marking_is_full(info))
        return MPP_NOK;

    RK_S32 pos_wr = info->pos_wr;

    if (pos_wr >= info->size)
        pos_wr -= info->size;

    info->ops[pos_wr] = *op;

    info->pos_wr++;
    if (info->pos_wr >= info->size * 2)
        info->pos_wr = 0;

    info->count++;

    return MPP_OK;
}

MPP_RET h264e_marking_rd_op(H264eMarkingInfo *info, H264eMmco *op)
{
    if (h264e_marking_is_empty(info))
        return MPP_NOK;

    RK_S32 pos_rd = info->pos_rd;

    if (pos_rd >= info->size)
        pos_rd -= info->size;

    *op = info->ops[pos_rd];

    info->pos_rd++;
    if (info->pos_rd >= info->size * 2)
        info->pos_rd = 0;

    info->count--;

    return MPP_OK;
}

void write_marking(MppWriteCtx *s, H264eMarkingInfo *marking)
{
    if (marking->idr_flag) {
        /* no_output_of_prior_pics_flag */
        mpp_writer_put_bits(s, marking->no_output_of_prior_pics, 1);

        /* long_term_reference_flag */
        mpp_writer_put_bits(s, marking->long_term_reference_flag, 1);

        // clear long_term_reference_flag flag
        marking->long_term_reference_flag = 0;
    } else {
        h264e_dbg_mmco("mmco count %d\n", marking->count);

        if (!h264e_marking_is_empty(marking)) {
            /* adaptive_ref_pic_marking_mode_flag */
            mpp_writer_put_bits(s, 1, 1);

            while (marking->count) {
                H264eMmco mmco;

                h264e_marking_rd_op(marking, &mmco);

                /* memory_management_control_operation */
                mpp_writer_put_ue(s, mmco.mmco);

                switch (mmco.mmco) {
                case 1 : {
                    /* difference_of_pic_nums_minus1 */
                    mpp_writer_put_ue(s, mmco.difference_of_pic_nums_minus1);
                } break;
                case 2 : {
                    /* long_term_pic_num */
                    mpp_writer_put_ue(s, mmco.long_term_pic_num );
                } break;
                case 3 : {
                    /* difference_of_pic_nums_minus1 */
                    mpp_writer_put_ue(s, mmco.difference_of_pic_nums_minus1);

                    /* long_term_frame_idx */
                    mpp_writer_put_ue(s, mmco.long_term_frame_idx );
                } break;
                case 4 : {
                    /* max_long_term_frame_idx_plus1 */
                    mpp_writer_put_ue(s, mmco.max_long_term_frame_idx_plus1);
                } break;
                case 5 : {
                } break;
                case 6 : {
                    /* long_term_frame_idx */
                    mpp_writer_put_ue(s, mmco.long_term_frame_idx);
                } break;
                default : {
                    mpp_err_f("invalid mmco %d\n", mmco.mmco);
                } break;
                }

                marking->count--;
            }

            /* memory_management_control_operation */
            mpp_writer_put_ue(s, 0);
        } else {
            /* adaptive_ref_pic_marking_mode_flag */
            mpp_writer_put_bits(s, 0, 1);
        }
    }
}

RK_S32 h264e_slice_read(H264eSlice *slice, void *p, RK_S32 size)
{
    BitReadCtx_t bit;
    RK_S32 ret = 0;
    RK_S32 val = 0;
    RK_S32 bit_cnt = 0;

    mpp_set_bitread_ctx(&bit, p, size);

    /* start_code */
    ret |= mpp_read_longbits(&bit, 32, (RK_U32 *)&val);

    /* forbidden_zero_bit */
    ret |= mpp_read_bits(&bit, 1, &val);

    /* nal_ref_idc */
    ret |= mpp_read_bits(&bit, 2, &slice->nal_reference_idc);

    /* nal_unit_type */
    ret |= mpp_read_bits(&bit, 5, &slice->nalu_type);

    /* first_mb_nr */
    ret = mpp_read_ue(&bit, &slice->first_mb_in_slice);

    /* slice_type */
    ret |= mpp_read_ue(&bit, &slice->slice_type);

    /* pic_parameter_set_id */
    ret |= mpp_read_ue(&bit, &slice->pic_parameter_set_id);

    /* frame_num */
    /* NOTE: vpu hardware fix 16 bit frame_num */
    ret |= mpp_read_bits(&bit, 16, &slice->frame_num);

    slice->idr_flag = (slice->nalu_type == 5);
    if (slice->idr_flag) {
        /* idr_pic_id */
        ret |= mpp_read_ue(&bit, &slice->idr_pic_id);
    }

    // pic_order_cnt_type here
    // vpu hardware is always zero. Just ignore

    // NOTE: Only P slice has num_ref_idx_override flag and ref_pic_list_modification flag
    if (slice->slice_type == 0) {
        /* num_ref_idx_override */
        ret |= mpp_read_bits(&bit, 1, &slice->num_ref_idx_override);

        /* ref_pic_list_modification_flag */
        ret |= mpp_read_bits(&bit, 1, &slice->ref_pic_list_modification_flag);
    }

    // NOTE: vpu hardware is always zero
    slice->ref_pic_list_modification_flag = 0;

    if (slice->nal_reference_idc) {
        if (slice->idr_flag) {
            /* no_output_of_prior_pics */
            ret |= mpp_read_bits(&bit, 1, &slice->no_output_of_prior_pics);

            /* long_term_reference_flag */
            ret |= mpp_read_bits(&bit, 1, &slice->long_term_reference_flag);
        } else {
            /* adaptive_ref_pic_buffering */
            ret |= mpp_read_bits(&bit, 1, &slice->adaptive_ref_pic_buffering);

            mpp_assert(slice->adaptive_ref_pic_buffering == 0);
        }
    }

    if (slice->entropy_coding_mode && slice->slice_type != H264_I_SLICE) {
        /* cabac_init_idc */
        ret |= mpp_read_ue(&bit, &slice->cabac_init_idc);
    }

    /* qp_delta */
    ret |= mpp_read_se(&bit, &slice->qp_delta);

    /* disable_deblocking_filter_idc */
    ret |= mpp_read_ue(&bit, &slice->disable_deblocking_filter_idc);

    /* slice_alpha_c0_offset_div2 */
    ret |= mpp_read_se(&bit, &slice->slice_alpha_c0_offset_div2);

    /* slice_beta_offset_div2 */
    ret |= mpp_read_se(&bit, &slice->slice_beta_offset_div2);

    h264e_dbg_slice("non-aligned   used bit %d\n", bit.used_bits);

    if (slice->entropy_coding_mode) {
        if (bit.num_remaining_bits_in_curr_byte_) {
            RK_U32 tmp = bit.num_remaining_bits_in_curr_byte_;

            /* cabac_aligned_bit */
            ret |= mpp_read_bits(&bit, tmp, &val);
        }
    }
    bit_cnt = bit.used_bits;

    h264e_dbg_slice("total aligned used bit %d\n", bit_cnt);

    return bit_cnt;
}

RK_S32 h264e_slice_write(H264eSlice *slice, void *p, RK_U32 size)
{
    MppWriteCtx stream;
    MppWriteCtx *s = &stream;
    RK_S32 i;
    RK_S32 bitCnt = 0;
    H264eMarkingInfo *marking = slice->marking;
    H264eRplmo rplmo;
    MPP_RET ret;

    mpp_writer_init(s, p, size);

    RK_U8 *tmp = (RK_U8 *)s->buffer;

    /* nal header */
    /* start_code_prefix 00 00 00 01 */
    mpp_writer_put_raw_bits(s, 0, 24);
    mpp_writer_put_raw_bits(s, 1, 8);
    /* forbidden_zero_bit */
    mpp_writer_put_raw_bits(s, 0, 1);

    /* nal_reference_idc */
    mpp_writer_put_raw_bits(s, slice->nal_reference_idc, 2);
    /* nalu_type */
    mpp_writer_put_raw_bits(s, slice->nalu_type, 5);

    /* slice header */
    /* start_mb_nr */
    mpp_writer_put_ue(s, slice->first_mb_in_slice);

    /* slice_type */
    mpp_writer_put_ue(s, slice->slice_type);

    /* pic_parameter_set_id */
    mpp_writer_put_ue(s, slice->pic_parameter_set_id);

    /* frame_num */
    mpp_writer_put_bits(s, slice->frame_num, 16);

    if (slice->nalu_type == 5) {
        /* idr_pic_id */
        mpp_writer_put_ue(s, slice->idr_pic_id);
        marking->idr_flag = 1;
    } else
        marking->idr_flag = 0;

    // Force to use poc type 0 here
    {
        RK_S32 pic_order_cnt_lsb = slice->pic_order_cnt_lsb;
        RK_S32 max_poc_lsb = (1 << slice->log2_max_poc_lsb) - 1;


        if (pic_order_cnt_lsb >= max_poc_lsb)
            pic_order_cnt_lsb -= max_poc_lsb;

        /* pic_order_cnt_lsb */
        mpp_writer_put_bits(s, pic_order_cnt_lsb, 16);
    }

    /* read reorder and check */
    ret = h264e_reorder_rd_op(slice->reorder, &rplmo);

    if (slice->slice_type == H264_P_SLICE && !ret) {
        /* num_ref_idx_override */
        mpp_writer_put_bits(s, slice->num_ref_idx_override, 1);

        /* ref_pic_list_reordering */
        mpp_writer_put_bits(s, 1, 1);

        slice->ref_pic_list_modification_flag = 1;
    } else
        slice->ref_pic_list_modification_flag = 0;

    if (slice->slice_type == H264_I_SLICE)
        mpp_assert(slice->ref_pic_list_modification_flag == 0);

    if (slice->ref_pic_list_modification_flag) {
        mpp_assert(ret == MPP_OK);
        /* modification_of_pic_nums_idc */
        mpp_writer_put_ue(s, rplmo.modification_of_pic_nums_idc);

        switch (rplmo.modification_of_pic_nums_idc) {
        case 0 :
        case 1 : {
            /* abs_diff_pic_num_minus1 */
            mpp_writer_put_ue(s, rplmo.abs_diff_pic_num_minus1);
        } break;
        case 2 : {
            /* long_term_pic_idx */
            mpp_writer_put_ue(s, rplmo.long_term_pic_idx);
        } break;
        default : {
            mpp_err_f("invalid modification_of_pic_nums_idc %d\n",
                      rplmo.modification_of_pic_nums_idc);
        } break;
        }

        /* modification_of_pic_nums_idc */
        mpp_writer_put_ue(s, 3);
    }

    // NOTE: ignore nal ref idc here
    h264e_dbg_mmco("nal_reference_idc %d idr_flag %d\n",
                   slice->nal_reference_idc, slice->idr_flag);

    if (slice->nal_reference_idc)
        write_marking(s, marking);

    if (slice->entropy_coding_mode && slice->slice_type != H264_I_SLICE) {
        /* cabac_init_idc */
        mpp_writer_put_ue(s, slice->cabac_init_idc);
    }

    /* qp_delta */
    mpp_writer_put_se(s, slice->qp_delta);

    /* disable_deblocking_filter_idc */
    mpp_writer_put_ue(s, 0);

    /* slice_alpha_c0_offset_div2 */
    mpp_writer_put_se(s, 0);

    /* slice_beta_offset_div2 */
    mpp_writer_put_se(s, 0);

    bitCnt = s->buffered_bits + s->byte_cnt * 8;

    if (s->buffered_bits) {
        // NOTE: only have aligned bit on cabac mode but cavlc also need to write byte cache to memory
        RK_S32 left = (8 - s->buffered_bits);
        for (i = 0; i < left; i++) {
            /* align_bit */
            mpp_writer_put_bits(s, 1, 1);
        }
    }

    // update on cabac mode
    if (slice->entropy_coding_mode)
        bitCnt = s->buffered_bits + s->byte_cnt * 8;

    RK_S32 pos = 0;
    char log[256];

    pos = sprintf(log + pos, "sw stream: ");
    for (i = 0; i < 16; i ++) {
        pos += sprintf(log + pos, "%02x ", tmp[i]);
    }
    pos += sprintf(log + pos, "\n");
    h264e_dbg_slice(log);

    return bitCnt;
}

static RK_S32 frame_no = 0;

RK_S32 h264e_slice_move(RK_U8 *dst, RK_U8 *src, RK_S32 dst_bit, RK_S32 src_bit, RK_S32 src_size)
{
    RK_S32 dst_byte = dst_bit / 8;
    RK_S32 src_byte = src_bit / 8;
    RK_S32 dst_bit_r = dst_bit & 7;
    RK_S32 src_bit_r = src_bit & 7;
    RK_S32 src_len = src_size - src_byte;
    RK_S32 diff_len = 0;

    if (src_bit_r == 0 && dst_bit_r == 0) {
        // direct copy
        if (h264e_debug & H264E_DBG_SLICE)
            mpp_log_f("direct copy %p -> %p %d\n", src, dst, src_len);

        memcpy(dst + dst_byte, src + src_byte, src_len);
        return diff_len;
    }

    RK_U8 *psrc = src + src_byte;
    RK_U8 *pdst = dst + dst_byte;

    RK_U16 tmp16a, tmp16b, tmp16c, last_tmp;
    RK_U8 tmp0, tmp1;
    RK_U32 loop = src_len;
    RK_U32 i = 0;
    RK_U32 src_zero_cnt = 0;
    RK_U32 dst_zero_cnt = 0;
    RK_U32 dst_len = 0;

    if (h264e_debug & H264E_DBG_SLICE)
        mpp_log("bit [%d %d] [%d %d] loop %d\n", src_bit, dst_bit,
                src_bit_r, dst_bit_r, loop);

    last_tmp = pdst[0] >> (8 - dst_bit_r) << (8 - dst_bit_r);

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
            tmp16c = tmp16b >> dst_bit_r | (last_tmp >> (8 - dst_bit_r) <<  (16 - dst_bit_r));
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
