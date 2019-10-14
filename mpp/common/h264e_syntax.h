#ifndef __H264E_SYNTAX_H__
#define __H264E_SYNTAX_H__

#include "rk_type.h"
#include "h264_syntax.h"
#include "mpp_rc.h"

#define H264_BIT_DEPTH      8
#define H264_QP_BD_OFFSET   (6*(H264_BIT_DEPTH-8))

typedef struct  h264e_osd_pos_t {
    RK_U32    lt_pos_x : 8;
    RK_U32    lt_pos_y : 8;
    RK_U32    rd_pos_x : 8;
    RK_U32    rd_pos_y : 8;
} h264e_osd_pos; //OSD_POS0-7

typedef enum H264eHwType_t {
    H264E_RKV,
    H264E_VPU
} H264eHwType;

#define CTRL_LEVELS          7  /* DO NOT CHANGE THIS */
#define CHECK_POINTS_MAX    10  /* DO NOT CHANGE THIS */
#define RC_TABLE_LENGTH     10  /* DO NOT CHANGE THIS */

typedef struct VepuQpCtrl_s {
    RK_S32 wordCntPrev[CHECK_POINTS_MAX];  /* Real bit count */
    RK_S32 checkPointDistance;
    RK_S32 checkPoints;
    RK_S32 nonZeroCnt;
    RK_S32 frameBitCnt;
} VepuQpCtrl;

/*
 * Overall configuration required by hardware
 * Currently support vepu and rkvenc
 */
typedef struct H264eHwCfg_t {
    H264eHwType hw_type;

    /* Input data parameter */
    RK_S32 width;
    RK_S32 height;
    RK_S32 hor_stride;
    RK_S32 ver_stride;
    RK_S32 input_format; /* Hardware config format */
    RK_S32 r_mask_msb;
    RK_S32 g_mask_msb;
    RK_S32 b_mask_msb;
    RK_S32 uv_rb_swap; /* u/v or R/B planar bit swap flag */
    RK_S32 alpha_swap; /* alpha/RGB bit swap flag */
    RK_U32 color_conversion_coeff_a;
    RK_U32 color_conversion_coeff_b;
    RK_U32 color_conversion_coeff_c;
    RK_U32 color_conversion_coeff_e;
    RK_U32 color_conversion_coeff_f;

    /* Buffer parameter */
    RK_U32 input_luma_addr;
    RK_U32 input_cb_addr;
    RK_U32 input_cr_addr;
    RK_U32 output_strm_limit_size;
    RK_U32 output_strm_addr;
    /*
     * For vpu
     * 0 - inter
     * 1 - intra
     * 2 - mvc-inter*/

    VepuQpCtrl qpCtrl;
    /*
    * For rkvenc
    * RKVENC_FRAME_TYPE_*
    */
    RK_S32 frame_type;
    RK_S32 pre_frame_type;

    RK_S32 cabac_init_idc;
    RK_S32 frame_num;
    RK_S32 pic_order_cnt_lsb;
    RK_S32 filter_disable;
    RK_S32 idr_pic_id;
    RK_S32 slice_alpha_offset;
    RK_S32 slice_beta_offset;
    RK_S32 inter4x4_disabled;

    /* rate control relevant */
    RK_S32 qp_prev;
    RK_S32 qp;
    RK_S32 qp_min;
    RK_S32 qp_max;
    RK_S32 mad_qp_delta;
    RK_S32 mad_threshold;
    RK_S32 pre_bit_diff;

    /*
     * VEPU MB rate control parameter
     * VEPU MB can have max 10 check points.
     * On each check point hardware will check the target bit and
     * error bits and change qp according to delta qp step
     */
    RK_S32 slice_size_mb_rows;
    RK_S32 cp_distance_mbs;
    RK_S32 cp_target[CHECK_POINTS_MAX];
    RK_S32 target_error[9]; //for rkv there are 9 levels
    RK_S32 delta_qp[9];

    /* RKVENC extra syntax below */
    RK_S32 link_table_en;
    RK_S32 keyframe_max_interval;

    RK_S32 roi_en;
    RK_S32 osd_mode; //0: disable osd, 1:palette type 0(congfigurable mode), 2:palette type 1(fixed mode).
    RK_S32 preproc_en;
    RK_S32 coding_type;      /* SliceType: H264_P_SLICE = 0 H264_I_SLICE = 2 */
} H264eHwCfg;

typedef struct h264e_feedback_t {
    /* rc */
    RcHalResult *result;
    RK_U32 hw_status; /* 0:corret, 1:error */
    RK_S32 qp_sum;
    RK_S32 cp[10];
    RK_S32 mad_count;
    RK_S32 rlc_count;
    RK_U32 out_strm_size;
    RK_U32 out_hw_strm_size;
    RK_S64 sse_sum;

    /* for VEPU future extansion */
    //TODO: add nal size table feedback
} h264e_feedback;

