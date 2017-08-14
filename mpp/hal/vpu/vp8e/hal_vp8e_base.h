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

#ifndef __HAL_VP8E_BASE_H__
#define __HAL_VP8E_BASE_H__

#include "rk_type.h"

#include "mpp_device.h"
#include "mpp_hal.h"
#include "mpp_mem.h"

#include "vpu.h"

#include "hal_task.h"

#include "vp8e_syntax.h"

#include "hal_vp8e_entropy.h"
#include "hal_vp8e_putbit.h"

#define VP8_PROB_COUNT_MV_OFFSET     (222)
#define VP8_PROB_COUNT_BUF_SIZE      (244*2)

#define mask_7b         (RK_U32)0x0000007F

#define INPUT_YUV420PLANAR         0x00
#define INPUT_YUV420SEMIPLANAR     0x01
#define INPUT_YUYV422INTERLEAVED   0x02
#define INPUT_UYVY422INTERLEAVED   0x03
#define INPUT_RGB565               0x04
#define INPUT_RGB555               0x05
#define INPUT_RGB444               0x06
#define INPUT_RGB888               0x07
#define INPUT_RGB101010            0x08
#define INPUT_YUYV422TILED         0x09

#define IVF_HDR_BYTES       32
#define IVF_FRM_BYTES       12

typedef enum vp8_frm_type_e {
    VP8E_FRM_KEY = 0,
    VP8E_FRM_P,
} Vp8FrmType;

typedef struct vp8e_hal_vpu_buffers_t {
    MppBufferGroup  hw_buf_grp;

    MppBuffer   hw_rec_buf[2];
    MppBuffer   hw_luma_buf;
    MppBuffer   hw_cbcr_buf[2];
    MppBuffer   hw_cabac_table_buf;
    MppBuffer   hw_size_table_buf;
    MppBuffer   hw_segment_map_buf;
    MppBuffer   hw_prob_count_buf;
    MppBuffer   hw_mv_output_buf;
    MppBuffer   hw_out_buf;
} Vp8eVpuBuf;

#define PENALTY_TABLE_SIZE 128

/**
 * pps definition
 */

#define SGM_CNT 4

typedef struct sgm_t {
    RK_U8   map_modified;
    RK_S32  id_cnt[SGM_CNT];
} Sgm;

typedef struct {
    Sgm     sgm;
    RK_S32  qp;
    RK_U8   segment_enabled;
    RK_S32  qp_sgm[SGM_CNT];
    RK_S32  level_sgm[SGM_CNT];
} Pps;

typedef struct {
    Pps    *store;
    RK_S32 size;
    Pps    *pps;
    Pps    *prev_pps;
    RK_S32 qp_sgm[SGM_CNT];
    RK_S32 level_sgm[SGM_CNT];
} Vp8ePps;


/**
 * QP definition
 */

#define QINDEX_RANGE    128

typedef struct {
    RK_S32 quant[2];
    RK_S32 zbin[2];
    RK_S32 round[2];
    RK_S32 dequant[2];
} Vp8eQp;

/**
 * sps definition
 */

typedef struct vp8e_sps_t {
    RK_S32 pic_width_in_mbs;
    RK_S32 pic_height_in_mbs;
    RK_S32 pic_width_in_pixel;
    RK_S32 pic_height_in_pixel;
    RK_S32 horizontal_scaling;
    RK_S32 vertical_scaling;
    RK_S32 color_type;
    RK_S32 clamp_type;
    RK_S32 dct_partitions;
    RK_S32 partition_cnt;
    RK_S32 profile;
    RK_S32 filter_type;
    RK_S32 filter_level;
    RK_S32 filter_sharpness;
    RK_S32 quarter_pixel_mv;
    RK_S32 split_mv;
    RK_S32 sing_bias[3];

    RK_S32 auto_filter_level;
    RK_S32 auto_filter_sharpness;
    RK_U8  filter_delta_enable;
    RK_S32 mode_delta[4];
    RK_S32 old_mode_delta[4];
    RK_S32 ref_delta[4];
    RK_S32 old_ref_delta[4];
    RK_S32 refresh_entropy;
} Vp8eSps;

/**
 * entropy definition
 */

