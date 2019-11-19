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

#include "h264e_sps.h"

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

#define MAX_GOP_SIZE                8
#define MAX_GOP_FMT_CNT             (MAX_GOP_SIZE+1)
#define MAX_GOP_FMT_SIZE            5
#define MAX_GOP_FMT_BUF_STUFF       16
#define MAX_GOP_FMT_BUF_SIZE        (MAX_GOP_FMT_CNT * MAX_GOP_FMT_SIZE + MAX_GOP_FMT_BUF_STUFF)

#define H264E_ST_GOP_FLAG           (0x00000001)
#define H264E_ST_GOP_WITH_LT_REF    (0x00000002)
#define H264E_LT_GOP_FLAG           (0x00000010)

#define REF_BY_RECN(idx)            (0x00000001 << idx)
#define REF_BY_REFR(idx)            (0x00000001 << idx)

typedef struct H264eDpbFrmCfg_t {
    /* request current frame to be IDR frame */
    RK_S32              force_idr;

    /* request current frame to be a software all PSkip frame */
    RK_S32              force_pskip;

    /*
     * request current frame to be mark as a long-reference frame
     * -1   - not forced to be marked as long-term reference frame
     */
    RK_S32              force_lt_idx;

    /*
     * request current frame use long-term reference frame as its reference frame
     * -1   - not forced to use long-term reference frame as reference frame
     */
    RK_S32              force_ref_lt_idx;
} H264eDpbFrmCfg;

typedef struct  H264eDpbFrm_t {
    RK_S32              slot_idx;
    // frame index in frames
    RK_S32              seq_idx;
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
    EncFrmStatus        status;

    /* H264_I_SLICE / H264_P_SLICE */
    H264SliceType       frame_type;
    /* frame number from H264eSlice */
    RK_S32              frame_num;
    RK_S32              lt_idx;
    /* poc from H264eSlice */
    RK_S32              poc;
    /* pts from input MppFrame */
    RK_S64              pts;
} H264eDpbFrm;

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
    H264eReorderInfo    *reorder;
    H264eMarkingInfo    *marking;

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

    /*
     * dpb mode
     * 0 - Default two frame swap mode. Only use refr and recn frame
     * 1 - Dpb with user hierarchy mode
     */
    RK_S32              mode;

    // overall frame counter
    RK_S32              seq_idx;

    // status and count for one gop structure
    // idr_gop  - for intra / IDR frame group of picture
    // st_gop   - for short-term reference group of picture in TSVC mode
    // lt_gop   - for long-term reference group of picture in SVC mode
    RK_S32              idr_gop_len;
    RK_S32              idr_gop_cnt;
    RK_S32              idr_gop_idx;

    RK_S32              st_gop_len;
    RK_S32              st_gop_cnt;
    RK_S32              st_gop_idx;

    RK_S32              lt_gop_len;
    RK_S32              lt_gop_cnt;
    RK_S32              lt_gop_idx;

    RK_S32              poc_lsb;
    RK_S32              last_frm_num;
    RK_S32              lt_ref_idx;

    // slot counter
    RK_S32              total_cnt;

    // for reference frame list generation
    RK_S32              dpb_size;
    RK_S32              st_size;
    RK_S32              lt_size;
    H264eDpbFrm         *curr;
    H264eDpbFrm         *refr;
    H264eDpbFrm         *list[H264E_MAX_REFS_CNT];
    H264eDpbFrm         *stref[H264E_MAX_REFS_CNT];
    H264eDpbFrm         *ltref[H264E_MAX_REFS_CNT];
    RK_S32              curr_max_lt_idx;
    RK_S32              next_max_lt_idx;

    /*
     * ref_inf bit info:
     * bit 0 - intra flag
     * bit 1 - idr flag
     * bit 2 - non-ref flag
     * bit 3 - long-term flag
     */
    EncFrmStatus        ref_inf[MAX_GOP_FMT_CNT];
    RK_U32              ref_sta[MAX_GOP_FMT_CNT];
    RK_U32              ref_cnt[MAX_GOP_FMT_CNT];
    RK_S32              ref_dist[MAX_GOP_FMT_CNT];

    // frame storage
    H264eDpbFrm         frames[H264E_MAX_REFS_CNT + 1];
} H264eDpb;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h264e_dpb_init(H264eDpb *dpb, H264eReorderInfo *reorder, H264eMarkingInfo *marking);
MPP_RET h264e_dpb_deinit(H264eDpb *dpb);

/* copy function is for dpb backup and restore */
MPP_RET h264e_dpb_copy(H264eDpb *dst, H264eDpb *src);

/*
 * Setup gop group config using MppEncCfgSet and SynH264eSps
 * This config function will be called on the following cases:
 *
 * 1. sps reference number changed which will change totol dpb slot count
 * 2. gop size changed which will change the max frame number and max poc lsb
 * 3. gop ref relation changed will change tsvc / vgop setup
 */
MPP_RET h264e_dpb_set_cfg(H264eDpb *dpb, MppEncCfgSet* cfg, SynH264eSps *sps);

/*
 * Setup current frame config using flags
 * This config function will be called before each frame is encoded:
 *
 * idr      - current frame is force to IDR or not
 * lt_ref   - current frame is marked as longterm reference
 */
MPP_RET h264e_dpb_set_curr(H264eDpb *dpb, H264eDpbFrmCfg *cfg);

/*
 * Setup current frame and reference frame
 */
void h264e_dpb_build_list(H264eDpb *dpb);
void h264e_dpb_build_marking(H264eDpb *dpb);
void h264e_dpb_curr_ready(H264eDpb *dpb);

#define h264e_dpb_dump_frms(dpb) h264e_dpb_dump_frm(dpb, __FUNCTION__)

void h264e_dpb_dump_frm(H264eDpb *dpb, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_DPB_H__ */
