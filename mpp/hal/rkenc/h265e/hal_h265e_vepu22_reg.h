/*
 *
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


#ifndef __HAL_H265E_VEPU22_REG_H__
#define __HAL_H265E_VEPU22_REG_H__

#include "rk_type.h"

typedef struct H265e_REGS_t {
    // 0x0000
    struct {
        RK_U32    debugmode : 1;
        RK_U32    reserve0  : 2;
        RK_U32    use_po_conf : 1;
        RK_U32    reserve1 : 28;
    } sw_reg0;

    // 0x0004 Current program counter
    struct {
        RK_U32    cur_pc;
    } sw_reg1;

    // 0x0008
    RK_U32 sw_reg2;
    // 0x000c Debugger control
    struct {
        RK_U32    step_mask_enable : 1;
        RK_U32    reserve : 31;
    } sw_reg3;

    // 0x0010 Debugger control
    struct {
        RK_U32    step : 1;
        RK_U32    resume : 1;
        RK_U32    stable_brk : 1;
        RK_U32    imm_brk : 1;
        RK_U32    reserve : 28;
    } sw_reg4;

    // 0x0014 V-CPU debugger index register
    struct {
        RK_U32    debug_index : 8;
        RK_U32    write_debug : 1;
        RK_U32    read_debug : 1;
        RK_U32    reserve : 28;
    } sw_reg5;

    // 0x0018 V-CPU debugger write data register
    struct {
        RK_U32    write_data_debug;
    } sw_reg6;

    // 0x001c V-CPU debugger read data register
    struct {
        RK_U32    read_data_debug;
    } sw_reg7;

    // 0x0020 FIO CRTL, READY
    struct {
        RK_U32    fio_addr : 16;
        RK_U32    rw_flag : 1;
        RK_U32    reserve : 14;
        RK_U32    ready : 1;
    } sw_reg8;

    // 0x0024 FIO data
    struct {
        RK_U32    fio_data;
    } sw_reg9;

    // 0x0028
    RK_U32 sw_reg10;

    // 0x002c
    RK_U32 sw_reg11;

    // 0x0030 interrupt status
    struct {
        RK_U32    init_vpu : 1;
        RK_U32    dec_pic_hdr : 1;
        RK_U32    fini_seq : 1;
        RK_U32    dec_pic : 1;
        RK_U32    set_framebuf : 1;
        RK_U32    flush_decoder : 1;
        RK_U32    reserve0 : 2;
        RK_U32    get_fw_version : 1;
        RK_U32    query_decoder : 1;
        RK_U32    sleep_vpu : 1;
        RK_U32    wakeup_vpu : 1;
        RK_U32    change_inst : 1;
        RK_U32    reserve1 : 1;
        RK_U32    create_inst : 1;
        RK_U32    bitstream_empty : 1;
        RK_U32    reserve2 : 16;
    } sw_reg12;

    // 0x0034 interrupt reason clear
    struct {
        RK_U32    init_vpu_clr : 1;
        RK_U32    dec_pic_hdr_clr : 1;
        RK_U32    fini_seq_clr : 1;
        RK_U32    dec_pic_clr : 1;
        RK_U32    set_framebuf_clr : 1;
        RK_U32    flush_decoder_clr : 1;
        RK_U32    reserve0 : 2;
        RK_U32    get_fw_version_clr : 1;
        RK_U32    query_decoder_clr : 1;
        RK_U32    sleep_vpu_clr : 1;
        RK_U32    wakeup_vpu_clr : 1;
        RK_U32    change_inst_clr : 1;
        RK_U32    reserve1 : 1;
        RK_U32    create_inst_clr : 1;
        RK_U32    bitstream_empty_clr : 1;
        RK_U32    reserve2 : 16;
    } sw_reg13;

    // 0x0038 send host interrupt cmd to vpu
    struct {
        RK_U32    host_int : 1;
        RK_U32    reserve : 31;
    } sw_reg14;

    // 0x003c vpu interrupt clear
    struct {
        RK_U32    vpu_int_clr : 1;
        RK_U32    reserve : 31;
    } sw_reg15;

    // 0x0040 host interrupt clear
    struct {
        RK_U32    host_int_clr : 1;
        RK_U32    reserve : 31;
    } sw_reg16;

    // 0x0044 VPU Interrupt status
    struct {
        RK_U32    vpu_int_sts : 1;
        RK_U32    reserve : 31;
    } sw_reg17;

    // 0x0048 Interrupt Enable
    struct {
        RK_U32    init_vpu_enable : 1;
        RK_U32    dec_pic_hdr_enable : 1;
        RK_U32    fini_seq_enable : 1;
        RK_U32    dec_pic_enable : 1;
        RK_U32    set_framebuf_enable : 1;
        RK_U32    flush_decoder_enable : 1;
        RK_U32    reserve0 : 2;
        RK_U32    get_fw_version_enable : 1;
        RK_U32    query_decoder_enable : 1;
        RK_U32    sleep_vpu_enable : 1;
        RK_U32    wakeup_vpu_enable : 1;
        RK_U32    change_inst_enable : 1;
        RK_U32    reserve1 : 1;
        RK_U32    create_inst_enable : 1;
        RK_U32    update_bitstream_enable : 1;
        RK_U32    reserve2 : 16;
    } sw_reg18;

    // 0x004c VPU Interrupt Reason
    struct {
        RK_U32    init_vpu_done : 1;
        RK_U32    dec_pic_hdr_done : 1;
        RK_U32    fini_seq_done : 1;
        RK_U32    dec_pic_done : 1;
        RK_U32    set_framebuf_done : 1;
        RK_U32    flush_decoder_done : 1;
        RK_U32    reserve0 : 2;
        RK_U32    get_fw_version_done : 1;
        RK_U32    query_decoder_done : 1;
        RK_U32    sleep_vpu_done : 1;
        RK_U32    wakeup_vpu_done : 1;
        RK_U32    change_inst_done : 1;
        RK_U32    reserve1 : 1;
        RK_U32    create_inst_done : 1;
        RK_U32    bitstream_enmpty : 1;
        RK_U32    reserve2 : 16;
    } sw_reg19;

    // 0x0050 VPU reset request
    struct {
        RK_U32    crst_req : 8;
        RK_U32    brst_req : 8;
        RK_U32    arst_req : 8;
        RK_U32    varst_req : 1;
        RK_U32    vrst_req : 1;
        RK_U32    mrst_req : 1;
        RK_U32    reserve : 5;
    } sw_reg20;

    // 0x0054 VPU reset status
    struct {
        RK_U32    crst_sts : 8;
        RK_U32    brst_sts : 8;
        RK_U32    arst_sts : 8;
        RK_U32    varst_sts : 1;
        RK_U32    vrst_sts : 1;
        RK_U32    mrst_sts : 1;
        RK_U32    reserve : 5;
    } sw_reg21;

    // 0x0058 V-CPU restart request
    struct {
        RK_U32    vcpu_restart : 1;
        RK_U32    reserve : 31;
    } sw_reg22;

    // 0x005c Clock gating control register
    struct {
        RK_U32    cclk_en : 8;
        RK_U32    bclk_en : 8;
        RK_U32    aclk_en : 8;
        RK_U32    aclk_cpu_en : 1;
        RK_U32    cvclk_en : 1;
        RK_U32    mclk_en : 1;
        RK_U32    reserve : 5;
    } sw_reg23;

    // 0x0060 VPU remaps addresses
    struct {
        RK_U32    remap_size : 9;
        RK_U32    remap_berr : 1;
        RK_U32    remap_attr : 1;
        RK_U32    remap_parse_size_en : 1;
        RK_U32    remap_idx : 4;
        RK_U32    endian : 4;
        RK_U32    axiid_porc : 4;
        RK_U32    reserve : 7;
        RK_U32    remap_globen : 1;
    } sw_reg24;

    // 0x0064 Remap region base address in virtual address space
    struct {
        RK_U32    reserve : 12;
        RK_U32    vpu_remap_vaddr : 20;
    } sw_reg25;

    // 0x0068 Remap region base address in phys address space
    struct {
        RK_U32    reserve : 12;
        RK_U32    vpu_remap_paddr : 20;
    } sw_reg26;

    // 0x006c VPU Start Request
    struct {
        RK_U32    vpu_remap_core_start : 1;
        RK_U32    reserve : 31;
    } sw_reg27;

    // 0x0070 VPU Busy Status
    struct {
        RK_U32    vpu_busy_status : 1;
        RK_U32    reserve : 31;
    } sw_reg28;

    // 0x0074 Reserved for Report VPU status
    struct {
        RK_U32    vpu_halt_status : 5;
        RK_U32    reserve : 27;
    } sw_reg29;

    // 0x0078 Reserved for VPU status reproting
    struct {
        RK_U32    vpu_core_parse_status : 1;
        RK_U32    reserve : 31;
    } sw_reg30;

    // 0x007c Reserved for VPU status reproting
    struct {
        RK_U32    vpu_vcore_dec_status : 1;
        RK_U32    reserve : 31;
    } sw_reg31;


    // 0x0080~0x00fc
    RK_U32 sw_reg32_63[32];

    // 0x0100 Command to run
    struct {
        RK_U32    command;
    } sw_reg64;

    // 0x0104
    struct {
        RK_U32    vcore_index : 16;
        RK_U32    reserve : 16;
    } sw_reg65;

    // 0x0108
    struct {
        RK_U32    inst_index : 16;
        RK_U32    codec_std : 16;
    } sw_reg66;

    // 0x010c
    union {
        struct {
            RK_U32 set_param_option : 2;
            RK_U32 reserve : 30;
        };

        RK_U32 init_option;

        struct {
            RK_U32 sfb_option : 3;
            RK_U32 setup_start : 1;
            RK_U32 setup_done : 1;
            RK_U32 reserve0 : 11;
            RK_U32 fb_endian : 4;
            RK_U32 fbc_mode : 6;
            RK_U32 reserve1 : 6;
        };
    } sw_reg67;

    // 0x0110
    struct {
        RK_U32 run_cmd_status : 2;
        RK_U32 reserve : 30;
    } sw_reg68;

    // 0x0114
    struct {
        RK_U32 fail_reason : 32;
    } sw_reg69;

    // 0x0118
    union {
        struct {
            RK_U32 reserve : 12;
            RK_U32 code_buf_base : 20; // aligned to 4KB range
        };

        RK_U32 ret_fw_version;
    } sw_reg70;

    // 0x011c
    union {
        struct {
            RK_U32 hw_name : 4;
            RK_U32 reserve : 28;
        };

        RK_U32 code_size; // aligned to 4KB range
    } sw_reg71;

    // 0x0120
    union {
        struct {
            RK_U32 hw_version : 4;
            RK_U32 reserve0 : 28;
        };

        struct {
            RK_U32 reserve1 : 4;
            RK_U32 bs_start_addr : 28;
        };

        struct {
            RK_U32 dpb_stride : 16;
            RK_U32 dpb_cbcr_interleave : 1;
            RK_U32 dpb_color_format : 3;
            RK_U32 bwb_output_format : 3;
            RK_U32 wtl_align_mode : 1;
            RK_U32 axi_id : 4;
            RK_U32 wtl_enable : 1;
            RK_U32 nv21_enable : 1;
            RK_U32 reserve2 : 2;
        };

        struct {
            RK_U32 code_endian : 4;
            RK_U32 code_axiid : 4;
            RK_U32 reserve3 : 24;
        };
    } sw_reg72;

    // 0x0124
    union {
        struct {
            RK_U32 dpb_height : 16;
            RK_U32 dpb_width : 16;
        };

        struct {
            RK_U32 reserve0 : 4;
            RK_U32 bs_buf_size : 28; // aligned 128bit/16byte
        };

        struct {
            RK_U32 use_debug : 1;
            RK_U32 cache_disable : 1;
            RK_U32 reserve1 : 6;
            RK_U32 uart_option : 4;
            RK_U32 reserve2 : 20;
        };

        RK_U32 ret_cur_sp;

        RK_U32 cur_sp;
    } sw_reg73;

    // 0x0128
    union {
        struct {
            RK_U32 fb_num_end : 5;
            RK_U32 reserve0 : 3;
            RK_U32 fb_num_start : 5;
            RK_U32 reserve1 : 19;
        };

        struct {
            RK_U32 bs_endian : 4;
            RK_U32 bs_wrap_around_flag : 1;
            RK_U32 bs_slice_wr_ptr_interrupt_flag : 1;
            RK_U32 reserve2 : 26;
        };
    } sw_reg74;


    // 0x012c
    union {
        RK_U32 codec_std;
        struct {
            RK_U32 explict_end : 1;
            RK_U32 stream_end : 1;
            RK_U32 reserve0 : 30;
        };
    } sw_reg75;

    // 0x0130
    union {
        RK_U32 rd_ptr;
        RK_U32 hw_date;
    } sw_reg76;

    // 0x0134
    union {
        RK_U32 time_out_cnt;
        RK_U32 wr_ptr;
        RK_U32 hw_revision;
    } sw_reg77;

    // 0x0138
    struct {
        RK_U32 work_buf_base;
    } sw_reg78;

    // 0x013c
    union {
        struct {
            RK_U32 gop_work_buf_size : 9;
            RK_U32 reserve0 : 23;
        } ;

        RK_U32 common_work_buf_size;
    } sw_reg79;

    // 0x0140
    struct {
        RK_U32 work_buf_endian : 4;
        RK_U32 reserve0 : 28;
    } sw_reg80;

    // 0x0144
    struct {
        RK_U32 temp_buf_base;
    } sw_reg81;

    // 0x0148
    struct {
        RK_U32 temp_buf_size;
    } sw_reg82;

    // 0x014c
    struct {
        RK_U32 temp_buf_endian : 4;
        RK_U32 reserve0 : 28;
    } sw_reg83;

    // 0x0150
    struct {
        RK_U32 sec_axi_base;
    } sw_reg84;

    // 0x0154
    union {
        RK_U32 sec_axi_mem_size;
        struct {
            RK_U32 fbc_c_stride : 16;
            RK_U32 fbc_y_stride : 16;
        } ;
    } sw_reg85;

    // 0x0158
    union {
        struct {
            RK_U32 reserve0 : 11;
            RK_U32 sec_axi_rdo_enable : 1;
            RK_U32 reserve1 : 3;
            RK_U32 sec_axi_lf_row : 1;
            RK_U32 reserve2 : 16;
        } ;

        RK_U32 addr_sub_sampled_fb;
    } sw_reg86;

    // 0x015c
    union {
        struct {
            RK_U32 custom_gop_param_en : 1;
            RK_U32 custom_gop_pic_param_0_en : 1;
            RK_U32 custom_gop_pic_param_1_en : 1;
            RK_U32 custom_gop_pic_param_2_en : 1;
            RK_U32 custom_gop_pic_param_3_en : 1;
            RK_U32 custom_gop_pic_param_4_en : 1;
            RK_U32 custom_gop_pic_param_5_en : 1;
            RK_U32 custom_gop_pic_param_6_en : 1;
            RK_U32 custom_gop_pic_param_7_en : 1;
            RK_U32 reserve0 : 1;
            RK_U32 custom_gop_pic_lambda_0_en : 1;
            RK_U32 custom_gop_pic_lambda_1_en : 1;
            RK_U32 custom_gop_pic_lambda_2_en : 1;
            RK_U32 custom_gop_pic_lambda_3_en : 1;
            RK_U32 custom_gop_pic_lambda_4_en : 1;
            RK_U32 custom_gop_pic_lambda_5_en : 1;
            RK_U32 custom_gop_pic_lambda_6_en : 1;
            RK_U32 custom_gop_pic_lambda_7_en : 1;
            RK_U32 reserve1 : 14;
        } ;

        struct {
            RK_U32 neutral_chroma_indication_flag : 1;
            RK_U32 reserve2 : 2;
            RK_U32 aspect_ratio_info_present_flag : 1;
            RK_U32 oversacn_info_present_flag : 1;
            RK_U32 video_signal_type_present_flag : 1;
            RK_U32 colour_description_present_flag : 1;
            RK_U32 chroma_loc_info_present_flag : 1;
            RK_U32 default_display_window_flag : 1;
            RK_U32 bitstream_restriction_flag : 1;
            RK_U32 reserve3 : 22;
        } ;

        RK_U32 sub_sampled_one_fb_size;

        RK_U32 addr_report_base;

        struct {
            RK_U32 enable_set_seq_src_size : 1;
            RK_U32 enable_set_seq_param : 1;
            RK_U32 enable_set_seq_gop_param : 1;
            RK_U32 enable_set_seq_intra_param : 1;
            RK_U32 enable_set_seq_conf_win_top_bot : 1;
            RK_U32 enable_set_seq_conf_win_left_right : 1;
            RK_U32 enable_set_seq_frame_rate : 1;
            RK_U32 enable_set_seq_independent_slice : 1;
            RK_U32 enable_set_seq_dependent_slice : 1;
            RK_U32 enable_set_seq_intra_refresh : 1;
            RK_U32 enable_set_enc_param : 1;
            RK_U32 reserve4 : 1;
            RK_U32 enable_set_rc_param : 1;
            RK_U32 enable_set_rc_min_max_qp : 1;
            RK_U32 enable_set_rc_bit_ratio_layer_0_3 : 1;
            RK_U32 enable_set_rc_bit_ratio_layer_4_7 : 1;
            RK_U32 reserve5 : 2;
            RK_U32 enable_set_num_units_in_tick : 1;
            RK_U32 enable_set_time_scale : 1;
            RK_U32 enable_set_num_ticks_poc_diff_one : 1;
            RK_U32 enable_set_num_rc_trans_rate : 1;
            RK_U32 enable_set_num_rc_target_rate : 1;
            RK_U32 reserve6 : 2;
            RK_U32 enable_set_nr_param : 1;
            RK_U32 enable_set_nr_weight : 1;
            RK_U32 reserve7 : 5;
        } ;
    } sw_reg87;

    // 0x0160
    union {
        RK_U32 report_size;
        RK_U32 aspect_ratio_idc;
        RK_U32 luma_base0;

        struct {
            RK_U32 src_width : 16;
            RK_U32 src_height : 16;
        } ;

        struct {
            RK_U32 custom_gop_size : 4;
            RK_U32 drive_lambda_weight : 1;
            RK_U32 reserve : 27;
        } ;
    } sw_reg88;

    // 0x0164
    union {
        struct {
            RK_U32 sar_width : 16;
            RK_U32 sar_height : 16;
        } ;

        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve0 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 temporal_id : 4;
            RK_U32 reserve1 : 4;
        } ;

        RK_U32 report_param;
        RK_U32 cb_base0;
    } sw_reg89;

    // 0x0168
    union {
        struct {
            RK_U32 implicitly_header_encoder : 1;
            RK_U32 vcl : 1;
            RK_U32 vps : 1;
            RK_U32 sps : 1;
            RK_U32 pps : 1;
            RK_U32 aud : 1;
            RK_U32 eos : 1;
            RK_U32 eob : 1;
            RK_U32 sei : 1;
            RK_U32 vui : 1;
            RK_U32 reserve0 : 22;
        } ;

        RK_U32 cr_base0;
        RK_U32 fbc_luma_offset_base0;
        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve1 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 reserve2 : 4;
        } ;

        RK_U32 overscan_approriate_flag;
    } sw_reg90;

    // 0x016c
    union {
        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve0 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 temporal_id : 4;
            RK_U32 reserve1 : 4;
        } ;

        struct {
            RK_U32 pic_skip_flag : 1;
            RK_U32 use_force_pic_qp : 1;
            RK_U32 force_pic_qp_i : 6;
            RK_U32 force_pic_qp_p : 6;
            RK_U32 force_pic_qp_b : 6;
            RK_U32 use_force_pic_type : 1;
            RK_U32 forece_pic_type : 3;
            RK_U32 reserve2 : 5;
            RK_U32 force_src_pic_qp_flag : 1;
            RK_U32 force_src_pic_type_flag : 1;
            RK_U32 reserve3 : 1;
        } ;

        RK_U32 fbc_chroma_offset_base0;

        struct {
            RK_U32 video_format : 3;
            RK_U32 video_full_range_flag : 1;
            RK_U32 colour_primaries : 8;
            RK_U32 transfer_characteristics : 8;
            RK_U32 maxtrix_coeffs : 8;
            RK_U32 reserve4 : 4;
        } ;

        struct {
            RK_U32 profile_idc : 3;
            RK_U32 level_idc : 9;
            RK_U32 tier_idc : 2;
            RK_U32 bit_depth : 4;
            RK_U32 chroma_format_idc : 2;
            RK_U32 use_lossless_coding : 1;
            RK_U32 constrained_intra_pred : 1;
            RK_U32 chroma_cb_qp_offset : 5;
            RK_U32 chroma_cr_qp_offset : 5;
        } ;
    } sw_reg91;

    // 0x0170
    union {
        RK_U32 src_idx;
        struct {
            RK_U32 gop_preset_idx : 8;
            RK_U32 reserve0 : 24;
        } ;

        RK_U32 luma_base1;

        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve1 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 temporal_id : 4;
            RK_U32 reserve2 : 4;
        } ;

        struct {
            RK_U32 chroma_sample_loc_type_top_field : 16;
            RK_U32 chroma_sample_loc_type_bottom_field : 16;
        } ;
    } sw_reg92;

    // 0x0174
    union {
        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve0 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 temporal_id : 4;
            RK_U32 reserve1 : 4;
        } ;

        struct {
            RK_U32 def_disp_win_left_offset : 16;
            RK_U32 def_disp_win_right_offset : 16;
        } ;

        RK_U32 cb_base1;
        RK_U32 src_addr_y;

        struct {
            RK_U32 decoding_refresh_type : 3;
            RK_U32 intra_qp : 6;
            RK_U32 reserve3 : 7;
            RK_U32 intra_period : 16;
        } ;
    } sw_reg93;

    // 0x0178
    union {
        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve0 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 temporal_id : 4;
            RK_U32 reserve1 : 4;
        } ;

        struct {
            RK_U32 conformace_window_size_top : 16;
            RK_U32 conformace_window_size_bottom : 16;
        } ;

        struct {
            RK_U32 def_disp_win_top_offset : 16;
            RK_U32 def_disp_win_bottom_offset : 16;
        } ;

        RK_U32 cr_base1;
        RK_U32 fbc_luma_offset_base1;
        RK_U32 src_addr_u;
    } sw_reg94;

    // 0x017c
    union {
        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve0 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 temporal_id : 4;
            RK_U32 reserve1 : 4;
        } ;

        struct {
            RK_U32 conformace_window_size_left : 16;
            RK_U32 conformace_window_size_right : 16;
        } ;

        RK_U32 fbc_chroma_offset_base1;
        RK_U32 src_addr_v;
    } sw_reg95;

    // 0x0180
    union {
        RK_U32 frame_rate_idc;
        // A picture type of 7th picture in the custom GOP
        struct {
            RK_U32 pic_type : 2;
            RK_U32 poc_offset : 4;
            RK_U32 pic_qp : 6;
            RK_U32 reserve0 : 2;
            RK_U32 ref_poc_l0 : 5;
            RK_U32 ref_poc_l1 : 5;
            RK_U32 temporal_id : 4;
            RK_U32 reserve1 : 4;
        } ;

        struct {
            RK_U32 src_c_stride : 16;
            RK_U32 src_y_stride : 16;
        } ;

        RK_U32 luma_base2;
    } sw_reg96;

    // 0x0184
    union {
        struct {
            RK_U32 src_frame_format : 3;
            RK_U32 src_pixel_format : 3;
            RK_U32 src_endian : 4;
            RK_U32 reserve2 : 22;
        } ;

        RK_U32 cb_base2;

        struct {
            RK_U32 slice_mode : 3;
            RK_U32 reserve : 13;
            RK_U32 argument : 16;
        } ;
    } sw_reg97;


    // 0x0188
    union {
        RK_U32 lambda_weigth;
        RK_U32 cr_base2;

        struct {
            RK_U32 slice_mode : 3;
            RK_U32 reserve : 13;
            RK_U32 argument : 16;
        } ;

        RK_U32 prefix_se_nal_data_addr;
    } sw_reg98;

    // 0x018c
    union {
        RK_U32 fbc_chroma_offset_base2;

        struct {
            RK_U32 prefix_sei_nal_data_enble_flag : 1;
            RK_U32 prefix_sei_nal_timing_flag : 1;
            RK_U32 reserve0 : 14;
            RK_U32 prefix_sei_nal_data_size : 16;
        } ;

        RK_U32 lambda_weight;

        struct {
            RK_U32 intra_refresh_mode : 3;
            RK_U32 reserve1 : 13;
            RK_U32 intra_refresh_argument : 16;
        } ;
    } sw_reg99;

    // 0x0190
    union {
        RK_U32 lambda_weight;
        RK_U32 luma_base3;
        RK_U32 sufffix_sei_nal_data_addr : 1;
        struct {
            RK_U32 use_recommend_enc_param : 2;
            RK_U32 enable_ctu_qp_map : 1;
            RK_U32 use_scaling_list : 1;
            RK_U32 use_cu_size : 3;
            RK_U32 use_tmvp : 1;
            RK_U32 reserve0 : 1;
            RK_U32 max_num_merge : 3;
            RK_U32 use_dynamic_merge_8x8 : 1;
            RK_U32 use_dynamic_merge_16x16 : 1;
            RK_U32 use_dynamic_merge_32x32 : 1;
            RK_U32 disable_dbk : 1;
            RK_U32 use_lf_cross_slice_boundary : 1;
            RK_U32 beta_offset_div2 : 4;
            RK_U32 tc_offset_div2 : 4;
            RK_U32 reserve1 : 2;
            RK_U32 use_intra_in_inter_slice : 1;
            RK_U32 reserve2 : 4;
        } ;

        RK_U32 src_timestamp_low;
    } sw_reg100;

    // 0x0194
    union {
        RK_U32 cb_base_fbc_cbcr_base3;
        struct {
            RK_U32 suffix_sei_nal_data_enable_flag : 1;
            RK_U32 suffix_sei_timing_flag : 1;
            RK_U32 reserve0 : 14;
            RK_U32 suffix_sei_nal_data_size : 16;
        } ;

        RK_U32 src_timestamp_high;
        RK_U32 lamba_weight;
    } sw_reg101;

    // 0x0198
    union {
        struct {
            RK_U32 en_rate_control : 1;
            RK_U32 en_cu_level_rc : 1;
            RK_U32 en_hvs_qp : 1;
            RK_U32 en_hvs_qp_scale : 1;
            RK_U32 hvs_qp_scale : 3;
            RK_U32 bit_alloc_mode : 2;
            RK_U32 init_buf_levelx8 : 4;
            RK_U32 en_seq_roi : 1;
            RK_U32 initial_rc_qp : 6;
            RK_U32 initial_delay : 12;
        } ;

        RK_U32 lamba_weight;
        RK_U32 cr_base_fbc_y_offset_base3;

        struct {
            RK_U32 use_src_longterm_pic : 1;
            RK_U32 use_ref_longterm_pic : 1;
            RK_U32 reserve0 : 14;
            RK_U32 ref_longterm_pic_poc : 16;
        } ;
    } sw_reg102;

    // 0x019c
    union {
        RK_U32 fbc_chroma_offset_base3;

        struct {
            RK_U32 min_qp : 6;
            RK_U32 max_qp : 6;
            RK_U32 max_delta_qp : 6;
            RK_U32 intra_qp_offset : 14;
        } ;

        RK_U32 lambda_weight;
    } sw_reg103;

    // 0x01a0
    union {
        RK_U32 lambda_weight;

        struct {
            RK_U32 fixed_bit_ratio_0 : 8;
            RK_U32 fixed_bit_ratio_1 : 8;
            RK_U32 fixed_bit_ratio_2 : 8;
            RK_U32 fixed_bit_ratio_3 : 8;
        } ;

        RK_U32 luma_base4;

        struct {
            RK_U32 roi_enable_flag : 1;
            RK_U32 roi_delta_qp : 7;
            RK_U32 ctu_mode_enbale_flag : 1;
            RK_U32 ctu_qp_map_enbale_flag : 1;
            RK_U32 reserve0 : 2;
            RK_U32 ctu_map_endian : 4;
            RK_U32 ctu_map_stride : 16;
        } ;
    } sw_reg104;

    // 0x01a4
    union {
        RK_U32 lambda_weight;
        RK_U32 roi_map_addr;
        RK_U32 cb_base_fbc_cbcr_base4;
        struct {
            RK_U32 fixed_bit_ratio_4 : 8;
            RK_U32 fixed_bit_ratio_5 : 8;
            RK_U32 fixed_bit_ratio_6 : 8;
            RK_U32 fixed_bit_ratio_7 : 8;
        } ;
    } sw_reg105;

    // 0x01a8
    union {
        RK_U32 cr_base_fbc_y_offset_base4;
        struct {
            RK_U32 reserve0 : 24;
            RK_U32 pic_idx : 8;
        } ;

        struct {
            RK_U32 enable_nr_y : 1;
            RK_U32 enable_nr_cb : 1;
            RK_U32 enable_nr_cr : 1;
            RK_U32 enable_nr_est_noise : 1;
            RK_U32 enable_noise_sigma_y : 7;
            RK_U32 enable_noise_sigma_cb : 7;
            RK_U32 enable_noise_sigma_cr : 7;
            RK_U32 reserve1 : 7;
        } ;

        RK_U32 ctu_mode_map_addr;
    } sw_reg106;

    // 0x01ac
    union {
        RK_U32 ctu_qp_map_addr;
        RK_U32 fbc_chroma_offset_base4;
        struct {
            RK_U32 pic_indep_slice_num : 16;
            RK_U32 pic_dep_slice_num : 16;
        } ;

        struct {
            RK_U32 nr_intra_weight_y : 5;
            RK_U32 nr_intra_weight_cb : 5;
            RK_U32 nr_intra_weight_cr : 5;
            RK_U32 nr_inter_weight_y : 5;
            RK_U32 nr_inter_weight_cb : 5;
            RK_U32 nr_inter_weight_cr : 5;
            RK_U32 reserve0 : 2;
        } ;
    } sw_reg107;

    // 0x01b0
    union {
        struct {
            RK_U32 reserve0 : 1;
            RK_U32 pic_skip : 7;
            RK_U32 reserve1 : 24;
        } ;
        RK_U32 luma_base5;
        RK_U32 num_units_in_tick;
    } sw_reg108;

    // 0x01b4
    union {
        RK_U32 time_scale;
        RK_U32 cb_base_fbc_cbcr_base5;
        RK_U32 pic_num_intra;
    } sw_reg109;

    // 0x01b8
    union {
        RK_U32 num_ticks_poc_diff_one;
        RK_U32 pic_num_merge;
        RK_U32 pic_num_intra;
        RK_U32 cr_base_fbc_y_offset_base5;
    } sw_reg110;

    // 0x01bc
    union {
        RK_U32 fbc_chroma_offset_base5;
        RK_U32 trans_rate;
    } sw_reg111;

    // 0x01c0
    union {
        RK_U32 luma_base6;
        RK_U32 traget_rate;
        RK_U32 pic_num_skip;
    } sw_reg112;

    // 0x01c4
    union {
        RK_U32 cb_base_fbc_y_offset_base6;
        RK_U32 pic_avg_cu_qp;
    } sw_reg113;

    // 0x01c8
    union {
        RK_U32 cr_base_fbc_y_offset_base6;
        RK_U32 pic_byte;
        RK_U32 cr_base_fbc_y_base6;
    } sw_reg114;

    // 0x01cc
    union {
        RK_U32 gop_pic_idx;
        RK_U32 min_fb_num;
        RK_U32 fbc_chroma_offset_base6;
    } sw_reg115;

    // 0x01d0
    union {
        RK_U32 pic_poc;
        RK_U32 nal_info_to_be_encoded;
        RK_U32 luma_base7;
    } sw_reg116;

    // 0x01d4
    union {
        RK_U32 cb_base_fbc_cbcr_base7;
        RK_U32 frame_cycle;
    } sw_reg117;

    // 0x01d8
    union {
        RK_U32 cr_base_fbc_y_offset_base7;
        RK_U32 used_src_pic_idx;
        RK_U32 ret_min_sec_buf_num;
    } sw_reg118;

    // 0x01dc
    union {
        RK_U32 pic_num;
        RK_U32 fbc_chroma_offset_base7;
    } sw_reg119;

    // 0x01e0
    union {
        RK_U32 col_mv_buf_base0;
        struct {
            RK_U32 pic_type : 16;
            RK_U32 reserve0 : 16;
        } ;
    } sw_reg120;

    // 0x01e4
    struct {
        RK_U32 col_mv_buf_base1;
    } sw_reg121;

    // 0x01e8
    struct {
        RK_U32 col_mv_buf_base2;
    } sw_reg122;

    // 0x01ec
    struct {
        RK_U32 col_mv_buf_base3;
    } sw_reg123;

    // 0x01f0
    struct {
        RK_U32 col_mv_buf_base4;
    } sw_reg124;

    // 0x01f4
    struct {
        RK_U32 col_mv_buf_base5;
    } sw_reg125;

    // 0x01f8
    struct {
        RK_U32 col_mv_buf_base6;
    } sw_reg126;

    // 0x01fc
    struct {
        RK_U32 col_mv_buf_base7;
    } sw_reg127;
} H265eVepu22Regs;
#endif
