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

#ifndef __H264E_DPB_H__
#define __H264E_DPB_H__

#include "mpp_buffer.h"

#include "mpp_enc_refs.h"
#include "h264e_slice.h"

/*
 * H.264 encoder dpb structure info
 *
 *     +----------+ DPB +-> build list +-> REF_LIST
 *     |             +                        +
 *     v             v                        v
 * FRM_BUF_GRP    DPB_FRM +------+------> get_reorder
 *     +             +           |
 *     |             v           |
 *     +--------> FRM_BUF        +------> get_mmco
 *
 * H264eDpb is overall structure contain the whole dpb info.
 * It is composed H264eDpbFrm and H264eDpbList.
 *
 */

#define H264E_REF_MAX               16
#define H264E_MAX_BUF_CNT           2

#define REF_BY_RECN(idx)            (0x00000001 << idx)
#define REF_BY_REFR(idx)            (0x00000001 << idx)

typedef struct  H264eDpb_t          H264eDpb;
typedef struct  H264eDpbFrm_t       H264eDpbFrm;
typedef struct  H264eFrmBuf_t       H264eFrmBuf;
typedef struct  H264eFrmBufGrp_t    H264eFrmBufGrp;

/*
 * Split reference frame configure to two parts
 * The first part is slice depended info like poc / frame_num, and frame
 * type and flags.
 * The other part is gop structure depended info like gop index, ref_status
 * and ref_frm_index. This part is inited from dpb gop hierarchy info.
 */
typedef struct H264eDpbFrmInfo_t {
    /*
     * 0 - inter frame
     * 1 - intra frame
     */
    RK_U32              is_intra    : 1;

    /*
     * Valid when is_intra is true
     * 0 - normal intra frame
     * 1 - IDR frame
     */
    RK_U32              is_idr      : 1;

    /*
     * 0 - mark as reference frame
     * 1 - mark as non-refernce frame
     */
    RK_U32              is_non_ref  : 1;

    /*
     * Valid when is_non_ref is false
     * 0 - mark as short-term reference frame
     * 1 - mark as long-term refernce frame
     */
    RK_U32              is_lt_ref   : 1;
    RK_U32              stuff       : 28;

} H264eDpbFrmInfo;

typedef struct  H264eDpbFrm_t {
    H264eDpb            *dpb;
    // frame index in frames
    RK_S32              frm_cnt;
    // gop index in one gop structure
    RK_S32              gop_idx;
    RK_S32              gop_cnt;

    RK_U32              on_used;
    RK_U32              inited;

    /*
     * Frame refer status (current)
     * Each frame should init a ref_status for other frames reference currnt
     * frame.
     *
     * Example 1: I0 -> P1 -> P2 -> P3
     * ref_status 0x0   0x2   0x4   0x8
     *
     * Example 2: I0 -> P1
     *             \\-------> P2
     *              \-------------> P3
     * ref_status 0xe   0x0   0x0   0x0
     *
     * The corresponding bit with gop_idx will be true if the frame is
     * referneced by the frame with gop_idx.
     *
     * When the ref_status bit is all clear then this frame can be freed.
     */
    RK_U32              ref_status;
    RK_U32              ref_count;
    // indicate which frame current frame is refering
    RK_S32              ref_dist;

    /* frame status */
    MppEncFrmStatus     info;
    /* flag for marking process */
    RK_S32              marked_unref;

    /*
     * ENC_FRAME_TYPE in mpp_rc.h
     * 0 - INTER_P_FRAME
     * 1 - INTER_B_FRAME
     * 2 - INTRA_FRAME
     */
    RK_S32              frame_type;
    /* frame number from H264eSlice */
    RK_S32              frame_num;
    RK_S32              lt_idx;
    /* poc from H264eSlice */
    RK_S32              poc;
    /* pts from input MppFrame */
    RK_S64              pts;

    H264eFrmBuf         *buf;
} H264eDpbFrm;

typedef struct H264eFrmBuf_t {
    RK_U32              is_used;
    // buf0 for normal pixel buffer
    // buf1 for scaled buffer
    MppBuffer           buf[H264E_MAX_BUF_CNT];
} H264eFrmBuf;

