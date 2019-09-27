/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H265E_VEPU22_DEF_H__
#define __HAL_H265E_VEPU22_DEF_H__

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_hal.h"


typedef enum  {
    H265E_LITTLE_ENDIAN,
    H265E_BIG_ENDIAN
} H265E_STREAM_ENDIAN;

#define H265E_MAIN_TILER (0)
#define H265E_MAIN_LEVEL41 (0)
/*
* HEVC Profile enum type
*/
typedef enum  {
    H265E_Profile_Unknown      = 0x0,
    H265E_Profile_Main         = 0x1,
    H265E_Profile_Main10       = 0x2,
    /*
     * Main10 profile with HDR SEI support.
     */
    H265E_Profile_Main10HDR10  = 0x1000,
    H265E_Profile_Max          = 0x7FFFFFFF
} H265E_PROFILE_TYPE;

typedef enum {
    H265E_PIC_TYPE_I = 0,
    H265E_PIC_TYPE_P = 1,
    /*
     * not support in H265 Encoder
     */
    H265E_PIC_TYPE_B = 2,
    H265E_PIC_TYPE_IDR = 3,
    H265E_PIC_TYPE_CRA = 4,
} H265E_PIC_TYPE;

typedef enum {
    H265E_SET_CFG_INIT = 0,
    H265E_SET_PREP_CFG = 1,
    H265E_SET_RC_CFG = 2,
    H265E_SET_CODEC_CFG = 4,
    H265E_SET_ALL_CFG = (H265E_SET_PREP_CFG | H265E_SET_RC_CFG | H265E_SET_CODEC_CFG)
} H265eCfgOption;

typedef enum {
    H265E_CFG_SEQ_SRC_SIZE_CHANGE             = (1 << 0),
    H265E_CFG_SEQ_PARAM_CHANGE                = (1 << 1),
    H265E_CFG_GOP_PARAM_CHANGE                = (1 << 2),
    H265E_CFG_INTRA_PARAM_CHANGE              = (1 << 3),
    H265E_CFG_CONF_WIN_TOP_BOT_CHANGE         = (1 << 4),
    H265E_CFG_CONF_WIN_LEFT_RIGHT_CHANGE      = (1 << 5),
    H265E_CFG_FRAME_RATE_CHANGE               = (1 << 6),
    H265E_CFG_INDEPENDENT_SLICE_CHANGE        = (1 << 7),
    H265E_CFG_DEPENDENT_SLICE_CHANGE          = (1 << 8),
    H265E_CFG_INTRA_REFRESH_CHANGE            = (1 << 9),
    H265E_CFG_PARAM_CHANGE                    = (1 << 10),
    H265E_CFG_CHANGE_RESERVED                 = (1 << 11),
    H265E_CFG_RC_PARAM_CHANGE                 = (1 << 12),
    H265E_CFG_RC_MIN_MAX_QP_CHANGE            = (1 << 13),
    H265E_CFG_RC_TARGET_RATE_LAYER_0_3_CHANGE = (1 << 14),
    H265E_CFG_RC_TARGET_RATE_LAYER_4_7_CHANGE = (1 << 15),
    H265E_CFG_SET_NUM_UNITS_IN_TICK           = (1 << 18),
    H265E_CFG_SET_TIME_SCALE                  = (1 << 19),
    H265E_CFG_SET_NUM_TICKS_POC_DIFF_ONE      = (1 << 20),
    H265E_CFG_RC_TRANS_RATE_CHANGE            = (1 << 21),
    H265E_CFG_RC_TARGET_RATE_CHANGE           = (1 << 22),
    H265E_CFG_ROT_PARAM_CHANGE                = (1 << 23),
    H265E_CFG_NR_PARAM_CHANGE                 = (1 << 24),
    H265E_CFG_NR_WEIGHT_CHANGE                = (1 << 25),
    H265E_CFG_SET_VCORE_LIMIT                 = (1 << 27),
    H265E_CFG_CHANGE_SET_PARAM_ALL            = (0xFFFFFFFF),
} H265eCfgMask;


