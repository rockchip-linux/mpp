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

#include "h264e_syntax.h"
#include "h264e_sps.h"
#include "mpp_enc_ref.h"

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

typedef struct  H264eDpbFrm_t {
    RK_S32              slot_idx;
    // frame index in frames
    RK_S32              seq_idx;

    union {
        RK_U32          on_used;
        struct {
            RK_U32      dpb_used    : 8;
            RK_U32      hal_used    : 8;
        };
    };

    /* frame status */
    EncFrmStatus        status;

    /* frame number from H264eSlice */
    RK_S32              frame_num;
    RK_S32              lt_idx;
    /* poc from H264eSlice */
    RK_S32              poc;
    /* pts from input MppFrame */
    RK_S64              pts;
} H264eDpbFrm;

/* runtime status record */
typedef struct H264eDpbRt_t {
    RK_S32              last_seq_idx;
    RK_S32              last_is_ref;
    RK_S32              last_frm_num;
    RK_S32              last_poc_lsb;
    RK_S32              last_poc_msb;
} H264eDpbRt;

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

    MppEncCpbInfo       info;
    RK_S32              next_max_lt_idx;
    RK_S32              curr_max_lt_idx;
    RK_S32              st_size;
    RK_S32              lt_size;
    RK_S32              used_size;

    RK_S32              dpb_size;
    RK_S32              total_cnt;

    /* status on dpb rebuild is needed */
    RK_S32              max_frm_num;
    RK_S32              max_poc_lsb;
    RK_S32              poc_type;

    RK_S32              last_frm_num;
    RK_S32              last_poc_lsb;
    RK_S32              last_poc_msb;

    H264eDpbFrm         *curr;
    H264eDpbFrm         *refr;
    H264eDpbFrm         *list[H264E_MAX_REFS_CNT];
    H264eDpbFrm         *stref[H264E_MAX_REFS_CNT];
    H264eDpbFrm         *ltref[H264E_MAX_REFS_CNT];
    H264eDpbFrm         *map[H264E_MAX_REFS_CNT + 1];

    // frame storage
    H264eDpbRt          rt;
    H264eDpbRt          rt_bak;
    H264eDpbFrm         frames[H264E_MAX_REFS_CNT + 1];
    H264eDpbFrm         frm_bak[H264E_MAX_REFS_CNT + 1];
} H264eDpb;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h264e_dpb_init(H264eDpb *dpb, H264eReorderInfo *reorder, H264eMarkingInfo *marking);
MPP_RET h264e_dpb_deinit(H264eDpb *dpb);

MPP_RET h264e_dpb_setup(H264eDpb *dpb, MppEncCfgSet* cfg, H264eSps *sps);

/*
 * Setup current frame config using flags
 * This config function will be called before each frame is encoded:
 *
 * idr      - current frame is force to IDR or not
 * lt_ref   - current frame is marked as longterm reference
 */
MPP_RET h264e_dpb_proc(H264eDpb *dpb, EncCpbStatus *cpb);

/*
 * hal usage flag mark / unmark function
 */
MPP_RET h264e_dpb_hal_start(H264eDpb *dpb, RK_S32 slot_idx);
MPP_RET h264e_dpb_hal_end(H264eDpb *dpb, RK_S32 slot_idx);

void h264e_dpb_check(H264eDpb *dpb, EncCpbStatus *cpb);

#define h264e_dpb_dump_frms(dpb) h264e_dpb_dump_frm(dpb, __FUNCTION__, __LINE__)

void h264e_dpb_dump_frm(H264eDpb *dpb, const char *caller, RK_S32 line);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_DPB_H__ */
