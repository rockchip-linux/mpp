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

#ifndef __H265E_DPB_H__
#define __H265E_DPB_H__

#include "mpp_buffer.h"
#include "h265e_slice.h"
#include "h265_syntax.h"
#include "mpp_enc_cfg.h"

/*
 * H.265 encoder dpb structure info
 *
 *     +----------+ DPB +-> build list +-> REF_LIST
 *     |             +                        +
 *     v             v                        v
 * FRM_BUF_GRP    DPB_FRM +------+------> RPS
 *     +             +
 *     |             v
 *     +--------> FRM_BUF
 *
 * H265eDpb is overall structure contain the whole dpb info.
 * It is composed H265eDpbFrm and H265eRpsList.
 *
 */

#define H265E_MAX_BUF_CNT   8
#define H265_MAX_GOP        64          ///< max. value of hierarchical GOP size
#define H265_MAX_GOP_CNT    (H265_MAX_GOP + 1)

#define RPSLIST_MAX         (H265_MAX_GOP * 3)

typedef struct  H265eDpb_t          H265eDpb;
typedef struct  H265eReferencePictureSet_e H265eReferencePictureSet;
typedef struct  H265eRefPicListModification_e H265eRefPicListModification;
typedef struct  H265eSlice_e  H265eSlice;


typedef struct  H265eDpb_t          H265eDpb;

/*
 * Split reference frame configure to two parts
 * The first part is slice depended info like poc / frame_num, and frame
 * type and flags.
 * The other part is gop structure depended info like gop index, ref_status
 * and ref_frm_index. This part is inited from dpb gop hierarchy info.
 */

typedef struct  H265eDpbFrm_t {
    H265eDpb            *dpb;

    RK_S32              slot_idx;
    RK_S32              seq_idx;
    RK_S32              gop_idx;
    RK_S32              gop_cnt;
    EncFrmStatus        status;

    RK_U32              on_used;
    RK_U32              inited;

    RK_U32              is_long_term;
    RK_U32              used_by_cur;
    RK_U32              check_lt_msb;
    RK_S32              poc;

    H265eSlice          *slice;

    RK_S64              pts;
    RK_S64              dts;

    RK_U32              is_key_frame;
} H265eDpbFrm;

typedef struct H265eRpsList_e {
    RK_S32 lt_num;
    RK_S32 st_num;
    RK_S32 vgop_size;
    RK_S32 gop_len;
    RK_S32 poc_cur_list;

    RK_S32 poc[RPSLIST_MAX];
#if 0
    RK_S32 SliceType[RPSLIST_MAX];
    RK_S32 DeltaPocIdx[RPSLIST_MAX];
    RK_S32 IsLongTerm[RPSLIST_MAX];
    RK_S32 IsShortTerm[RPSLIST_MAX];
    RK_S32 GopFirstFrmFlg[RPSLIST_MAX];
    RK_U32 m_bRefenced[RPSLIST_MAX];
#endif
    RK_S32 delta_poc_idx[H265_MAX_GOP];

    RK_U32 used_by_cur[MAX_REFS];

    H265eRefPicListModification  *m_RefPicListModification;
} H265eRpsList;

typedef struct H265eDpbCfg_t {
    RK_S32  maxNumReferences;
    RK_S32  bBPyramid;
    RK_S32  bOpenGOP;
    RK_S32  nLongTerm;
    RK_S32  gop_len;
    RK_S32  vgop_size;

    RK_S32  nDeltaPocIdx[30];

    EncFrmStatus ref_inf[H265_MAX_GOP_CNT];

    RK_S32  tot_poc_num;
    /* request current frame to be IDR frame */
    RK_S32  force_idr;

    /* request current frame to be a software all PSkip frame */
    RK_S32  force_pskip;
    /*
     * request current frame to be mark as a long-reference frame
     * -1   - not forced to be marked as long-term reference frame
     */
    RK_S32  force_lt_idx;

    /*
     * request current frame use long-term reference frame as its reference frame
     * -1   - not forced to use long-term reference frame as reference frame
     */
    RK_S32  force_ref_lt_idx;
} H265eDpbCfg;

/*
 * dpb frame arrangement
 *
 * If dpb size is 3 then dpb frame will be total 4 frames.
 * Then the frame 3 is always current frame and frame 0~2 is reference frame
 * in the gop structure.
 *
 * When one frame is encoded all it info will be moved to its gop position for
 * next frame encoding.
 */
typedef struct H265eDpb_t {
    H265eDpbCfg        *cfg;

    RK_S32             seq_idx;
    RK_S32             gop_idx;
    // status and count for one gop structure
    // idr_gop  - for intra / IDR frame group of picture
    RK_S32             idr_gop_len;
    RK_S32             idr_gop_cnt;
    RK_S32             idr_gop_idx;

    RK_S32              last_idr;
    RK_S32              poc_cra;
    RK_U32              refresh_pending;
    RK_U32              is_long_term;
    RK_S32              max_ref_l0;
    RK_S32              max_ref_l1;
    RK_S32              is_open_gop;
    RK_U32              idr_gap;

    H265eRpsList        RpsList;
    H265eDpbFrm         *curr;
    H265eDpbFrm         frame_list[MAX_REFS + 1];
} H265eDpb;

#ifdef __cplusplus
extern "C" {
#endif

void h265e_gop_init(H265eRpsList *RpsList, H265eDpbCfg *cfg, int procCurr);
void h265e_set_ref_list(H265eRpsList *RpsList, H265eReferencePictureSet *m_pRps);
MPP_RET h265e_dpb_init(H265eDpb **dpb, H265eDpbCfg *cfg);
MPP_RET h265e_dpb_deinit(H265eDpb *dpb);
MPP_RET h265e_dpb_setup_buf_size(H265eDpb *dpb, RK_U32 size[], RK_U32 count);
MPP_RET h265e_dpb_get_curr(H265eDpb *dpb);
H265eDpbFrm *h265e_dpb_get_refr(H265eDpbFrm *frm);
void h265e_dpb_build_list(H265eDpb *dpb);
MppBuffer h265e_dpb_frm_get_buf(H265eDpbFrm *frm, RK_S32 index);
MPP_RET h265e_dpb_set_cfg(H265eDpbCfg *dpb_cfg, MppEncCfgSet* cfg);

#define h265e_dpb_dump_frms(dpb) h265e_dpb_dump_frm(dpb, __FUNCTION__)

void h265e_dpb_dump_frm(H265eDpb *dpb, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* __H265E_DPB_H__ */