typedef enum {
    H265E_SRC_YUV_420 = 0,

    /*
     * 3Plane 1.Y, 2.U, 3.V YYYYYYYYY UUUU VVVV
     */
    H265E_SRC_YUV_420_YU12 = 0,

    /*
     * 3Plane 1.Y, 2.V, 3.U YYYYYYYYYY VVVV UUUU
     */
    H265E_SRC_YUV_420_YV12,

    /*
     * 2 Plane 1.Y 2. UV   YYYYYYYYY UVUVUVUVUVU
     */
    H265E_SRC_YUV_420_NV12,

    /*
     * 2 Plane 1.Y 2. VU   YYYYYYYYY VUVUVUVUVUV
     */
    H265E_SRC_YUV_420_NV21,
    H265E_SRC_YUV_420_MAX,
} H265E_SRC_FORMAT;


typedef enum {
    LINEAR_FRAME_MAP            = 0,

    /*
     * Tiled frame vertical map type
     */
    TILED_FRAME_V_MAP           = 1,

    /*
     * Tiled frame horizontal map type
     */
    TILED_FRAME_H_MAP           = 2,

    /*
     * Tiled mixed field vertical map type
     */
    TILED_FIELD_V_MAP           = 3,

    /*
     * Tiled mixed field vertical map type
     */
    TILED_MIXED_V_MAP           = 4,

    /*
     * Tiled frame horizontal map type
     */
    TILED_FRAME_MB_RASTER_MAP   = 5,

    /*
     * Tiled frame horizontal map type
     */
    TILED_FIELD_MB_RASTER_MAP   = 6,

    /*
     * Tiled frame no bank map. WAVE4 and CODA7L do not support tiledmap type for frame buffer.
     */
    TILED_FRAME_NO_BANK_MAP     = 7,

    /*
     * Tiled field no bank map. WAVE4 and CODA7L do not support tiledmap type for frame buffer.
     */
    TILED_FIELD_NO_BANK_MAP     = 8,

    /*
     * Linear field map type. WAVE4 and CODA7L do not support tiledmap type for frame buffer.
     */
    LINEAR_FIELD_MAP            = 9,

    CODA_TILED_MAP_TYPE_MAX     = 10,

    /*
     * Compressed framebuffer type
     */
    COMPRESSED_FRAME_MAP        = 10,

    /*
     * Tiled sub-CTU(32x32) map type - within a sub-CTU,
     * the first row of 16x32 is read in vertical direction and then the second row of 16x32 is read.
     */
    TILED_SUB_CTU_MAP           = 11,

    /*
     * AFBC(ARM Frame Buffer Compression) compressed frame map type
     */
    ARM_COMPRESSED_FRAME_MAP    = 12,
    TILED_MAP_TYPE_MAX
} H265E_TiledMapType;

#define H265E_MAX_NUM_TEMPORAL_LAYER   7
#define H265E_MAX_GOP_NUM              8
#define H265E_MIN_PIC_WIDTH            256
#define H265E_MIN_PIC_HEIGHT           128
#define H265E_MAX_PIC_WIDTH            1920
#define H265E_MAX_PIC_HEIGHT           1080
#define MAX_ROI_NUMBER                 64

typedef enum {
    /*
     * gop size and intra period define by custom
     */
    PRESET_IDX_CUSTOM_GOP       = 0,

    /*
     * All Intra, gopsize = 1
     */
    PRESET_IDX_ALL_I            = 1,

    /*
     * Consecutive P, cyclic gopsize = 1
     */
    PRESET_IDX_IPP              = 2,

    /*
     * Consecutive P, cyclic gopsize = 4
     */
    PRESET_IDX_IPPPP            = 6,
} H265E_GOP_PRESET_IDX;

typedef enum {
    H265E_OPT_COMMON          = 0,
    H265E_OPT_CUSTOM_GOP      = 1,
    H265E_OPT_CUSTOM_HEADER   = 2,
    H265E_OPT_VUI             = 3,
    H265E_OPT_ALL_PARAM       = 0xffffffff
} H265E_SET_PARAM_OPTION;

