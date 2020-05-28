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

#define MODULE_TAG "h265e_slice"

#include <string.h>

#include "mpp_log.h"
#include "h265e_codec.h"
#include "h265e_slice.h"

H265eDpbFrm* get_ref_pic(H265eDpbFrm *frame_list, int poc)
{
    RK_S32 index = 0;
    H265eDpbFrm *frame = NULL;

    h265e_dbg_func("enter\n");
    for (index = 0; index < MAX_REFS; index++) {
        frame = &frame_list[index];
        if (frame->inited && frame->poc == poc) {
            break;
        }
    }
    h265e_dbg_func("leave\n");
    return frame;
}

H265eDpbFrm* get_lt_ref_pic(H265eDpbFrm *frame_list, H265eSlice *slice, int poc, RK_U32 pocHasMsb)
{
    RK_S32 index = 0;
    H265eDpbFrm *frame = NULL;
    H265eDpbFrm* stPic = &frame_list[MAX_REFS - 1];
    RK_S32 pocCycle = 1 << slice->m_sps->m_bitsForPOC;

    h265e_dbg_func("enter\n");
    if (!pocHasMsb) {
        poc = poc % pocCycle;
    }

    for (index = MAX_REFS - 1 ; index >= 0; index--) {
        frame = &frame_list[index];
        if (frame->on_used && frame->poc != slice->poc && frame->slice->is_referenced) {
            int picPoc = frame->poc;
            if (!pocHasMsb) {
                picPoc = picPoc % pocCycle;
            }

            if (poc == picPoc) {
                if (frame->is_long_term) {
                    return frame;
                } else {
                    stPic = frame;
                }
            }
        }

    }
    h265e_dbg_func("leave\n");
    return stPic;
}