typedef struct h264e_control_extra_info_cfg_t {
    /* common cfg */
    RK_U32 pic_luma_width;
    RK_U32 pic_luma_height;
    RK_S32 enable_cabac;
    RK_S32 transform8x8_mode;
    RK_S32 chroma_qp_index_offset;
    RK_S32 pic_init_qp;
    RK_S32 frame_rate;
    //RK_S32 rotation_enable; //0: 0/180 degrees, 1: 90/270 degrees //TODO: add rotation

    /* additional cfg only for rkv */
    RK_U32 input_image_format;
    RK_S32 profile_idc;
    RK_S32 level_idc; //TODO: may be removed later, get from sps/pps instead
    RK_S32 keyframe_max_interval;
    RK_S32 second_chroma_qp_index_offset;
    RK_S32 pps_id; //TODO: may be removed later, get from sps/pps instead
} h264e_control_extra_info_cfg;

typedef struct h264e_control_extra_info_t {
    RK_U32 size;              // preProcess->lumWidth
    RK_U8  buf[256];             // preProcess->
} h264e_control_extra_info;

typedef struct SynH264eVui_t {
    RK_U32      vui_present;

    RK_S32      aspect_ratio_info_present;
    RK_S32      aspect_ratio_idc;
    RK_S32      sar_width;
    RK_S32      sar_height;

    RK_S32      overscan_info_present;
    RK_S32      overscan_appropriate_flag;

    RK_S32      signal_type_present;
    RK_S32      vidformat;
    RK_S32      fullrange;
    RK_S32      color_description_present;
    RK_S32      colorprim;
    RK_S32      transfer;
    RK_S32      colmatrix;

    RK_S32      chroma_loc_info_present;
    RK_S32      chroma_loc_top;
    RK_S32      chroma_loc_bottom;

    RK_S32      timing_info_present;
    RK_U32      num_units_in_tick;
    RK_U32      time_scale;
    RK_S32      fixed_frame_rate;

    RK_S32      nal_hrd_parameters_present;
    RK_S32      vcl_hrd_parameters_present;

    struct {
        RK_S32  cpb_cnt;
        RK_S32  bit_rate_scale;
        RK_S32  cpb_size_scale;
        RK_S32  bit_rate_value;
        RK_S32  cpb_size_value;
        RK_S32  bit_rate_unscaled;
        RK_S32  cpb_size_unscaled;
        RK_S32  cbr_hrd;

        RK_S32  initial_cpb_removal_delay_length;
        RK_S32  cpb_removal_delay_length;
        RK_S32  dpb_output_delay_length;
        RK_S32  time_offset_length;
    } hrd;

    RK_S32      pic_struct_present;
    RK_S32      bitstream_restriction;
    RK_S32      motion_vectors_over_pic_boundaries;
    RK_S32      max_bytes_per_pic_denom;
    RK_S32      max_bits_per_mb_denom;
    RK_S32      log2_max_mv_length_horizontal;
    RK_S32      log2_max_mv_length_vertical;
    RK_S32      num_reorder_frames;
    RK_S32      max_dec_frame_buffering;
} SynH264eVui;

typedef struct SynH264eSps_t {
    RK_S32      profile_idc;

    RK_S32      constraint_set0;
    RK_S32      constraint_set1;
    RK_S32      constraint_set2;
    RK_S32      constraint_set3;
    RK_S32      constraint_set4;
    RK_S32      constraint_set5;

    RK_S32      level_idc;
    RK_S32      sps_id;

    RK_S32      chroma_format_idc;
    /* scaling list is in pps */

    RK_S32      log2_max_frame_num_minus4;
    RK_S32      pic_order_cnt_type;
    /* poc type 0 */
    RK_S32      log2_max_poc_lsb_minus4;

    RK_S32      num_ref_frames;
    RK_S32      gaps_in_frame_num_value_allowed;
    RK_S32      pic_width_in_mbs;
    RK_S32      pic_height_in_mbs;

    // alway frame encoding
    RK_S32      frame_mbs_only;
    RK_S32      direct8x8_inference;

    RK_S32      cropping;
    struct {
        RK_S32  left;
        RK_S32  right;
        RK_S32  top;
        RK_S32  bottom;
    } crop;

    SynH264eVui    vui;
} SynH264eSps;

typedef struct SynH264ePps_t {
    RK_S32      pps_id;
    RK_S32      sps_id;

    // 0 - CAVLC 1 - CABAC
    RK_S32      entropy_coding_mode;

    RK_S32      bottom_field_pic_order_in_frame_present;
    RK_S32      num_slice_groups;

    RK_S32      num_ref_idx_l0_default_active;
    RK_S32      num_ref_idx_l1_default_active;

    RK_S32      weighted_pred;
    RK_S32      weighted_bipred_idc;

    RK_S32      pic_init_qp;
    RK_S32      pic_init_qs;

    RK_S32      chroma_qp_index_offset;
    RK_S32      second_chroma_qp_index_offset;

    RK_S32      deblocking_filter_control;
    RK_S32      constrained_intra_pred;
    RK_S32      redundant_pic_cnt;

    RK_S32      transform_8x8_mode;

    // Only support flat and default scaling list
    RK_S32      pic_scaling_list_present;
} SynH264ePps;

#endif