typedef enum {
    H265E_PARAM_CHANEGED_COMMON          = 1,
    H265E_PARAM_CHANEGED_CUSTOM_GOP      = 2,
    H265E_PARAM_CHANEGED_VUI             = 4,
    H265E_PARAM_CHANEGED_REGISTER_BUFFER = 8
} H265E_PARAM_CHANEGED;

/*
 * this is a data structure for setting CTU level options (ROI, CTU mode, CTU QP) in HEVC encoder.
 */
typedef struct {
    /*
     * It enables ROI map.
     */
    RK_U32 roi_enable;

    /*
     * It specifies the delta QP for ROI important level.
     */
    RK_U32 roi_delta_qp;

    /*
     * Endianess of ROI CTU map.
     */
    RK_U32 map_endian;
    /*
     * Stride of CTU-level ROI/mode/QP map
     * Set this with (Width  + CTB_SIZE - 1) / CTB_SIZE
     */
    RK_U32 map_stride;

    /*
     * It enables CTU QP map that allows CTUs to be encoded with the given QPs.
     * NOTE: rc_enable should be turned off for this, encoding with the given CTU QPs.
     */
    RK_U32 ctu_qp_enable;
} H265e_CTU;

typedef struct {
    /*
     * It enables to encode the prefix SEI NAL which is given by host.
     */
    RK_U8 prefix_sei_nal_enable;

    /*
     * A flag whether to encode PREFIX_SEI_DATA with a picture of this command or with a source picture of the buffer at the moment
     * 0 : encode PREFIX_SEI_DATA when a source picture is encoded.
     * 1 : encode PREFIX_SEI_DATA at this command.
     */
    RK_U8 prefix_sei_data_order;

    /* enables to encode the suffix SEI NAL which is given by host.*/
    RK_U8 suffix_sei_nal_enable;

    /*
     * A flag whether to encode SUFFIX_SEI_DATA with a picture of this command or with a source picture of the buffer at the moment
     * 0 : encode SUFFIX_SEI_DATA when a source picture is encoded.
     * 1 : encode SUFFIX_SEI_DATA at this command.
     */
    RK_U8 suffix_sei_data_enc_order;

    /*
     * The total byte size of the prefix SEI
     */
    RK_U32 prefix_sei_data_size;

    /*
     * The start address of the total prefix SEI NALs to be encoded
     */
    RK_U32 prefix_sei_nal_addr;

    /*
     * The total byte size of the suffix SEI
     */
    RK_U32 suffix_sei_data_size;

    /*
     * The start address of the total suffix SEI NALs to be encoded
     */
    RK_U32 suffix_sei_nal_addr;
} H265E_SEI;

/*
 * This is a data structure for setting VUI parameters in HEVC encoder.
 */
typedef struct {
    /*
     * VUI parameter flag
     */
    RK_U32 flags;

    /*
     * aspect_ratio_idc
     */
    RK_U32 aspect_ratio_idc;

    /*
     * sar_width, sar_height (only valid when aspect_ratio_idc is equal to 255)
     */
    RK_U32 sar_size;
    /*
     * overscan_appropriate_flag
     */
    RK_U32 over_scan_appropriate;

    /*
     * VUI parameter flag
     */
    RK_U32 signal;

    /*
     * chroma_sample_loc_type_top_field, chroma_sample_loc_type_bottom_field
     */
    RK_U32 chroma_sample_loc;

    /*
     * def_disp_win_left_offset, def_disp_win_right_offset
     */
    RK_U32 disp_win_left_right;

    /*
     * def_disp_win_top_offset, def_disp_win_bottom_offset
     */
    RK_U32 disp_win_top_bottom;
} H265e_VUI;

/*
 * This is a data structure for custom GOP parameters of the given picture.
 */
