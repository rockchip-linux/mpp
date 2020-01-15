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

#define MODULE_TAG  "h265e_dpb"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "h265e_codec.h"
#include "h265e_dpb.h"

void h265e_gop_init(H265eRpsList *RpsList, H265eDpbCfg *cfg, int poc_cur)
{
    RK_S32 SliceTypeTemp = 0;
    RK_S32 i;

    h265e_dbg_func("enter\n");
    RpsList->gop_len = cfg->gop_len;
    RpsList->lt_num = cfg->nLongTerm;
    RpsList->vgop_size = cfg->vgop_size;
    RpsList->poc_cur_list = poc_cur;

    h265e_dbg_dpb("gop_init nLongTerm %d vgop_size %d ", cfg->nLongTerm, cfg->vgop_size);

    mpp_assert(RpsList->vgop_size <= H265_MAX_GOP);

    if (poc_cur % RpsList->vgop_size == 0) {
        for (i = 0; i < RpsList->vgop_size; i++) {
            RpsList->delta_poc_idx[i] = cfg->nDeltaPocIdx[i];
            h265e_dbg_dpb("nDeltaPocIdx[%d] = %d", i, cfg->nDeltaPocIdx[i]);
        }

        for (i = 0; i < RpsList->vgop_size; i++) {
            if (((poc_cur + i) % cfg->gop_len) == 0)
                SliceTypeTemp = I_SLICE;
            else
                SliceTypeTemp = P_SLICE;

            if (i != 0 && SliceTypeTemp == I_SLICE)
                break;
        }
    }
    h265e_dbg_func("leave\n");
}

void h265e_dpb_set_ref_list(H265eRpsList *RpsList, H265eReferencePictureSet *m_pRps)
{
    RK_S32 i;
    RK_S32 poc_cur_list = RpsList->poc_cur_list;
    RK_S32 gop_len = RpsList->gop_len;
    RK_S32 vgop_size = RpsList->vgop_size;
    RK_S32 lt_num = RpsList->lt_num;
    H265eRefPicListModification* refPicListModification = RpsList->m_RefPicListModification;

    h265e_dbg_func("enter\n");
    refPicListModification->m_refPicListModificationFlagL0 = 0;
    refPicListModification->m_refPicListModificationFlagL1 = 0;

    for (i = 0; i < REF_PIC_LIST_NUM_IDX; i ++) {
        refPicListModification->m_RefPicSetIdxL0[i] = 0;
        refPicListModification->m_RefPicSetIdxL0[i] = 0;
    }

    if (0 == lt_num || 1 == lt_num) {
        RK_S32 n = (poc_cur_list % gop_len) % vgop_size;
        RK_S32 ref_idx = -1;
        refPicListModification->m_refPicListModificationFlagL0 = 0;
        if (m_pRps->m_numberOfPictures > 1) {
            for (i = 0; i < m_pRps->m_numberOfPictures; i++) {
                if (m_pRps->delta_poc[i] == RpsList->delta_poc_idx[n]) {
                    ref_idx = i;
                    break;
                }
            }
            if (-1 == ref_idx) {
                mpp_err("Did not find the right reference picture");
                return;
            } else if (ref_idx != 0) {
                refPicListModification->m_refPicListModificationFlagL0 = 1;
                refPicListModification->m_RefPicSetIdxL0[0] = ref_idx;
                for ( i = 1; i < m_pRps->m_numberOfPictures - 1; i++) {
                    if (i != ref_idx)
                        refPicListModification->m_RefPicSetIdxL0[i] = i;
                }
                refPicListModification->m_RefPicSetIdxL0[ref_idx] = 0;
            }
        }
    } else if (lt_num == 2) {
        if ((poc_cur_list % gop_len) > 1 && m_pRps->m_numberOfPictures > 1) {
            refPicListModification->m_refPicListModificationFlagL0 = 0;
            if ((poc_cur_list % gop_len) % vgop_size == 0) {
                refPicListModification->m_refPicListModificationFlagL0 = 1;
                if ((poc_cur_list % gop_len) / vgop_size >= 2) {
                    if (((poc_cur_list % gop_len) / vgop_size) % 2 == 0) {
                        refPicListModification->m_refPicListModificationFlagL0 = 0;
                    } else if (((poc_cur_list % gop_len) / vgop_size) % 2 == 1) {
                        refPicListModification->m_RefPicSetIdxL0[0] = 1;
                        refPicListModification->m_RefPicSetIdxL0[1] = 0;
                    }
                }
            } else {
                RK_S32 n = (poc_cur_list % gop_len) % vgop_size;
                RK_S32 ref_idx = -1;
                RK_S32 deltaPoc[16] = { 0 };
                for (i = 0; i < m_pRps->num_negative_pic + m_pRps->num_positive_pic; i ++) {
                    deltaPoc[i] = m_pRps->delta_poc[i];
                }
                for (i = m_pRps->m_numberOfPictures - 1; i >= m_pRps->num_negative_pic + m_pRps->num_positive_pic; i--) {
                    deltaPoc[m_pRps->num_negative_pic + m_pRps->num_positive_pic + m_pRps->m_numberOfPictures - 1 - i] = m_pRps->delta_poc[i];
                }

                if (m_pRps->m_numberOfPictures > 1) {
                    for (i = 0; i < m_pRps->m_numberOfPictures; i++) {
                        if (deltaPoc[i] == RpsList->delta_poc_idx[n]) {
                            ref_idx = i;
                            break;
                        }
                    }
                    if (-1 == ref_idx) {
                        mpp_err("Did not find the right reference picture");
                        return;
                    } else if (ref_idx != 0) {
                        refPicListModification->m_refPicListModificationFlagL0 = 1;
                        refPicListModification->m_RefPicSetIdxL0[0] = ref_idx;
                        for (i = 1; i < m_pRps->m_numberOfPictures - 1; i++) {
                            if (i != ref_idx)
                                refPicListModification->m_RefPicSetIdxL0[i] = i;
                        }
                        refPicListModification->m_RefPicSetIdxL0[ref_idx] = 0;;
                    }
                }
            }
        } else {
            refPicListModification->m_refPicListModificationFlagL0 = 0;
        }
    } else if (lt_num == 3) {
        if ((poc_cur_list % gop_len) > 1 && m_pRps->m_numberOfPictures > 1) {
            refPicListModification->m_refPicListModificationFlagL0 = 0;
            if ((poc_cur_list % gop_len) % vgop_size == 0) {
                if ((poc_cur_list % gop_len) / vgop_size == 2) {
                    refPicListModification->m_refPicListModificationFlagL0 = 1;
                    refPicListModification->m_RefPicSetIdxL0[0] =  1;
                    refPicListModification->m_RefPicSetIdxL0[1] = 0;
                } else if ((poc_cur_list % gop_len) / vgop_size >= 3) {
                    if (((poc_cur_list % gop_len) / vgop_size) % 3 == 0) {
                        refPicListModification->m_refPicListModificationFlagL0 = 0;
                    } else if (((poc_cur_list % gop_len) / vgop_size) % 3 == 1) {
                        refPicListModification->m_refPicListModificationFlagL0 = 1;
                        refPicListModification->m_RefPicSetIdxL0[0] = 1;
                        refPicListModification->m_RefPicSetIdxL0[1] = 0;
                        refPicListModification->m_RefPicSetIdxL0[2] = 2;
                    } else if (((poc_cur_list % gop_len) / vgop_size) % 3 == 2) {
                        refPicListModification->m_refPicListModificationFlagL0 = 1;
                        refPicListModification->m_RefPicSetIdxL0[0] =  2;
                        refPicListModification->m_RefPicSetIdxL0[1] = 1;
                        refPicListModification->m_RefPicSetIdxL0[2] = 0;
                    }
                }
            }
        } else {
            refPicListModification->m_refPicListModificationFlagL0 = 0;
        }
    }
    refPicListModification->m_refPicListModificationFlagL1 = 0;

    h265e_dbg_func("leave\n");
}

