#ifndef __H264E_SYNTAX_H__
#define __H264E_SYNTAX_H__

#include "rk_type.h"
#include "h264_syntax.h"

#define H264_BIT_DEPTH      8
#define H264_QP_BD_OFFSET   (6*(H264_BIT_DEPTH-8))

typedef struct  h264e_osd_pos_t {
    RK_U32    lt_pos_x : 8;
    RK_U32    lt_pos_y : 8;
    RK_U32    rd_pos_x : 8;
    RK_U32    rd_pos_y : 8;
} h264e_osd_pos; //OSD_POS0-7

typedef struct h264e_syntax_t {
    RK_U32 frame_coding_type; //(VEPU)0: inter, 1: intra  (RKVENC)RKVENC_FRAME_TYPE_*
    RK_U32 slice_type;
    RK_S32 pic_init_qp; //COMB, TODO: merge with swreg59.pic_init_qp
    RK_S32 slice_alpha_offset; //COMB TODO: merge with swreg62.sli_beta_ofst
    RK_S32 slice_beta_offset; //COMB TODO: merge with swreg62.sli_alph_ofst
    RK_S32 chroma_qp_index_offset; //COMB
    RK_S32 second_chroma_qp_index_offset; //TODO: may be removed later, get from sps/pps instead
    RK_S32 deblocking_filter_control; //COMB
    RK_S32 disable_deblocking_filter_idc;
    RK_S32 filter_disable; //TODO: merge width disable_deblocking_filter_idc above
    RK_U16 idr_pic_id; //COMB
    RK_S32 pps_id; //COMB TODO: merge with swreg60.pps_id //TODO: may be removed later, get from sps/pps instead
    RK_S32 frame_num; //COMB TODO: swreg60.frm_num
    RK_S32 slice_size_mb_rows;
    RK_S32 h264_inter4x4_disabled; //COMB
    RK_S32 enable_cabac; //COMB
    RK_S32 transform8x8_mode; //COMB
    RK_S32 cabac_init_idc; //COMB TODO: merge with swreg60.cbc_init_idc
    RK_S32 pic_order_cnt_lsb;

    /* rate control relevant */
    RK_S32 qp;
    RK_S32 mad_qp_delta;
    RK_S32 mad_threshold;
    RK_S32 qp_min; //COMB, TODO: merge width swreg54.rc_min_qp
    RK_S32 qp_max; //COMB, TODO: merge width swreg54.rc_max_qp
    RK_S32 cp_distance_mbs;
    RK_S32 cp_target[10];
    RK_S32 target_error[7];
    RK_S32 delta_qp[7];


    RK_U32 output_strm_limit_size;      // outputStrmSize
    RK_U32 output_strm_addr;            // outputStrmBase
    RK_U32 pic_luma_width;              // preProcess->lumWidth
    RK_U32 pic_luma_height;             // preProcess->lumHeight
    RK_U32 input_luma_addr;             // inputLumBase
    RK_U32 input_cb_addr;               // inputCbBase
    RK_U32 input_cr_addr;               // inputCrBase
    RK_U32 input_image_format;          // inputImageFormat
    RK_U32 color_conversion_coeff_a;    //colorConversionCoeffA
    RK_U32 color_conversion_coeff_b;    //colorConversionCoeffB
    RK_U32 color_conversion_coeff_c;    //colorConversionCoeffC
    RK_U32 color_conversion_coeff_e;    //colorConversionCoeffE
    RK_U32 color_conversion_coeff_f;    //colorConversionCoeffF
    RK_U32 color_conversion_r_mask_msb; //rMaskMsb
    RK_U32 color_conversion_g_mask_msb; //gMaskMsb
    RK_U32 color_conversion_b_mask_msb; //bMaskMsb

    /* RKVENC extra syntax below */
    RK_S32 profile_idc; //TODO: may be removed later, get from sps/pps instead
    RK_S32 level_idc; //TODO: may be removed later, get from sps/pps instead
    RK_S32 link_table_en;
    RK_S32 keyframe_max_interval;

    RK_S32 mb_rc_mode; //0:frame/slice 1:mb; //0: disable mbrc, 1:slice/frame rc, 2:MB rc.
    RK_S32 roi_en;
    RK_S32 osd_mode; //0: disable osd, 1:palette type 0(congfigurable mode), 2:palette type 1(fixed mode).
    RK_S32 preproc_en;
} h264e_syntax;

typedef struct h264e_feedback_t {
    /* rc */
    RK_U32 hw_status; /* 0:corret, 1:error */
    RK_S32 qp_sum;
    RK_S32 cp[10];
    RK_S32 mad_count;
    RK_S32 rlc_count;
    RK_U32 out_strm_size;

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
