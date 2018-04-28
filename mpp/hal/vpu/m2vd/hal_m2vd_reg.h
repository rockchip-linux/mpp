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

#ifndef __HAL_M2VD_REG_H__
#define __HAL_M2VD_REG_H__

#include "m2vd_syntax.h"
#include "hal_task.h"
#include "mpp_hal.h"
#include "mpp_buf_slot.h"
#include "mpp_device.h"
#include <stdio.h>


extern RK_U32 m2vh_debug;

#define M2VH_DBG_FUNCTION          (0x00000001)
#define M2VH_DBG_REG               (0x00000002)
#define M2VH_DBG_DUMP_REG          (0x00000004)
#define M2VH_DBG_IRQ               (0x00000008)




#define M2VD_REG_NUM    159


typedef struct {
    RK_U32  build_version   : 3;
    RK_U32  product_IDen    : 1;
    RK_U32  minor_version   : 8;
    RK_U32  major_version   : 4;
    RK_U32  product_numer   : 16;
} ID_reg;

typedef struct {
    RK_U32  sw_dec_irq      : 1;
    RK_U32  sw_dec_irq_dis  : 1;
    RK_U32  reserve0        : 2;
    RK_U32  sw_dec_rdy_int  : 1;
    RK_U32  sw_dec_bus_int  : 1;
    RK_U32  sw_dec_buffer_int   : 1;
    RK_U32  reserve1        : 1;
    RK_U32  sw_dec_aso_int  : 1;
    RK_U32  sw_dec_slice_int    : 1;
    RK_U32  sw_dec_pic_inf  : 1;
    RK_U32  reserve2        : 1;
    RK_U32  sw_dec_error_int: 1;
    RK_U32  sw_dec_timeout  : 1;
    RK_U32  reserve3        : 18;
} Dec_Interrupt_reg;


typedef struct {
    RK_U32  sw_dec_out_tiled_e  : 1;
    RK_U32  sw_dec_latency      : 6;
    RK_U32  sw_pic_fixed_quant  : 1;
    RK_U32  sw_filtering_dis    : 1;
    RK_U32  sw_skip_mode        : 1;
    RK_U32  sw_dec_scmd_dis     : 1;
    RK_U32  sw_dec_adv_pre_dis  : 1;
    RK_U32  sw_priority_mode    : 1;            //chang
    RK_U32  sw_refbu2_thr       : 12;
    RK_U32  sw_refbu2_picid     : 5;
    RK_U32  reserve1            : 2;
} Device_config_reg1;


typedef struct {
    RK_U32  sw_stream_len       : 24;
    RK_U32  reserve1            : 1;
    RK_U32  sw_init_qp          : 6;
    RK_U32  reserve2           : 1;
} Dec_control_reg3;


typedef struct {
    RK_U32  sw_dec_in_endian    : 1;
    RK_U32  sw_dec_out_endian   : 1;
    RK_U32  sw_dec_inswap32_e   : 1;
    RK_U32  sw_dec_outswap32_e  : 1;
    RK_U32  sw_dec_strswap32_e  : 1;
    RK_U32  sw_dec_strendian_e  : 1;
    RK_U32  reserve3            : 26;
} Device_config_reg2;


typedef struct {
    RK_U32  sw_dec_axi_rn_id    : 8;
    RK_U32  sw_dec_axi_wr_id    : 8;
    RK_U32  sw_dec_max_burst    : 5;
    RK_U32  resever             : 1;
    RK_U32  sw_dec_data_disc_e  : 1;
    RK_U32  resever1            : 9;
} Device_config_reg3;

typedef struct {
    RK_U32  sw_dec_e            : 1;
    RK_U32  sw_refbu2_buf_e     : 1;
    RK_U32  sw_dec_out_dis      : 1;
    RK_U32  resever             : 1;
    RK_U32  sw_dec_clk_gate_e   : 1;
    RK_U32  sw_dec_timeout_e    : 1;
    RK_U32  sw_picord_count_e   : 1;
    RK_U32  sw_seq_mbaff_e      : 1;
    RK_U32  sw_reftopfirst_e    : 1;
    RK_U32  sw_ref_topfield_e   : 1;
    RK_U32  sw_write_mvs_e      : 1;
    RK_U32  sw_sorenson_e       : 1;
    RK_U32  sw_fwd_interlace_e  : 1;
    RK_U32  sw_pic_topfield_e   : 1 ;
    RK_U32  sw_pic_inter_e      : 1;
    RK_U32  sw_pic_b_e          : 1;
    RK_U32  sw_pic_fieldmode_e  : 1;
    RK_U32  sw_pic_interlace_e  : 1;
    RK_U32  sw_pjpeg_e          : 1;
    RK_U32  sw_divx3_e          : 1;
    RK_U32  sw_rlc_mode_e       : 1;
    RK_U32  sw_ch_8pix_ileav_e  : 1;
    RK_U32  sw_start_code_e     : 1;
    RK_U32  resever1            : 8;
    RK_U32 sw_dec_ahb_hlock_e  : 1;

} Dec_control_reg0;