MPP_RET h265e_dpb_set_cfg(H265eDpbCfg *dpb_cfg, MppEncCfgSet* cfg)
{
    MppEncGopRef *ref = &cfg->gop_ref;

    /*setup gop ref hierarchy (vgop) */
    if (!ref->gop_cfg_enable) {
        dpb_cfg->vgop_size = 1;
        return MPP_OK;
    }

    RK_S32 i = 0;
    RK_S32 st_gop_len   = ref->ref_gop_len;
    dpb_cfg->nLongTerm = 1;
    dpb_cfg->vgop_size = st_gop_len;
    for (i = 0; i < st_gop_len + 1; i++) {
        MppGopRefInfo *info = &ref->gop_info[i];
        RK_S32 is_non_ref   = info->is_non_ref;
        RK_S32 is_lt_ref    = info->is_lt_ref;
        RK_S32 temporal_id  = info->temporal_id;
        RK_S32 lt_idx       = info->lt_idx;
        RK_S32 ref_idx      = info->ref_idx;

        dpb_cfg->ref_inf[i].is_intra = (i == 0) ? (1) : (0);
        dpb_cfg->ref_inf[i].is_non_ref = is_non_ref;
        dpb_cfg->ref_inf[i].is_lt_ref = is_lt_ref;
        dpb_cfg->ref_inf[i].lt_idx = lt_idx;
        dpb_cfg->ref_inf[i].temporal_id = temporal_id;
        dpb_cfg->ref_inf[i].ref_dist = ref_idx - i;
        dpb_cfg->nDeltaPocIdx[i] = ref_idx - i;
    }
    return MPP_OK;
}