typedef struct vp8e_hal_entropy_t {
    RK_S32 skip_false_prob;
    RK_S32 intra_prob;
    RK_S32 last_prob;
    RK_S32 gf_prob;
    RK_S32 kf_y_mode_prob[4];
    RK_S32 y_mode_prob[4];
    RK_S32 kf_uv_mode_prob[3];
    RK_S32 uv_mode_prob[3];
    RK_S32 kf_b_mode_prob[10][10][9];
    RK_S32 b_mode_prob[9];
    RK_S32 coeff_prob[4][8][3][11];
    RK_S32 old_coeff_prob[4][8][3][11];
    RK_S32 mv_ref_prob[4];
    RK_S32 mv_prob[2][19];
    RK_S32 old_mv_prob[2][19];
    RK_S32 sub_mv_partprob[3];
    RK_S32 sub_mv_ref_prob[5][3];
    RK_S32 default_coeff_prob_flag;
    RK_S32 update_coeff_prob_flag;
    RK_S32 segment_prob[3];
} Vp8eHalEntropy;

/**
 * picture definition
 */

#define REF_FRAME_COUNT 3

typedef struct {
    RK_S32 lum_width;
    RK_S32 lum_height;
    RK_S32 ch_width;
    RK_S32 ch_height;
    RK_U32 lum;
    RK_U32 cb;
} HalVp8ePic;

typedef struct hal_vp8e_refpic_t {
    HalVp8ePic picture;
    Vp8eHalEntropy *entropy;
    RK_S32 poc;

    RK_U8 i_frame;
    RK_U8 p_frame;
    RK_U8 show;
    RK_U8 ipf;
    RK_U8 arf;
    RK_U8 grf;
    RK_U8 search;
    struct hal_vp8e_refpic_t *refPic;
} HalVp8eRefPic;

typedef struct {
    RK_S32 size;       /* Amount of allocated reference pictures */
    HalVp8ePic    input;      /* Input picture */
    HalVp8eRefPic ref_pic[REF_FRAME_COUNT + 1]; /* Reference picture store */
    HalVp8eRefPic ref_pic_list[REF_FRAME_COUNT]; /* Reference picture list */
    HalVp8eRefPic *cur_pic;    /* Pointer to picture under reconstruction */
    HalVp8eRefPic *last_pic;   /* Last picture */
} HalVp8ePicBuf;