typedef struct {
    RK_U32  sw_ref_frames       : 5;
    RK_U32  sw_topfieldfirst_e  : 1;
    RK_U32  sw_alt_scan_e       : 1;
    RK_U32  sw_mb_height_off    : 4;
    RK_U32  sw_pic_mb_height_p  : 8;
    RK_U32  sw_mb_width_off : 4;
    RK_U32  sw_pic_mb_width : 9;
} Dec_control_reg1;

typedef struct {
    RK_U32  sw_frame_pred_dct   : 1;
    RK_U32  sw_intra_vlc_tab        : 1;
    RK_U32  sw_intra_dc_prec    : 2;
    RK_U32  sw_con_mv_e     : 1;
    RK_U32  reserve             : 19;
    RK_U32  sw_qscale_type      : 1;
    RK_U32  reserve1            : 1;
    RK_U32  sw_stream_start_bit : 6;
} Dec_control_reg2;

typedef struct {
    RK_U32  reserve             : 1;
    RK_U32  sw_mv_accuracy_bwd  : 1;
    RK_U32  sw_mv_accuracy_fwd  : 1;
    RK_U32  sw_fcode_bwd_ver        : 4;
    RK_U32  sw_fcode_bwd_hor        : 4;
    RK_U32  sw_fcode_fwd_ver        : 4;
    RK_U32  sw_fcode_fwd_hor        : 4;
    RK_U32  sw_alt_scan_flag_e      : 1;
    RK_U32  reserve1                : 12;
} Dec_BaseAdd_Ref4_reg;


typedef struct {
    RK_U32  sw_startmb_y        : 8;
    RK_U32  sw_startmb_x        : 9;
    RK_U32  sw_apf_threshold    : 14;
    RK_U32  sw_reserve          : 1;
} Dec_Error_concealment_reg;

typedef struct {
    RK_U32  sw_dec_max_owidth   : 11;
    RK_U32  sw_dec_soren_prof   : 1;
    RK_U32  sw_dec_bus_width    : 2;
    RK_U32  sw_dec_synth_lan    : 2;
    RK_U32  sw_dec_bus_strd : 4;
    RK_U32  sw_ref_buff_exist   : 1;
    RK_U32  sw_dec_obuff_leve   : 1;
    RK_U32  sw_dec_pjpeg_exist  : 1;
    RK_U32  sw_vp6_prof         : 1;
    RK_U32  sw_dec_h264_prof    : 2;
    RK_U32  sw_dec_mpeg4_prof   : 2;
    RK_U32  sw_dec_jpeg_prof    : 1;
    RK_U32  sw_dec_vc1_prof : 2;
    RK_U32  sw_dec_mpeg2_prof   : 1;
} Dec_Synthesis_config_reg;

typedef struct {
    RK_U32  sw_refbu_y_offset   : 9;
    RK_U32  sw_reserve          : 3;
    RK_U32  sw_refbu_fparmod_e  : 1;
    RK_U32  sw_refbu_eval_e     : 1;
    RK_U32  sw_refbu_picid      : 5;
    RK_U32  sw_refbu_thr        : 12;
    RK_U32  sw_refbu_e          : 1;
} Dec_Refpicbuff_control_reg;

typedef struct {
    RK_U32  sw_refbu_intra_sum  : 16;
    RK_U32  sw_refbu_hit_sum    : 16;
} Dec_Refpicbuff_info1_reg;

typedef struct {
    RK_U32  sw_refbu_mv_sum : 22;
    RK_U32  sw_reserve          : 10;
} Dec_Refpicbuff_info2_reg;

