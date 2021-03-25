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

#ifndef __HAL_AV1D_VDPU_REG_H__
#define __HAL_AV1D_VDPU_REG_H__

#include "mpp_device.h"

/* swreg64 - swreg183 */
typedef struct VdpuAv1dBase_t {
    struct {
        RK_U32 sw_dec_out_ybase_msb   : 32;
    } swreg64;

    struct {
        RK_U32 sw_dec_out_ybase_lsb   : 32;
    } swreg65;

    struct {
        RK_U32 sw_refer0_ybase_msb   : 32;
    } swreg66;

    struct {
        RK_U32 sw_refer0_ybase_lsb   : 32;
    } swreg67;

    struct {
        RK_U32 sw_refer1_ybase_msb   : 32;
    } swreg68;

    struct {
        RK_U32 sw_refer1_ybase_lsb   : 32;
    } swreg69;

    struct {
        RK_U32 sw_refer2_ybase_msb   : 32;
    } swreg70;

    struct {
        RK_U32 sw_refer2_ybase_lsb   : 32;
    } swreg71;

    struct {
        RK_U32 sw_refer3_ybase_msb   : 32;
    } swreg72;

    struct {
        RK_U32 sw_refer3_ybase_lsb   : 32;
    } swreg73;

    struct {
        RK_U32 sw_refer4_ybase_msb   : 32;
    } swreg74;

    struct {
        RK_U32 sw_refer4_ybase_lsb   : 32;
    } swreg75;

    struct {
        RK_U32 sw_refer5_ybase_msb   : 32;
    } swreg76;

    struct {
        RK_U32 sw_refer5_ybase_lsb   : 32;
    } swreg77;

    struct {
        RK_U32 sw_refer6_ybase_msb   : 32;
    } swreg78;

    struct {
        RK_U32 sw_refer6_ybase_lsb   : 32;
    } swreg79;

    struct {
        RK_U32 sw_segment_read_base_msb   : 32;
    } swreg80;

    struct {
        RK_U32 sw_segment_read_base_lsb   : 32;
    } swreg81;

    struct {
        RK_U32 sw_global_model_base_msb   : 32;
    } swreg82;

    struct {
        RK_U32 sw_global_model_base_lsb   : 32;
    } swreg83;

    struct {
        RK_U32 sw_cdef_colbuf_base_msb   : 32;
    } swreg84;

    struct {
        RK_U32 sw_cdef_colbuf_base_lsb   : 32;
    } swreg85;

    struct {
        RK_U32 sw_cdef_left_colbuf_base_msb   : 32;
    } swreg86;

    struct {
        RK_U32 sw_cdef_left_colbuf_base_lsb   : 32;
    } swreg87;

    struct {
        RK_U32 sw_superres_colbuf_base_msb   : 32;
    } swreg88;

    struct {
        RK_U32 sw_superres_colbuf_base_lsb   : 32;
    } swreg89;

    struct {
        RK_U32 sw_lr_colbuf_base_msb   : 32;
    } swreg90;

    struct {
        RK_U32 sw_lr_colbuf_base_lsb   : 32;
    } swreg91;

    struct {
        RK_U32 sw_superres_left_colbuf_base_msb   : 32;
    } swreg92;

    struct {
        RK_U32 sw_superres_left_colbuf_base_lsb   : 32;
    } swreg93;

    struct {
        RK_U32 sw_filmgrain_base_msb   : 32;
    } swreg94;

    struct {
        RK_U32 sw_filmgrain_base_lsb   : 32;
    } swreg95;

    struct {
        RK_U32 sw_lr_left_colbuf_base_msb   : 32;
    } swreg96;

    struct {
        RK_U32 sw_lr_left_colbuf_base_lsb   : 32;
    } swreg97;

    struct {
        RK_U32 sw_dec_out_cbase_msb   : 32;
    } swreg98;

    struct {
        RK_U32 sw_dec_out_cbase_lsb   : 32;
    } swreg99;

    struct {
        RK_U32 sw_refer0_cbase_msb   : 32;
    } swreg100;

    struct {
        RK_U32 sw_refer0_cbase_lsb   : 32;
    } swreg101;

    struct {
        RK_U32 sw_refer1_cbase_msb   : 32;
    } swreg102;

    struct {
        RK_U32 sw_refer1_cbase_lsb   : 32;
    } swreg103;

    struct {
        RK_U32 sw_refer2_cbase_msb   : 32;
    } swreg104;

    struct {
        RK_U32 sw_refer2_cbase_lsb   : 32;
    } swreg105;

    struct {
        RK_U32 sw_refer3_cbase_msb   : 32;
    } swreg106;

    struct {
        RK_U32 sw_refer3_cbase_lsb   : 32;
    } swreg107;

    struct {
        RK_U32 sw_refer4_cbase_msb   : 32;
    } swreg108;

    struct {
        RK_U32 sw_refer4_cbase_lsb   : 32;
    } swreg109;

    struct {
        RK_U32 sw_refer5_cbase_msb   : 32;
    } swreg110;

    struct {
        RK_U32 sw_refer5_cbase_lsb   : 32;
    } swreg111;

    struct {
        RK_U32 sw_refer6_cbase_msb   : 32;
    } swreg112;

    struct {
        RK_U32 sw_refer6_cbase_lsb   : 32;
    } swreg113;

    struct {
        RK_U32 sw_dec_left_vert_filt_base_msb   : 32;
    } swreg114;

    struct {
        RK_U32 sw_dec_left_vert_filt_base_lsb   : 32;
    } swreg115;

    struct {
        RK_U32 sw_dec_left_bsd_ctrl_base_msb   : 32;
    } swreg116;

    struct {
        RK_U32 sw_dec_left_bsd_ctrl_base_lsb   : 32;
    } swreg117;

    RK_U32 reserved_118_131[14];
    struct {
        RK_U32 sw_dec_out_dbase_msb   : 32;
    } swreg132;

    struct {
        RK_U32 sw_dec_out_dbase_lsb   : 32;
    } swreg133;

    struct {
        RK_U32 sw_refer0_dbase_msb   : 32;
    } swreg134;

    struct {
        RK_U32 sw_refer0_dbase_lsb   : 32;
    } swreg135;

    struct {
        RK_U32 sw_refer1_dbase_msb   : 32;
    } swreg136;

    struct {
        RK_U32 sw_refer1_dbase_lsb   : 32;
    } swreg137;

    struct {
        RK_U32 sw_refer2_dbase_msb   : 32;
    } swreg138;

    struct {
        RK_U32 sw_refer2_dbase_lsb   : 32;
    } swreg139;

    struct {
        RK_U32 sw_refer3_dbase_msb   : 32;
    } swreg140;

    struct {
        RK_U32 sw_refer3_dbase_lsb   : 32;
    } swreg141;

    struct {
        RK_U32 sw_refer4_dbase_msb   : 32;
    } swreg142;

    struct {
        RK_U32 sw_refer4_dbase_lsb   : 32;
    } swreg143;

    struct {
        RK_U32 sw_refer5_dbase_msb   : 32;
    } swreg144;

    struct {
        RK_U32 sw_refer5_dbase_lsb   : 32;
    } swreg145;

    struct {
        RK_U32 sw_refer6_dbase_msb   : 32;
    } swreg146;

    struct {
        RK_U32 sw_refer6_dbase_lsb   : 32;
    } swreg147;

    RK_U32 reserved_148_165[18];
    struct {
        RK_U32 sw_tile_base_msb   : 32;
    } swreg166;

    struct {
        RK_U32 sw_tile_base_lsb   : 32;
    } swreg167;

    struct {
        RK_U32 sw_stream_base_msb   : 32;
    } swreg168;

    struct {
        RK_U32 sw_stream_base_lsb   : 32;
    } swreg169;

    struct {
        RK_U32 sw_prob_tab_out_base_msb   : 32;
    } swreg170;

    struct {
        RK_U32 sw_prob_tab_out_base_lsb   : 32;
    } swreg171;

    struct {
        RK_U32 sw_prob_tab_base_msb   : 32;
    } swreg172;

    struct {
        RK_U32 sw_prob_tab_base_lsb   : 32;
    } swreg173;

    struct {
        RK_U32 sw_mc_sync_curr_base_msb   : 32;
    } swreg174;

    struct {
        RK_U32 sw_mc_sync_curr_base_lsb   : 32;
    } swreg175;

    struct {
        RK_U32 sw_mc_sync_left_base_msb   : 32;
    } swreg176;

    struct {
        RK_U32 sw_mc_sync_left_base_lsb   : 32;
    } swreg177;

    struct {
        RK_U32 sw_dec_vert_filt_base_msb   : 32;
    } swreg178;

    struct {
        RK_U32 sw_dec_vert_filt_base_lsb   : 32;
    } swreg179;

    RK_U32 reserved_180_181[2];
    struct {
        RK_U32 sw_dec_bsd_ctrl_base_msb   : 32;
    } swreg182;

    struct {
        RK_U32 sw_dec_bsd_ctrl_base_lsb   : 32;
    } swreg183;
} VdpuAv1dBase;

