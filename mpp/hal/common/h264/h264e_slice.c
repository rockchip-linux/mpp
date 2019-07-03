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

#include "hal_h264e_com.h"

#include "h264e_dpb.h"
#include "h264e_slice.h"
#include "h264e_stream.h"

#define RD_LOG(bit, log, val) \
    do { \
        h264e_dpb_slice("%4d bit rd %s %d\n", bit.used_bits, log, val); \
    } while (0)

RK_S32 h264e_slice_read(H264eSlice *slice, void *p, RK_S32 size)
{
    BitReadCtx_t bit;
    RK_S32 ret = 0;
    RK_S32 val = 0;
    RK_S32 bit_cnt = 0;

    mpp_set_bitread_ctx(&bit, p, size);

    ret |= mpp_read_longbits(&bit, 32, (RK_U32 *)&val);
    RD_LOG(bit, "start code", val);

    ret |= mpp_read_bits(&bit, 1, &slice->forbidden_bit);
    RD_LOG(bit, "forbidden_zero_bit", slice->forbidden_bit);

    ret |= mpp_read_bits(&bit, 2, &slice->nal_reference_idc);
    RD_LOG(bit, "nal_ref_idc", slice->nal_reference_idc);

    ret |= mpp_read_bits(&bit, 5, &slice->nalu_type);
    RD_LOG(bit, "nal_unit_type", slice->nalu_type);

    ret = mpp_read_ue(&bit, &slice->start_mb_nr);
    RD_LOG(bit, "first_mb_nr", slice->start_mb_nr);

    ret |= mpp_read_ue(&bit, &slice->slice_type);
    RD_LOG(bit, "slice_type", slice->slice_type);

    ret |= mpp_read_ue(&bit, &slice->pic_parameter_set_id);
    RD_LOG(bit, "pic_parameter_set_id", slice->pic_parameter_set_id);

    ret |= mpp_read_bits(&bit, 16, &slice->frame_num);
    RD_LOG(bit, "frame_num", slice->frame_num);

    slice->idr_flag = (slice->nalu_type == 5);
    if (slice->idr_flag) {
        ret |= mpp_read_ue(&bit, &slice->idr_pic_id);
        RD_LOG(bit, "idr_pic_id", slice->idr_pic_id);
    }

    // pic_order_cnt_type here
    // vpu hardware is always zero. Just ignore

    // NOTE: Only P slice has num_ref_idx_override flag and ref_pic_list_modification flag
    if (slice->slice_type == 0) {
        ret |= mpp_read_bits(&bit, 1, &slice->num_ref_idx_override);
        RD_LOG(bit, "num_ref_idx_override", slice->num_ref_idx_override);

        ret |= mpp_read_bits(&bit, 1, &slice->ref_pic_list_modification_flag);
        RD_LOG(bit, "ref_pic_list_modification_flag",
               slice->ref_pic_list_modification_flag);
    }

    // NOTE: hardware is always zero
    slice->ref_pic_list_modification_flag = 0;

    if (slice->nal_reference_idc) {
        if (slice->idr_flag) {
            ret |= mpp_read_bits(&bit, 1, &slice->no_output_of_prior_pics);
            RD_LOG(bit, "no_output_of_prior_pics",
                   slice->no_output_of_prior_pics);

            ret |= mpp_read_bits(&bit, 1, &slice->long_term_reference_flag);
            RD_LOG(bit, "long_term_reference_flag",
                   slice->long_term_reference_flag);
        } else {
            ret |= mpp_read_bits(&bit, 1, &slice->adaptive_ref_pic_buffering);
            RD_LOG(bit, "adaptive_ref_pic_buffering",
                   slice->adaptive_ref_pic_buffering);

            mpp_assert(slice->adaptive_ref_pic_buffering == 0);
        }
    }

    if (slice->entropy_coding_mode && slice->slice_type != H264E_HAL_SLICE_TYPE_I) {
        ret |= mpp_read_ue(&bit, &slice->cabac_init_idc);
        RD_LOG(bit, "cabac_init_idc", slice->cabac_init_idc);
    }

    ret |= mpp_read_se(&bit, &slice->qp_delta);
    RD_LOG(bit, "qp_delta", slice->qp_delta);

    ret |= mpp_read_ue(&bit, &slice->disable_deblocking_filter_idc);
    RD_LOG(bit, "disable_deblocking_filter_idc",
           slice->disable_deblocking_filter_idc);

    ret |= mpp_read_se(&bit, &slice->slice_alpha_c0_offset_div2);
    RD_LOG(bit, "slice_alpha_c0_offset_div2",
           slice->slice_alpha_c0_offset_div2);

    ret |= mpp_read_se(&bit, &slice->slice_beta_offset_div2);
    RD_LOG(bit, "slice_beta_offset_div2", slice->slice_beta_offset_div2);

    h264e_dpb_slice("non-aligned   used bit %d\n", bit.used_bits);

    if (slice->entropy_coding_mode) {
        if (bit.num_remaining_bits_in_curr_byte_) {
            RK_U32 tmp = bit.num_remaining_bits_in_curr_byte_;

            ret |= mpp_read_bits(&bit, tmp, &val);
            RD_LOG(bit, "cabac aligned bit", tmp);
        }
    }
    bit_cnt = bit.used_bits;

    h264e_dpb_slice("total aligned used bit %d\n", bit_cnt);

    return bit_cnt;
}

