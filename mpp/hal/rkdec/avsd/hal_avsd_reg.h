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

#ifndef __HAL_AVSD_REG_H__
#define __HAL_AVSD_REG_H__

#include "mpp_device.h"

#include "parser_api.h"
#include "hal_avsd_api.h"
#include "avsd_syntax.h"

#define AVSD_HAL_DBG_ERROR             (0x00000001)
#define AVSD_HAL_DBG_ASSERT            (0x00000002)
#define AVSD_HAL_DBG_WARNNING          (0x00000004)
#define AVSD_HAL_DBG_TRACE             (0x00000008)

#define AVSD_HAL_DBG_OFFSET            (0x00010000)

extern RK_U32 avsd_hal_debug;

#define AVSD_HAL_DBG(level, fmt, ...)\
do {\
    if (level & avsd_hal_debug)\
    { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define AVSD_HAL_TRACE(fmt, ...)\
do {\
    if (AVSD_HAL_DBG_TRACE & avsd_hal_debug)\
    { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)


#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {    \
        ret = MPP_ERR_INIT; \
        AVSD_HAL_DBG(AVSD_HAL_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
    }\
} while (0)


#define FUN_CHECK(val)\
do{\
    if ((val) < 0) {\
        AVSD_HAL_DBG(AVSD_HAL_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
        goto __FAILED; \
    }\
} while (0)


//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if (!(val)) {\
        ret = MPP_ERR_MALLOC; \
        mpp_err_f("malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
    }\
} while (0)


#define FIELDPICTURE    0
#define FRAMEPICTURE    1

enum {
    IFRAME = 0,
    PFRAME = 1,
    BFRAME = 2
};


typedef struct avsd_hal_picture_t {
    RK_U32 valid;
    RK_U32 pic_type;
    RK_U32 pic_code_type;
    RK_U32 picture_distance;

    RK_S32 slot_idx;
} AvsdHalPic_t;


typedef struct avsd_hal_ctx_t {

    MppBufSlots              frame_slots;
    MppBufSlots              packet_slots;
    MppBufferGroup           buf_group;
    IOInterruptCB            init_cb;
    MppDevCtx                dev_ctx;
    AvsdSyntax_t             syn;
    RK_U32                  *p_regs;
    MppBuffer                mv_buf;

    AvsdHalPic_t             pic[3];
    //!< add for control
    RK_U32                   first_field;
    RK_U32                   prev_pic_structure;
    RK_U32                   prev_pic_code_type;
    RK_S32                   future2prev_past_dist;
    RK_S32                   work0;
    RK_S32                   work1;
    RK_S32                   work_out;
    RK_U32                   data_offset;

    RK_U32                   frame_no;
} AvsdHalCtx_t;

#define AVSD_REGISTERS     60

typedef struct {
    RK_U32 sw00;
    struct {
        RK_U32 dec_e : 1;
        RK_U32 reserve0 : 3;
        RK_U32 dec_irq_dis : 1;
        RK_U32 dec_abort_e : 1;
        RK_U32 reserve1 : 2;
        RK_U32 dec_irq : 1;
        RK_U32 reserve2 : 2;
        RK_U32 dec_abort_int : 1;
        RK_U32 dec_rdy_int : 1;
        RK_U32 dec_bus_int : 1;
        RK_U32 dec_buffer_int : 1;
        RK_U32 dec_aso_int : 1;
        RK_U32 dec_error_int : 1;
        RK_U32 dec_slice_int : 1;
        RK_U32 dec_timeout : 1;
        RK_U32 reserve3 : 5;
        RK_U32 dec_pic_inf : 1;
        RK_U32 reserve4 : 7;
    } sw01;
    union {
        struct {
            RK_U32 dec_max_burst : 5;
            RK_U32 dec_scmd_dis : 1;
            RK_U32 dec_adv_pre_dis : 1;
            RK_U32 tiled_mode_lsb : 1;
            RK_U32 dec_out_endian : 1;
            RK_U32 dec_in_endian : 1;
            RK_U32 dec_clk_gate_e : 1;
            RK_U32 dec_latency : 6;
            RK_U32 dec_out_tiled_e : 1;
            RK_U32 dec_2chan_dis : 1;
            RK_U32 dec_outswap32_e : 1;
            RK_U32 dec_inswap32_e : 1;
            RK_U32 dec_strendian_e : 1;
            RK_U32 dec_strswap32_e : 1;
            RK_U32 dec_timeout_e : 1;
            RK_U32 dec_axi_rd_id : 8;
        };
        struct {
            RK_U32 reserve0 : 5;
            RK_U32 priority_mode : 3;
            RK_U32 reserve1 : 9;
            RK_U32 tiled_mode_msb : 1;
            RK_U32 dec_data_disc_e : 1;
            RK_U32 reserve2 : 13;
        };
    } sw02;
    struct {
        RK_U32 dec_axi_wr_id : 8;
        RK_U32 dec_ahb_hlock_e : 1;
        RK_U32 picord_count_e : 1;
        RK_U32 seq_mbaff_e : 1;
        RK_U32 reftopfirst_e : 1;
        RK_U32 write_mvs_e : 1;
        RK_U32 pic_fixed_quant : 1;
        RK_U32 filtering_dis : 1;
        RK_U32 dec_out_dis : 1;
        RK_U32 ref_topfield_e : 1;
        RK_U32 sorenson_e : 1;
        RK_U32 fwd_interlace_e : 1;
        RK_U32 pic_topfiled_e : 1;
        RK_U32 pic_inter_e : 1;
        RK_U32 pic_b_e : 1;
        RK_U32 pic_fieldmode_e : 1;
        RK_U32 pic_interlace_e : 1;
        RK_U32 pjpeg_e : 1;
        RK_U32 divx3_e : 1;
        RK_U32 skip_mode : 1;
        RK_U32 rlc_mode_e : 1;
        RK_U32 dec_mode : 4;
    } sw03;
    struct {
        RK_U32 pic_refer_flag : 1;
        RK_U32 reverse0 : 10;
        RK_U32 pic_mb_height_p : 8;
        RK_U32 mb_width_off : 4;
        RK_U32 pic_mb_width : 9;
    } sw04;
    union {
        struct {
            RK_U32 fieldpic_flag_e : 1;
            RK_S32 reserve0 : 31;
        };
        struct {
            RK_U32 beta_offset : 5;
            RK_U32 alpha_offset : 5;
            RK_U32 reserve1 : 16;
            RK_U32 strm_start_bit : 6;
        };
    } sw05;

    struct {
        RK_U32 stream_len : 24;
        RK_U32 stream_len_ext : 1;
        RK_U32 init_qp : 6;
        RK_U32 start_code_e : 1;
    } sw06;
    struct {
        RK_U32 reserve0 : 25;
        RK_U32 avs_h264_h_ext : 1;
        RK_U32 reserve1 : 6;
    } sw07;
    RK_U32 sw08;
    RK_U32 sw09;
    RK_U32 sw10;
    RK_U32 sw11;
    struct {
        RK_U32 rlc_vlc_base : 32;
    } sw12;
    union {
        struct {
            RK_U32 dec_out_base : 32;
        };
        struct { //!< left move 10bit
            RK_U32 reserve0 : 11;
            RK_U32 dpb_ilace_mode : 1;
            RK_U32 reserve1 : 20;
        };
    } sw13;
    union {
        RK_U32 refer0_base : 32;
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer0_topc_e : 1;
            RK_U32 refer0_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw14;
    union {
        struct {
            RK_U32 refer1_base : 32;
        };
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer1_topc_e : 1;
            RK_U32 refer1_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw15;
    union {
        struct {
            RK_U32 refer2_base : 32;
        };
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer2_topc_e : 1;
            RK_U32 refer2_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw16;
    union {
        struct {
            RK_U32 refer3_base : 32;
        };
        struct { //!< left move 10bit
            RK_U32 reserve0 : 10;
            RK_U32 refer3_topc_e : 1;
            RK_U32 refer3_field_e : 1;
            RK_U32 reserve1 : 20;
        };
    } sw17;
    struct {
        RK_U32 prev_anc_type : 1;
        RK_U32 reverse0 : 31;
    } sw18;
    RK_U32 sw19_27[9];
    struct {
        RK_U32 ref_invd_cur_0 : 16;
        RK_U32 ref_invd_cur_1 : 16;
    } sw28;
    struct {
        RK_U32 ref_invd_cur_2 : 16;
        RK_U32 ref_invd_cur_3 : 16;
    } sw29;
    struct {
        RK_U32 ref_dist_cur_0 : 16;
        RK_U32 ref_dist_cur_1 : 16;
    } sw30;
    struct {
        RK_U32 ref_dist_cur_2 : 16;
        RK_U32 ref_dist_cur_3 : 16;
    } sw31;

    struct {
        RK_U32 ref_invd_col_0 : 16;
        RK_U32 ref_invd_col_1 : 16;
    } sw32;
    struct {
        RK_U32 ref_invd_col_2 : 16;
        RK_U32 ref_invd_col_3 : 16;
    } sw33;
    struct {
        RK_U32 reserve0 : 2;
        RK_U32 pred_bc_tap_1_1 : 10;
        RK_U32 pred_bc_tap_1_0 : 10;
        RK_U32 pred_bc_tap_0_3 : 10;
    } sw34;
    struct {
        RK_U32 reserve0 : 12;
        RK_U32 pred_bc_tap_1_3 : 10;
        RK_U32 pred_bc_tap_1_2 : 10;
    } sw35;
    RK_U32 sw36_40[5];
    struct {
        RK_U32 dir_mv_base : 32;
    } sw41;
    struct {
        RK_U32 ref_delta_cur_3 : 3;
        RK_U32 ref_delta_cur_2 : 3;
        RK_U32 ref_delta_cur_1 : 3;
        RK_U32 ref_delta_cur_0 : 3;
        RK_U32 ref_delta_col_3 : 3;
        RK_U32 ref_delta_col_2 : 3;
        RK_U32 ref_delta_col_1 : 3;
        RK_U32 ref_delta_col_0 : 3;
        RK_U32 weight_qp_1 : 8;
    } sw42;
    struct {
        RK_U32 weight_qp_5 : 8;
        RK_U32 weight_qp_4 : 8;
        RK_U32 weight_qp_3 : 8;
        RK_U32 weight_qp_2 : 8;
    } sw43;
    struct {
        RK_U32 weight_qp_0 : 8;
        RK_U32 qp_delta_cr : 6;
        RK_U32 qp_delta_cb : 6;
        RK_U32 pb_field_enhance_e : 1;
        RK_U32 no_fwd_ref_e : 1;
        RK_U32 avs_aec_e : 1;
        RK_U32 weight_qp_model : 2;
        RK_U32 weight_qp_e : 1;
        RK_U32 dec_avsp_ena : 1;
        RK_U32 reserve0 : 5;
    } sw44;
    struct {
        RK_U32 dir_mv_base2 : 32;
    } sw45;
    RK_U32 sw46;
    RK_U32 sw47;
    struct {
        RK_U32 reserve0 : 14;
        RK_U32 startmb_y : 9;
        RK_U32 startmb_x : 9;
    } sw48;
    struct {
        RK_U32 reserve0 : 2;
        RK_U32 pred_bc_tap_0_2 : 10;
        RK_U32 pred_bc_tap_0_1 : 10;
        RK_U32 pred_bc_tap_0_0 : 10;
    } sw49;
    RK_U32 sw50;
    struct {
        RK_U32 refbu_y_offset : 9;
        RK_U32 reserve0 : 3;
        RK_U32 refbu_fparmod_e : 1;
        RK_U32 refbu_eval_e : 1;
        RK_U32 refbu_picid : 5;
        RK_U32 refbu_thr : 12;
        RK_U32 refbu_e : 1;
    } sw51;
    struct {
        RK_U32 refbu_intra_sum : 16;
        RK_U32 refbu_hit_sum : 16;
    } sw52;
    struct {
        RK_U32 refbu_y_mv_sum : 22;
        RK_U32 reserve0 : 10;
    } sw53;
    RK_U32 sw54;
    struct {
        RK_U32 apf_threshold : 14;
        RK_U32 refbu2_picid : 5;
        RK_U32 refbu2_thr : 12;
        RK_U32 refbu2_buf_e : 1;
    } sw55;

    struct {
        RK_U32 refbu_bot_sum : 16;
        RK_U32 refbu_top_sum : 16;
    };
    RK_U32 sw57;
    struct {
        RK_U32 reserve0 : 31;
        RK_U32 serv_merge_dis : 1;
    } sw58;
    RK_U32 sw59;
} AvsdRegs_t;




#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET set_defalut_parameters(AvsdHalCtx_t *p_hal);
MPP_RET set_regs_parameters(AvsdHalCtx_t *p_hal, HalDecTask *task);
MPP_RET update_parameters(AvsdHalCtx_t *p_hal);
#ifdef  __cplusplus
}
#endif

#endif /*__HAL_AVSD_REG_H__*/
