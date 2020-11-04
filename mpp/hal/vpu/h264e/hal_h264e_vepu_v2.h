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

#ifndef __HAL_H264E_VEPU_V2_H__
#define __HAL_H264E_VEPU_V2_H__

#include "mpp_enc_cfg.h"
#include "mpp_rc.h"
#include "vepu_common.h"

#define H264E_HAL_SET_REG(reg, addr, val)                                    \
    do {                                                                     \
        reg[(addr)>>2] = (RK_U32)(val);                                      \
        if (hal_h264e_debug & 0/*H264E_HAL_LOG_INFO*/)                              \
            mpp_log("line(%d) set reg[%03d/%03x]: %08x", __LINE__, (addr)>>2, addr, val); \
    } while (0)

typedef enum H264eVpuFrameType_t {
    H264E_VPU_FRAME_P = 0,
    H264E_VPU_FRAME_I = 1
} H264eVpuFrameType;

#define VEPU_CTRL_LEVELS            7
#define VEPU_CHECK_POINTS_MAX       10

typedef struct HalH264eVepuInput_t {
    /* Hardware config format */
    RK_S32          src_fmt;
    RK_S32          src_w;
    RK_S32          src_h;

    VepuStrideCfg   stride_cfg;
    RK_S32          pixel_stride;

    size_t          size_y;
    size_t          size_c;

    RK_U32          offset_cb;
    RK_U32          offset_cr;

    RK_U8           r_mask_msb;
    RK_U8           g_mask_msb;
    RK_U8           b_mask_msb;
    RK_U8           swap_8_in;
    RK_U8           swap_16_in;
    RK_U8           swap_32_in;

    RK_U32          color_conversion_coeff_a;
    RK_U32          color_conversion_coeff_b;
    RK_U32          color_conversion_coeff_c;
    RK_U32          color_conversion_coeff_e;
    RK_U32          color_conversion_coeff_f;
} HalH264eVepuPrep;

typedef struct HalH264eVepuFrmAddr_t {
    // original frame Y/Cb/Cr/RGB address
    RK_U32          orig[3];
    // reconstruction frame
    RK_U32          recn[2];
    RK_U32          refr[2];
} HalH264eVepuAddr;

/*
 * Vepu buffer allocater
 * There are three internal buffer for Vepu encoder:
 * 1. cabac table input buffer
 * 2. nal size table output buffer
 * 3. recon / refer frame buffer
 */
typedef struct HalH264eVepuBufs_t {
    MppBufferGroup  group;

    /* cabac table buffer */
    RK_S32          cabac_init_idc;
    MppBuffer       cabac_table;

    /*
     * nal size table buffer
     * table size must be 64-bit multiple, space for zero at the end of table
     * Atleast 1 macroblock row in every slice
     */
    RK_S32          mb_h;
    RK_S32          nal_tab_size;
    MppBuffer       nal_size_table;

    /*
     * recon / refer frame buffer
     * sync with encoder using slot index
     */
    size_t          frm_size;
    size_t          yuv_size;
    RK_S32          frm_cnt;
    MppBuffer       frm_buf[H264E_MAX_REFS_CNT + 1];
} HalH264eVepuBufs;

typedef struct HalH264eVepuMbRc_t {
    /* VEPU MB rate control parameter for config to hardware */
    RK_S32          qp_init;
    RK_S32          qp_min;
    RK_S32          qp_max;

    /*
     * VEPU MB can have max 10 check points (cp).
     *
     * On each check point hardware will check the target bit and
     * error bits and change qp according to delta qp step
     *
     * cp_distance_mbs  check point distance in mbs (0 = disabled)
     * cp_target        bitrate target at each check point
     * cp_error         error bit level step for each delta qp
     * cp_delta_qp      delta qp applied on when on bit rate error amount
     */
    RK_S32          cp_distance_mbs;
    RK_S32          cp_target[VEPU_CHECK_POINTS_MAX];
    RK_S32          cp_error[VEPU_CTRL_LEVELS];
    RK_S32          cp_delta_qp[VEPU_CTRL_LEVELS];

    /*
     * MAD based QP adjustment
     * mad_qp_change    [-8..7]
     * mad_threshold    MAD threshold div256
     */
    RK_S32          mad_qp_change;
    RK_S32          mad_threshold;

    /* slice split by mb row (0 = one slice) */
    RK_S32          slice_size_mb_rows;

    /* favor and penalty for mode decision */

    /*
     * VEPU MB rate control parameter which is read from hardware
     * out_strm_size    output stream size (bits)
     * qp_sum           QP Sum div2 output
     * rlc_count        RLC codeword count div4 output max 255*255*384/4
     */
    RK_U32          hdr_strm_size;
    RK_U32          hdr_free_size;
    RK_U32          out_strm_size;
    RK_S32          qp_sum;
    RK_S32          rlc_count;

    RK_S32          cp_usage[VEPU_CHECK_POINTS_MAX];
    /* Macroblock count with MAD value under threshold output */
    RK_S32          less_mad_count;
    /* MB count output */
    RK_S32          mb_count;

    /* hardware encoding status 0 - corret 1 - error */
    RK_U32          hw_status;
} HalH264eVepuMbRc;