#define WR_LOG(log, val) \
    do { \
        h264e_dpb_slice("B %2d b %d val %08x - %s %d\n", \
                        s->byte_cnt, s->buffered_bits, s->byte_buffer, \
                        log, val); \
    } while (0)

RK_S32 h264e_slice_write(H264eSlice *slice, void *p, RK_U32 size)
{
    H264eStream stream;
    H264eStream *s = &stream;
    RK_S32 i;
    RK_S32 bitCnt = 0;

    h264e_stream_init(s, p, size);

    RK_U8 *tmp = (RK_U8 *)s->buffer;

    /* nal header */
    h264e_stream_put_bits(s, 0, 24, "start_code_zero");
    h264e_stream_put_bits(s, 1, 8, "start_code_one");
    WR_LOG("start_code", 1);

    h264e_stream_put_bits(s, 0, 1, "forbidden_bit");
    WR_LOG("forbidden_bit", 0);

    h264e_stream_put_bits(s, slice->nal_reference_idc, 2, "nal_reference_idc");
    WR_LOG("nal_reference_idc", slice->nal_reference_idc);

    h264e_stream_put_bits(s, slice->nalu_type, 5, "nalu_type");
    WR_LOG("nalu_type", slice->nalu_type);

    /* slice header */
    h264e_stream_write_ue(s, slice->start_mb_nr, "start_mb_nr");
    WR_LOG("first_mb_nr", slice->start_mb_nr);

    h264e_stream_write_ue(s, slice->slice_type, "slice_type");
    WR_LOG("slice_type", slice->slice_type);

    h264e_stream_write_ue(s, slice->pic_parameter_set_id, "pic_parameter_set_id");
    WR_LOG("pic_parameter_set_id", slice->pic_parameter_set_id);

    h264e_stream_put_bits_with_detect(s, slice->frame_num, 16, "frame_num");
    WR_LOG("frame_num", slice->frame_num);

    slice->idr_flag = (slice->nalu_type == 5);
    if (slice->idr_flag) {
        h264e_stream_write_ue(s, slice->idr_pic_id, "idr_pic_id");
        WR_LOG("idr_pic_id", slice->idr_pic_id);
    }

    // Force to use poc type 0 here
    {
        RK_S32 pic_order_cnt_lsb = slice->pic_order_cnt_lsb;
        RK_S32 max_poc_lsb = (1 << slice->log2_max_poc_lsb) - 1;


        if (pic_order_cnt_lsb >= max_poc_lsb)
            pic_order_cnt_lsb -= max_poc_lsb;

        h264e_stream_put_bits_with_detect(s, pic_order_cnt_lsb, 16, "pic_order_cnt_lsb");
        WR_LOG("pic_order_cnt_lsb", pic_order_cnt_lsb);
    }

    if (slice->slice_type == 0) {
        h264e_stream_put_bits_with_detect(s, slice->num_ref_idx_override, 1,
                                          "num_ref_idx_override");
        WR_LOG("num_ref_idx_override", slice->num_ref_idx_override);

        h264e_stream_put_bits_with_detect(s, slice->ref_pic_list_modification_flag, 1,
                                          "ref_pic_list_reordering");
        WR_LOG("ref_pic_list_reordering", slice->ref_pic_list_modification_flag);
    }

    if (slice->ref_pic_list_modification_flag) {
        h264e_stream_write_ue(s, slice->modification_of_pic_nums_idc,
                              "modification_of_pic_nums_idc");
        WR_LOG("modification_of_pic_nums_idc", slice->modification_of_pic_nums_idc);

        switch (slice->modification_of_pic_nums_idc) {
        case 0 :
        case 1 : {
            h264e_stream_write_ue(s, slice->abs_diff_pic_num_minus1,
                                  "abs_diff_pic_num_minus1");
            WR_LOG("abs_diff_pic_num_minus1", slice->abs_diff_pic_num_minus1);
        } break;
        case 2 : {
            h264e_stream_write_ue(s, slice->long_term_pic_idx,
                                  "long_term_pic_idx");
            WR_LOG("long_term_pic_idx", slice->long_term_pic_idx);
        } break;
        default : {
            mpp_err_f("invalid modification_of_pic_nums_idc %d\n",
                      slice->modification_of_pic_nums_idc);
        } break;
        }

        h264e_stream_write_ue(s, 3,
                              "modification_of_pic_nums_idc");
        WR_LOG("modification_of_pic_nums_idc", 3);
    }

    // NOTE: ignore nal ref idc here
    h264e_dpb_mmco("nal_reference_idc %d idr_flag %d\n",
                   slice->nal_reference_idc, slice->idr_flag);

    if (slice->nal_reference_idc) {
        if (slice->idr_flag) {
            h264e_stream_put_bits_with_detect(s, slice->no_output_of_prior_pics, 1,
                                              "no_output_of_prior_pics_flag");
            WR_LOG("no_output_of_prior_pics_flag", slice->no_output_of_prior_pics);

            h264e_stream_put_bits_with_detect(s, slice->long_term_reference_flag, 1,
                                              "long_term_reference_flag");
            WR_LOG("long_term_reference_flag", slice->long_term_reference_flag);

            // clear long_term_reference_flag flag
            slice->long_term_reference_flag = 0;
        } else {
            h264e_dpb_mmco("mmco count %d\n", slice->mmco_cnt);

            if (!list_empty(&slice->mmco)) {
                H264eMmco *mmco, *n;

                h264e_stream_put_bits_with_detect(s, 1, 1,
                                                  "adaptive_ref_pic_marking_mode_flag");
                WR_LOG("adaptive_ref_pic_marking_mode_flag", 1);

                list_for_each_entry_safe(mmco, n, &slice->mmco, H264eMmco, list) {
                    h264e_dpb_mmco("del mmco %p at %p list %p %p\n",
                                   mmco, slice, mmco->list.prev, mmco->list.next);

                    list_del_init(&mmco->list);

                    h264e_stream_write_ue(s, mmco->mmco,
                                          "memory_management_control_operation");
                    WR_LOG("memory_management_control_operation", mmco->mmco);

                    switch (mmco->mmco) {
                    case 1 : {
                        h264e_stream_write_ue(s, mmco->difference_of_pic_nums_minus1,
                                              "difference_of_pic_nums_minus1");
                        WR_LOG("difference_of_pic_nums_minus1",
                               mmco->difference_of_pic_nums_minus1);
                    } break;
                    case 2 : {
                        h264e_stream_write_ue(s, mmco->long_term_pic_num ,
                                              "long_term_pic_num");
                        WR_LOG("long_term_pic_num",
                               mmco->long_term_pic_num);
                    } break;
                    case 3 : {
                        h264e_stream_write_ue(s, mmco->difference_of_pic_nums_minus1,
                                              "difference_of_pic_nums_minus1");
                        WR_LOG("difference_of_pic_nums_minus1",
                               mmco->difference_of_pic_nums_minus1);

                        h264e_stream_write_ue(s, mmco->long_term_frame_idx ,
                                              "long_term_frame_idx");
                        WR_LOG("long_term_frame_idx", mmco->long_term_frame_idx);
                    } break;
                    case 4 : {
                        h264e_stream_write_ue(s, mmco->max_long_term_frame_idx_plus1,
                                              "max_long_term_frame_idx_plus1");
                        WR_LOG("max_long_term_frame_idx_plus1",
                               mmco->max_long_term_frame_idx_plus1);
                    } break;
                    case 5 : {
                    } break;
                    case 6 : {
                        h264e_stream_write_ue(s, mmco->long_term_frame_idx,
                                              "long_term_frame_idx");
                        WR_LOG("long_term_frame_idx", mmco->long_term_frame_idx);
                    } break;
                    default : {
                        mpp_err_f("invalid mmco %d\n", mmco->mmco);
                    } break;
                    }

                    h264e_dpb_mmco("free %p\n", mmco);
                    MPP_FREE(mmco);

                    slice->mmco_cnt--;
                }

                h264e_stream_write_ue(s, 0,
                                      "memory_management_control_operation");
            } else {
                h264e_stream_put_bits_with_detect(s, 0, 1,
                                                  "adaptive_ref_pic_marking_mode_flag");
                WR_LOG("adaptive_ref_pic_marking_mode_flag", 0);
            }

            mpp_assert(slice->mmco_cnt == 0);
        }
    }

    if (slice->entropy_coding_mode && slice->slice_type != H264E_HAL_SLICE_TYPE_I) {
        h264e_stream_write_ue(s, slice->cabac_init_idc, "cabac_init_idc");
        WR_LOG("cabac_init_idc", slice->cabac_init_idc);
    }

    h264e_stream_write_se(s, slice->qp_delta, "qp_delta");
    WR_LOG("qp_delta", slice->qp_delta);

    h264e_stream_write_ue(s, 0, "disable_deblocking_filter_idc");
    WR_LOG("disable_deblocking_filter_idc", slice->disable_deblocking_filter_idc);

    h264e_stream_write_se(s, 0, "slice_alpha_c0_offset_div2");
    WR_LOG("slice_alpha_c0_offset_div2", slice->slice_alpha_c0_offset_div2);

    h264e_stream_write_se(s, 0, "slice_beta_offset_div2");
    WR_LOG("slice_beta_offset_div2", slice->slice_beta_offset_div2);

    bitCnt = s->buffered_bits + s->byte_cnt * 8;

    if (s->buffered_bits) {
        // NOTE: only have aligned bit on cabac mode but cavlc also need to write byte cache to memory
        RK_S32 left = (8 - s->buffered_bits);
        for (i = 0; i < left; i++) {
            h264e_stream_put_bits_with_detect(s, 1, 1, "align_bit");
            WR_LOG("align_bit", 1);
        }
    }

    // update on cabac mode
    if (slice->entropy_coding_mode)
        bitCnt = s->buffered_bits + s->byte_cnt * 8;

    WR_LOG("all done", 0);

    RK_S32 pos = 0;
    char log[256];

    pos = sprintf(log + pos, "sw stream: ");
    for (i = 0; i < 16; i ++) {
        pos += sprintf(log + pos, "%02x ", tmp[i]);
    }
    pos += sprintf(log + pos, "\n");
    h264e_dpb_slice(log);

    return bitCnt;
}

