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

#ifndef __HAL_H264D_VDPU2_REG_TBL_H__
#define __HAL_H264D_VDPU2_REG_TBL_H__

#include "rk_type.h"

/* Number registers for the decoder */
#define DEC_VDPU_REGISTERS          159

typedef struct {
    RK_U32 sw00_49[50];
    struct {
        RK_U32 dec_tiled_msb : 1;
        RK_U32 adtion_latency : 6;
        RK_U32 dec_fixed_quant : 1;
        RK_U32 dblk_flt_dis : 1;
        RK_U32 skip_sel : 1;
        RK_U32 dec_ascmd0_dis : 1;
        RK_U32 adv_pref_dis : 1;
        RK_U32 dec_tiled_lsb : 1;
        RK_U32 refbuf_thrd : 12;
        RK_U32 refbuf_pid : 5;
        RK_U32 reverse0 : 2;
    } sw50;
    struct {
        RK_U32 stream_len : 24;
        RK_U32 stream_len_ext : 1;
        RK_U32 qp_init_val : 6;
        RK_U32 reverse0 : 1;
    } sw51;
    struct {
        RK_U32 ydim_mbst : 8;
        RK_U32 xdim_mbst : 9;
        RK_U32 adv_pref_thrd : 14;
        RK_U32 reverse0 : 1;
    } sw52;
    struct {
        RK_U32 dec_fmt_sel : 4;
        RK_U32 reverse0 : 28;
    } sw53;
    struct {
        RK_U32 dec_in_endian : 1;
        RK_U32 dec_out_endian : 1;
        RK_U32 dec_in_wordsp : 1;
        RK_U32 dec_out_wordsp : 1;
        RK_U32 dec_strm_wordsp : 1;
        RK_U32 dec_strendian_e : 1;
        RK_U32 reverse0 : 26;
    } sw54;
    struct {
        RK_U32 dec_irq : 1;
        RK_U32 dec_irq_dis : 1;
        RK_U32 reverse0 : 2;
        RK_U32 dec_rdy_sts : 1;
        RK_U32 pp_bus_sts : 1;
        RK_U32 buf_emt_sts : 1;
        RK_U32 reverse1 : 1;
        RK_U32 aso_det_sts : 1;
        RK_U32 slice_det_sts : 1;
        RK_U32 bslice_det_sts : 1;
        RK_U32 reverse2 : 1;
        RK_U32 error_det_sts : 1;
        RK_U32 timeout_det_sts : 1;
        RK_U32 reverse3 : 18;
    } sw55;
    struct {
        RK_U32 dec_axi_id_rd : 8;
        RK_U32 dec_axi_id_wr : 8;
        RK_U32 dec_max_burlen : 5;
        RK_U32 bus_pos_sel : 1;
        RK_U32 dec_data_discd_en : 1;
        RK_U32 axi_sel : 1;
        RK_U32 reverse0 : 8;
    } sw56;
    struct {
        RK_U32 dec_st_work : 1;
        RK_U32 refpic_buf2_en : 1;
        RK_U32 dec_wr_extmen_dis : 1;
        RK_U32 reverse0 : 1;
        RK_U32 dec_clkgate_en : 1;
        RK_U32 timeout_sts_en : 1;
        RK_U32 rd_cnt_tab_en : 1;
        RK_U32 sequ_mbaff_en : 1;
        RK_U32 first_reftop_en : 1;
        RK_U32 reftop_en : 1;
        RK_U32 dmmv_wr_en : 1;
        RK_U32 sorspa_en : 1;
        RK_U32 fwd_refpic_mode_sel : 1;
        RK_U32 pic_decfield_sel : 1;
        RK_U32 pic_type_sel0 : 1;
        RK_U32 pic_type_sel1 : 1;
        RK_U32 curpic_stru_sel : 1;
        RK_U32 curpic_code_sel : 1;
        RK_U32 prog_jpeg_en : 1;
        RK_U32 divx3_en : 1;
        RK_U32 rlc_mode_en : 1;
        RK_U32 addit_ch_fmt_wen : 1;
        RK_U32 st_code_exit : 1;
        RK_U32 reverse1 : 2;
        RK_U32 inter_dblspeed : 1;
        RK_U32 intra_dblspeed : 1;
        RK_U32 intra_dbl3t : 1;
        RK_U32 pref_sigchan : 1;
        RK_U32 cache_en : 1;
        RK_U32 reverse2 : 1;
        RK_U32 dec_timeout_mode : 1;
    } sw57;
    struct {
        RK_U32 soft_rst : 1;
        RK_U32 reverse0 : 31;
    } sw58;
    struct {
        RK_U32 reverse0 : 2;
        RK_U32 pflt_set0_tap2 : 10;
        RK_U32 pflt_set0_tap1 : 10;
        RK_U32 pflt_set0_tap0 : 10;
    } sw59;
    struct {
        RK_U32 addit_ch_st_adr : 32;
    } sw60;
    struct {
        RK_U32 qtable_st_adr : 32;
    } sw61;
    struct {
        RK_U32 dmmv_st_adr : 32;
    } sw62;
    struct {
        RK_U32 dec_out_st_adr : 32;
    } sw63;
    struct {
        RK_U32 rlc_vlc_st_adr : 32;
    } sw64;
    struct {
        RK_U32 refbuf_y_offset : 9;
        RK_U32 reserve0 : 3;
        RK_U32 refbuf_fildpar_mode_e : 1;
        RK_U32 refbuf_idcal_e : 1;
        RK_U32 refbuf_picid : 5;
        RK_U32 refbuf_thr_level : 12;
        RK_U32 refbuf_e : 1;
    } sw65;
    RK_U32 sw66;
    RK_U32 sw67;
    struct {
        RK_U32 refbuf_sum_bot : 16;
        RK_U32 refbuf_sum_top : 16;
    } sw68;
    struct {
        RK_U32 luma_sum_intra : 16;
        RK_U32 refbuf_sum_hit : 16;
    } sw69;
    struct {
        RK_U32 ycomp_mv_sum : 22;
        RK_U32 reserve0 : 10;
    } sw70;
    RK_U32 sw71;
    RK_U32 sw72;
    RK_U32 sw73;
    struct {
        RK_U32 init_reflist_pf4 : 5;
        RK_U32 init_reflist_pf5 : 5;
        RK_U32 init_reflist_pf6 : 5;
        RK_U32 init_reflist_pf7 : 5;
        RK_U32 init_reflist_pf8 : 5;
        RK_U32 init_reflist_pf9 : 5;
        RK_U32 reverse0 : 2;
    } sw74;
    struct {
        RK_U32 init_reflist_pf10 : 5;
        RK_U32 init_reflist_pf11 : 5;
        RK_U32 init_reflist_pf12 : 5;
        RK_U32 init_reflist_pf13 : 5;
        RK_U32 init_reflist_pf14 : 5;
        RK_U32 init_reflist_pf15 : 5;
        RK_U32 reverse0 : 2;
    } sw75;
    struct {
        RK_U32 num_ref_idx0 : 16;
        RK_U32 num_ref_idx1 : 16;
    } sw76;
    struct {
        RK_U32 num_ref_idx2 : 16;
        RK_U32 num_ref_idx3 : 16;
    } sw77;
    struct {
        RK_U32 num_ref_idx4 : 16;
        RK_U32 num_ref_idx5 : 16;
    } sw78;
    struct {
        RK_U32 num_ref_idx6 : 16;
        RK_U32 num_ref_idx7 : 16;
    } sw79;
    struct {
        RK_U32 num_ref_idx8 : 16;
        RK_U32 num_ref_idx9 : 16;
    } sw80;
    struct {
        RK_U32 num_ref_idx10 : 16;
        RK_U32 num_ref_idx11 : 16;
    } sw81;
    struct {
        RK_U32 num_ref_idx12 : 16;
        RK_U32 num_ref_idx13 : 16;
    } sw82;
    struct {
        RK_U32 num_ref_idx14 : 16;
        RK_U32 num_ref_idx15 : 16;
    } sw83;
    union {
        RK_U32 ref0_st_addr;
        struct {
            RK_U32 ref0_closer_sel : 1;
            RK_U32 ref0_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw84;
    union {
        RK_U32 ref1_st_addr;
        struct {
            RK_U32 ref1_closer_sel : 1;
            RK_U32 ref1_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw85;
    union {
        RK_U32 ref2_st_addr;
        struct {
            RK_U32 ref2_closer_sel : 1;
            RK_U32 ref2_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw86;
    union {
        RK_U32 ref3_st_addr;
        struct {
            RK_U32 ref3_closer_sel : 1;
            RK_U32 ref3_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw87;
    union {
        RK_U32 ref4_st_addr;
        struct {
            RK_U32 ref4_closer_sel : 1;
            RK_U32 ref4_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw88;
    union {
        RK_U32 ref5_st_addr;
        struct {
            RK_U32 ref5_closer_sel : 1;
            RK_U32 ref5_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw89;
    union {
        RK_U32 ref6_st_addr;
        struct {
            RK_U32 ref6_closer_sel : 1;
            RK_U32 ref6_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw90;
    union {
        RK_U32 ref7_st_addr;
        struct {
            RK_U32 ref7_closer_sel : 1;
            RK_U32 ref7_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw91;
    union {
        RK_U32 ref8_st_addr;
        struct {
            RK_U32 ref8_closer_sel : 1;
            RK_U32 ref8_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw92;
    union {
        RK_U32 ref9_st_addr;
        struct {
            RK_U32 ref9_closer_sel : 1;
            RK_U32 ref9_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw93;
    union {
        RK_U32 ref10_st_addr;
        struct {
            RK_U32 ref10_closer_sel : 1;
            RK_U32 ref10_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw94;
    union {
        RK_U32 ref11_st_addr;
        struct {
            RK_U32 ref11_closer_sel : 1;
            RK_U32 ref11_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw95;
    union {
        RK_U32 ref12_st_addr;
        struct {
            RK_U32 ref12_closer_sel : 1;
            RK_U32 ref12_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw96;
    union {
        RK_U32 ref13_st_addr;
        struct {
            RK_U32 ref13_closer_sel : 1;
            RK_U32 ref13_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw97;
    union {
        RK_U32 ref14_st_addr;
        struct {
            RK_U32 ref14_closer_sel : 1;
            RK_U32 ref14_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw98;
    union {
        RK_U32 ref15_st_addr;
        struct {
            RK_U32 ref15_closer_sel : 1;
            RK_U32 ref15_field_en : 1;
            RK_U32 reverse0 : 30;
        };
    } sw99;
    struct {
        RK_U32 init_reflist_df0 : 5;
        RK_U32 init_reflist_df1 : 5;
        RK_U32 init_reflist_df2 : 5;
        RK_U32 init_reflist_df3 : 5;
        RK_U32 init_reflist_df4 : 5;
        RK_U32 init_reflist_df5 : 5;
        RK_U32 reverse0 : 2;
    } sw100;
    struct {
        RK_U32 init_reflist_df6 : 5;
        RK_U32 init_reflist_df7 : 5;
        RK_U32 init_reflist_df8 : 5;
        RK_U32 init_reflist_df9 : 5;
        RK_U32 init_reflist_df10 : 5;
        RK_U32 init_reflist_df11 : 5;
        RK_U32 reverse0 : 2;
    } sw101;
    struct {
        RK_U32 init_reflist_df12 : 5;
        RK_U32 init_reflist_df13 : 5;
        RK_U32 init_reflist_df14 : 5;
        RK_U32 init_reflist_df15 : 5;
        RK_U32 reverse0 : 12;
    } sw102;
    struct {
        RK_U32 init_reflist_db0 : 5;
        RK_U32 init_reflist_db1 : 5;
        RK_U32 init_reflist_db2 : 5;
        RK_U32 init_reflist_db3 : 5;
        RK_U32 init_reflist_db4 : 5;
        RK_U32 init_reflist_db5 : 5;
        RK_U32 reverse0 : 2;
    } sw103;
    struct {
        RK_U32 init_reflist_db6 : 5;
        RK_U32 init_reflist_db7 : 5;
        RK_U32 init_reflist_db8 : 5;
        RK_U32 init_reflist_db9 : 5;
        RK_U32 init_reflist_db10 : 5;
        RK_U32 init_reflist_db11 : 5;
        RK_U32 reverse0 : 2;
    } sw104;
    struct {
        RK_U32 init_reflist_db12 : 5;
        RK_U32 init_reflist_db13 : 5;
        RK_U32 init_reflist_db14 : 5;
        RK_U32 init_reflist_db15 : 5;
        RK_U32 reverse0 : 12;
    } sw105;
    struct {
        RK_U32 init_reflist_pf0 : 5;
        RK_U32 init_reflist_pf1 : 5;
        RK_U32 init_reflist_pf2 : 5;
        RK_U32 init_reflist_pf3 : 5;
        RK_U32 reverse0 : 12;
    } sw106;
    struct {
        RK_U32 refpic_term_flag : 32;
    } sw107;
    struct {
        RK_U32 refpic_valid_flag : 32;
    } sw108;
    struct {
        RK_U32 strm_start_bit : 6;
        RK_U32 reverse0 : 26;
    } sw109;
    struct {
        RK_U32 pic_mb_w : 9;
        RK_U32 pic_mb_h : 8;
        RK_U32 flt_offset_cb_qp : 5;
        RK_U32 flt_offset_cr_qp : 5;
        RK_U32 reverse0 : 5;
    } sw110;
    struct {
        RK_U32 max_refnum : 5;
        RK_U32 reverse0 : 11;
        RK_U32 wp_bslice_sel : 2;
        RK_U32 reverse1 : 14;
    } sw111;
    struct {
        RK_U32 curfrm_num : 16;
        RK_U32 cur_frm_len : 5;
        RK_U32 reverse0 : 9;
        RK_U32 rpcp_flag : 1;
        RK_U32 dblk_ctrl_flag : 1;
    } sw112;
    struct {
        RK_U32 idr_pic_id : 16;
        RK_U32 refpic_mk_len : 11;
        RK_U32 reverse0 : 5;
    } sw113;
    struct {
        RK_U32 poc_field_len : 8;
        RK_U32 reverse0 : 6;
        RK_U32 max_refidx0 : 5;
        RK_U32 max_refidx1 : 5;
        RK_U32 pps_id : 5;
    } sw114;
    struct {
        RK_U32 fieldpic_flag_exist : 1;
        RK_U32 scl_matrix_en : 1;
        RK_U32 tranf_8x8_flag_en : 1;
        RK_U32 const_intra_en : 1;
        RK_U32 weight_pred_en : 1;
        RK_U32 cabac_en : 1;
        RK_U32 monochr_en : 1;
        RK_U32 dlmv_method_en : 1;
        RK_U32 idr_pic_flag : 1;
        RK_U32 reverse0 : 23;
    } sw115;
    RK_U32 sw116_158[43];
} H264dVdpuRegs_t;

#endif /*__HAL_H264D_VDPU_REG_TBL_H__*/
