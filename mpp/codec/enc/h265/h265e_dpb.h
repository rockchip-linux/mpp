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
    RK_S32 poc_cur_list;

    RK_S32 poc[RPSLIST_MAX];

    RK_U32 used_by_cur[MAX_REFS];

    H265eRefPicListModification  *m_RefPicListModification;
} H265eRpsList;


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
    RK_S32             seq_idx;
    RK_S32             gop_idx;
    // status and count for one gop structure

    RK_S32              last_idr;
    RK_S32              poc_cra;
    RK_U32              refresh_pending;
    RK_S32              max_ref_l0;
    RK_S32              max_ref_l1;

    H265eRpsList        RpsList;
    H265eDpbFrm         *curr;
    H265eDpbFrm         frame_list[MAX_REFS + 1];
} H265eDpb;

#ifdef __cplusplus
extern "C" {
#endif

void h265e_set_ref_list(H265eRpsList *RpsList, H265eReferencePictureSet *m_pRps);
MPP_RET h265e_dpb_init(H265eDpb **dpb);
MPP_RET h265e_dpb_deinit(H265eDpb *dpb);
MPP_RET h265e_dpb_setup_buf_size(H265eDpb *dpb, RK_U32 size[], RK_U32 count);
MPP_RET h265e_dpb_get_curr(H265eDpb *dpb);
void h265e_dpb_build_list(H265eDpb *dpb, EncCpbStatus *cpb);
void h265e_dpb_proc_cpb(H265eDpb *dpb, EncCpbStatus *cpb);

#define h265e_dpb_dump_frms(dpb) h265e_dpb_dump_frm(dpb, __FUNCTION__)

void h265e_dpb_dump_frm(H265eDpb *dpb, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* __H265E_DPB_H__ */