void h265e_slice_set_ref_list(H265eDpbFrm *frame_list, H265eSlice *slice)
{
    H265eReferencePictureSet *rps = slice->m_rps;
    H265eDpbFrm* refPic = NULL;
    H265eDpbFrm* refPicSetStCurr0[MAX_REFS];
    H265eDpbFrm* refPicSetStCurr1[MAX_REFS];
    H265eDpbFrm* refPicSetLtCurr[MAX_REFS];
    RK_S32 numPocStCurr0 = 0;
    RK_S32 numPocStCurr1 = 0;
    RK_S32 numPocLtCurr = 0;
    RK_S32 i, cIdx = 0, rIdx = 0;

    h265e_dbg_func("enter\n");
    if (slice->m_sliceType == I_SLICE) {
        memset(slice->m_refPicList, 0, sizeof(slice->m_refPicList));
        memset(slice->m_numRefIdx,  0, sizeof(slice->m_numRefIdx));
        return;
    }
    for (i = 0; i < rps->num_negative_pic; i++) {
        if (rps->m_used[i]) {
            refPic = get_ref_pic(frame_list, slice->poc + rps->delta_poc[i]);
            refPic->is_long_term = 0;
            refPicSetStCurr0[numPocStCurr0] = refPic;
            numPocStCurr0++;
            refPic->check_lt_msb = 0;
        }
    }

    for (; i < rps->num_negative_pic + rps->num_positive_pic; i++) {
        if (rps->m_used[i]) {
            refPic = get_ref_pic(frame_list,  slice->poc + rps->delta_poc[i]);
            refPic->is_long_term = 0;
            refPicSetStCurr1[numPocStCurr1] = refPic;
            numPocStCurr1++;
            refPic->check_lt_msb = 0;
        }
    }

    for (i = rps->num_negative_pic + rps->num_positive_pic + rps->num_long_term_pic - 1;
         i > rps->num_negative_pic + rps->num_positive_pic - 1; i--) {
        if (rps->m_used[i]) {
            refPic = get_lt_ref_pic(frame_list, slice, rps->m_RealPoc[i], rps->check_lt_msb[i]);
            refPic->is_long_term = 1;
            /*due to only one ref if num_negative_pic > 0 set lt not used*/
            /* if (rps->num_negative_pic > 0) {
                 rps->m_used[i] = 0;
             }*/
            refPicSetLtCurr[numPocLtCurr] = refPic;
            numPocLtCurr++;
        }
        if (refPic == NULL) {
            refPic = get_lt_ref_pic(frame_list, slice, rps->m_RealPoc[i], rps->check_lt_msb[i]);
        }
        refPic->check_lt_msb = rps->check_lt_msb[i];
    }

    // ref_pic_list_init
    H265eDpbFrm* rpsCurrList0[MAX_REFS + 1];
    H265eDpbFrm* rpsCurrList1[MAX_REFS + 1];
    RK_S32 numPocTotalCurr = numPocStCurr0 + numPocStCurr1 + numPocLtCurr;

    for (i = 0; i < numPocStCurr0; i++, cIdx++) {
        rpsCurrList0[cIdx] = refPicSetStCurr0[i];
    }

    for (i = 0; i < numPocStCurr1; i++, cIdx++) {
        rpsCurrList0[cIdx] = refPicSetStCurr1[i];
    }

    for (i = 0; i < numPocLtCurr; i++, cIdx++) {
        rpsCurrList0[cIdx] = refPicSetLtCurr[i];
    }

    mpp_assert(cIdx == numPocTotalCurr);

    if (slice->m_sliceType == B_SLICE) {
        cIdx = 0;
        for (i = 0; i < numPocStCurr1; i++, cIdx++) {
            rpsCurrList1[cIdx] = refPicSetStCurr1[i];
        }

        for (i = 0; i < numPocStCurr0; i++, cIdx++) {
            rpsCurrList1[cIdx] = refPicSetStCurr0[i];
        }

        for (i = 0; i < numPocLtCurr; i++, cIdx++) {
            rpsCurrList1[cIdx] = refPicSetLtCurr[i];
        }

        mpp_assert(cIdx == numPocTotalCurr);
    }

    memset(slice->m_bIsUsedAsLongTerm, 0, sizeof(slice->m_bIsUsedAsLongTerm));

    for ( rIdx = 0; rIdx < slice->m_numRefIdx[0]; rIdx++) {
        cIdx = slice->m_RefPicListModification.m_refPicListModificationFlagL0 ? slice->m_RefPicListModification.m_RefPicSetIdxL0[rIdx] : (RK_U32)rIdx % numPocTotalCurr;
        mpp_assert(cIdx >= 0 && cIdx < numPocTotalCurr);
        slice->m_refPicList[0][rIdx] = rpsCurrList0[cIdx];
        slice->m_bIsUsedAsLongTerm[0][rIdx] = (cIdx >= numPocStCurr0 + numPocStCurr1);
    }

    if (slice->m_sliceType != B_SLICE) {
        slice->m_numRefIdx[1] = 0;
        memset(slice->m_refPicList[1], 0, sizeof(slice->m_refPicList[1]));
    } else {
        for (rIdx = 0; rIdx < slice->m_numRefIdx[1]; rIdx++) {
            cIdx = slice->m_RefPicListModification.m_refPicListModificationFlagL1 ? slice->m_RefPicListModification.m_RefPicSetIdxL1[rIdx] : (RK_U32)rIdx % numPocTotalCurr;
            mpp_assert(cIdx >= 0 && cIdx < numPocTotalCurr);
            slice->m_refPicList[1][rIdx] = rpsCurrList1[cIdx];
            slice->m_bIsUsedAsLongTerm[1][rIdx] = (cIdx >= numPocStCurr0 + numPocStCurr1);
        }
    }

    h265e_dbg_func("leave\n");
}

