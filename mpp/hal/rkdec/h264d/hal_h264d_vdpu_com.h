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

#ifndef __HAL_H264D_VDPU_COM_H__
#define __HAL_H264D_VDPU_COM_H__

#include "rk_type.h"

#include "hal_h264d_global.h"

#define VDPU_CABAC_TAB_SIZE        (3680)        /* bytes */
#define VDPU_POC_BUF_SIZE          (34*4)        /* bytes */
#define VDPU_SCALING_LIST_SIZE     (6*16+2*64)   /* bytes */

extern const RK_U32 vdpu_cabac_table[VDPU_CABAC_TAB_SIZE / 4];
extern const RK_U32 vdpu_value_list[34];

typedef struct h264d_vdpu_dpb_info_t {
    RK_U8     valid;

    RK_S32    slot_index;
    RK_U32    is_long_term;
    RK_S32    top_poc;
    RK_S32    bot_poc;
    RK_U16    frame_num;
    RK_U32    long_term_frame_idx;
    RK_U32    long_term_pic_num;
    RK_U32    top_used;
    RK_U32    bot_used;
    RK_U32    view_id;
    RK_U32    colmv_is_used;
    RK_U32    field_flag;
    RK_U32    is_ilt_flag;
    RK_U32    voidx;

    RK_U8     have_same;
    RK_U32    new_dpb_idx;
} H264dVdpuDpbInfo_t;

typedef struct h264d_vdpu_ref_pic_info_t {
    RK_U32    valid;

    RK_S32    dpb_idx;
    RK_S32    bottom_flag;
} H264dVdpuRefPicInfo_t;

typedef struct h264d_vdpu_priv_t {
    RK_U32                 layed_id;

    H264dVdpuDpbInfo_t     old_dpb[2][16];
    H264dVdpuDpbInfo_t     new_dpb[16];
    H264dVdpuDpbInfo_t     *ilt_dpb;
    H264dVdpuRefPicInfo_t  refinfo[3][32]; //!< listP listB0 list1
} H264dVdpuPriv_t;

typedef struct h264d_vdpu_buf_t {
    RK_U32 valid;

    MppBuffer buf;
    void *cabac_ptr;
    void *poc_ptr;
    void *sclst_ptr;
    void *regs;
} H264dVdpuBuf_t;

typedef struct h264d_refs_list_t {
    RK_S32 idx;
    RK_S32 cur_poc;

    RK_S32 lt_flag;
    RK_S32 ref_poc;
    RK_S32 ref_picnum;
} H264dRefsList_t;

typedef struct h264d_vdpu_reg_ctx_t {
    H264dVdpuBuf_t reg_buf[3];

    MppBuffer buf;
    void *cabac_ptr;
    void *poc_ptr;
    void *sclst_ptr;
    void *regs;
} H264dVdpuRegCtx_t;

#ifdef __cplusplus
extern "C" {
#endif

RK_U32 vdpu_ver_align(RK_U32 val);
RK_U32 vdpu_hor_align(RK_U32 val);

MPP_RET adjust_input(H264dVdpuPriv_t *priv,
                     DXVA_Slice_H264_Long *p_long,
                     DXVA_PicParams_H264_MVC  *pp);

RK_S32 compare_p(const void *a, const void *b);
RK_S32 compare_b0(const void *a, const void *b);
RK_S32 compare_b1(const void *a, const void *b);

#ifdef __cplusplus
}
#endif

#endif