/* get buffer at init */
MPP_RET h265e_dpb_init_curr(H265eDpb *dpb, H265eDpbFrm *frm)
{
    h265e_dbg_func("enter\n");
    mpp_assert(!frm->on_used);

    frm->dpb = dpb;

    if (!frm->slice) {
        frm->slice = mpp_calloc(H265eSlice, 1);
    }

    frm->inited = 1;
    frm->on_used = 1;
    frm->seq_idx = dpb->seq_idx;
    dpb->seq_idx++;

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

MPP_RET h265e_dpb_get_curr(H265eDpb *dpb)
{
    RK_U32 i;

    h265e_dbg_func("enter\n");
    for (i = 0; i < MPP_ARRAY_ELEMS(dpb->frame_list); i++) {
        if (!dpb->frame_list[i].on_used) {
            dpb->curr = &dpb->frame_list[i];
            h265e_dbg_dpb("get free dpb slot_index %d", dpb->curr->slot_idx);
            break;
        }
    }
    h265e_dpb_init_curr(dpb, dpb->curr);
    h265e_dbg_func("leave\n");
    return MPP_OK;
}

/* put buffer at deinit */
MPP_RET h265e_dpb_frm_deinit(H265eDpbFrm *frm)
{

    MPP_FREE(frm->slice);
    frm->inited = 0;
    h265e_dbg_func("leave\n");
    return MPP_OK;
}

MPP_RET h265e_dpb_init(H265eDpb **dpb, H265eDpbCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H265eDpb *p = NULL;
    RK_U32 i;

    h265e_dbg_func("enter\n");
    if (NULL == dpb || NULL == cfg) {
        mpp_err_f("invalid parameter %p %p\n", dpb, cfg);
        return MPP_ERR_VALUE;
    }

    h265e_dbg_dpb("max  ref frm num %d\n", cfg->maxNumReferences);
    h265e_dbg_dpb("bBPyramid %d\n", cfg->bBPyramid);
    h265e_dbg_dpb("nLongTerm %d\n", cfg->nLongTerm);

    if (cfg->maxNumReferences < 0 || cfg->maxNumReferences > MAX_REFS) {
        return MPP_ERR_VALUE;
    }

    p = mpp_calloc_size(H265eDpb, sizeof(H265eDpb));

    if (NULL == p)
        return MPP_ERR_MALLOC;

    p->last_idr = 0;
    p->poc_cra = 0;
    p->max_ref_l0 = cfg->maxNumReferences;
    p->max_ref_l1 = cfg->bBPyramid ? 2 : 1;
    p->is_open_gop = cfg->bOpenGOP;
    p->is_long_term = cfg->nLongTerm > 0;
    p->idr_gap = 30;
    p->cfg = cfg;
    H265eRpsList *rps_list = &p->RpsList;

    rps_list->lt_num = 0;
    rps_list->st_num = 0;

    memset(rps_list->poc, 0, sizeof(rps_list->poc));

    rps_list->m_RefPicListModification = mpp_calloc(H265eRefPicListModification, 1);


    for (i = 0; i < MPP_ARRAY_ELEMS(p->frame_list); i++)
        p->frame_list[i].slot_idx = i;

    mpp_assert(dpb);
    *dpb = p;
    h265e_dbg_func("leave\n");
    return ret;
}

MPP_RET h265e_dpb_deinit(H265eDpb *dpb)
{
    RK_U32 i;

    h265e_dbg_func("enter\n");
    for (i = 0; i < MPP_ARRAY_ELEMS(dpb->frame_list); i++) {
        if (dpb->frame_list[i].inited)
            h265e_dpb_frm_deinit(&dpb->frame_list[i]);
    }

    MPP_FREE(dpb->RpsList.m_RefPicListModification);

    MPP_FREE(dpb);

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

enum NALUnitType getNalUnitType(H265eDpb *dpb, int curPOC)
{

    h265e_dbg_func("enter\n");
    if (curPOC == 0) {
        return NAL_IDR_W_RADL;
    }
    if (dpb->curr->is_key_frame) {
        if (dpb->is_open_gop) {
            return NAL_CRA_NUT;
        } else {
            return NAL_IDR_W_RADL;
        }
    }
    if (dpb->poc_cra > 0) {
        if (curPOC < dpb->poc_cra) {
            // All leading pictures are being marked as TFD pictures here since current encoder uses all
            // reference pictures while encoding leading pictures. An encoder can ensure that a leading
            // picture can be still decodable when random accessing to a CRA/CRANT/BLA/BLANT picture by
            // controlling the reference pictures used for encoding that leading picture. Such a leading
            // picture need not be marked as a TFD picture.
            return NAL_RASL_R;
        }
    }
    if (dpb->last_idr > 0) {
        if (curPOC < dpb->last_idr) {
            return NAL_RADL_R;
        }
    }

    h265e_dbg_func("leave\n");
    return NAL_TRAIL_R;
}

static inline int getLSB(int poc, int maxLSB)
{
    if (poc >= 0) {
        return poc % maxLSB;
    } else {
        return (maxLSB - ((-poc) % maxLSB)) % maxLSB;
    }
}

void sort_delta_poc(H265eReferencePictureSet *rps)
{
    // sort in increasing order (smallest first)
    RK_S32 j, k;
    for (j = 1; j < rps->m_numberOfPictures; j++) {
        RK_S32 deltaPOC = rps->delta_poc[j];
        RK_U32 used = rps->m_used[j];
        for (k = j - 1; k >= 0; k--) {
            int temp = rps->delta_poc[k];
            if (deltaPOC < temp) {
                rps->delta_poc[k + 1] = temp;
                rps->m_used[k + 1] =  rps->m_used[k];
                rps->delta_poc[k] = deltaPOC;
                rps->m_used[k] = used;
            }
        }
    }

    // flip the negative values to largest first
    RK_S32 numNegPics =  rps->num_negative_pic;
    for (j = 0, k = numNegPics - 1; j < numNegPics >> 1; j++, k--) {
        RK_S32 deltaPOC = rps->delta_poc[j];
        RK_U32 used = rps->m_used[j];
        rps->delta_poc[j] = rps->delta_poc[k];
        rps->m_used[j] = rps->m_used[k];
        rps->delta_poc[k] =  deltaPOC;
        rps->m_used[k] = used;
    }
}


void h265e_dpb_apply_rps(H265eDpb *dpb, H265eReferencePictureSet *rps, int curPoc)
{
    H265eDpbFrm *outPic = NULL;
    RK_S32 i, isReference;
    // loop through all pictures in the reference picture buffer
    RK_U32 index = 0;
    H265eDpbFrm *frame_list = &dpb->frame_list[0];

    h265e_dbg_func("enter\n");
    for (index = 0; index < MPP_ARRAY_ELEMS(dpb->frame_list); index++) {
        outPic = &frame_list[index];
        if (!outPic->inited || !outPic->slice->is_referenced) {
            continue;
        }

        isReference = 0;
        // loop through all pictures in the Reference Picture Set
        // to see if the picture should be kept as reference picture
        for (i = 0; i < rps->num_positive_pic + rps->num_negative_pic; i++) {
            h265e_dbg_dpb("outPic->slice->poc %d,curPoc %d dealt %d", outPic->slice->poc, curPoc, rps->delta_poc[i]);
            if (!outPic->is_long_term && outPic->slice->poc == curPoc + rps->delta_poc[i]) {
                isReference = 1;
                outPic->used_by_cur = (rps->m_used[i] == 1);
                outPic->is_long_term = 0;
            }
        }

        for (; i < rps->m_numberOfPictures; i++) {
            if (rps->check_lt_msb[i] == 0) {
                if (outPic->is_long_term && (outPic->slice->poc == rps->m_RealPoc[i])) {
                    isReference = 1;
                    outPic->used_by_cur = (rps->m_used[i] == 1);
                }
            } else {
                if (outPic->is_long_term && (outPic->slice->poc == rps->m_RealPoc[i])) {
                    isReference = 1;
                    outPic->used_by_cur = (rps->m_used[i] == 1);
                }
            }
        }

        // mark the picture as "unused for reference" if it is not in
        // the Reference Picture Set
        if (outPic->slice->poc != curPoc && isReference == 0) {
            h265e_dbg_dpb("free unreference buf poc %d", outPic->slice->poc);
            outPic->slice->is_referenced = 0;
            outPic->used_by_cur = 0;
            outPic->on_used = 0;
            outPic->is_long_term = 0;
        }
    }
    h265e_dbg_func("leave\n");
}

void h265e_dpb_dec_refresh_marking(H265eDpb *dpb, RK_S32 poc_cur, enum NALUnitType nalUnitType)
{
    RK_U32 index = 0;

    h265e_dbg_func("enter\n");
    if (nalUnitType == NAL_BLA_W_LP
        || nalUnitType == NAL_BLA_W_RADL
        || nalUnitType == NAL_BLA_N_LP
        || nalUnitType == NAL_IDR_W_RADL
        || nalUnitType == NAL_IDR_N_LP) { // IDR or BLA picture
        // mark all pictures as not used for reference
        H265eDpbFrm *frame_List = &dpb->frame_list[0];
        for (index = 0; index < MPP_ARRAY_ELEMS(dpb->frame_list); index++) {
            H265eDpbFrm *frame = &frame_List[index];
            if (frame->inited && (frame->poc != poc_cur)) {
                frame->slice->is_referenced = 0;
                frame->is_long_term = 0;
                if (frame->poc < poc_cur) {
                    frame->used_by_cur = 0;
                    frame->on_used = 0;
                }
            }
        }

        if (nalUnitType == NAL_BLA_W_LP
            || nalUnitType == NAL_BLA_W_RADL
            || nalUnitType == NAL_BLA_N_LP) {
            dpb->poc_cra = poc_cur;
        }
    } else { // CRA or No DR
        if (dpb->refresh_pending == 1 && poc_cur > dpb->poc_cra) { // CRA reference marking pending
            H265eDpbFrm *frame_list = &dpb->frame_list[0];
            for (index = 0; index < MPP_ARRAY_ELEMS(dpb->frame_list); index++) {

                H265eDpbFrm *frame = &frame_list[index];
                if (frame->inited && frame->poc != poc_cur && frame->poc != dpb->poc_cra) {
                    frame->slice->is_referenced = 0;
                    frame->on_used = 0;
                }
            }

            dpb->refresh_pending = 0;
        }
        if (nalUnitType == NAL_CRA_NUT) { // CRA picture found
            dpb->refresh_pending = 1;
            dpb->poc_cra = poc_cur;
        }
    }
    h265e_dbg_func("leave\n");
}

// Function will arrange the long-term pictures in the decreasing order of poc_lsb_lt,
// and among the pictures with the same lsb, it arranges them in increasing delta_poc_msb_cycle_lt value
void h265e_dpb_arrange_lt_rps(H265eDpb *dpb, H265eSlice *slice)
{
    H265eReferencePictureSet *rps = slice->m_rps;
    RK_U32 tempArray[MAX_REFS];
    RK_S32 offset = rps->num_negative_pic + rps->num_positive_pic;
    RK_S32 i, j, ctr = 0;
    RK_S32 maxPicOrderCntLSB = 1 << slice->m_sps->m_bitsForPOC;
    RK_S32 numLongPics;
    RK_S32 currMSB = 0, currLSB = 0;

    // Arrange long-term reference pictures in the correct order of LSB and MSB,
    // and assign values for pocLSBLT and MSB present flag
    RK_S32 longtermPicsPoc[MAX_REFS], longtermPicsLSB[MAX_REFS], indices[MAX_REFS];
    RK_S32 longtermPicsRealPoc[MAX_REFS];
    RK_S32 longtermPicsMSB[MAX_REFS];
    RK_U32 mSBPresentFlag[MAX_REFS];

    h265e_dbg_func("enter\n");
    if (!rps->num_long_term_pic) {
        return;
    }
    memset(longtermPicsPoc, 0, sizeof(longtermPicsPoc));  // Store POC values of LTRP
    memset(longtermPicsLSB, 0, sizeof(longtermPicsLSB));  // Store POC LSB values of LTRP
    memset(longtermPicsMSB, 0, sizeof(longtermPicsMSB));  // Store POC LSB values of LTRP
    memset(longtermPicsRealPoc, 0, sizeof(longtermPicsRealPoc));
    memset(indices, 0, sizeof(indices));                  // Indices to aid in tracking sorted LTRPs
    memset(mSBPresentFlag, 0, sizeof(mSBPresentFlag));    // Indicate if MSB needs to be present

    // Get the long-term reference pictures

    for (i = rps->m_numberOfPictures - 1; i >= offset; i--, ctr++) {
        longtermPicsPoc[ctr] = rps->poc[i];                                  // LTRP POC
        longtermPicsRealPoc[ctr] = rps->m_RealPoc[i];
        longtermPicsLSB[ctr] = getLSB(longtermPicsPoc[ctr], maxPicOrderCntLSB); // LTRP POC LSB
        indices[ctr] = i;
        longtermPicsMSB[ctr] = longtermPicsPoc[ctr] - longtermPicsLSB[ctr];
    }

    numLongPics = rps->num_long_term_pic;
    mpp_assert(ctr == numLongPics);

    // Arrange pictures in decreasing order of MSB;
    for (i = 0; i < numLongPics; i++) {
        for (j = 0; j < numLongPics - 1; j++) {
            if (longtermPicsMSB[j] < longtermPicsMSB[j + 1]) {
                MPP_SWAP(RK_S32, longtermPicsPoc[j], longtermPicsPoc[j + 1]);
                MPP_SWAP(RK_S32, longtermPicsRealPoc[j], longtermPicsRealPoc[j + 1]);
                MPP_SWAP(RK_S32, longtermPicsLSB[j], longtermPicsLSB[j + 1]);
                MPP_SWAP(RK_S32, longtermPicsMSB[j], longtermPicsMSB[j + 1]);
                MPP_SWAP(RK_S32, indices[j], indices[j + 1]);
            }
        }
    }

    for (i = 0; i < numLongPics; i++) {
        if ((slice->poc % dpb->idr_gap) / maxPicOrderCntLSB > 0) {
            mSBPresentFlag[i] = 1;
        }
    }

    // tempArray for usedByCurr flag
    memset(tempArray, 0, sizeof(tempArray));
    for (i = 0; i < numLongPics; i++) {
        tempArray[i] = rps->m_used[indices[i]] ? 1 : 0;
    }

    // Now write the final values;
    ctr = 0;
    // currPicPoc = currMSB + currLSB
    currLSB = getLSB(slice->poc % dpb->idr_gap, maxPicOrderCntLSB);
    currMSB = (slice->poc % dpb->idr_gap) - currLSB;

    for (i = rps->m_numberOfPictures - 1; i >= offset; i--, ctr++) {
        rps->poc[i] = longtermPicsPoc[ctr];
        rps->delta_poc[i] = -(slice->poc % dpb->idr_gap) + longtermPicsPoc[ctr];
        rps->m_used[i] = tempArray[ctr];
        rps->m_pocLSBLT[i] = longtermPicsLSB[ctr];
        rps->m_deltaPOCMSBCycleLT[i] = (currMSB - (longtermPicsPoc[ctr] - longtermPicsLSB[ctr])) / maxPicOrderCntLSB;
        rps->m_deltaPocMSBPresentFlag[i] = mSBPresentFlag[ctr];

        mpp_assert(rps->m_deltaPOCMSBCycleLT[i] >= 0); // Non-negative value
    }

    for (i = rps->m_numberOfPictures - 1, ctr = 1; i >= offset; i--, ctr++) {
        for (j = rps->m_numberOfPictures - 1 - ctr; j >= offset; j--) {
            // Here at the encoder we know that we have set the full POC value for the LTRPs, hence we
            // don't have to check the MSB present flag values for this constraint.
            mpp_assert(rps->m_RealPoc[i] != rps->m_RealPoc[j]); // If assert fails, LTRP entry repeated in RPS!!!
        }
    }

    h265e_dbg_func("leave\n");
}

void h265e_dpb_compute_rps(H265eDpb *dpb, RK_S32 curPoc, H265eSlice *slice, RK_U32 isTsvcAndLongterm)
{

    H265eRpsList *RpsList = &dpb->RpsList;
    RK_S32 i, j, k, last_index = MAX_REFS - 1;
    RK_S32 numLongTermRefPic = 0;
    RK_S32 nLongTermRefPicPoc[MAX_NUM_LONG_TERM_REF_PIC_POC];
    RK_S32 nLongTermRefPicRealPoc[MAX_NUM_LONG_TERM_REF_PIC_POC];
    RK_U32 isMsbValid[MAX_NUM_LONG_TERM_REF_PIC_POC];
    RK_U32 isShortTermValid[MAX_REFS];
    memset(isShortTermValid, 1, sizeof(RK_U32)*MAX_REFS);
    RK_S32 nShortTerm = 0;

    h265e_dbg_func("enter\n");
    if (isTsvcAndLongterm) {
        memset(&slice->m_localRPS, 0, sizeof(H265eReferencePictureSet));
        H265eReferencePictureSet* m_pRPS = (H265eReferencePictureSet*)&slice->m_localRPS;

        RK_S32 *nRefPicDeltaPoc = mpp_malloc(RK_S32, RpsList->vgop_size);
        RK_S32** pRefPicDeltaPoc = mpp_malloc(RK_S32*, RpsList->vgop_size);

        for (i = 0; i < RpsList->vgop_size; i++) {
            pRefPicDeltaPoc[i] = mpp_malloc(RK_S32, RpsList->vgop_size);
        }
        nRefPicDeltaPoc[0] = 0;
        pRefPicDeltaPoc[0][0] = 0;

        if ((RpsList->poc_cur_list % dpb->idr_gap)) {
            if (RpsList->lt_num || RpsList->poc_cur_list >= RpsList->vgop_size) {
                nRefPicDeltaPoc[0] = 1;
                pRefPicDeltaPoc[0][0] = -RpsList->vgop_size;
            } else {
                nRefPicDeltaPoc[0] = 1;
                pRefPicDeltaPoc[0][0] = RpsList->delta_poc_idx[0];
            }
        }

        for (i = 1; i < RpsList->vgop_size; i++) {
            nRefPicDeltaPoc[i] = 1;
            pRefPicDeltaPoc[i][0] = RpsList->delta_poc_idx[i];
            for ( j = i + 1; j < RpsList->vgop_size; j++) {
                if (RpsList->delta_poc_idx[j] + j < i) {
                    RK_U32 isSame = 0;
                    for ( k = 0; k < nRefPicDeltaPoc[i]; k++) {
                        if (pRefPicDeltaPoc[i][k] == RpsList->delta_poc_idx[j] + j - i) {
                            isSame = 1;
                            break;
                        }
                    }
                    if (!isSame) {
                        pRefPicDeltaPoc[i][nRefPicDeltaPoc[i]] = RpsList->delta_poc_idx[j] + j - i;
                        nRefPicDeltaPoc[i]++;
                    }
                }
            }
        }

        for (i = 0; i < RpsList->vgop_size; i++) {
            RK_U32 isSame = 0;
            for (j = 0; j < nRefPicDeltaPoc[i]; j++) {
                if (i && pRefPicDeltaPoc[i][j] == -i) {
                    isSame = 1;
                    break;
                }
            }

            if (!isSame && i) {
                pRefPicDeltaPoc[i][nRefPicDeltaPoc[i]++] = -i;
            }
            if (nRefPicDeltaPoc[i] + (RpsList->lt_num < 2 ? 0 : RpsList->lt_num - 1) > 4) {
                mpp_err("Error: Reference picture in DPB is out of range, do not be more than 4, exit now");
                return;
            }
        }
        if (RpsList->lt_num <= 2) {
            RK_S32 idx_rps, n;
            for (idx_rps = 0; idx_rps < 16; idx_rps++) {
                m_pRPS->delta_poc[idx_rps] = 0;
                m_pRPS->m_used[idx_rps] = 0;
            }
            //for (int i = 0; i < nRefPicDeltaPoc[(RpsList->pocCurrInList % m_nIdrGap) % RpsList->vgop_size]; i++)
            //{
            //  m_pRPS->m_deltaPOC[i] = pRefPicDeltaPoc[(RpsList->pocCurrInList % m_nIdrGap) % RpsList->vgop_size][i];
            //  m_pRPS->m_used[i] = true;
            //}
            n = (RpsList->poc_cur_list % dpb->idr_gap) % RpsList->vgop_size;
            if (n > 1) {
                for (i = 0; i < nRefPicDeltaPoc[n] - 1; i++) {
                    for (j = i + 1; j < nRefPicDeltaPoc[n]; j++) {
                        if (pRefPicDeltaPoc[n][j] > pRefPicDeltaPoc[n][i]) {
                            RK_S32 m = pRefPicDeltaPoc[n][i];
                            pRefPicDeltaPoc[n][i] = pRefPicDeltaPoc[n][j];
                            pRefPicDeltaPoc[n][j] = m;
                        }
                    }
                    m_pRPS->delta_poc[i] = pRefPicDeltaPoc[n][i];
                    m_pRPS->m_used[i] = 1;
                }
                m_pRPS->delta_poc[nRefPicDeltaPoc[n] - 1] = pRefPicDeltaPoc[n][nRefPicDeltaPoc[n] - 1];
                m_pRPS->m_used[nRefPicDeltaPoc[n] - 1] = 1;
            } else {
                m_pRPS->delta_poc[0] = pRefPicDeltaPoc[n][0];
                m_pRPS->m_used[0] = 1;
            }

            m_pRPS->m_numberOfPictures = nRefPicDeltaPoc[(RpsList->poc_cur_list % dpb->idr_gap) % RpsList->vgop_size];
            m_pRPS->num_negative_pic = nRefPicDeltaPoc[(RpsList->poc_cur_list % dpb->idr_gap) % RpsList->vgop_size];
        }
        MPP_FREE(nRefPicDeltaPoc);
        for (i = 0; i < RpsList->vgop_size; i++) {
            MPP_FREE(pRefPicDeltaPoc[i]);
        }
        MPP_FREE(pRefPicDeltaPoc);

        H265eDpbFrm *pic = &dpb->frame_list[last_index]; // m_picList.last(); TODO
        if (RpsList->lt_num > 2) {
            RK_S32 idx;
            RK_S32 nDeltaPoc[MAX_REFS] = { 0 };

            m_pRPS->num_negative_pic = 1;
            if (0 == ((curPoc % dpb->idr_gap) % RpsList->vgop_size)) {
                m_pRPS->num_negative_pic = 0;
            }
            m_pRPS->num_positive_pic = 0;
            for (idx = 0; idx < m_pRPS->num_negative_pic; idx++) {
                m_pRPS->delta_poc[idx] = RpsList->delta_poc_idx[(curPoc % dpb->idr_gap) % RpsList->vgop_size];
                nDeltaPoc[idx] = RpsList->delta_poc_idx[(curPoc % dpb->idr_gap) % RpsList->vgop_size];
            }
            for (idx = m_pRPS->num_negative_pic; idx < MAX_REFS; idx++) {
                m_pRPS->delta_poc[idx] = 0;
            }
            memset(isShortTermValid, 0, sizeof(RK_U32)*MAX_REFS);
            nShortTerm = m_pRPS->num_negative_pic + m_pRPS->num_positive_pic;
            for (i = last_index; i >= 0; i++) {

                pic = &dpb->frame_list[i];
                if (pic->on_used && !pic->is_long_term && pic->slice->is_referenced && pic->poc != curPoc) {
                    for (i = 0; i < nShortTerm; i++) {
                        if (pic->poc == curPoc + m_pRPS->delta_poc[i]) {
                            isShortTermValid[i] = 1;
                        }
                    }
                }
            }
            memset(m_pRPS->delta_poc, 0, MAX_REFS * sizeof(RK_S32));
            m_pRPS->num_negative_pic = 0;
            for (idx = 0; idx < 4; idx++) {
                if (isShortTermValid[idx]) {
                    m_pRPS->delta_poc[m_pRPS->num_negative_pic] = nDeltaPoc[idx];
                    m_pRPS->m_used[m_pRPS->num_negative_pic] = 1;
                    m_pRPS->num_negative_pic++;
                }
            }
            memset(isShortTermValid, 1, sizeof(RK_U32)*MAX_REFS);
            memset(isMsbValid, 0, sizeof(RK_U32)*MAX_NUM_LONG_TERM_REF_PIC_POC);
        }
        m_pRPS->m_interRPSPrediction = 0;

        nShortTerm = m_pRPS->num_negative_pic + m_pRPS->num_positive_pic;
        h265e_dbg_dpb("nShortTerm = %d", nShortTerm);
        for (i = last_index; i >= 0; i--) {
            pic = &dpb->frame_list[i]; //m_picList.last(); TODO
            if (pic->on_used && pic->is_long_term && pic->slice->is_referenced && pic->poc != curPoc &&
                (curPoc - pic->poc <= RpsList->vgop_size * RpsList->lt_num || 0 == RpsList->lt_num)) {
                nLongTermRefPicRealPoc[numLongTermRefPic] = pic->poc;
                nLongTermRefPicPoc[numLongTermRefPic] = (pic->poc % dpb->idr_gap);
                isMsbValid[numLongTermRefPic] = (pic->poc % dpb->idr_gap) >= (RK_U32)(1 << slice->m_sps->m_bitsForPOC);
                numLongTermRefPic++;

                h265e_dbg_dpb("numLongTermRefPic = %d \n", numLongTermRefPic);
                for ( j = 0; j < nShortTerm; j++) {
                    if (pic->poc == curPoc + m_pRPS->delta_poc[j]) {
                        isShortTermValid[j] = 0;
                    }
                }
            }

            if (pic->on_used && !pic->is_long_term && !pic->slice->is_referenced && pic->poc != curPoc) {
                for ( j = 0; j < nShortTerm; j++) {
                    if (pic->poc == curPoc + m_pRPS->delta_poc[j]) {
                        isShortTermValid[j] = 0;
                    }
                }
            }
        }

        if (nShortTerm) {
            RK_S32 deltaPoc[16] = { 0 };
            RK_S32 nInValid = 0;

            for (i = 0; i < MAX_REFS; i++) {
                deltaPoc[i] = m_pRPS->delta_poc[i];
            }
            for (i = 0; i < MAX_REFS; i++) {
                if (isShortTermValid[i]) {
                    m_pRPS->delta_poc[nInValid] = deltaPoc[i];
                    nInValid++;
                }
            }
            nInValid = 0;
            for (i = 0; i < nShortTerm; i++) {
                if (!isShortTermValid[i]) {
                    nInValid++;
                }
            }
            for (i = nShortTerm - nInValid; i < nShortTerm; i++) {
                m_pRPS->delta_poc[i] = 0;
                m_pRPS->m_used[i] = 0;
            }
            m_pRPS->num_negative_pic = 0;
            m_pRPS->num_positive_pic = 0;
            for (i = 0; i < nShortTerm - nInValid; i++) {
                if (m_pRPS->delta_poc[i] < 0) {
                    m_pRPS->num_negative_pic++;
                }
                if (m_pRPS->delta_poc[i] > 0) {
                    m_pRPS->num_positive_pic++;
                }
            }
        }

        nShortTerm = m_pRPS->num_negative_pic + m_pRPS->num_positive_pic;
        mpp_assert(nShortTerm < MAX_REFS);
        if (nShortTerm + numLongTermRefPic >= MAX_REFS) {
            numLongTermRefPic = MAX_REFS - 1 - nShortTerm;
        }
        m_pRPS->num_long_term_pic = numLongTermRefPic;
        m_pRPS->m_numberOfPictures = m_pRPS->num_negative_pic
                                     + m_pRPS->num_positive_pic + m_pRPS->num_long_term_pic;
        for ( i = 0; i < numLongTermRefPic; i++) {
            h265e_dbg_dpb("numLongTermRefPic %d nShortTerm %d", numLongTermRefPic, nShortTerm);
            m_pRPS->poc[i + nShortTerm] = nLongTermRefPicPoc[i];
            m_pRPS->m_RealPoc[i + nShortTerm] = nLongTermRefPicRealPoc[i];
            m_pRPS->m_used[i + nShortTerm] = 1;
            m_pRPS->check_lt_msb[i + nShortTerm] = isMsbValid[i];
        }
        if (slice->m_sliceType == I_SLICE) {
            m_pRPS->m_interRPSPrediction = 0;
            m_pRPS->num_long_term_pic = 0;
            m_pRPS->num_negative_pic = 0;
            m_pRPS->num_positive_pic = 0;
            m_pRPS->m_numberOfPictures = 0;
        }

        slice->m_rps = m_pRPS;
        slice->m_bdIdx = -1;

        h265e_dpb_apply_rps(dpb, slice->m_rps, curPoc);
        h265e_dpb_arrange_lt_rps(dpb, slice);
        h265e_dpb_set_ref_list(RpsList, m_pRPS);
        memcpy(&slice->m_RefPicListModification, RpsList->m_RefPicListModification,
               sizeof(H265eRefPicListModification));
    } else {
        //slice->isIRAP(), slice->getLocalRPS(), slice->getSPS()->getMaxDecPicBuffering(0), bVaryDltFrmNum
        RK_U32 isRAP = (slice->m_nalUnitType >= 16) && (slice->m_nalUnitType <= 23);
        H265eReferencePictureSet * rps = &slice->m_localRPS;
        RK_U32 maxDecPicBuffer = slice->m_sps->m_maxDecPicBuffering[0];
        RK_U32 bVaryDltFrmNum = 0;
        RK_S32 idx = 0;
        if (dpb->is_long_term) {

            RK_S32 nDeltaPoc[MAX_REFS] = { 0 };
            H265eDpbFrm *rpcPic;

            rps->num_negative_pic = 0;
            rps->num_positive_pic = 0;
            for (idx = 0; idx < rps->num_negative_pic; idx++) {
                rps->delta_poc[idx] = -(idx + 1);
                nDeltaPoc[idx] = -(idx + 1);
            }
            for (idx = rps->num_negative_pic; idx < MAX_REFS; idx++) {
                rps->delta_poc[idx] = 0;
            }
            memset(isShortTermValid, 0, sizeof(RK_U32)*MAX_REFS);
            nShortTerm = rps->num_negative_pic + rps->num_positive_pic;

            for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dpb->frame_list); i++) {
                rpcPic = &dpb->frame_list[i];
                if (rpcPic->on_used && !rpcPic->is_long_term && rpcPic->slice->is_referenced
                    && rpcPic->poc != curPoc) {
                    for (j = 0; j < nShortTerm; j++) {
                        if (rpcPic->poc == curPoc + rps->delta_poc[j]) {
                            isShortTermValid[j] = 1;
                        }
                    }
                }
            }
            memset(rps->delta_poc, 0, MAX_REFS * sizeof(int));
            rps->num_negative_pic = 0;
            for (idx = 0; idx < 4; idx++) {
                if (isShortTermValid[idx]) {
                    rps->delta_poc[rps->num_negative_pic] = nDeltaPoc[idx];
                    rps->m_used[rps->num_negative_pic] = 1;
                    rps->num_negative_pic++;
                }
            }
            memset(isShortTermValid, 1, sizeof(RK_U32)*MAX_REFS);
            memset(isMsbValid, 0, sizeof(RK_U32)*MAX_NUM_LONG_TERM_REF_PIC_POC);
            rps->m_interRPSPrediction = 0;
            nShortTerm = rps->num_negative_pic + rps->num_positive_pic;

            for (i = 0; i < MAX_REFS; i++) {
                rpcPic = &dpb->frame_list[i];
                if (rpcPic->on_used && rpcPic->is_long_term && rpcPic->slice->is_referenced
                    && rpcPic->poc != curPoc) {
                    nLongTermRefPicRealPoc[numLongTermRefPic] = rpcPic->poc;

                    nLongTermRefPicPoc[numLongTermRefPic] = (rpcPic->poc % dpb->idr_gap);

                    isMsbValid[numLongTermRefPic] = (rpcPic->poc % (RK_S32)dpb->idr_gap) >=
                                                    (RK_S32)(1 << rpcPic->slice->m_sps->m_bitsForPOC);

                    numLongTermRefPic++;

                    for ( j = 0; j < nShortTerm; j++) {
                        if (rpcPic->poc == curPoc + rps->delta_poc[j]) {
                            isShortTermValid[j] = 0;
                        }
                    }
                }

                if (!rpcPic->is_long_term && !rpcPic->slice->is_referenced && rpcPic->poc != curPoc) {
                    for (j = 0; j < nShortTerm; j++) {
                        if (rpcPic->poc == curPoc + rps->delta_poc[j]) {
                            isShortTermValid[j] = 0;
                        }
                    }
                }
            }

            if (nShortTerm) {
                RK_S32 deltaPoc[16] = { 0 };
                for ( i = 0; i < MAX_REFS; i++) {
                    deltaPoc[i] = rps->delta_poc[i];
                }
                int nInValid = 0;
                for ( i = 0; i < MAX_REFS; i++) {
                    if (isShortTermValid[i]) {
                        rps->delta_poc[nInValid] = deltaPoc[i];
                        nInValid++;
                    }
                }
                nInValid = 0;
                for ( i = 0; i < nShortTerm; i++) {
                    if (!isShortTermValid[i]) {
                        nInValid++;
                    }
                }
                for (i = nShortTerm - nInValid; i < nShortTerm; i++) {
                    rps->delta_poc[i] = 0;
                    rps->m_used[i] = 0;
                }
                rps->num_negative_pic = 0;
                rps->num_positive_pic = 0;
                for (i = 0; i < nShortTerm - nInValid; i++) {
                    if (rps->delta_poc[i] < 0) {
                        rps->num_negative_pic++;
                    }
                    if (rps->delta_poc[i] > 0) {
                        rps->num_positive_pic++;
                    }
                }
            }

            nShortTerm = rps->num_negative_pic + rps->num_positive_pic;
            mpp_assert(nShortTerm < MAX_REFS);
            if (nShortTerm + numLongTermRefPic >= MAX_REFS) {
                numLongTermRefPic = MAX_REFS - 1 - nShortTerm;
            }
            rps->num_long_term_pic = numLongTermRefPic;
            rps->m_numberOfPictures = rps->num_negative_pic
                                      + rps->num_positive_pic + rps->num_long_term_pic;
            for (i = 0; i < numLongTermRefPic; i++) {
                rps->poc[i + nShortTerm] = nLongTermRefPicPoc[i];
                rps->m_RealPoc[i + nShortTerm] = nLongTermRefPicRealPoc[i];
                rps->m_used[i + nShortTerm] = 1;
                rps->check_lt_msb[i + nShortTerm] = isMsbValid[i];
            }

            if (slice->m_sliceType == I_SLICE) {
                rps->m_interRPSPrediction = 0;
                rps->num_long_term_pic = 0;
                rps->num_negative_pic = 0;
                rps->num_positive_pic = 0;
                rps->m_numberOfPictures = 0;
            }
        } else {
            RK_U32 poci = 0, numNeg = 0, numPos = 0;
            RK_S32 dealtIdx = -1;
            for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dpb->frame_list); i++) {
                H265eDpbFrm *rpcPic = &dpb->frame_list[i];
                h265e_dbg_dpb("rpcPic->inited = %d maxDecPicBuffer %d", rpcPic->inited, maxDecPicBuffer);
                if (rpcPic->inited && poci < maxDecPicBuffer - 1) {
                    h265e_dbg_dpb("curpoc %d rm_poc %d is_referenced %d", curPoc, rpcPic->poc, rpcPic->slice->is_referenced);
                    if ((rpcPic->poc != curPoc) && (rpcPic->slice->is_referenced)) {

                        if (poci == 1 && !bVaryDltFrmNum) {
                            continue;
                        }
                        if (rpcPic->poc - curPoc !=  dealtIdx) {
                            continue;
                        }
                        rps->poc[poci] = rpcPic->poc;
                        rps->delta_poc[poci] = rps->poc[poci] - curPoc;
                        (rps->delta_poc[poci] < 0) ? numNeg++ : numPos++;
                        rps->m_used[poci] = !isRAP;
                        poci++;
                    }
                }
            }
            h265e_dbg_dpb("poci %d, numPos %d numNeg %d", poci, numPos, numNeg);
            rps->m_numberOfPictures = poci;
            rps->num_positive_pic = numPos;
            rps->num_negative_pic = numNeg;
            rps->num_long_term_pic = 0;
            rps->m_interRPSPrediction = 0;          // To be changed later when needed
            sort_delta_poc(rps);
        }
    }
    h265e_dbg_func("leave\n");
}

