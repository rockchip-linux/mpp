/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef __HAL_M2VD_VDPU2_REG_H__
#define __HAL_M2VD_VDPU2_REG_H__

#include "rk_type.h"

extern RK_U32 m2vh_debug;

#define M2VD_VDPU2_REG_NUM    159

typedef struct M2vdVdpu2Reg_t {
    RK_U32                 ppReg[50];

    struct {
        RK_U32  dec_out_tiled_e  : 1;
        RK_U32  dec_latency      : 6;
        RK_U32  pic_fixed_quant  : 1;
        RK_U32  filtering_dis    : 1;
        RK_U32  skip_mode        : 1;
        RK_U32  dec_scmd_dis     : 1;
        RK_U32  dec_adv_pre_dis  : 1;
        RK_U32  priority_mode    : 1;   //chang
        RK_U32  refbu2_thr       : 12;
        RK_U32  refbu2_picid     : 5;
        RK_U32  reserve1         : 2;
    } sw50;

    struct {
        RK_U32  stream_len       : 24;
        RK_U32  reserve1         : 1;
        RK_U32  init_qp          : 6;
        RK_U32  reserve2         : 1;
    } sw51;

    struct {
        RK_U32  startmb_y        : 8;
        RK_U32  startmb_x        : 9;
        RK_U32  apf_threshold    : 14;
        RK_U32  reserve          : 1;
    } sw52;

    struct {
        RK_U32  sw_dec_mode;
    } sw53;

    struct {
        RK_U32  dec_in_endian    : 1;
        RK_U32  dec_out_endian   : 1;
        RK_U32  dec_inswap32_e   : 1;
        RK_U32  dec_outswap32_e  : 1;
        RK_U32  dec_strswap32_e  : 1;
        RK_U32  dec_strendian_e  : 1;
        RK_U32  reserve3         : 26;
    } sw54;

    struct {
        RK_U32  dec_irq          : 1;
        RK_U32  dec_irq_dis      : 1;
        RK_U32  reserve0         : 2;
        RK_U32  dec_rdy_int      : 1;
        RK_U32  dec_bus_int      : 1;
        RK_U32  dec_buffer_int   : 1;
        RK_U32  reserve1         : 1;
        RK_U32  dec_aso_int      : 1;
        RK_U32  dec_slice_int    : 1;
        RK_U32  dec_pic_inf      : 1;
        RK_U32  reserve2         : 1;
        RK_U32  dec_error_int    : 1;
        RK_U32  dec_timeout      : 1;
        RK_U32  reserve3         : 18;
    } sw55;

    struct {
        RK_U32  dec_axi_rn_id    : 8;
        RK_U32  dec_axi_wr_id    : 8;
        RK_U32  dec_max_burst    : 5;
        RK_U32  resever          : 1;
        RK_U32  dec_data_disc_e  : 1;
        RK_U32  resever1         : 9;
    } sw56;

    struct {
        RK_U32  dec_e            : 1;
        RK_U32  refbu2_buf_e     : 1;
        RK_U32  dec_out_dis      : 1;
        RK_U32  resever          : 1;
        RK_U32  dec_clk_gate_e   : 1;
        RK_U32  dec_timeout_e    : 1;
        RK_U32  picord_count_e   : 1;
        RK_U32  seq_mbaff_e      : 1;
        RK_U32  reftopfirst_e    : 1;
        RK_U32  ref_topfield_e   : 1;
        RK_U32  write_mvs_e      : 1;
        RK_U32  sorenson_e       : 1;
        RK_U32  fwd_interlace_e  : 1;
        RK_U32  pic_topfield_e   : 1;
        RK_U32  pic_inter_e      : 1;
        RK_U32  pic_b_e          : 1;
        RK_U32  pic_fieldmode_e  : 1;
        RK_U32  pic_interlace_e  : 1;
        RK_U32  pjpeg_e          : 1;
        RK_U32  divx3_e          : 1;
        RK_U32  rlc_mode_e       : 1;
        RK_U32  ch_8pix_ileav_e  : 1;
        RK_U32  start_code_e     : 1;
        RK_U32  resever1         : 8;
        RK_U32 dec_ahb_hlock_e   : 1;

    } sw57;

    RK_U32                 reserve0[3];

    struct {
        RK_U32 slice_table;
    } sw61;

    struct {
        RK_U32 directmv_reg;
    } sw62;

    struct {
        RK_U32 cur_pic_base;
    } sw63;

    struct {
        RK_U32 VLC_base;
    } sw64;

    RK_U32                 reserve1[55];

    struct {
        RK_U32  ref_frames       : 5;
        RK_U32  topfieldfirst_e  : 1;
        RK_U32  alt_scan_e       : 1;
        RK_U32  mb_height_off    : 4;
        RK_U32  pic_mb_height_p  : 8;
        RK_U32  mb_width_off     : 4;
        RK_U32  pic_mb_width     : 9;
    } sw120;

    RK_U32                 reserve2;

    struct {
        RK_U32  frame_pred_dct   : 1;
        RK_U32  intra_vlc_tab    : 1;
        RK_U32  intra_dc_prec    : 2;
        RK_U32  con_mv_e         : 1;
        RK_U32  reserve          : 19;
        RK_U32  qscale_type      : 1;
        RK_U32  reserve1         : 1;
        RK_U32  stream_start_bit : 6;
    } sw122;

    RK_U32                 reserve3[8];

    struct {
        RK_U32  ref0;
    } sw131;

    RK_U32                 reserve4[2];

    struct {
        RK_U32 ref2;
    } sw134;

    struct {
        RK_U32 ref3;
    } sw135;

    struct {
        RK_U32  reserve          : 1;
        RK_U32  mv_accuracy_bwd  : 1;
        RK_U32  mv_accuracy_fwd  : 1;
        RK_U32  fcode_bwd_ver    : 4;
        RK_U32  fcode_bwd_hor    : 4;
        RK_U32  fcode_fwd_ver    : 4;
        RK_U32  fcode_fwd_hor    : 4;
        RK_U32  alt_scan_flag_e  : 1;
        RK_U32  reserve1         : 12;
    } sw136;

    RK_U32                 reserve5[11];

    struct {
        RK_U32 ref1;

    } sw148;

    RK_U32                 reserve6[10];

} M2vdVdpu2Reg;

#endif