typedef struct {
    /*
     * picture type of #th picture in the custom GOP
     * the value define in H265E_PIC_TYPE(B frame not support)
     */
    RK_U32 type;

    /*
     * A POC offset of #th picture in the custom GOP
     */
    RK_U32 offset;

    /*
     * A quantization parameter of #th picture in the custom GOP
     */
    RK_U32 qp;

    /*
     * POC offset of reference L0 of #th picture in the custom GOP
     */
    RK_U32 ref_poc_l0;

    /*
     * POC offset of reference L1 of #th picture in the custom GOP
     * Not support B frame in H265 Encoder,so the value of ref_poc_l1
     * is always 0
     */
    RK_U32 ref_poc_l1;

    /*
     * A temporal ID of #th picture in the custom GOP
     */
    RK_U32 temporal_id;
} H265e_Custom_Gop_PIC;


/*
 * This is a data structure for custom GOP parameters.
 */
typedef struct {
    /*
     * size of the custome GOP (the value can be 0~8)
     */
    RK_U32 custom_gop_size;

    /*
     * It derives a lamda weight internally instead of using lamda weight specified.
     */
    RK_U32 use_derive_lambda_weight;

    /*
     * Picture parameters of #th picture in the custom GOP
     */
    H265e_Custom_Gop_PIC pic[H265E_MAX_GOP_NUM];

    /*
     * A lamda weight of #th picture in the custom GOP
     */
    RK_U32 gop_pic_lambda[H265E_MAX_GOP_NUM];
} H265e_Custom_Gop;

typedef struct {
    /*
     * Whether host encode a header implicitly or not.
     * If this value is 1, below encode options will be ignored
     */
    int implicitHeaderEncode;

    /*
     * a flag to encode VCL nal unit explicitly
     */
    int encodeVCL;

    /*
     * a flag to encode VPS nal unit explicitly
     */
    int encodeVPS;

    /*
     * a flag to encode SPS nal unit explicitly
     */
    int encodeSPS;

    /*
     * a flag to encode PPS nal unit explicitly
     */
    int encodePPS;

    /*
     * a flag to encode AUD nal unit explicitly
     */
    int encodeAUD;

    /*
     * a flag to encode EOS nal unit explicitly
     */
    int encodeEOS;

    /*
     * a flag to encode EOB nal unit explicitly
     */
    int encodeEOB;

    /*
     * a flag to encode VUI nal unit explicitly
     */
    int encodeVUI;
} EncCodeOpt;