typedef struct H264eFrmBufGrp_t {
    MppBufferGroup      group;
    // buffer count for each frame buffers
    RK_U32              count;
    // buffer size for each buffer in frame buffers
    RK_U32              size[H264E_MAX_BUF_CNT];
    // frame buffer set
    H264eFrmBuf         bufs[H264E_REF_MAX];
} H264eFrmBufGrp;

typedef struct H264eDpbCfg_t {
    RK_S32              ref_frm_num;
    RK_S32              log2_max_frm_num;
    RK_S32              log2_max_poc_lsb;
} H264eDpbCfg;

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
typedef struct H264eDpb_t {
    H264eDpbCfg         cfg;

    /*
     * ref_frm_num  - max reference frame number
     * max_lt_idx   - max long-term reference picture index
     * max_frm_num  - max frame_num
     * max_poc_lsb  - max picture order count lsb
     */
    RK_S32              ref_frm_num;
    RK_S32              max_lt_idx;
    RK_S32              max_frm_num;
    RK_S32              max_poc_lsb;

    // status and count for one gop structure
    RK_S32              seq_cnt;
    RK_S32              seq_idx;
    RK_S32              gop_len;
    RK_S32              gop_cnt;
    RK_S32              gop_idx;

    RK_S32              curr_frm_num;
    RK_S32              next_frm_num;

    // slot counter
    RK_S32              total_cnt;
    RK_S32              max_st_cnt;
    RK_S32              max_lt_cnt;
    RK_S32              curr_idx;

    // for reference frame list generation
    RK_S32              dpb_size;
    RK_S32              st_size;
    RK_S32              lt_size;
    H264eDpbFrm         *curr;
    H264eDpbFrm         *list[H264E_REF_MAX];
    H264eDpbFrm         *stref[H264E_REF_MAX];
    H264eDpbFrm         *ltref[H264E_REF_MAX];
    RK_S32              need_reorder;
    RK_S32              curr_max_lt_idx;
    RK_S32              next_max_lt_idx;

    // for mmco unreferece marking process
    RK_U32              unref_cnt;
    H264eDpbFrm         *unref[H264E_REF_MAX];

    /*
     * ref_inf bit info:
     * bit 0 - intra flag
     * bit 1 - idr flag
     * bit 2 - non-ref flag
     * bit 3 - long-term flag
     */
    MppEncFrmStatus     ref_inf[MAX_GOP_FMT_CNT];
    RK_U32              ref_sta[MAX_GOP_FMT_CNT];
    RK_U32              ref_cnt[MAX_GOP_FMT_CNT];
    RK_S32              ref_dist[MAX_GOP_FMT_CNT];

    // buffer management
    H264eFrmBufGrp      buf_grp;

    // frame storage
    H264eDpbFrm         *frames;
} H264eDpb;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h264e_dpb_init(H264eDpb **dpb, H264eDpbCfg *cfg);
MPP_RET h264e_dpb_deinit(H264eDpb *dpb);

MPP_RET h264e_dpb_setup_buf_size(H264eDpb *dpb, RK_U32 size[], RK_U32 count);
MPP_RET h264e_dpb_setup_hier(H264eDpb *dpb, MppEncHierCfg *cfg);

H264eDpbFrm *h264e_dpb_get_curr(H264eDpb *dpb, RK_S32 new_seq);
H264eDpbFrm *h264e_dpb_get_refr(H264eDpbFrm *frm);
void h264e_dpb_build_list(H264eDpb *dpb);
void h264e_dpb_build_marking(H264eDpb *dpb);
void h264e_dpb_curr_ready(H264eDpb *dpb);

MppBuffer h264e_dpb_frm_get_buf(H264eDpbFrm *frm, RK_S32 index);

#define h264e_dpb_dump_frms(dpb) h264e_dpb_dump_frm(dpb, __FUNCTION__)

void h264e_dpb_dump_frm(H264eDpb *dpb, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_DPB_H__ */
