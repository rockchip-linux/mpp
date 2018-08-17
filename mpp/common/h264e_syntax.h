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
    RK_S32 coding_type;      /* SliceType: P_SLICE = 0 I_SLICE = 2 */
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

#endif