typedef struct hal_h265e_cfg {
    /*
     * A profile indicator
     * 1 : main
     * 2 : main10
     */
    RK_U8 profile;

    /*
     * only support to level 4.1
     */
    RK_U8 level;

    /*
     * A tier indicator
     * 0 : main
     * 1 : high
     */
    RK_U8 tier;

    /*
     * A chroma format indecator, only support YUV420
     */
    RK_U8 chroma_idc;

    /*
     * the source's width and height
     */
    RK_U16 width;
    RK_U16 height;

    /*
     * the source's width stride and height stride
     */
    RK_U16 width_stride;
    RK_U16 height_stride;
    /*
     * bitdepth,only support 8 bits(only support 8 bits)
     */
    RK_U8 bit_depth;

    /*
     * source yuv's format. The value is defined in H265E_FrameBufferFormat(only support YUV420)
     * the value could be YU12,YV12,NV12,NV21
     */
    RK_U8 src_format;

    RK_U8 src_endian;
    RK_U8 bs_endian;
    RK_U8 fb_endian;
    RK_U8 frame_rate;
    RK_U8 frame_skip;
    RK_U32 bit_rate;

    RK_U32 fbc_map_type;
    RK_U32 line_buf_int_en;
    RK_U32 slice_int_enable;
    RK_U32 ring_buffer_enable;

    EncCodeOpt codeOption;

    /*
     * value 1:enables lossless coding
     */
    RK_S32 lossless_enable;

    /*
     * value 1:enables constrained intra prediction
     */
    RK_S32 const_intra_pred_flag;

    /*
     * The value of chroma(cb) qp offset
     */
    RK_S32 chroma_cb_qp_offset;

    /*
     * The value of chroma(cr) qp offset
     */
    RK_S32 chroma_cr_qp_offset;


    /*
     * A GOP structure option,not support B slice
     * 0: Custom GOP
     * 1 : I-I-I-I,..I (all intra, gop_size=1)
     * 2 : I-P-P-P,...P(consecutive P, gop_size=1)
     * 6 : I-P-P-P-P,(consecutive P, gop_size=4)
     */
    RK_S32 gop_idx;

    /*
     * An intra picture refresh mode
     * 0 : Non-IRAP
     * 1 : CRA
     * 2 : IDR
     */
    RK_S32 decoding_refresh_type;

    /*
     * A quantization parameter of intra picture
     */
    RK_S32 intra_qp;

    /*
     * A period of intra picture in GOP size
     */
    RK_S32 intra_period;

    /*
     * A conformance window size of TOP,BUTTOM,LEFT,RIGHT
     */
    RK_U16 conf_win_top;
    RK_U16 conf_win_bot;
    RK_U16 conf_win_left;
    RK_U16 conf_win_right;

    /*
     * A slice mode for independent slice
     * 0 : no multi-slice
     * 1 : Slice in CTU number
     * 2 : Slice in number of byte
     */
    RK_U32 independ_slice_mode;
    /*
     * The number of CTU or bytes for a slice when independ_slice_mode is set with 1 or 2.
     */
    RK_U32 independ_slice_mode_arg;
    /*
     * slice mode for dependent slice
     * 0 : no multi-slice
     * 1 : Slice in CTU number
     * 2 : Slice in number of byte
     */
    RK_U32 depend_slice_mode;

    /*
     * The number of CTU or bytes for a slice when depend_slice_mode is set with 1 or 2.
     */
    RK_U32 depend_slice_mode_arg;

    /*
     * An intra refresh mode
     * 0 : No intra refresh
     * 1 : Row
     * 2 : Column
     * 3 : Step size in CTU
     */
    RK_U32 intra_refresh_mode;

    /*
     * The number of CTU (only valid when intraRefreshMode is 3.)
     */
    RK_U32 intra_refresh_arg;

    /*
     * It uses one of the recommended encoder parameter presets.
     * 0 : Custom
     * 1 : Recommend enc params (slow encoding speed, highest picture quality)
     * 2 : Boost mode (normal encoding speed, normal picture quality)
     * 3 : Fast mode (high encoding speed, low picture quality)
     */
    RK_U8 use_recommend_param;

    /*
     * It enables a scaling list
     */
    RK_U8 scaling_list_enable;

    /*
     * It specifies CU size.
     * 3'b001: 8x8
     * 3'b010: 16x16
     * 3'b100 : 32x32
     */
    RK_U8 cu_size_mode;

    /*
     * It enables temporal motion vector prediction.
     */
    RK_U8 tmvp_enable;

    /*
     * It enables wave-front parallel processing.
     * This process is not support in H265 Encoder,so the value of it always 0
     */
    RK_U8 wpp_enable;

    /*
     * Maximum number of merge candidates (0~2)
     */
    RK_U8 max_num_merge;

    /*
     * It enables dynamic merge 8x8 candidates.
     */
    RK_U8 dynamic_merge_8x8_enable;

    /*
     * It enables dynamic merge 16x16 candidates.
     */
    RK_U8 dynamic_merge_16x16_enable;
    /*
     * It enables dynamic merge 32x32 candidates.
     */
    RK_U8 dynamic_merge_32x32_enable;

    /*
     * It disables in-loop deblocking filtering.
     */
    RK_U8 disable_deblk;

    /*
     * it enables filtering across slice boundaries for in-loop deblocking.
     */
    RK_U8 lf_cross_slice_boundary_enable;

    /*
     * BetaOffsetDiv2 for deblocking filter
     */
    RK_U8 beta_offset_div2;

    /*
     * TcOffsetDiv3 for deblocking filter
     */
    RK_U8 tc_offset_div2;

    /*
     * It enables transform skip for an intra CU.
     */
    RK_U8 skip_intra_trans;

    /*
     * It enables SAO (sample adaptive offset).
     */
    RK_U8 sao_enable;

    /*
     * It enables to make intra CUs in an inter slice.
     */
    RK_U8 intra_in_inter_slice_enable;

    /*
     * It enables intra NxN PUs.
     */
    RK_U8 intra_nxn_enable;

    /*
     * specifies intra QP offset relative to inter QP (Only available when rc_enable is enabled)
     */
    RK_S8 intra_qp_offset;

    /*
     * It specifies encoder initial delay,Only available when RateControl is enabled
     * (encoder initial delay = initial_delay * init_buf_levelx8 / 8)
     */
    int init_buf_levelx8;

    /*
     * specifies picture bits allocation mode.
     * Only available when RateControl is enabled and GOP size is larger than 1
     * 0: More referenced pictures have better quality than less referenced pictures
     * 1: All pictures in a GOP have similar image quality
     * 2: Each picture bits in a GOP is allocated according to FixedRatioN
     */
    RK_U8 bit_alloc_mode;

    /*
     * A fixed bit ratio (1 ~ 255) for each picture of GOP's bitallocation
     * N = 0 ~ (MAX_GOP_SIZE - 1)
     * MAX_GOP_SIZE = 8
     * For instance when MAX_GOP_SIZE is 3, FixedBitRaio0
     * to FixedBitRaio2 can be set as 2, 1, and 1 repsectively for
     * the fixed bit ratio 2:1:1. This is only valid when BitAllocMode is 2.
    */
    RK_U8 fixed_bit_ratio[H265E_MAX_GOP_NUM];

    /*
     * enable rate control
     */
    RK_U32 rc_enable;

    /*
     * enable CU level rate control
     */
    RK_U8 cu_level_rc_enable;

    /*
     * enable CU QP adjustment for subjective quality enhancement
     */
    RK_U8 hvs_qp_enable;

    /*
     * enable QP scaling factor for CU QP adjustment when hvs_qp_enable = 1
     */
    RK_U8 hvs_qp_scale_enable;

    /*
     * A QP scaling factor for CU QP adjustment when hvs_qp_enable = 1
     */
    RK_S8 hvs_qp_scale;

    /*
     * A minimum QP for rate control
     */
    RK_S8 min_qp;

    /*
     * A maximum QP for rate control
     */
    RK_S8 max_qp;

    /*
     * A maximum delta QP for rate control
     */
    RK_S8 max_delta_qp;

    /*
     * A peak transmission bitrate in bps
     */
    RK_U32 trans_rate;

    /*
     * It specifies the number of time units of a clock
     * operating at the frequency time_scale Hz
     */
    RK_U32 num_units_in_tick;

    /*
     * It specifies the number of time units that pass in one second
     */
    RK_U32 time_scale;

    /*
     * It specifies the number of clock ticks corresponding to a
     * difference of picture order count values equal to 1
     */
    RK_U32 num_ticks_poc_diff_one;

    /*
     * The value of initial QP by host.
     * This value is meaningless if INITIAL_RC_QP == 63
     */
    int initial_rc_qp;

    /*
     * enables noise reduction algorithm to Y/Cb/Cr component.
     */
    RK_U8 nr_y_enable;
    RK_U8 nr_cb_enable;
    RK_U8 nr_cr_enable;

    /*
     * enables noise estimation for reduction.
     * When this is disabled, noise estimation is carried out ouside VPU.
     */
    RK_U8 nr_noise_est_enable;
    /*
     * It specifies Y/Cb/Cr noise standard deviation if no use of noise estimation (nr_noise_est_enable=0)
     */
    RK_U8 nr_noise_sigma_y;
    RK_U8 nr_noise_sigma_cb;
    RK_U8 nr_noise_sigma_cr;

    /* A weight to Y noise level for intra picture (0 ~ 31).
     * nr_intra_weight_y/4 is multiplied to the noise level that has been estimated.
     * This weight is put for intra frame to be filtered more strongly or more weakly than just with the estimated noise level.
     */
    RK_U8 nr_intra_weight_y;

    /*
     * A weight to Cb noise level for intra picture (0 ~ 31).
     */
    RK_U8 nr_intra_weight_cb;

    /*
     * A weight to Cr noise level for intra picture (0 ~ 31).
     */
    RK_U8 nr_intra_weight_cr;

    /* A weight to Y noise level for inter picture (0 ~ 31).
     * nr_inter_weight_y/4 is multiplied to the noise level that has been estimated.
     * This weight is put for inter frame to be filtered more strongly or more weakly than just with the estimated noise level.
     */
    RK_U8 nr_inter_weight_y;

    /*
     * A weight to Cb noise level for inter picture (0 ~ 31).
     */
    RK_U8 nr_inter_weight_cb;

    /*
     * A weight to Cr noise level for inter picture (0 ~ 31).
     */
    RK_U8 nr_inter_weight_cr;
    /*
     * a minimum QP for intra picture (0 ~ 51).
     * It is only available when rc_enable is 1.
     */
    RK_U8 intra_min_qp;

    /*
     * a maximum QP for intra picture (0 ~ 51).
     * It is only available when rc_enable is 1.
     */
    RK_U8 intra_max_qp;

    RK_U32 initial_delay;

    RK_U8 hrd_rbsp_in_vps;
    RK_U8 hrd_rbsp_in_vui;
    RK_U32 vui_rbsp;

    /*
     * The size of the HRD rbsp data
     */
    RK_U32 hrd_rbsp_data_size;

    /*
     * The address of the HRD rbsp data
     */
    RK_U32 hrd_rbsp_data_addr;

    /*
     * The size of the VUI rbsp data
     */
    RK_U32 vui_rbsp_data_size;

    /*
     * The address of the VUI rbsp data
     */
    RK_U32 vui_rbsp_data_addr;

    RK_U8 use_long_term;
    RK_U8 use_cur_as_longterm_pic;
    RK_U8 use_longterm_ref;

    H265e_Custom_Gop gop;
    H265e_CTU ctu;
    H265e_VUI vui;
    H265E_SEI sei;

    /*
     * define which type of parameters are changed,
     * only support common paramter chanegd now,see H265eCommonCfgMask
     */
    RK_U32 cfg_option;

    /*
     * define which parameters are changed,see H265E_SET_PARAM_OPTION
     */
    RK_U32 cfg_mask;
} HalH265eCfg;