typedef struct {
    RK_U32  sw_reserve          : 25;
    RK_U32  sw_dec_rtl_rom      : 1;
    RK_U32  sw_dec_rv_prof      : 2;
    RK_U32  sw_ref_buff2_exist  : 1;
    RK_U32  sw_dec_divx_prof    : 1;
    RK_U32  sw_dec_refbu_ilace  : 1;
    RK_U32  sw_dec_jpeg_exten   : 1;
} Dec_Syn_configinfo_reg;

typedef struct {
    RK_U32  sw_refbu_top_sum    : 16;
    RK_U32  sw_refbu_bot_sum    : 16;
} Dec_Refpicbuff_info3_reg;

typedef struct {
    RK_U32  sw_reserve0         : 24;
    RK_U32  sw_dec_vc1          : 1;
    RK_U32  sw_dec_vp6          : 1;
    RK_U32  sw_dec_jpeg         : 1;
    RK_U32  sw_dec_sorenson     : 1;
    RK_U32  sw_dec_mpeg2        : 1;
    RK_U32  sw_dec_mpeg4        : 1;
    RK_U32  sw_dec_h264         : 1;
    RK_U32  sw_reserve1         : 2;
} Dec_fuse_reg;

typedef struct {
    RK_U32  debug_dec_mb_count  : 13;
    RK_U32  debug_referreq1     : 1;
    RK_U32  debug_referreq0     : 1;
    RK_U32  debug_filter_req        : 1;
    RK_U32  debug_framerdy      : 1;
    RK_U32  debug_strm_da_e : 1;
    RK_U32  debug_res_c_req     : 1;
    RK_U32  debug_res_y_req : 1;
    RK_U32  debug_rlc_req       : 1;
    RK_U32  debug_mv_req        : 10;
} Dec_Debug_reg;

typedef struct {
    RK_U32  sw_reserve          : 2;
    RK_U32  sw_dec_ch8pix_base  : 30;
} Dec_BaseAdd_ch8pix_reg;

typedef struct M2VDRegSet_t {
    RK_U32 ppReg[50];
    Device_config_reg1            config1;
    Dec_control_reg3              stream_buffinfo;
    Dec_Error_concealment_reg     error_position;
    RK_U32                        sw_dec_mode;
    Device_config_reg2            config2;
    Dec_Interrupt_reg             interrupt;
    Device_config_reg3            config3;
    Dec_control_reg0              control;
    RK_U32                        reserve0[2];
    Dec_BaseAdd_ch8pix_reg        ch8pix;
    RK_U32                        slice_table;
    RK_U32                        directmv_reg;
    RK_U32                        cur_pic_base;
    RK_U32                        VLC_base;
    Dec_Refpicbuff_control_reg    refbuff_ctl;
    RK_U32                        reserve1;
    Dec_Syn_configinfo_reg        syn_cfg;
    Dec_Refpicbuff_info3_reg      refbuff_info3;
    Dec_Refpicbuff_info1_reg      refbuff_info1;
    Dec_Refpicbuff_info2_reg      refbuff_info2;
    Dec_Synthesis_config_reg      syn_config;
    RK_U32                        reserve2;
    Dec_Debug_reg                 debug_info;
    RK_U32                        reserve3[46];
    Dec_control_reg1              pic_params;
    RK_U32                        reserve4;
    Dec_control_reg2              stream_bitinfo;
    RK_U32                        reserve5[8];
    RK_U32                        ref0;
    RK_U32                        reserve6[2];
    RK_U32                        ref2;
    RK_U32                        ref3;
    Dec_BaseAdd_Ref4_reg          dec_info;
    RK_U32                        reserve7[11];
    RK_U32                        ref1;
    RK_U32                        reserve8[10];


} M2VDRegSet;

typedef struct M2VDHalContext_t {
    MppBufSlots packet_slots;
    MppBufSlots frame_slots;
    M2VDRegSet  regs;
    MppBufferGroup group;
    MppBuffer qp_table;
    RK_U32 dec_frame_cnt;
    IOInterruptCB int_cb;
    MppDevCtx dev_ctx;
    FILE *fp_reg_in;
    FILE *fp_reg_out;
} M2VDHalContext;

MPP_RET hal_m2vd_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_m2vd_deinit  (void *hal);
MPP_RET hal_m2vd_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_m2vd_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_m2vd_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_m2vd_reset   (void *hal);
MPP_RET hal_m2vd_flush   (void *hal);
MPP_RET hal_m2vd_control (void *hal, RK_S32 cmd_type, void *param);

#endif