void h265e_dpb_build_list(H265eDpb *dpb)
{
    RK_S32 poc_cur = dpb->curr->slice->poc;
    H265eSlice* slice = dpb->curr->slice;
    RK_U32 bGPBcheck = 0;
    RK_S32 i;

    h265e_dbg_func("enter\n");
    if (dpb->cfg->nLongTerm > 0 || dpb->cfg->vgop_size > 1) {
        h265e_gop_init(&dpb->RpsList, dpb->cfg, poc_cur);
        if ((0 == ((poc_cur % dpb->idr_gap) % dpb->cfg->vgop_size) ||
             0 == poc_cur % dpb->idr_gap) && dpb->cfg->nLongTerm) {
            h265e_dbg_dpb("set current frame is longterm");
            dpb->curr->is_long_term = 1;
        }
    }

    if (getNalUnitType(dpb, poc_cur) == NAL_IDR_W_RADL ||
        getNalUnitType(dpb, poc_cur) == NAL_IDR_N_LP) {
        dpb->last_idr = poc_cur;
    }

    slice->last_idr = dpb->last_idr;
    slice->m_temporalLayerNonReferenceFlag = !slice->is_referenced;
    // Set the nal unit type
    slice->m_nalUnitType = getNalUnitType(dpb, poc_cur);

    // If the slice is un-referenced, change from _R "referenced" to _N "non-referenced" NAL unit type
    if (slice->m_temporalLayerNonReferenceFlag) {
        switch (slice->m_nalUnitType) {
        case NAL_TRAIL_R:
            slice->m_nalUnitType = NAL_TRAIL_N;
            break;
        case NAL_RADL_R:
            slice->m_nalUnitType = NAL_RADL_N;
            break;
        case NAL_RASL_R:
            slice->m_nalUnitType  = NAL_RASL_N;
            break;
        default:
            break;
        }
    }

    // Do decoding refresh marking if any
    h265e_dpb_dec_refresh_marking(dpb, poc_cur, slice->m_nalUnitType);
    if (0 == dpb->cfg->nLongTerm && dpb->cfg->vgop_size < 2) {
        h265e_dbg_dpb("build list poc %d", poc_cur);
        h265e_dpb_compute_rps(dpb, poc_cur, slice, 0);
        slice->m_rps = &slice->m_localRPS;
        // Force use of RPS from slice, rather than from SPS
        slice->m_bdIdx = -1;
        // Mark pictures in m_piclist as unreferenced if they are not included in RPS
        h265e_dpb_apply_rps(dpb, slice->m_rps, poc_cur);
        h265e_dpb_arrange_lt_rps(dpb, slice);
    } else {
        h265e_dpb_compute_rps(dpb, poc_cur, slice, 1);
    }

    slice->m_numRefIdx[L0] =  MPP_MIN(dpb->max_ref_l0, slice->m_rps->m_numberOfPictures); // Ensuring L0 contains just the -ve POC
    slice->m_numRefIdx[L1] =  MPP_MIN(dpb->max_ref_l1, slice->m_rps->m_numberOfPictures);

    h265e_slice_set_ref_list(dpb->frame_list, slice);

    // Slice type refinement
    if ((slice->m_sliceType == B_SLICE) && (slice->m_numRefIdx[L1] == 0)) {
        slice->m_sliceType = P_SLICE;
    }

    if (slice->m_sliceType == B_SLICE) {
        // TODO: Can we estimate this from lookahead?
        slice->m_colFromL0Flag = 0;

        RK_U32 bLowDelay = 1;
        RK_S32 curPOC = slice->poc;
        RK_S32 refIdx = 0;

        for (refIdx = 0; refIdx < slice->m_numRefIdx[L0] && bLowDelay; refIdx++) {
            if (slice->m_refPicList[L0][refIdx]->poc > curPOC) {
                bLowDelay = 0;
            }
        }

        for (refIdx = 0; refIdx < slice->m_numRefIdx[L1] && bLowDelay; refIdx++) {
            if (slice->m_refPicList[L1][refIdx]->poc > curPOC) {
                bLowDelay = 0;
            }
        }

        slice->m_bCheckLDC = bLowDelay;
    } else {
        slice->m_bCheckLDC = 1;
    }

    h265e_slice_set_ref_poc_list(slice);
    if (slice->m_sliceType == B_SLICE) {
        if (slice->m_numRefIdx[L0] == slice->m_numRefIdx[L1]) {
            bGPBcheck = 0;
            for (i = 0; i < slice->m_numRefIdx[L1]; i++) {
                if (slice->m_refPOCList[L1][i] != slice->m_refPOCList[L0][i]) {
                    bGPBcheck = 0;
                    break;
                }
            }
        }
    }

    slice->m_bLMvdL1Zero = bGPBcheck;
    slice->m_nextSlice = 0;
    slice->tot_poc_num = dpb->cfg->tot_poc_num;
    if (slice->m_sliceType == I_SLICE) {
        slice->tot_poc_num = dpb->cfg->tot_poc_num = 0;
    } else {
        if (dpb->cfg->nLongTerm || dpb->cfg->vgop_size > 1) {
            slice->tot_poc_num = dpb->cfg->tot_poc_num = slice->m_localRPS.m_numberOfPictures;
        } else {
            slice->tot_poc_num = dpb->cfg->tot_poc_num = 1;
        }
    }

    if (dpb->cfg->vgop_size > 2) {
        RK_U32 index = dpb->curr->gop_idx;
        dpb->curr->status.temporal_id = dpb->cfg->ref_inf[index].temporal_id;
    }
    h265e_dbg_func("leave\n");
}