typedef struct {
    /*
     * the address of source(yuv) data for encoding
     */
    RK_U32 src_addr;

    /*
     * the size of source(yuv) data for encoding
     */
    RK_U32 src_size;

    /*
     * the address of bitstream buffer
     */
    RK_U32 bs_addr;

    /*
     * the size of bitstream buffer
     */
    RK_U32 bs_size;

    /*
     * the fd of roi map buffer
     */
    RK_U32 roi_fd;

    /*
     * the fd of ctu qp buffer
     */
    RK_U32 ctu_qp_fd;

    /*
     * the flag that stream is end
     */
    RK_U32 stream_end;

    /*
     * skip current frame
     */
    RK_U32 skip_pic;

    /*
     * A flag to use a force picture quantization parameter
     */
    RK_U32 force_qp_enable;

    /*
     * Force picture quantization parameter for I picture
     */
    RK_U32 force_qp_I;

    /*
     Force picture quantization parameter for P picture
     */
    RK_U32 force_qp_P;

    /*
     * A flag to use a force picture type
     */
    RK_U32 force_frame_type_enable;

    /*
     * A force picture type (I, P, IDR, CRA)
     */
    RK_U32 force_frame_type;
} EncInfo;

typedef struct hal_h265e_header {
    RK_U32         buf;
    RK_U32         size;
} H265E_STREAM_HEAD;

typedef struct hal_h265e_vepu22_result {
    RK_U32 bs_size;
    RK_U32 enc_pic_cnt;
    RK_U32 pic_type;
    RK_U32 num_of_slice;
    RK_U32 pick_skipped;
    RK_U32 num_intra;
    RK_U32 num_merge;
    RK_U32 num_skip_block;
    RK_U32 avg_ctu_qp;
    RK_S32 recon_frame_index;
    RK_U32 gop_idx;
    RK_U32 poc;
    RK_U32 src_idx;
    RK_U32 fail_reason;
} H265eVepu22Result;

#endif