H264eMmco *h264e_slice_gen_mmco(RK_S32 id, RK_S32 arg0, RK_S32 arg1)
{
    H264eMmco *mmco = mpp_malloc(H264eMmco, 1);
    if (NULL == mmco) {
        mpp_err_f("failed to malloc mmco structure\n");
        return NULL;
    }
    mmco->mmco = id;
    switch (id) {
    case 0 : {
    } break;
    case 1 : {
        mmco->difference_of_pic_nums_minus1 = arg0;
    } break;
    case 2 : {
        mmco->long_term_pic_num = arg0;
    } break;
    case 3 : {
        mmco->difference_of_pic_nums_minus1 = arg0;
        mmco->long_term_pic_num = arg1;
    } break;
    case 4 : {
        mmco->max_long_term_frame_idx_plus1 = arg0;
    } break;
    case 5 : {
    } break;
    case 6 : {
        mmco->long_term_frame_idx = arg0;
    } break;
    default : {
        mpp_err_f("invalid mmco %d\n", id);
    } break;
    }

    INIT_LIST_HEAD(&mmco->list);

    h264e_dpb_mmco("gen mmco %p\n", mmco);

    return mmco;
}

MPP_RET h264e_slice_add_mmco(H264eSlice *slice, H264eMmco *mmco)
{
    list_add_tail(&mmco->list, &slice->mmco);
    slice->mmco_cnt++;

    h264e_dpb_mmco("add mmco %p to %p list %p %p\n",
                   mmco, slice, mmco->list.prev, mmco->list.next);

    return MPP_OK;
}