/* swreg320 - swreg511 */
typedef struct VdpuAv1dPPCfg_t {

    struct {
        RK_U32 sw_pp_out_e          : 1;
        RK_U32 sw_pp_cr_first       : 1;
        RK_U32 sw_pp_out_mode       : 1;
        RK_U32 sw_pp_out_tile_e     : 1;
        RK_U32 sw_pp_status         : 4;
        RK_U32 sw_pp_in_blk_size    : 3;
        RK_U32 sw_pp_out_p010_fmt   : 2;
        RK_U32 sw_pp_out_rgb_fmt    : 5;
        RK_U32 sw_rgb_range_max     : 12;
        RK_U32 sw_pp_rgb_planar     : 1;
        RK_U32 reserved0            : 1;
    } swreg320;

    struct {
        RK_U32 sw_rgb_range_min   : 9;
        RK_U32 sw_pp_tile_size    : 2;
        RK_U32 reserved0          : 5;
        RK_U32 sw_pp_in_swap      : 4;
        RK_U32 sw_pp_out_swap     : 4;
        RK_U32 sw_pp_in_a1_swap   : 4;
        RK_U32 sw_pp_in_a2_swap   : 4;
    } swreg321;

    struct {
        RK_U32 sw_scale_hratio     : 18;
        RK_U32 sw_pp_out_format    : 5;
        RK_U32 sw_ver_scale_mode   : 2;
        RK_U32 sw_hor_scale_mode   : 2;
        RK_U32 sw_pp_in_format     : 5;
    } swreg322;

    struct {
        RK_U32 sw_scale_wratio      : 18;
        RK_U32 sw_rangemap_coef_c   : 5;
        RK_U32 sw_rangemap_coef_y   : 5;
        RK_U32 sw_pp_vc1_adv_e      : 1;
        RK_U32 sw_ycbcr_range       : 1;
        RK_U32 sw_rangemap_c_e      : 1;
        RK_U32 sw_rangemap_y_e      : 1;
    } swreg323;

    struct {
        RK_U32 sw_hscale_invra   : 16;
        RK_U32 sw_wscale_invra   : 16;
    } swreg324;

    struct {
        RK_U32 sw_pp_out_lu_base_msb   : 32;
    } swreg325;

    struct {
        RK_U32 sw_pp_out_lu_base_lsb   : 32;
    } swreg326;

    struct {
        RK_U32 sw_pp_out_ch_base_msb   : 32;
    } swreg327;

    struct {
        RK_U32 sw_pp_out_ch_base_lsb   : 32;
    } swreg328;

    struct {
        RK_U32 sw_pp_out_c_stride   : 16;
        RK_U32 sw_pp_out_y_stride   : 16;
    } swreg329;

    struct {
        RK_U32 sw_crop_starty     : 13;
        RK_U32 sw_rotation_mode   : 2;
        RK_U32 reserved0          : 1;
        RK_U32 sw_crop_startx     : 13;
        RK_U32 sw_flip_mode       : 2;
        RK_U32 sw_pad_sel         : 1;
    } swreg330;

    struct {
        RK_U32 sw_pp_in_height   : 16;
        RK_U32 sw_pp_in_width    : 16;
    } swreg331;

    struct {
        RK_U32 sw_pp_out_height   : 16;
        RK_U32 sw_pp_out_width    : 16;
    } swreg332;

    struct {
        RK_U32 sw_pp_out_lu_bot_base_msb   : 32;
    } swreg333;

    struct {
        RK_U32 sw_pp_out_lu_bot_base_lsb   : 32;
    } swreg334;

    struct {
        RK_U32 sw_pp_crop2_starty          : 13;
        RK_U32 reserved0                   : 3;
        RK_U32 sw_pp_crop2_startx          : 13;
        RK_U32 reserved1                   : 3;
        // RK_U32 sw_pp_out_ch_bot_base_msb   : 32;
    } swreg335;

    struct {
        RK_U32 sw_pp_crop2_out_height      : 16;
        RK_U32 sw_pp_crop2_out_width       : 16;
        // RK_U32 sw_pp_out_ch_bot_base_lsb   : 32;
    } swreg336;

    struct {
        RK_U32 sw_pp_in_c_stride   : 16;
        RK_U32 sw_pp_in_y_stride   : 16;
    } swreg337;

    struct {
        RK_U32 sw_pp_in_lu_base_msb   : 32;
    } swreg338;

    struct {
        RK_U32 sw_pp_in_lu_base_lsb   : 32;
    } swreg339;

    struct {
        RK_U32 sw_pp_in_ch_base_msb   : 32;
    } swreg340;

    struct {
        RK_U32 sw_pp_in_ch_base_lsb   : 32;
    } swreg341;

    struct {
        RK_U32 sw_pp1_out_e          : 1;
        RK_U32 sw_pp1_cr_first       : 1;
        RK_U32 sw_pp1_out_mode       : 1;
        RK_U32 sw_pp1_out_tile_e     : 1;
        RK_U32 reserved0             : 7;
        RK_U32 sw_pp1_out_p010_fmt   : 2;
        RK_U32 sw_pp1_out_rgb_fmt    : 5;
        RK_U32 reserved1             : 12;
        RK_U32 sw_pp1_rgb_planar     : 1;
        RK_U32 reserved2             : 1;
    } swreg342;

    struct {
        RK_U32 reserved0          : 9;
        RK_U32 sw_pp1_tile_size   : 2;
        RK_U32 reserved1          : 9;
        RK_U32 sw_pp1_out_swap    : 4;
        RK_U32 reserved2          : 8;
    } swreg343;

    struct {
        RK_U32 sw_pp1_scale_hratio     : 18;
        RK_U32 sw_pp1_out_format       : 5;
        RK_U32 sw_pp1_ver_scale_mode   : 2;
        RK_U32 sw_pp1_hor_scale_mode   : 2;
        RK_U32 reserved0               : 5;
    } swreg344;

    struct {
        RK_U32 sw_pp1_scale_wratio   : 18;
        RK_U32 reserved0             : 14;
    } swreg345;

    struct {
        RK_U32 sw_pp1_hscale_invra   : 16;
        RK_U32 sw_pp1_wscale_invra   : 16;
    } swreg346;

    struct {
        RK_U32 sw_pp1_out_lu_base_msb   : 32;
    } swreg347;

    struct {
        RK_U32 sw_pp1_out_lu_base_lsb   : 32;
    } swreg348;

    struct {
        RK_U32 sw_pp1_out_ch_base_msb   : 32;
    } swreg349;

    struct {
        RK_U32 sw_pp1_out_ch_base_lsb   : 32;
    } swreg350;

    struct {
        RK_U32 sw_pp1_out_c_stride   : 16;
        RK_U32 sw_pp1_out_y_stride   : 16;
    } swreg351;

    struct {
        RK_U32 sw_pp1_crop_starty     : 13;
        RK_U32 sw_pp1_rotation_mode   : 2;
        RK_U32 reserved0              : 1;
        RK_U32 sw_pp1_crop_startx     : 13;
        RK_U32 sw_pp1_flip_mode       : 2;
        RK_U32 sw_pp1_pad_sel         : 1;
    } swreg352;

    struct {
        RK_U32 sw_pp1_in_height   : 16;
        RK_U32 sw_pp1_in_width    : 16;
    } swreg353;

    struct {
        RK_U32 sw_pp1_out_height   : 16;
        RK_U32 sw_pp1_out_width    : 16;
    } swreg354;

    struct {
        RK_U32 sw_pp1_out_lu_bot_base_msb   : 32;
    } swreg355;

    struct {
        RK_U32 sw_pp1_out_lu_bot_base_lsb   : 32;
    } swreg356;

    struct {
        RK_U32 sw_pp1_crop2_starty          : 13;
        RK_U32 reserved0                    : 3;
        RK_U32 sw_pp1_crop2_startx          : 13;
        RK_U32 reserved1                    : 3;
        // RK_U32 sw_pp1_out_ch_bot_base_msb   : 32;
    } swreg357;

    struct {
        RK_U32 sw_pp1_crop2_out_height      : 16;
        RK_U32 sw_pp1_crop2_out_width       : 16;
        // RK_U32 sw_pp1_out_ch_bot_base_lsb   : 32;
    } swreg358;

    struct {
        RK_U32 sw_pp2_out_e          : 1;
        RK_U32 sw_pp2_cr_first       : 1;
        RK_U32 sw_pp2_out_mode       : 1;
        RK_U32 sw_pp2_out_tile_e     : 1;
        RK_U32 reserved0             : 7;
        RK_U32 sw_pp2_out_p010_fmt   : 2;
        RK_U32 sw_pp2_out_rgb_fmt    : 5;
        RK_U32 reserved1             : 12;
        RK_U32 sw_pp2_rgb_planar     : 1;
        RK_U32 reserved2             : 1;
    } swreg359;

    struct {
        RK_U32 reserved0          : 9;
        RK_U32 sw_pp2_tile_size   : 2;
        RK_U32 reserved1          : 9;
        RK_U32 sw_pp2_out_swap    : 4;
        RK_U32 reserved2          : 8;
    } swreg360;

    struct {
        RK_U32 sw_pp2_scale_hratio     : 18;
        RK_U32 sw_pp2_out_format       : 5;
        RK_U32 sw_pp2_ver_scale_mode   : 2;
        RK_U32 sw_pp2_hor_scale_mode   : 2;
        RK_U32 reserved0               : 5;
    } swreg361;

    struct {
        RK_U32 sw_pp2_scale_wratio   : 18;
        RK_U32 reserved0             : 5;
        RK_U32 reserved1             : 5;
        RK_U32 reserved2             : 1;
        RK_U32 reserved4             : 1;
        RK_U32 reserved3             : 1;
        RK_U32 reserved5             : 1;
    } swreg362;

    struct {
        RK_U32 sw_pp2_hscale_invra   : 16;
        RK_U32 sw_pp2_wscale_invra   : 16;
    } swreg363;

    struct {
        RK_U32 sw_pp2_out_lu_base_msb   : 32;
    } swreg364;

    struct {
        RK_U32 sw_pp2_out_lu_base_lsb   : 32;
    } swreg365;

    struct {
        RK_U32 sw_pp2_out_ch_base_msb   : 32;
    } swreg366;

    struct {
        RK_U32 sw_pp2_out_ch_base_lsb   : 32;
    } swreg367;

    struct {
        RK_U32 sw_pp2_out_c_stride   : 16;
        RK_U32 sw_pp2_out_y_stride   : 16;
    } swreg368;

    struct {
        RK_U32 sw_pp2_crop_starty     : 13;
        RK_U32 sw_pp2_rotation_mode   : 2;
        RK_U32 reserved0              : 1;
        RK_U32 sw_pp2_crop_startx     : 13;
        RK_U32 sw_pp2_flip_mode       : 2;
        RK_U32 sw_pp2_pad_sel         : 1;
    } swreg369;

    struct {
        RK_U32 sw_pp2_in_height   : 16;
        RK_U32 sw_pp2_in_width    : 16;
    } swreg370;

    struct {
        RK_U32 sw_pp2_out_height   : 16;
        RK_U32 sw_pp2_out_width    : 16;
    } swreg371;

    struct {
        // RK_U32 sw_pp2_out_b_base_msb        : 32;
        RK_U32 sw_pp2_out_lu_bot_base_msb   : 32;
    } swreg372;

    struct {
        // RK_U32 sw_pp2_out_b_base_lsb        : 32;
        RK_U32 sw_pp2_out_lu_bot_base_lsb   : 32;
    } swreg373;

    struct {
        RK_U32 sw_pp2_crop2_starty          : 13;
        RK_U32 reserved0                    : 3;
        RK_U32 sw_pp2_crop2_startx          : 13;
        RK_U32 reserved1                    : 3;
        // RK_U32 sw_pp2_out_ch_bot_base_msb   : 32;
    } swreg374;

    struct {
        RK_U32 sw_pp2_crop2_out_height      : 16;
        RK_U32 sw_pp2_crop2_out_width       : 16;
        // RK_U32 sw_pp2_out_ch_bot_base_lsb   : 32;
    } swreg375;

    struct {
        RK_U32 sw_pp3_out_e          : 1;
        RK_U32 sw_pp3_cr_first       : 1;
        RK_U32 sw_pp3_out_mode       : 1;
        RK_U32 sw_pp3_out_tile_e     : 1;
        RK_U32 reserved0             : 7;
        RK_U32 sw_pp3_out_p010_fmt   : 2;
        RK_U32 sw_pp3_out_rgb_fmt    : 5;
        RK_U32 reserved1             : 12;
        RK_U32 sw_pp3_rgb_planar     : 1;
        RK_U32 reserved2             : 1;
    } swreg376;

    struct {
        RK_U32 reserved0          : 9;
        RK_U32 sw_pp3_tile_size   : 2;
        RK_U32 reserved1          : 9;
        RK_U32 sw_pp3_out_swap    : 4;
        RK_U32 reserved2          : 8;
    } swreg377;

    struct {
        RK_U32 sw_pp3_scale_hratio     : 18;
        RK_U32 sw_pp3_out_format       : 5;
        RK_U32 sw_pp3_ver_scale_mode   : 2;
        RK_U32 sw_pp3_hor_scale_mode   : 2;
        RK_U32 reserved0               : 5;
    } swreg378;

    struct {
        RK_U32 sw_pp3_scale_wratio   : 18;
        RK_U32 reserved0             : 5;
        RK_U32 reserved1             : 5;
        RK_U32 reserved2             : 1;
        RK_U32 reserved4             : 1;
        RK_U32 reserved3             : 1;
        RK_U32 reserved5             : 1;
    } swreg379;

    struct {
        RK_U32 sw_pp3_hscale_invra   : 16;
        RK_U32 sw_pp3_wscale_invra   : 16;
    } swreg380;

    struct {
        // RK_U32 sw_pp3_out_r_base_msb    : 32;
        RK_U32 sw_pp3_out_lu_base_msb   : 32;
    } swreg381;

    struct {
        // RK_U32 sw_pp3_out_r_base_lsb    : 32;
        RK_U32 sw_pp3_out_lu_base_lsb   : 32;
    } swreg382;

    struct {
        // RK_U32 sw_pp3_out_g_base_msb    : 32;
        RK_U32 sw_pp3_out_ch_base_msb   : 32;
    } swreg383;

    struct {
        // RK_U32 sw_pp3_out_g_base_lsb    : 32;
        RK_U32 sw_pp3_out_ch_base_lsb   : 32;
    } swreg384;

    struct {
        RK_U32 sw_pp3_out_c_stride   : 16;
        RK_U32 sw_pp3_out_y_stride   : 16;
    } swreg385;

    struct {
        RK_U32 sw_pp3_crop_starty     : 13;
        RK_U32 sw_pp3_rotation_mode   : 2;
        RK_U32 reserved0              : 1;
        RK_U32 sw_pp3_crop_startx     : 13;
        RK_U32 sw_pp3_flip_mode       : 2;
        RK_U32 sw_pp3_pad_sel         : 1;
    } swreg386;

    struct {
        RK_U32 sw_pp3_in_height   : 16;
        RK_U32 sw_pp3_in_width    : 16;
    } swreg387;

    struct {
        RK_U32 sw_pp3_out_height   : 16;
        RK_U32 sw_pp3_out_width    : 16;
    } swreg388;

    struct {
        // RK_U32 sw_pp3_out_b_base_msb        : 32;
        RK_U32 sw_pp3_out_lu_bot_base_msb   : 32;
    } swreg389;

    struct {
        // RK_U32 sw_pp3_out_b_base_lsb        : 32;
        RK_U32 sw_pp3_out_lu_bot_base_lsb   : 32;
    } swreg390;

    struct {
        RK_U32 sw_pp3_crop2_starty          : 13;
        RK_U32 reserved0                    : 3;
        RK_U32 sw_pp3_crop2_startx          : 13;
        RK_U32 reserved1                    : 3;
        // RK_U32 sw_pp3_out_ch_bot_base_msb   : 32;
    } swreg391;

    struct {
        RK_U32 sw_pp3_crop2_out_height      : 16;
        RK_U32 sw_pp3_crop2_out_width       : 16;
        // RK_U32 sw_pp3_out_ch_bot_base_lsb   : 32;
    } swreg392;

    struct {
        RK_U32 sw_pp3_out_alpha   : 8;
        RK_U32 sw_pp2_out_alpha   : 8;
        RK_U32 sw_pp1_out_alpha   : 8;
        RK_U32 sw_pp0_out_alpha   : 8;
    } swreg393;

    struct {
        RK_U32 sw_pp1_dup_ver   : 8;
        RK_U32 sw_pp1_dup_hor   : 8;
        RK_U32 sw_pp0_dup_ver   : 8;
        RK_U32 sw_pp0_dup_hor   : 8;
    } swreg394;

    struct {
        RK_U32 sw_pp3_dup_ver   : 8;
        RK_U32 sw_pp3_dup_hor   : 8;
        RK_U32 sw_pp2_dup_ver   : 8;
        RK_U32 sw_pp2_dup_hor   : 8;
    } swreg395;

    struct {
        RK_U32 sw_pp0_scale1_out_height   : 16;
        RK_U32 sw_pp0_scale1_out_width    : 16;
    } swreg396;

    struct {
        RK_U32 sw_pp1_scale1_out_height   : 16;
        RK_U32 sw_pp1_scale1_out_width    : 16;
    } swreg397;

    struct {
        RK_U32 sw_pp2_scale1_out_height   : 16;
        RK_U32 sw_pp2_scale1_out_width    : 16;
    } swreg398;

    struct {
        RK_U32 sw_pp3_scale1_out_height   : 16;
        RK_U32 sw_pp3_scale1_out_width    : 16;
    } swreg399;

    struct {
        RK_U32 sw_dec_in_pool_base_msb   : 32;
    } swreg400;

    struct {
        RK_U32 sw_dec_in_pool_base_lsb   : 32;
    } swreg401;

    struct {
        RK_U32 sw_dec_in_bufpool_size   : 32;
    } swreg402;

    struct {
        RK_U32 sw_dec_out_pool_base_msb   : 32;
    } swreg403;

    struct {
        RK_U32 sw_dec_out_pool_base_lsb   : 32;
    } swreg404;

    struct {
        RK_U32 sw_dec_out_bufpool_size   : 32;
    } swreg405;

    struct {
        RK_U32 sw_dec_priv_pool_base_msb   : 32;
    } swreg406;

    struct {
        RK_U32 sw_dec_priv_pool_base_lsb   : 32;
    } swreg407;

    struct {
        RK_U32 sw_dec_priv_pool_size   : 32;
    } swreg408;

    struct {
        RK_U32 sw_dec_in_nsa_id     : 4;
        RK_U32 sw_dec_out_nsa_id    : 4;
        RK_U32 sw_dec_priv_nsa_id   : 4;
        RK_U32 sw_dec_pub_nsa_id    : 4;
        RK_U32 reserved0            : 16;
    } swreg409;

    struct {
        RK_U32 sw_contrast_off1     : 12;
        RK_U32 sw_contrast_off2     : 12;
        RK_U32 reserved0            : 2;
        RK_U32 sw_dither_select_b   : 2;
        RK_U32 sw_dither_select_g   : 2;
        RK_U32 sw_dither_select_r   : 2;
    } swreg410;

    struct {
        RK_U32 sw_color_coefff    : 10;
        RK_U32 sw_contrast_thr2   : 10;
        RK_U32 sw_contrast_thr1   : 10;
        RK_U32 reserved0          : 2;
    } swreg411;

    struct {
        RK_U32 sw_color_coeffa1   : 16;
        RK_U32 sw_color_coeffa2   : 16;
    } swreg412;

    struct {
        RK_U32 sw_color_coeffb   : 16;
        RK_U32 sw_color_coeffc   : 16;
    } swreg413;

    struct {
        RK_U32 sw_color_coeffd   : 16;
        RK_U32 sw_color_coeffe   : 16;
    } swreg414;

    struct {
        RK_U32 sw_pp1_contrast_off1     : 12;
        RK_U32 sw_pp1_contrast_off2     : 12;
        RK_U32 reserved0                : 2;
        RK_U32 sw_pp1_dither_select_b   : 2;
        RK_U32 sw_pp1_dither_select_g   : 2;
        RK_U32 sw_pp1_dither_select_r   : 2;
    } swreg415;

    struct {
        RK_U32 sw_pp1_color_coefff    : 10;
        RK_U32 sw_pp1_contrast_thr2   : 10;
        RK_U32 sw_pp1_contrast_thr1   : 10;
        RK_U32 reserved0              : 2;
    } swreg416;

    struct {
        RK_U32 sw_pp1_color_coeffa1   : 16;
        RK_U32 sw_pp1_color_coeffa2   : 16;
    } swreg417;

    struct {
        RK_U32 sw_pp1_color_coeffb   : 16;
        RK_U32 sw_pp1_color_coeffc   : 16;
    } swreg418;

    struct {
        RK_U32 sw_pp1_color_coeffd   : 16;
        RK_U32 sw_pp1_color_coeffe   : 16;
    } swreg419;

    struct {
        RK_U32 sw_pp2_contrast_off1     : 12;
        RK_U32 sw_pp2_contrast_off2     : 12;
        RK_U32 reserved0                : 2;
        RK_U32 sw_pp2_dither_select_b   : 2;
        RK_U32 sw_pp2_dither_select_g   : 2;
        RK_U32 sw_pp2_dither_select_r   : 2;
    } swreg420;

    struct {
        RK_U32 sw_pp2_color_coefff    : 10;
        RK_U32 sw_pp2_contrast_thr2   : 10;
        RK_U32 sw_pp2_contrast_thr1   : 10;
        RK_U32 reserved0              : 2;
    } swreg421;

    struct {
        RK_U32 sw_pp2_color_coeffa1   : 16;
        RK_U32 sw_pp2_color_coeffa2   : 16;
    } swreg422;

    struct {
        RK_U32 sw_pp2_color_coeffb   : 16;
        RK_U32 sw_pp2_color_coeffc   : 16;
    } swreg423;

    struct {
        RK_U32 sw_pp2_color_coeffd   : 16;
        RK_U32 sw_pp2_color_coeffe   : 16;
    } swreg424;

    struct {
        RK_U32 sw_pp3_contrast_off1     : 12;
        RK_U32 sw_pp3_contrast_off2     : 12;
        RK_U32 reserved0                : 2;
        RK_U32 sw_pp3_dither_select_b   : 2;
        RK_U32 sw_pp3_dither_select_g   : 2;
        RK_U32 sw_pp3_dither_select_r   : 2;
    } swreg425;

    struct {
        RK_U32 sw_pp3_color_coefff    : 10;
        RK_U32 sw_pp3_contrast_thr2   : 10;
        RK_U32 sw_pp3_contrast_thr1   : 10;
        RK_U32 reserved0              : 2;
    } swreg426;

    struct {
        RK_U32 sw_pp3_color_coeffa1   : 16;
        RK_U32 sw_pp3_color_coeffa2   : 16;
    } swreg427;

    struct {
        RK_U32 sw_pp3_color_coeffb   : 16;
        RK_U32 sw_pp3_color_coeffc   : 16;
    } swreg428;

    struct {
        RK_U32 sw_pp3_color_coeffd   : 16;
        RK_U32 sw_pp3_color_coeffe   : 16;
    } swreg429;

    struct {
        RK_U32 sw_delogo0_h             : 10;
        RK_U32 sw_delogo0_w             : 10;
        RK_U32 reserved0                : 8;
        RK_U32 sw_delogo0_show_border   : 1;
        RK_U32 sw_delogo0_mode          : 3;
    } swreg430;

    struct {
        RK_U32 sw_delogo0_y   : 16;
        RK_U32 sw_delogo0_x   : 16;
    } swreg431;

    struct {
        RK_U32 sw_delogo0_fillV   : 10;
        RK_U32 sw_delogo0_fillU   : 10;
        RK_U32 sw_delogo0_fillY   : 10;
        RK_U32 reserved0          : 2;
    } swreg432;

    struct {
        RK_U32 sw_delogo1_h             : 14;
        RK_U32 sw_delogo1_w             : 14;
        RK_U32 sw_delogo1_show_border   : 1;
        RK_U32 sw_delogo1_mode          : 3;
    } swreg433;

    struct {
        RK_U32 sw_delogo1_y   : 16;
        RK_U32 sw_delogo1_x   : 16;
    } swreg434;

    struct {
        RK_U32 sw_delogo1_fillV   : 10;
        RK_U32 sw_delogo1_fillU   : 10;
        RK_U32 sw_delogo1_fillY   : 10;
        RK_U32 reserved0          : 2;
    } swreg435;

    struct {
        RK_U32 sw_delogo0_ratio_h   : 16;
        RK_U32 sw_delogo0_ratio_w   : 16;
    } swreg436;

    struct {
        RK_U32 sw_pp1_hcale_invra_ext    : 8;
        RK_U32 sw_pp1_wscale_invra_ext   : 8;
        RK_U32 sw_pp0_hcale_invra_ext    : 8;
        RK_U32 sw_pp0_wscale_invra_ext   : 8;
    } swreg437;

    struct {
        RK_U32 sw_pp0_x_filter_size   : 12;
        RK_U32 sw_pp0_y_filter_size   : 12;
        RK_U32 reserved0              : 8;
    } swreg438;

    struct {
        RK_U32 sw_pp0_lanczos_tbl_base_msb   : 32;
    } swreg439;

    struct {
        RK_U32 sw_pp0_lanczos_tbl_base_lsb   : 32;
    } swreg440;

    struct {
        RK_U32 sw_pp1_x_filter_size   : 12;
        RK_U32 sw_pp1_y_filter_size   : 12;
        RK_U32 reserved0              : 8;
    } swreg441;

    struct {
        RK_U32 sw_pp1_lanczos_tbl_base_msb   : 32;
    } swreg442;

    struct {
        RK_U32 sw_pp1_lanczos_tbl_base_lsb   : 32;
    } swreg443;

    struct {
        RK_U32 sw_pp3_hcale_invra_ext    : 8;
        RK_U32 sw_pp3_wscale_invra_ext   : 8;
        RK_U32 sw_pp2_hcale_invra_ext    : 8;
        RK_U32 sw_pp2_wscale_invra_ext   : 8;
    } swreg444;

    struct {
        RK_U32 sw_pp2_x_filter_size   : 12;
        RK_U32 sw_pp2_y_filter_size   : 12;
        RK_U32 reserved0              : 8;
    } swreg445;

    struct {
        RK_U32 sw_pp2_lanczos_tbl_base_msb   : 32;
    } swreg446;

    struct {
        RK_U32 sw_pp2_lanczos_tbl_base_lsb   : 32;
    } swreg447;

    struct {
        RK_U32 sw_pp3_x_filter_size   : 12;
        RK_U32 sw_pp3_y_filter_size   : 12;
        RK_U32 reserved0              : 8;
    } swreg448;

    struct {
        RK_U32 sw_pp3_lanczos_tbl_base_msb   : 32;
    } swreg449;

    struct {
        RK_U32 sw_pp3_lanczos_tbl_base_lsb   : 32;
    } swreg450;

    struct {
        RK_U32 sw_pp4_out_e          : 1;
        RK_U32 sw_pp4_cr_first       : 1;
        RK_U32 sw_pp4_out_mode       : 1;
        RK_U32 sw_pp4_out_tile_e     : 1;
        RK_U32 reserved0             : 7;
        RK_U32 sw_pp4_out_p010_fmt   : 2;
        RK_U32 sw_pp4_out_rgb_fmt    : 5;
        RK_U32 reserved1             : 12;
        RK_U32 sw_pp4_rgb_planar     : 1;
        RK_U32 reserved2             : 1;
    } swreg451;

    struct {
        RK_U32 reserved0         : 20;
        RK_U32 sw_pp4_out_swap   : 4;
        RK_U32 reserved1         : 8;
    } swreg452;

    struct {
        RK_U32 sw_pp4_scale_hratio     : 18;
        RK_U32 sw_pp4_out_format       : 5;
        RK_U32 sw_pp4_ver_scale_mode   : 2;
        RK_U32 sw_pp4_hor_scale_mode   : 2;
        RK_U32 reserved0               : 5;
    } swreg453;

    struct {
        RK_U32 sw_pp4_scale_wratio   : 18;
        RK_U32 reserved0             : 5;
        RK_U32 reserved1             : 5;
        RK_U32 reserved2             : 1;
        RK_U32 reserved4             : 1;
        RK_U32 reserved3             : 1;
        RK_U32 reserved5             : 1;
    } swreg454;

    struct {
        RK_U32 sw_pp4_hscale_invra   : 16;
        RK_U32 sw_pp4_wscale_invra   : 16;
    } swreg455;

    struct {
        // RK_U32 sw_pp4_out_r_base_msb    : 32;
        RK_U32 sw_pp4_out_lu_base_msb   : 32;
    } swreg456;

    struct {
        // RK_U32 sw_pp4_out_r_base_lsb    : 32;
        RK_U32 sw_pp4_out_lu_base_lsb   : 32;
    } swreg457;

    struct {
        // RK_U32 sw_pp4_out_g_base_msb    : 32;
        RK_U32 sw_pp4_out_ch_base_msb   : 32;
    } swreg458;

    struct {
        // RK_U32 sw_pp4_out_g_base_lsb    : 32;
        RK_U32 sw_pp4_out_ch_base_lsb   : 32;
    } swreg459;

    struct {
        RK_U32 sw_pp4_out_c_stride   : 16;
        RK_U32 sw_pp4_out_y_stride   : 16;
    } swreg460;

    struct {
        RK_U32 sw_pp4_crop_starty     : 13;
        RK_U32 sw_pp4_rotation_mode   : 2;
        RK_U32 reserved0              : 1;
        RK_U32 sw_pp4_crop_startx     : 13;
        RK_U32 sw_pp4_flip_mode       : 2;
        RK_U32 sw_pp4_pad_sel         : 1;
    } swreg461;

    struct {
        RK_U32 sw_pp4_in_height   : 16;
        RK_U32 sw_pp4_in_width    : 16;
    } swreg462;

    struct {
        RK_U32 sw_pp4_out_height   : 16;
        RK_U32 sw_pp4_out_width    : 16;
    } swreg463;

    struct {
        // RK_U32 sw_pp4_out_b_base_msb        : 32;
        RK_U32 sw_pp4_out_lu_bot_base_msb   : 32;
    } swreg464;

    struct {
        // RK_U32 sw_pp4_out_b_base_lsb        : 32;
        RK_U32 sw_pp4_out_lu_bot_base_lsb   : 32;
    } swreg465;

    struct {
        RK_U32 sw_pp4_crop2_starty          : 13;
        RK_U32 reserved0                    : 3;
        RK_U32 sw_pp4_crop2_startx          : 13;
        RK_U32 reserved1                    : 3;
        // RK_U32 sw_pp4_out_ch_bot_base_msb   : 32;
    } swreg466;

    struct {
        RK_U32 sw_pp4_crop2_out_height      : 16;
        RK_U32 sw_pp4_crop2_out_width       : 16;
        // RK_U32 sw_pp4_out_ch_bot_base_lsb   : 32;
    } swreg467;

    struct {
        RK_U32 sw_pp4_contrast_off1     : 12;
        RK_U32 sw_pp4_contrast_off2     : 12;
        RK_U32 reserved0                : 2;
        RK_U32 sw_pp4_dither_select_b   : 2;
        RK_U32 sw_pp4_dither_select_g   : 2;
        RK_U32 sw_pp4_dither_select_r   : 2;
    } swreg468;

    struct {
        RK_U32 sw_pp4_color_coefff    : 10;
        RK_U32 sw_pp4_contrast_thr2   : 10;
        RK_U32 sw_pp4_contrast_thr1   : 10;
        RK_U32 reserved0              : 2;
    } swreg469;

    struct {
        RK_U32 sw_pp4_color_coeffa1   : 16;
        RK_U32 sw_pp4_color_coeffa2   : 16;
    } swreg470;

    struct {
        RK_U32 sw_pp4_color_coeffb   : 16;
        RK_U32 sw_pp4_color_coeffc   : 16;
    } swreg471;

    struct {
        RK_U32 sw_pp4_color_coeffd   : 16;
        RK_U32 sw_pp4_color_coeffe   : 16;
    } swreg472;

    struct {
        RK_U32 sw_pp4_out_alpha          : 8;
        RK_U32 sw_pp4_dup_hor            : 8;
        RK_U32 sw_pp4_hcale_invra_ext    : 8;
        RK_U32 sw_pp4_wscale_invra_ext   : 8;
    } swreg473;

    struct {
        RK_U32 sw_pp4_x_filter_size   : 12;
        RK_U32 sw_pp4_y_filter_size   : 12;
        RK_U32 reserved0              : 8;
    } swreg474;

    struct {
        RK_U32 sw_pp4_lanczos_tbl_base_msb   : 32;
    } swreg475;

    struct {
        RK_U32 sw_pp4_lanczos_tbl_base_lsb   : 32;
    } swreg476;

    struct {
        RK_U32 sw_pp5_out_e          : 1;
        RK_U32 sw_pp5_cr_first       : 1;
        RK_U32 sw_pp5_out_mode       : 1;
        RK_U32 sw_pp5_out_tile_e     : 1;
        RK_U32 reserved0             : 7;
        RK_U32 sw_pp5_out_p010_fmt   : 2;
        RK_U32 sw_pp5_out_rgb_fmt    : 5;
        RK_U32 reserved1             : 12;
        RK_U32 sw_pp5_rgb_planar     : 1;
        RK_U32 reserved2             : 1;
    } swreg477;

    struct {
        RK_U32 reserved0         : 20;
        RK_U32 sw_pp5_out_swap   : 4;
        RK_U32 reserved1         : 8;
    } swreg478;

    struct {
        RK_U32 sw_pp5_scale_hratio     : 18;
        RK_U32 sw_pp5_out_format       : 5;
        RK_U32 sw_pp5_ver_scale_mode   : 2;
        RK_U32 sw_pp5_hor_scale_mode   : 2;
        RK_U32 reserved0               : 5;
    } swreg479;

    struct {
        RK_U32 sw_pp5_scale_wratio   : 18;
        RK_U32 reserved0             : 5;
        RK_U32 reserved1             : 5;
        RK_U32 reserved2             : 1;
        RK_U32 reserved4             : 1;
        RK_U32 reserved3             : 1;
        RK_U32 reserved5             : 1;
    } swreg480;

    struct {
        RK_U32 sw_pp5_hscale_invra   : 16;
        RK_U32 sw_pp5_wscale_invra   : 16;
    } swreg481;

    struct {
        // RK_U32 sw_pp5_out_r_base_msb    : 32;
        RK_U32 sw_pp5_out_lu_base_msb   : 32;
    } swreg482;

    struct {
        // RK_U32 sw_pp5_out_r_base_lsb    : 32;
        RK_U32 sw_pp5_out_lu_base_lsb   : 32;
    } swreg483;

    struct {
        // RK_U32 sw_pp5_out_g_base_msb    : 32;
        RK_U32 sw_pp5_out_ch_base_msb   : 32;
    } swreg484;

    struct {
        // RK_U32 sw_pp5_out_g_base_lsb    : 32;
        RK_U32 sw_pp5_out_ch_base_lsb   : 32;
    } swreg485;

    struct {
        RK_U32 sw_pp5_out_c_stride   : 16;
        RK_U32 sw_pp5_out_y_stride   : 16;
    } swreg486;

    struct {
        RK_U32 sw_pp5_crop_starty     : 13;
        RK_U32 sw_pp5_rotation_mode   : 2;
        RK_U32 reserved0              : 1;
        RK_U32 sw_pp5_crop_startx     : 13;
        RK_U32 sw_pp5_flip_mode       : 2;
        RK_U32 sw_pp5_pad_sel         : 1;
    } swreg487;

    struct {
        RK_U32 sw_pp5_in_height   : 16;
        RK_U32 sw_pp5_in_width    : 16;
    } swreg488;

    struct {
        RK_U32 sw_pp5_out_height   : 16;
        RK_U32 sw_pp5_out_width    : 16;
    } swreg489;

    struct {
        // RK_U32 sw_pp5_out_b_base_msb        : 32;
        RK_U32 sw_pp5_out_lu_bot_base_msb   : 32;
    } swreg490;

    struct {
        // RK_U32 sw_pp5_out_b_base_lsb        : 32;
        RK_U32 sw_pp5_out_lu_bot_base_lsb   : 32;
    } swreg491;

    struct {
        RK_U32 sw_pp5_crop2_starty          : 13;
        RK_U32 reserved0                    : 3;
        RK_U32 sw_pp5_crop2_startx          : 13;
        RK_U32 reserved1                    : 3;
        // RK_U32 sw_pp5_out_ch_bot_base_msb   : 32;
    } swreg492;

    struct {
        RK_U32 sw_pp5_crop2_out_height      : 16;
        RK_U32 sw_pp5_crop2_out_width       : 16;
        // RK_U32 sw_pp5_out_ch_bot_base_lsb   : 32;
    } swreg493;

    struct {
        RK_U32 sw_pp5_contrast_off1     : 12;
        RK_U32 sw_pp5_contrast_off2     : 12;
        RK_U32 reserved0                : 2;
        RK_U32 sw_pp5_dither_select_b   : 2;
        RK_U32 sw_pp5_dither_select_g   : 2;
        RK_U32 sw_pp5_dither_select_r   : 2;
    } swreg494;

    struct {
        RK_U32 sw_pp5_color_coefff    : 10;
        RK_U32 sw_pp5_contrast_thr2   : 10;
        RK_U32 sw_pp5_contrast_thr1   : 10;
        RK_U32 reserved0              : 2;
    } swreg495;

    struct {
        RK_U32 sw_pp5_color_coeffa1   : 16;
        RK_U32 sw_pp5_color_coeffa2   : 16;
    } swreg496;

    struct {
        RK_U32 sw_pp5_color_coeffb   : 16;
        RK_U32 sw_pp5_color_coeffc   : 16;
    } swreg497;

    struct {
        RK_U32 sw_pp5_color_coeffd   : 16;
        RK_U32 sw_pp5_color_coeffe   : 16;
    } swreg498;

    struct {
        RK_U32 sw_pp5_out_alpha          : 8;
        RK_U32 sw_pp5_dup_hor            : 8;
        RK_U32 sw_pp5_hcale_invra_ext    : 8;
        RK_U32 sw_pp5_wscale_invra_ext   : 8;
    } swreg499;

    struct {
        RK_U32 sw_pp5_x_filter_size   : 12;
        RK_U32 sw_pp5_y_filter_size   : 12;
        RK_U32 reserved0              : 8;
    } swreg500;

    struct {
        RK_U32 sw_pp5_lanczos_tbl_base_msb   : 32;
    } swreg501;

    struct {
        RK_U32 sw_pp5_lanczos_tbl_base_lsb   : 32;
    } swreg502;

    struct {
        RK_U32 sw_pp1_virtual_top      : 4;
        RK_U32 sw_pp1_virtual_left     : 4;
        RK_U32 sw_pp1_virtual_bottom   : 4;
        RK_U32 sw_pp1_virtual_right    : 4;
        RK_U32 sw_pp0_virtual_top      : 4;
        RK_U32 sw_pp0_virtual_left     : 4;
        RK_U32 sw_pp0_virtual_bottom   : 4;
        RK_U32 sw_pp0_virtual_right    : 4;
    } swreg503;

    struct {
        RK_U32 sw_pp0_afbc_tile_base_msb   : 32;
    } swreg504;

    struct {
        RK_U32 sw_pp0_afbc_tile_base_lsb   : 32;
    } swreg505;

    struct {
        RK_U32 sw_pp1_afbc_tile_base_msb   : 32;
    } swreg506;

    struct {
        RK_U32 sw_pp1_afbc_tile_base_lsb   : 32;
    } swreg507;

    struct {
        RK_U32 sw_pp0_padV           : 10;
        RK_U32 sw_pp0_padU           : 10;
        RK_U32 sw_pp0_padY           : 10;
        RK_U32 sw_pp0_src_sel_mode   : 2;
    } swreg508;

    struct {
        RK_U32 sw_pp1_padV           : 10;
        RK_U32 sw_pp1_padU           : 10;
        RK_U32 sw_pp1_padY           : 10;
        RK_U32 sw_pp1_src_sel_mode   : 2;
    } swreg509;

    struct {
        RK_U32 sw_pp2_padV           : 10;
        RK_U32 sw_pp2_padU           : 10;
        RK_U32 sw_pp2_padY           : 10;
        RK_U32 sw_pp2_src_sel_mode   : 2;
    } swreg510;

    struct {
        RK_U32 sw_pp3_padV           : 10;
        RK_U32 sw_pp3_padU           : 10;
        RK_U32 sw_pp3_padY           : 10;
        RK_U32 sw_pp3_src_sel_mode   : 2;
    } swreg511;

} VdpuAv1dPPCfg;