typedef void *HalH264eVepuMbRcCtx;

typedef struct HalH264eVepuStreamAmend_t {
    RK_S32          enable;
    H264eSlice      *slice;
    H264ePrefixNal  *prefix;
    RK_S32          slice_enabled;

    RK_U8           *src_buf;
    RK_U8           *dst_buf;
    RK_S32          buf_size;

    MppPacket       packet;
    RK_S32          buf_base;
    RK_S32          old_length;
    RK_S32          new_length;
} HalH264eVepuStreamAmend;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 exp_golomb_signed(RK_S32 val);

/* buffer management function */
MPP_RET h264e_vepu_buf_init(HalH264eVepuBufs *bufs);
MPP_RET h264e_vepu_buf_deinit(HalH264eVepuBufs *bufs);

MPP_RET h264e_vepu_buf_set_cabac_idc(HalH264eVepuBufs *bufs, RK_S32 idc);
MPP_RET h264e_vepu_buf_set_frame_size(HalH264eVepuBufs *bufs, RK_S32 w, RK_S32 h);

MppBuffer h264e_vepu_buf_get_nal_size_table(HalH264eVepuBufs *bufs);
MppBuffer h264e_vepu_buf_get_frame_buffer(HalH264eVepuBufs *bufs, RK_S32 index);

/* preprocess setup function */
MPP_RET h264e_vepu_prep_setup(HalH264eVepuPrep *prep, MppEncPrepCfg *cfg);
MPP_RET h264e_vepu_prep_get_addr(HalH264eVepuPrep *prep, MppBuffer buffer,
                                 RK_U32 (*addr)[3]);

/* macroblock bitrate control function */
MPP_RET h264e_vepu_mbrc_init(HalH264eVepuMbRcCtx *ctx, HalH264eVepuMbRc *mbrc);
MPP_RET h264e_vepu_mbrc_deinit(HalH264eVepuMbRcCtx ctx);

MPP_RET h264e_vepu_mbrc_setup(HalH264eVepuMbRcCtx ctx, MppEncCfgSet *cfg);
MPP_RET h264e_vepu_slice_split_cfg(H264eSlice *slice, HalH264eVepuMbRc *mbrc,
                                   EncRcTask *rc_task, MppEncCfgSet *set_cfg);

/*
 * generate hardware MB rc config by:
 * 1 - HalH264eVepuMbRcCtx ctx
 *     The previous frame encoding status
 * 2 - RcSyntax
 *     Provide current frame target bitrate related info
 * 3 - EncFrmStatus
 *     Provide dpb related info like I / P frame, temporal id, refer distance
 *
 * Then output the HalH264eVepuMbRc for register generation
 */
MPP_RET h264e_vepu_mbrc_prepare(HalH264eVepuMbRcCtx ctx, HalH264eVepuMbRc *mbrc,
                                EncRcTask *rc_task);
MPP_RET h264e_vepu_mbrc_update(HalH264eVepuMbRcCtx ctx, HalH264eVepuMbRc *mbrc);

MPP_RET h264e_vepu_stream_amend_init(HalH264eVepuStreamAmend *ctx);
MPP_RET h264e_vepu_stream_amend_deinit(HalH264eVepuStreamAmend *ctx);
MPP_RET h264e_vepu_stream_amend_config(HalH264eVepuStreamAmend *ctx,
                                       MppPacket packet, MppEncCfgSet *cfg,
                                       H264eSlice *slice, H264ePrefixNal *prefix);
MPP_RET h264e_vepu_stream_amend_proc(HalH264eVepuStreamAmend *ctx);
MPP_RET h264e_vepu_stream_amend_sync_ref_idc(HalH264eVepuStreamAmend *ctx);

#ifdef __cplusplus
}
#endif

#endif