MPP_RET h264e_slice_update(H264eSlice *slice, void *p)
{
    H264eDpb *dpb = (H264eDpb *)p;
    H264eDpbFrm *frm = &dpb->frames[dpb->curr_idx];

    slice->nal_reference_idc = !frm->info.is_non_ref;
    h264e_dpb_slice("gop %d nal_reference_idc %d is_non_ref %d\n",
                    frm->gop_idx, slice->nal_reference_idc,
                    frm->info.is_non_ref);

    // update reorder according to current dpb status
    if (dpb->need_reorder) {
        H264eDpbFrm *ref = h264e_dpb_get_refr(frm);

        h264e_dpb_slice("reorder to frm %d gop %d idx %d\n",
                        ref->frm_cnt, ref->gop_cnt, ref->gop_idx);

        slice->ref_pic_list_modification_flag = 1;
        mpp_assert(!ref->info.is_non_ref);

        slice->modification_of_pic_nums_idc = (ref->info.is_lt_ref) ? (2) : (0);
        if (ref->info.is_lt_ref) {
            slice->modification_of_pic_nums_idc = 2;
            slice->long_term_pic_idx = ref->lt_idx;

            h264e_dpb_slice("reorder lt idx %d \n", slice->long_term_pic_idx);
        } else {
            /* Only support ref pic num less than current pic num case */
            slice->modification_of_pic_nums_idc = 0;
            slice->abs_diff_pic_num_minus1 = MPP_ABS(frm->frame_num - ref->frame_num) - 1;

            h264e_dpb_slice("reorder st cur %d ref %d diff - 1 %d\n",
                            frm->frame_num, ref->frame_num,
                            slice->abs_diff_pic_num_minus1);
        }

        dpb->need_reorder = 0;
    }

    // remove frame if they are not used any more
    // NOTE: only add marking on reference frame
    INIT_LIST_HEAD(&slice->mmco);
    slice->mmco_cnt = 0;

    if (!frm->info.is_non_ref) {
        H264eMmco *mmco = NULL;

        if (frm->info.is_lt_ref) {
            if (frm->info.is_idr) {
                // IDR long-term is directly flag
                slice->long_term_reference_flag = 1;
            } else {
                if (dpb->next_max_lt_idx != dpb->curr_max_lt_idx) {
                    RK_S32 max_lt_idx_p1 = dpb->next_max_lt_idx + 1;

                    mmco = h264e_slice_gen_mmco(4, max_lt_idx_p1, 0);
                    h264e_dpb_mmco("add mmco 4 %d\n", max_lt_idx_p1);

                    h264e_slice_add_mmco(slice, mmco);
                    dpb->curr_max_lt_idx = dpb->next_max_lt_idx;
                }

                mmco = h264e_slice_gen_mmco(6, frm->lt_idx, 0);
                h264e_dpb_mmco("add mmco 6 %d\n", frm->lt_idx);
                h264e_slice_add_mmco(slice, mmco);
            }
        }

        if (dpb->unref_cnt) {
            RK_U32 i = 0;
            RK_S32 cur_frm_num = frm->frame_num;

            for (i = 0; i < dpb->unref_cnt; i++) {
                H264eDpbFrm *unref = dpb->unref[i];
                RK_S32 unref_frm_num = unref->frame_num;

                if (!unref->info.is_lt_ref) {
                    RK_S32 difference_of_pic_nums_minus1 = MPP_ABS(unref_frm_num - cur_frm_num) - 1;

                    h264e_dpb_mmco("cur_frm_num %d unref_frm_num %d\n", cur_frm_num, unref_frm_num);
                    h264e_dpb_mmco("add mmco st 1 %d\n", difference_of_pic_nums_minus1);

                    mmco = h264e_slice_gen_mmco(1, difference_of_pic_nums_minus1, 0);
                } else {
                    h264e_dpb_mmco("add mmco lt 2 %d\n", unref->lt_idx);

                    mmco = h264e_slice_gen_mmco(2, unref->lt_idx, 0);
                }
                h264e_slice_add_mmco(slice, mmco);

                /* update frame status here */
                unref->marked_unref = 0;
                unref->info.is_non_ref = 1;
                unref->on_used = 0;

                h264e_dpb_mmco("set frm %d gop %d idx %d to not used\n",
                               unref->frm_cnt, unref->gop_cnt, unref->gop_idx);

                // decrease dpb size here for sometime frame will be remove later
                dpb->dpb_size--;
            }
            dpb->unref_cnt = 0;
        }

        if (hal_h264e_debug & H264E_DBG_SLICE)
            h264e_dpb_dump_frms(dpb);
    }

    return MPP_OK;
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
        if (hal_h264e_debug & H264E_DBG_SLICE)
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

    if (hal_h264e_debug & H264E_DBG_SLICE)
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
            if (hal_h264e_debug & H264E_DBG_SLICE)
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

        if (hal_h264e_debug & H264E_DBG_SLICE) {
            if (i < 10) {
                mpp_log("%03d src [%04x] -> [%04x] + last [%04x] -> %04x\n", i, tmp16a, tmp16b, last_tmp, tmp16c);
            }
            if (i >= loop - 10) {
                mpp_log("%03d src [%04x] -> [%04x] + last [%04x] -> %04x\n", i, tmp16a, tmp16b, last_tmp, tmp16c);
            }
        }

        if (dst_zero_cnt == 2 && pdst[0] <= 0x3) {
            if (hal_h264e_debug & H264E_DBG_SLICE)
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