typedef struct VdpuAv1dRegSet_t {
    struct {
        RK_U32 reserved0   : 4;
        RK_U32 reserved1   : 8;
        RK_U32 reserved2   : 4;
        RK_U32 reserved3   : 4;
        RK_U32 reserved4   : 12;
    } swreg0;

    struct {
        RK_U32 sw_dec_e                 : 1;
        RK_U32 reserved0                : 1;
        RK_U32 sw_dec_bus_int_dis       : 1;
        RK_U32 sw_dec_timeout_source    : 1;

        RK_U32 sw_dec_irq_dis           : 1;
        RK_U32 sw_dec_abort_e           : 1;
        RK_U32 sw_dec_self_reset_dis    : 1;
        RK_U32 sw_dec_tile_int_e        : 1;

        RK_U32 sw_dec_irq               : 1;
        RK_U32 reserved1                : 2;
        RK_U32 sw_dec_abort_int         : 1;

        RK_U32 sw_dec_rdy_int           : 1;
        RK_U32 sw_dec_bus_int           : 1;
        RK_U32 sw_dec_buffer_int        : 1;
        RK_U32 reserved2                : 1;

        RK_U32 sw_dec_error_int         : 1;
        RK_U32 reserved3                : 1;
        RK_U32 sw_dec_timeout           : 1;
        RK_U32 reserved4                : 2;
        RK_U32 sw_dec_ext_timeout_int   : 1;
        RK_U32 reserved5                : 1;
        RK_U32 sw_dec_tile_int          : 1;

        RK_U32 reserved6                : 8;
    } swreg1;

    struct {
        RK_U32 reserved0           : 4;
        RK_U32 sw_drm_e            : 1;
        RK_U32 reserved1           : 5;
        RK_U32 sw_dec_clk_gate_e   : 1;
        RK_U32 reserved2           : 1;

        RK_U32 sw_dec_tab_swap     : 4;
        RK_U32 reserved3           : 4;
        RK_U32 sw_dec_dirmv_swap   : 4;
        RK_U32 sw_dec_pic_swap     : 4;
        RK_U32 sw_dec_strm_swap    : 4;
    } swreg2;

    struct {
        RK_U32 reserved0                 : 8;
        RK_U32 sw_dec_out_ec_bypass      : 1;
        RK_U32 reserved1                 : 3;
        RK_U32 sw_write_mvs_e            : 1;
        RK_U32 reserved2                 : 1;
        RK_U32 sw_filtering_dis          : 1;
        RK_U32 sw_dec_out_dis            : 1;
        RK_U32 sw_dec_out_ec_byte_word   : 1;
        RK_U32 reserved3                 : 9;
        RK_U32 sw_skip_mode              : 1;
        RK_U32 sw_dec_mode               : 5;
    } swreg3;

    struct {
        RK_U32 sw_ref_frames          : 4;
        RK_U32 reserved0              : 2;
        RK_U32 sw_pic_height_in_cbs   : 13;
        RK_U32 sw_pic_width_in_cbs    : 13;
    } swreg4;

    struct {
        RK_U32 sw_ref_scaling_enable           : 1;
        RK_U32 sw_filt_level_base_gt32         : 1;
        RK_U32 sw_error_resilient              : 1;
        RK_U32 sw_force_interger_mv            : 1;

        RK_U32 sw_allow_intrabc                : 1;
        RK_U32 sw_allow_screen_content_tools   : 1;
        RK_U32 sw_reduced_tx_set_used          : 1;
        RK_U32 sw_enable_dual_filter           : 1;

        RK_U32 sw_enable_jnt_comp              : 1;
        RK_U32 sw_allow_filter_intra           : 1;
        RK_U32 sw_enable_intra_edge_filter     : 1;
        RK_U32 sw_tempor_mvp_e                 : 1;//RK_U32 reserved0                       : 1;

        RK_U32 sw_allow_interintra             : 1;
        RK_U32 sw_allow_masked_compound        : 1;
        RK_U32 sw_enable_cdef                  : 1;
        RK_U32 sw_switchable_motion_mode       : 1;

        RK_U32 sw_show_frame                   : 1;
        RK_U32 sw_superres_is_scaled           : 1;
        RK_U32 sw_allow_warp                   : 1;
        RK_U32 sw_disable_cdf_update           : 1;

        RK_U32 sw_preskip_segid                : 1;
        RK_U32 sw_delta_lf_present             : 1;
        RK_U32 sw_delta_lf_multi               : 1;
        RK_U32 sw_delta_lf_res_log             : 2;

        // RK_U32 reserved1                       : -14;
        RK_U32 sw_strm_start_bit               : 7;
    } swreg5;

    struct {
        RK_U32 sw_stream_len   : 32;
    } swreg6;

    struct {
        RK_U32 sw_delta_q_present            : 1;
        RK_U32 sw_delta_q_res_log            : 2;
        RK_U32 sw_cdef_damping               : 2;
        RK_U32 sw_cdef_bits                  : 2;
        RK_U32 sw_apply_grain                : 1;
        RK_U32 sw_num_y_points_b             : 1;
        RK_U32 sw_num_cb_points_b            : 1;
        RK_U32 sw_num_cr_points_b            : 1;
        RK_U32 sw_overlap_flag               : 1;
        RK_U32 sw_clip_to_restricted_range   : 1;
        RK_U32 sw_chroma_scaling_from_luma   : 1;
        RK_U32 sw_random_seed                : 16;
        RK_U32 sw_blackwhite_e               : 1;
        RK_U32 reserved0                     : 1;
    } swreg7;

    struct {
        RK_U32 sw_scaling_shift        : 4;
        RK_U32 sw_bit_depth_c_minus8   : 2;
        RK_U32 sw_bit_depth_y_minus8   : 2;
        RK_U32 sw_quant_base_qindex    : 8;
        RK_U32 sw_idr_pic_e            : 1;
        RK_U32 sw_superres_pic_width   : 15;
    } swreg8;

    struct {
        RK_U32 reserved0                   : 2;
        RK_U32 sw_ref4_sign_bias           : 1;
        RK_U32 sw_ref5_sign_bias           : 1;
        RK_U32 sw_ref6_sign_bias           : 1;
        RK_U32 sw_mf1_type                 : 3;
        RK_U32 sw_mf2_type                 : 3;
        RK_U32 sw_mf3_type                 : 3;
        RK_U32 sw_scale_denom_minus9       : 3;
        RK_U32 sw_last_active_seg          : 3;
        RK_U32 sw_context_update_tile_id   : 12;
    } swreg9;

    struct {
        RK_U32 sw_tile_transpose           : 1;
        RK_U32 sw_tile_enable              : 1;
        RK_U32 sw_multicore_full_width     : 8;
        RK_U32 sw_num_tile_rows_8k_av1     : 7;
        RK_U32 sw_num_tile_cols_8k         : 7;
        RK_U32 sw_multicore_tile_start_x   : 8;
    } swreg10;

    struct {
        RK_U32 sw_use_temporal3_mvs                 : 1;
        RK_U32 sw_use_temporal2_mvs                 : 1;
        RK_U32 sw_use_temporal1_mvs                 : 1;
        RK_U32 sw_use_temporal0_mvs                 : 1;
        RK_U32 sw_comp_pred_mode                    : 2;
        RK_U32 reserved0                            : 1;
        RK_U32 sw_high_prec_mv_e                    : 1;
        RK_U32 sw_mcomp_filt_type                   : 3;
        RK_U32 sw_multicore_expect_context_update   : 1;
        RK_U32 sw_multicore_sbx_offset              : 7;
        RK_U32 sw_multicore_tile_col                : 7;
        RK_U32 reserved1                            : 1;
        RK_U32 sw_transform_mode                    : 3;
        RK_U32 sw_dec_tile_size_mag                 : 2;
    } swreg11;

    struct {
        RK_U32 reserved0                    : 2;
        RK_U32 sw_seg_quant_sign            : 8;
        RK_U32 sw_max_cb_size               : 3;
        RK_U32 sw_min_cb_size               : 3;
        RK_U32 sw_av1_comp_pred_fixed_ref   : 3;
        RK_U32 sw_multicore_tile_width      : 7;
        RK_U32 sw_pic_height_pad            : 3;
        RK_U32 sw_pic_width_pad             : 3;
    } swreg12;

    struct {
        RK_U32 sw_segment_e                : 1;
        RK_U32 sw_segment_upd_e            : 1;
        RK_U32 sw_segment_temp_upd_e       : 1;
        RK_U32 sw_comp_pred_var_ref0_av1   : 3;
        RK_U32 sw_comp_pred_var_ref1_av1   : 3;
        RK_U32 sw_lossless_e               : 1;
        RK_U32 reserved0                   : 1;
        RK_U32 sw_qp_delta_ch_ac_av1       : 7;
        RK_U32 sw_qp_delta_ch_dc_av1       : 7;
        RK_U32 sw_qp_delta_y_dc_av1        : 7;
    } swreg13;

    struct {
        RK_U32 sw_quant_seg0               : 8;
        RK_U32 sw_filt_level_seg0          : 6;
        RK_U32 sw_skip_seg0                : 1;
        RK_U32 sw_refpic_seg0          : 4;
        RK_U32 sw_filt_level_delta0_seg0   : 7;
        RK_U32 sw_filt_level0              : 6;
    } swreg14;

    struct {
        RK_U32 sw_quant_seg1               : 8;
        RK_U32 sw_filt_level_seg1          : 6;
        RK_U32 sw_skip_seg1                : 1;
        RK_U32 sw_refpic_seg1          : 4;
        RK_U32 sw_filt_level_delta0_seg1   : 7;
        RK_U32 sw_filt_level1              : 6;
    } swreg15;

    struct {
        RK_U32 sw_quant_seg2               : 8;
        RK_U32 sw_filt_level_seg2          : 6;
        RK_U32 sw_skip_seg2                : 1;
        RK_U32 sw_refpic_seg2          : 4;
        RK_U32 sw_filt_level_delta0_seg2   : 7;
        RK_U32 sw_filt_level2              : 6;
    } swreg16;

    struct {
        RK_U32 sw_quant_seg3               : 8;
        RK_U32 sw_filt_level_seg3          : 6;
        RK_U32 sw_skip_seg3                : 1;
        RK_U32 sw_refpic_seg3          : 4;
        RK_U32 sw_filt_level_delta0_seg3   : 7;
        RK_U32 sw_filt_level3              : 6;
    } swreg17;

    struct {
        RK_U32 sw_quant_seg4               : 8;
        RK_U32 sw_filt_level_seg4          : 6;
        RK_U32 sw_skip_seg4                : 1;
        RK_U32 sw_refpic_seg4          : 4;
        RK_U32 sw_filt_level_delta0_seg4   : 7;
        RK_U32 sw_lr_type                  : 6;
    } swreg18;

    struct {
        RK_U32 sw_quant_seg5               : 8;
        RK_U32 sw_filt_level_seg5          : 6;
        RK_U32 sw_skip_seg5                : 1;
        RK_U32 sw_refpic_seg5              : 4;
        RK_U32 sw_filt_level_delta0_seg5   : 7;
        RK_U32 sw_lr_unit_size             : 6;
    } swreg19;

    struct {
        RK_U32 sw_filt_level_delta1_seg0   : 7;
        RK_U32 sw_filt_level_delta2_seg0   : 7;
        RK_U32 sw_filt_level_delta3_seg0   : 7;
        RK_U32 sw_global_mv_seg0           : 1;
        RK_U32 sw_mf1_last_offset          : 9;
        RK_U32 reserved0                   : 1;
    } swreg20;

    struct {
        RK_U32 sw_filt_level_delta1_seg1   : 7;
        RK_U32 sw_filt_level_delta2_seg1   : 7;
        RK_U32 sw_filt_level_delta3_seg1   : 7;
        RK_U32 sw_global_mv_seg1           : 1;
        RK_U32 sw_mf1_last2_offset         : 9;
        RK_U32 reserved0                   : 1;
    } swreg21;

    struct {
        RK_U32 sw_filt_level_delta1_seg2   : 7;
        RK_U32 sw_filt_level_delta2_seg2   : 7;
        RK_U32 sw_filt_level_delta3_seg2   : 7;
        RK_U32 sw_global_mv_seg2           : 1;
        RK_U32 sw_mf1_last3_offset         : 9;
        RK_U32 reserved0                   : 1;
    } swreg22;

    struct {
        RK_U32 sw_filt_level_delta1_seg3   : 7;
        RK_U32 sw_filt_level_delta2_seg3   : 7;
        RK_U32 sw_filt_level_delta3_seg3   : 7;
        RK_U32 sw_global_mv_seg3           : 1;
        RK_U32 sw_mf1_golden_offset        : 9;
        RK_U32 reserved0                   : 1;
    } swreg23;

    struct {
        RK_U32 sw_filt_level_delta1_seg4   : 7;
        RK_U32 sw_filt_level_delta2_seg4   : 7;
        RK_U32 sw_filt_level_delta3_seg4   : 7;
        RK_U32 sw_global_mv_seg4           : 1;
        RK_U32 sw_mf1_bwdref_offset        : 9;
        RK_U32 reserved0                   : 1;
    } swreg24;

    struct {
        RK_U32 sw_filt_level_delta1_seg5   : 7;
        RK_U32 sw_filt_level_delta2_seg5   : 7;
        RK_U32 sw_filt_level_delta3_seg5   : 7;
        RK_U32 sw_global_mv_seg5           : 1;
        RK_U32 sw_mf1_altref2_offset       : 9;
        RK_U32 reserved0                   : 1;
    } swreg25;

    struct {
        RK_U32 sw_filt_level_delta1_seg6   : 7;
        RK_U32 sw_filt_level_delta2_seg6   : 7;
        RK_U32 sw_filt_level_delta3_seg6   : 7;
        RK_U32 sw_global_mv_seg6           : 1;
        RK_U32 sw_mf1_altref_offset        : 9;
        RK_U32 reserved0                   : 1;
    } swreg26;

    struct {
        RK_U32 sw_filt_level_delta1_seg7   : 7;
        RK_U32 sw_filt_level_delta2_seg7   : 7;
        RK_U32 sw_filt_level_delta3_seg7   : 7;
        RK_U32 sw_global_mv_seg7           : 1;
        RK_U32 sw_mf2_last_offset          : 9;
        RK_U32 reserved0                   : 1;
    } swreg27;

    struct {
        RK_U32 sw_cb_offset          : 9;
        RK_U32 sw_cb_luma_mult       : 8;
        RK_U32 sw_cb_mult            : 8;
        RK_U32 sw_quant_delta_v_dc   : 7;
    } swreg28;

    struct {
        RK_U32 sw_cr_offset          : 9;
        RK_U32 sw_cr_luma_mult       : 8;
        RK_U32 sw_cr_mult            : 8;
        RK_U32 sw_quant_delta_v_ac   : 7;
    } swreg29;

    struct {
        RK_U32 sw_filt_ref_adj_5   : 7;
        RK_U32 sw_filt_ref_adj_4   : 7;
        RK_U32 sw_filt_mb_adj_1    : 7;
        RK_U32 sw_filt_mb_adj_0    : 7;
        RK_U32 sw_filt_sharpness   : 3;
        RK_U32 reserved0           : 1;
    } swreg30;

    struct {
        RK_U32 sw_quant_seg6               : 8;
        RK_U32 sw_filt_level_seg6          : 6;
        RK_U32 sw_skip_seg6                : 1;
        RK_U32 sw_refpic_seg6              : 4;
        RK_U32 sw_filt_level_delta0_seg6   : 7;
        RK_U32 sw_skip_ref0                : 4;
        RK_U32 reserved0                   : 2;
    } swreg31;

    struct {
        RK_U32 sw_quant_seg7               : 8;
        RK_U32 sw_filt_level_seg7          : 6;
        RK_U32 sw_skip_seg7                : 1;
        RK_U32 sw_refpic_seg7              : 4;
        RK_U32 sw_filt_level_delta0_seg7   : 7;
        RK_U32 sw_skip_ref1                : 4;
        RK_U32 reserved0                   : 2;
    } swreg32;

    struct {
        RK_U32 sw_ref0_height   : 16;
        RK_U32 sw_ref0_width    : 16;
    } swreg33;

    struct {
        RK_U32 sw_ref1_height   : 16;
        RK_U32 sw_ref1_width    : 16;
    } swreg34;

    struct {
        RK_U32 sw_ref2_height   : 16;
        RK_U32 sw_ref2_width    : 16;
    } swreg35;

    struct {
        RK_U32 sw_ref0_ver_scale   : 16;
        RK_U32 sw_ref0_hor_scale   : 16;
    } swreg36;

    struct {
        RK_U32 sw_ref1_ver_scale   : 16;
        RK_U32 sw_ref1_hor_scale   : 16;
    } swreg37;

    struct {
        RK_U32 sw_ref2_ver_scale   : 16;
        RK_U32 sw_ref2_hor_scale   : 16;
    } swreg38;

    struct {
        RK_U32 sw_ref3_ver_scale   : 16;
        RK_U32 sw_ref3_hor_scale   : 16;
    } swreg39;

    struct {
        RK_U32 sw_ref4_ver_scale   : 16;
        RK_U32 sw_ref4_hor_scale   : 16;
    } swreg40;

    struct {
        RK_U32 sw_ref5_ver_scale   : 16;
        RK_U32 sw_ref5_hor_scale   : 16;
    } swreg41;

    struct {
        RK_U32 sw_ref6_ver_scale   : 16;
        RK_U32 sw_ref6_hor_scale   : 16;
    } swreg42;
    struct {
        RK_U32 sw_ref3_height   : 16;
        RK_U32 sw_ref3_width    : 16;
    } swreg43;

    struct {
        RK_U32 sw_ref4_height   : 16;
        RK_U32 sw_ref4_width    : 16;
    } swreg44;

    struct {
        RK_U32 sw_ref5_height   : 16;
        RK_U32 sw_ref5_width    : 16;
    } swreg45;

    struct {
        RK_U32 sw_ref6_height   : 16;
        RK_U32 sw_ref6_width    : 16;
    } swreg46;

    struct {
        RK_U32 sw_mf2_last2_offset    : 9;
        RK_U32 sw_mf2_last3_offset    : 9;
        RK_U32 sw_mf2_golden_offset   : 9;
        RK_U32 sw_qmlevel_y           : 4;
        RK_U32 reserved0              : 1;
    } swreg47;

    struct {
        RK_U32 sw_mf2_bwdref_offset    : 9;
        RK_U32 sw_mf2_altref2_offset   : 9;
        RK_U32 sw_mf2_altref_offset    : 9;
        RK_U32 sw_qmlevel_u            : 4;
        RK_U32 reserved0               : 1;
    } swreg48;

    struct {
        RK_U32 sw_filt_ref_adj_6   : 7;
        RK_U32 sw_filt_ref_adj_7   : 7;
        RK_U32 sw_qmlevel_v        : 4;
        RK_U32 reserved0           : 14;
    } swreg49;

    struct {
        RK_U32 SW_DEC_MAX_OWIDTH   : 16;
        RK_U32 SW_DEC_AV1_PROF     : 1;
        RK_U32 reserved0           : 15;
    } swreg50;

    struct {
        RK_U32 sw_superres_chroma_step   : 14;
        RK_U32 sw_superres_luma_step     : 14;
        RK_U32 reserved0                 : 4;
    } swreg51;

    struct {
        RK_U32 sw_superres_init_chroma_subpel_x   : 14;
        RK_U32 sw_superres_init_luma_subpel_x     : 14;
        RK_U32 reserved0                          : 4;
    } swreg52;

    struct {
        RK_U32 sw_cdef_chroma_secondary_strength   : 16;
        RK_U32 sw_cdef_luma_secondary_strength     : 16;
    } swreg53;

    struct {
        RK_U32 reserved0                 : 5;
        RK_U32 SW_DEC_ADDR64_SUPPORTED   : 1;
        RK_U32 reserved1                 : 11;
        RK_U32 SW_DEC_RFC_EXIST          : 2;
        RK_U32 reserved2                 : 13;
    } swreg54;

    struct {
        RK_U32 sw_apf_threshold        : 16;
        RK_U32 reserved0               : 14;
        RK_U32 sw_apf_single_pu_mode   : 1;
        RK_U32 sw_apf_disable          : 1;
    } swreg55;

    struct {
        RK_U32 SW_DEC_MAX_OHEIGHT   : 15;
        RK_U32 SW_DEC_DATA_S_W      : 2;
        RK_U32 SW_DEC_ADDR_S_W      : 1;
        RK_U32 SW_DEC_DATA_M_W      : 2;
        RK_U32 SW_DEC_ADDR_M_W      : 1;
        RK_U32 SW_DEC_SLAVE_BUS     : 2;
        RK_U32 SW_DEC_MASTER_BUS    : 2;
        RK_U32 SW_DEC_JOINT         : 1;
        RK_U32 reserved0            : 6;
    } swreg56;

    struct {
        RK_U32 reserved0      : 6;
        RK_U32 fuse_dec_av1   : 1;
        RK_U32 reserved1      : 25;
    } swreg57;

    struct {
        RK_U32 sw_dec_max_burst        : 8;
        RK_U32 sw_dec_buswidth         : 3;
        RK_U32 sw_dec_multicore_mode   : 2;
        RK_U32 sw_dec_axi_wd_id_e      : 1;
        RK_U32 sw_dec_axi_rd_id_e      : 1;
        RK_U32 reserved0               : 2;
        RK_U32 sw_dec_mc_polltime      : 10;
        RK_U32 sw_dec_mc_pollmode      : 2;
        RK_U32 reserved1               : 3;
    } swreg58;

    struct {
        RK_U32 sw_filt_ref_adj_3   : 7;
        RK_U32 sw_filt_ref_adj_2   : 7;
        RK_U32 sw_filt_ref_adj_1   : 7;
        RK_U32 sw_filt_ref_adj_0   : 7;
        RK_U32 sw_ref0_sign_bias   : 1;
        RK_U32 sw_ref1_sign_bias   : 1;
        RK_U32 sw_ref2_sign_bias   : 1;
        RK_U32 sw_ref3_sign_bias   : 1;
    } swreg59;

    struct {
        RK_U32 sw_dec_axi_rd_id   : 16;
        RK_U32 sw_dec_axi_wr_id   : 16;
    } swreg60;

    struct {
        RK_U32 reserved0           : 12;
        RK_U32 fuse_pp_maxw_352    : 1;
        RK_U32 fuse_pp_maxw_720    : 1;
        RK_U32 fuse_pp_maxw_1280   : 1;
        RK_U32 fuse_pp_maxw_1920   : 1;
        RK_U32 fuse_pp_maxw_4k     : 1;
        RK_U32 reserved1           : 12;
        RK_U32 fuse_pp_ablend      : 1;
        RK_U32 fuse_pp_scaling     : 1;
        RK_U32 fuse_pp_pp          : 1;
    } swreg61;

    struct {
        RK_U32 sw_cu_location_y   : 16;
        RK_U32 sw_cu_location_x   : 16;
    } swreg62;

    struct {
        RK_U32 sw_perf_cycle_count   : 32;
    } swreg63;

    /* swreg64 - swreg183 */
    VdpuAv1dBase addr_cfg;

    struct {
        RK_U32 sw_cur_last_roffset   : 9;
        RK_U32 sw_cur_last_offset    : 9;
        RK_U32 sw_mf3_last_offset    : 9;
        RK_U32 sw_ref0_gm_mode       : 2;
        RK_U32 reserved0             : 3;
    } swreg184;

    struct {
        RK_U32 sw_cur_last2_roffset   : 9;
        RK_U32 sw_cur_last2_offset    : 9;
        RK_U32 sw_mf3_last2_offset    : 9;
        RK_U32 sw_ref1_gm_mode        : 2;
        RK_U32 reserved0              : 3;
    } swreg185;

    struct {
        RK_U32 sw_cur_last3_roffset   : 9;
        RK_U32 sw_cur_last3_offset    : 9;
        RK_U32 sw_mf3_last3_offset    : 9;
        RK_U32 sw_ref2_gm_mode        : 2;
        RK_U32 reserved0              : 3;
    } swreg186;

    struct {
        RK_U32 sw_cur_golden_roffset   : 9;
        RK_U32 sw_cur_golden_offset    : 9;
        RK_U32 sw_mf3_golden_offset    : 9;
        RK_U32 sw_ref3_gm_mode         : 2;
        RK_U32 reserved0               : 3;
    } swreg187;

    struct {
        RK_U32 sw_cur_bwdref_roffset   : 9;
        RK_U32 sw_cur_bwdref_offset    : 9;
        RK_U32 sw_mf3_bwdref_offset    : 9;
        RK_U32 sw_ref4_gm_mode         : 2;
        RK_U32 reserved0               : 3;
    } swreg188;

    struct {
        RK_U32 sw_dec_out_tybase_msb   : 32;
    } swreg189;

    struct {
        RK_U32 sw_dec_out_tybase_lsb   : 32;
    } swreg190;

    struct {
        RK_U32 sw_refer0_tybase_msb   : 32;
    } swreg191;

    struct {
        RK_U32 sw_refer0_tybase_lsb   : 32;
    } swreg192;

    struct {
        RK_U32 sw_refer1_tybase_msb   : 32;
    } swreg193;

    struct {
        RK_U32 sw_refer1_tybase_lsb   : 32;
    } swreg194;

    struct {
        RK_U32 sw_refer2_tybase_msb   : 32;
    } swreg195;

    struct {
        RK_U32 sw_refer2_tybase_lsb   : 32;
    } swreg196;

    struct {
        RK_U32 sw_refer3_tybase_msb   : 32;
    } swreg197;

    struct {
        RK_U32 sw_refer3_tybase_lsb   : 32;
    } swreg198;

    struct {
        RK_U32 sw_refer4_tybase_msb   : 32;
    } swreg199;

    struct {
        RK_U32 sw_refer4_tybase_lsb   : 32;
    } swreg200;

    struct {
        RK_U32 sw_refer5_tybase_msb   : 32;
    } swreg201;

    struct {
        RK_U32 sw_refer5_tybase_lsb   : 32;
    } swreg202;

    struct {
        RK_U32 sw_refer6_tybase_msb   : 32;
    } swreg203;

    struct {
        RK_U32 sw_refer6_tybase_lsb   : 32;
    } swreg204;

    RK_U32 reserved_205_222[18];
    struct {
        RK_U32 sw_dec_out_tcbase_msb   : 32;
    } swreg223;

    struct {
        RK_U32 sw_dec_out_tcbase_lsb   : 32;
    } swreg224;

    struct {
        RK_U32 sw_refer0_tcbase_msb   : 32;
    } swreg225;

    struct {
        RK_U32 sw_refer0_tcbase_lsb   : 32;
    } swreg226;

    struct {
        RK_U32 sw_refer1_tcbase_msb   : 32;
    } swreg227;

    struct {
        RK_U32 sw_refer1_tcbase_lsb   : 32;
    } swreg228;

    struct {
        RK_U32 sw_refer2_tcbase_msb   : 32;
    } swreg229;

    struct {
        RK_U32 sw_refer2_tcbase_lsb   : 32;
    } swreg230;

    struct {
        RK_U32 sw_refer3_tcbase_msb   : 32;
    } swreg231;

    struct {
        RK_U32 sw_refer3_tcbase_lsb   : 32;
    } swreg232;

    struct {
        RK_U32 sw_refer4_tcbase_msb   : 32;
    } swreg233;

    struct {
        RK_U32 sw_refer4_tcbase_lsb   : 32;
    } swreg234;

    struct {
        RK_U32 sw_refer5_tcbase_msb   : 32;
    } swreg235;

    struct {
        RK_U32 sw_refer5_tcbase_lsb   : 32;
    } swreg236;

    struct {
        RK_U32 sw_refer6_tcbase_msb   : 32;
    } swreg237;

    struct {
        RK_U32 sw_refer6_tcbase_lsb   : 32;
    } swreg238;

    RK_U32 reserved_239_256[18];
    struct {
        RK_U32 sw_cur_altref2_roffset   : 9;
        RK_U32 sw_cur_altref2_offset    : 9;
        RK_U32 sw_mf3_altref2_offset    : 9;
        RK_U32 sw_ref5_gm_mode          : 2;
        RK_U32 reserved0                : 3;
    } swreg257;

    struct {
        RK_U32 sw_strm_buffer_len   : 32;
    } swreg258;

    struct {
        RK_U32 sw_strm_start_offset   : 32;
    } swreg259;

    struct {
        RK_U32 reserved0                  : 21;
        RK_U32 sw_ppd_blend_exist         : 1;
        RK_U32 reserved1                  : 1;
        RK_U32 sw_ppd_dith_exist          : 1;
        RK_U32 sw_ablend_crop_e           : 1;
        RK_U32 sw_pp_format_p010_e        : 1;
        RK_U32 sw_pp_format_customer1_e   : 1;
        RK_U32 sw_pp_crop_exist           : 1;
        RK_U32 sw_pp_up_level             : 1;
        RK_U32 sw_pp_down_level           : 2;
        RK_U32 sw_pp_exist                : 1;
    } swreg260;

    struct {
        RK_U32 sw_dec_error_code   : 8;
        RK_U32 reserved0           : 24;
    } swreg261;

    struct {
        RK_U32 sw_cur_altref_roffset   : 9;
        RK_U32 sw_cur_altref_offset    : 9;
        RK_U32 sw_mf3_altref_offset    : 9;
        RK_U32 sw_ref6_gm_mode         : 2;
        RK_U32 reserved0               : 3;
    } swreg262;

    struct {
        RK_U32 sw_cdef_luma_primary_strength   : 32;
    } swreg263;

    struct {
        RK_U32 sw_cdef_chroma_primary_strength   : 32;
    } swreg264;

    struct {
        RK_U32 sw_axi_arqos               : 4;
        RK_U32 sw_axi_awqos               : 4;

        RK_U32 sw_axi_wr_ostd_threshold   : 10;
        RK_U32 sw_axi_rd_ostd_threshold   : 10;

        RK_U32 reserved0                  : 3;
        RK_U32 sw_axi_wr_4k_dis           : 1;
    } swreg265;

    struct {
        RK_U32 reserved0             : 5;
        RK_U32 sw_128bit_mode        : 1;
        RK_U32 reserved1             : 4;
        RK_U32 sw_wr_shaper_bypass   : 1;
        RK_U32 reserved2             : 19;
        RK_U32 sw_error_conceal_e    : 1;
        RK_U32 reserved3             : 1;
    } swreg266;

    RK_U32 reserved_267_297[31];
    struct {
        RK_U32 sw_superres_chroma_step_invra   : 16;
        RK_U32 sw_superres_luma_step_invra     : 16;
    } swreg298;

    struct {
        RK_U32 sw_dec_pred_dataout_cnt   : 32;
    } swreg299;

    struct {
        RK_U32 sw_dec_axi_r_len_cnt   : 32;
    } swreg300;

    struct {
        RK_U32 sw_dec_axi_r_dat_cnt   : 32;
    } swreg301;

    struct {
        RK_U32 sw_dec_axi_r_req_cnt   : 32;
    } swreg302;

    struct {
        RK_U32 sw_dec_axi_rlast_cnt   : 32;
    } swreg303;

    struct {
        RK_U32 sw_dec_axi_w_len_cnt   : 32;
    } swreg304;

    struct {
        RK_U32 sw_dec_axi_w_dat_cnt   : 32;
    } swreg305;

    struct {
        RK_U32 sw_dec_axi_w_req_cnt   : 32;
    } swreg306;

    struct {
        RK_U32 sw_dec_axi_wlast_cnt   : 32;
    } swreg307;

    struct {
        RK_U32 sw_dec_axi_w_ack   : 32;
    } swreg308;

    struct {
        RK_U32 hw_build_id   : 32;
    } swreg309;

    struct {
        RK_U32 hw_syn_id   : 16;
        RK_U32 reserved0   : 16;
    } swreg310;

    struct {
        RK_U32 reserved0   : 32;
    } swreg311;

    struct {
        RK_U32 reserved0   : 32;
    } swreg312;

    struct {
        RK_U32 reserved0   : 32;
    } swreg313;

    struct {
        RK_U32 sw_dec_alignment   : 16;
        RK_U32 reserved0          : 16;
    } swreg314;

    struct {
        RK_U32 sw_tile_left   : 32;
    } swreg315;

    RK_U32 reserved_316;
    struct {
        RK_U32 reserved0            : 28;
        RK_U32 sw_pp_line_cnt_sel   : 2;
        RK_U32 reserved1            : 2;
    } swreg317;

    struct {
        RK_U32 sw_ext_timeout_cycles       : 31;
        RK_U32 sw_ext_timeout_override_e   : 1;
    } swreg318;

    struct {
        RK_U32 sw_timeout_cycles       : 31;
        RK_U32 sw_timeout_override_e   : 1;
    } swreg319;

    VdpuAv1dPPCfg vdpu_av1d_pp_cfg;

} VdpuAv1dRegSet;

#endif