typedef struct {
    RK_U32 irq_disable;
    RK_U32 mbs_in_col;
    RK_U32 mbs_in_row;
    RK_U32 rounding_ctrl;
    RK_U32 frame_coding_type;
    RK_U32 coding_type;
    RK_U32 pixels_on_row;
    RK_U32 x_fill;
    RK_U32 y_fill;
    RK_U32 filter_disable;
    RK_U32 enable_cabac;
    RK_U32 input_format;
    RK_U32 input_rotation;
    RK_U32 output_strm_base;
    RK_U32 output_strm_size;
    RK_U32 first_free_bit;
    RK_U32 strm_start_msb;
    RK_U32 strm_start_lsb;
    RK_U32 size_tbl_base;
    RK_U32 int_slice_ready;
    RK_U32 rec_write_disable;
    RK_U32 recon_img_id;
    RK_U32 internal_img_lum_base_w;
    RK_U32 internal_img_chr_base_w;
    RK_U32 internal_img_lum_base_r[2];
    RK_U32 internal_img_chr_base_r[2];
    RK_U32 input_lum_base;
    RK_U32 input_cb_base;
    RK_U32 input_cr_base;
    RK_U32 cp_distance_mbs; //TODO maybe useless
    RK_U32 rlc_count;    //TODO read from reg
    RK_U32 qp_sum;   //TODO read from reg
    RK_U8  dmv_penalty[PENALTY_TABLE_SIZE];
    RK_U8  dmv_qpel_penalty[PENALTY_TABLE_SIZE];
    RK_U32 input_luma_base_offset;
    RK_U32 input_chroma_base_offset;
    RK_U32 disable_qp_mv;
    RK_U32 vs_next_luma_base;
    RK_U32 vs_mode;
    RK_U32 asic_cfg_reg;
    RK_S32 intra_16_favor;
    RK_S32 prev_mode_favor;
    RK_S32 inter_favor;
    RK_S32 skip_penalty;
    RK_S32 golden_penalty;
    RK_S32 diff_mv_penalty[3];
    RK_U32 mad_count;
    RK_U32 cir_start;
    RK_U32 cir_interval;
    RK_U32 intra_area_top;
    RK_U32 intra_area_left;
    RK_U32 intra_area_bottom;
    RK_U32 intra_area_right;
    RK_U32 roi1_top;
    RK_U32 roi1_left;
    RK_U32 roi1_bottom;
    RK_U32 roi1_right;
    RK_U32 roi2_top;
    RK_U32 roi2_left;
    RK_U32 roi2_bottom;
    RK_U32 roi2_right;
    RK_S32 roi1_delta_qp;
    RK_S32 roi2_delta_qp;
    RK_U32 mv_output_base;
    RK_U32 cabac_tbl_base;
    RK_U32 prob_count_base;
    RK_U32 segment_map_base;
    RK_U16 rgb_coeff_a;
    RK_U16 rgb_coeff_b;
    RK_U16 rgb_coeff_c;
    RK_U16 rgb_coeff_e;
    RK_U16 rgb_coeff_f;
    RK_U8  r_mask_msb;
    RK_U8  g_mask_msb;
    RK_U8  b_mask_msb;
    RK_U32 partition_Base[8];
    RK_U16 y1_quant_dc[4];
    RK_U16 y1_quant_ac[4];
    RK_U16 y2_quant_dc[4];
    RK_U16 y2_quant_ac[4];
    RK_U16 ch_quant_dc[4];
    RK_U16 ch_quant_ac[4];
    RK_U16 y1_zbin_dc[4];
    RK_U16 y1_zbin_ac[4];
    RK_U16 y2_zbin_dc[4];
    RK_U16 y2_zbin_ac[4];
    RK_U16 ch_zbin_dc[4];
    RK_U16 ch_zbin_ac[4];
    RK_U8  y1_round_dc[4];
    RK_U8  y1_round_ac[4];
    RK_U8  y2_round_dc[4];
    RK_U8  y2_round_ac[4];
    RK_U8  ch_round_dc[4];
    RK_U8  ch_round_ac[4];
    RK_U16 y1_dequant_dc[4];
    RK_U16 y1_dequant_ac[4];
    RK_U16 y2_dequant_dc[4];
    RK_U16 y2_dequant_ac[4];
    RK_U16 ch_dequant_dc[4];
    RK_U16 ch_dequant_ac[4];
    RK_U32 segment_enable;
    RK_U32 segment_map_update;
    RK_U32 mv_ref_idx[2];
    RK_U32 ref2_enable;
    RK_U32 bool_enc_value;
    RK_U32 bool_enc_value_bits;
    RK_U32 bool_enc_range;
    RK_U32 dct_partitions;
    RK_U32 filter_level[4];
    RK_U32 filter_sharpness;
    RK_U32 intra_mode_penalty[4];
    RK_U32 intra_b_mode_penalty[10];
    RK_U32 zero_mv_favor;
    RK_U32 split_mv_mode;
    RK_U32 split_penalty[4];
    RK_S32 lf_ref_delta[4];
    RK_S32 lf_mode_delta[4];
} Vp8eHwCfg;

typedef struct hal_vp8e_ctx_s {
    IOInterruptCB    int_cb;
    Vp8eFeedback     feedback;
    MppDevCtx        dev_ctx;

    void             *regs;
    void             *buffers;
    RK_U32           reg_size;

    MppEncCfgSet     *cfg;
    MppEncCfgSet     *set;

    MppHalApi        hal_api;

    Vp8eSps          sps;
    Vp8ePps          ppss;
    //TODO remove vp8_rc
    Vp8eRc           *rc;
    Vp8eHwCfg        hw_cfg;
    Vp8eHalEntropy   entropy;
    HalVp8ePicBuf    picbuf;
    Vp8ePutBitBuf    bitbuf[4];
    Vp8eQp           qp_y1[QINDEX_RANGE];
    Vp8eQp           qp_y2[QINDEX_RANGE];
    Vp8eQp           qp_ch[QINDEX_RANGE];
    RK_U32           prev_frame_lost;
    RK_U32           *p_out_buf[9];
    RK_U32           stream_size[9];
    RK_U32           frame_size;

    RK_U32           buffer_ready;
    RK_U64           frame_cnt;
    RK_U8            key_frm_next;
    RK_U32           gop_len;
    RK_U32           bit_rate;
    Vp8FrmType       frame_type;

    RK_U32           mb_per_frame;
    RK_U32           mb_per_row;
    RK_U32           mb_per_col;
} HalVp8eCtx;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET hal_vp8e_update_buffers(void *hal, HalTaskInfo *task);
MPP_RET hal_vp8e_enc_strm_code(void *hal, HalTaskInfo *info);
MPP_RET hal_vp8e_init_qp_table(void *hal);
MPP_RET hal_vp8e_setup(void *hal);
MPP_RET hal_vp8e_buf_free(void *hal);
#ifdef  __cplusplus
}
#endif

#endif /*__HAL_VP*E_BASE_H__*/