void h265e_slice_set_ref_poc_list(H265eSlice *slice)
{
    RK_S32 dir, numRefIdx;

    h265e_dbg_func("enter\n");

    for (dir = 0; dir < 2; dir++) {
        for (numRefIdx = 0; numRefIdx < slice->m_numRefIdx[dir]; numRefIdx++) {
            slice->m_refPOCList[dir][numRefIdx] = slice->m_refPicList[dir][numRefIdx]->poc;
        }
    }

    h265e_dbg_func("leave\n");
}

void h265e_slice_init(void *ctx, H265eSlice *slice, EncFrmStatus curr)
{
    H265eCtx *p = (H265eCtx *)ctx;
    H265eVps *vps = &p->vps;
    H265eSps *sps = &p->sps;
    H265ePps *pps = &p->pps;
    MppEncCfgSet *cfg = p->cfg;
    MppEncH265Cfg *codec = &cfg->codec.h265;

    h265e_dbg_func("enter\n");
    memset(slice, 0, sizeof(H265eSlice));
    slice->m_sps = sps;
    slice->m_vps = vps;
    slice->m_pps = pps;
    slice->m_numRefIdx[0] = 0;
    slice->m_numRefIdx[1] = 0;
    slice->m_colFromL0Flag = 1;
    slice->m_colRefIdx = 0;
    slice->m_bCheckLDC = 0;
    slice->m_sliceQpDeltaCb = 0;
    slice->m_sliceQpDeltaCr = 0;
    slice->m_maxNumMergeCand = 5;
    slice->m_bFinalized = 0;

    slice->m_cabacInitFlag = 0;
    slice->m_numEntryPointOffsets = 0;
    slice->m_enableTMVPFlag = sps->m_TMVPFlagsPresent;
    slice->m_picOutputFlag = 1;
    p->dpb->curr->is_key_frame = 0;
    if (curr.is_idr) {
        slice->m_sliceType = I_SLICE;
        p->dpb->curr->is_key_frame = 1;
        p->dpb->curr->status.is_intra = 1;
        p->dpb->gop_idx = 0;
    } else {
        slice->m_sliceType = P_SLICE;
        p->dpb->curr->status.is_intra = 0;
    }

    p->dpb->curr->status.val = curr.val;

    if (slice->m_sliceType  != B_SLICE)
        slice->is_referenced = 1;

    if (slice->m_pps->m_deblockingFilterOverrideEnabledFlag) {
        h265e_dbg_slice("to do in this case");
    } else {
        slice->m_deblockingFilterDisable = pps->m_picDisableDeblockingFilterFlag;
        slice->m_deblockingFilterBetaOffsetDiv2 = pps->m_deblockingFilterBetaOffsetDiv2;
        slice->m_deblockingFilterTcOffsetDiv2 = pps->m_deblockingFilterTcOffsetDiv2;
    }
    slice->m_saoEnabledFlag = codec->sao_cfg.slice_sao_luma_flag;
    slice->m_saoEnabledFlagChroma = codec->sao_cfg.slice_sao_chroma_flag;
    slice->m_maxNumMergeCand = codec->merge_cfg.max_mrg_cnd;
    slice->m_cabacInitFlag = codec->entropy_cfg.cabac_init_flag;
    slice->m_picOutputFlag = 1;
    slice->m_ppsId = pps->m_PPSId;
    if (slice->m_pps->m_bSliceChromaQpFlag) {
        slice->m_sliceQpDeltaCb = codec->trans_cfg.cb_qp_offset;
        slice->m_sliceQpDeltaCr = codec->trans_cfg.cr_qp_offset;
    }

    slice->poc = p->dpb->curr->seq_idx;
    slice->gop_idx = p->dpb->gop_idx;
    p->dpb->curr->gop_idx =  p->dpb->gop_idx++;
    p->dpb->curr->poc = slice->poc;
    if (curr.is_lt_ref)
        p->dpb->curr->is_long_term = 1;

    h265e_dbg_slice("slice->m_sliceType = %d slice->is_referenced = %d \n",
                    slice->m_sliceType, slice->is_referenced);
    h265e_dbg_func("leave\n");
}
