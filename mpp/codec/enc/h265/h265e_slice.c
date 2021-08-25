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

H265eDpbFrm* get_ref_pic(H265eDpbFrm *frame_list, RK_S32 poc)
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

H265eDpbFrm* get_lt_ref_pic(H265eDpbFrm *frame_list, H265eSlice *slice, RK_S32 poc, RK_U32 pocHasMsb)
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
            RK_S32 picPoc = frame->poc;
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

void h265e_slice_init(void *ctx, EncFrmStatus curr)
{
    H265eCtx *p = (H265eCtx *)ctx;
    H265eVps *vps = &p->vps;
    H265eSps *sps = &p->sps;
    H265ePps *pps = &p->pps;
    MppEncCfgSet *cfg = p->cfg;
    MppEncH265Cfg *codec = &cfg->codec.h265;
    H265eSlice *slice = p->dpb->curr->slice;
    p->slice = p->dpb->curr->slice;
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

    if (slice->m_sliceType  != B_SLICE && !curr.non_recn)
        slice->is_referenced = 1;

    if (slice->m_pps->m_deblockingFilterOverrideEnabledFlag) {
        h265e_dbg_slice("to do in this case");
    } else {
        slice->m_deblockingFilterDisable = pps->m_picDisableDeblockingFilterFlag;
        slice->m_deblockingFilterBetaOffsetDiv2 = pps->m_deblockingFilterBetaOffsetDiv2;
        slice->m_deblockingFilterTcOffsetDiv2 = pps->m_deblockingFilterTcOffsetDiv2;
    }
    slice->m_saoEnabledFlag = !codec->sao_cfg.slice_sao_luma_disable;
    slice->m_saoEnabledFlagChroma = !codec->sao_cfg.slice_sao_chroma_disable;
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

void code_st_refpic_set(MppWriteCtx *bitIf, H265eReferencePictureSet* rps, RK_S32 idx)
{
    if (idx > 0) {
        mpp_writer_put_bits(bitIf, rps->m_interRPSPrediction, 1); // inter_RPS_prediction_flag
    }
    if (rps->m_interRPSPrediction) {
        RK_S32 deltaRPS = rps->m_deltaRPS;
        RK_S32 j;

        mpp_writer_put_ue(bitIf, rps->m_deltaRIdxMinus1); // delta index of the Reference Picture Set used for prediction minus 1

        mpp_writer_put_bits(bitIf, (deltaRPS >= 0 ? 0 : 1), 1); //delta_rps_sign
        mpp_writer_put_ue(bitIf, abs(deltaRPS) - 1); // absolute delta RPS minus 1

        for (j = 0; j < rps->m_numRefIdc; j++) {
            RK_S32 refIdc = rps->m_refIdc[j];
            mpp_writer_put_bits(bitIf, (refIdc == 1 ? 1 : 0), 1); //first bit is "1" if Idc is 1
            if (refIdc != 1) {
                mpp_writer_put_bits(bitIf, refIdc >> 1, 1); //second bit is "1" if Idc is 2, "0" otherwise.
            }
        }
    } else {
        mpp_writer_put_ue(bitIf, rps->num_negative_pic);
        mpp_writer_put_ue(bitIf, rps->num_positive_pic);
        RK_S32 prev = 0;
        RK_S32 j;

        for (j = 0; j < rps->num_negative_pic; j++) {
            mpp_writer_put_ue(bitIf, prev - rps->delta_poc[j] - 1);
            prev = rps->delta_poc[j];
            mpp_writer_put_bits(bitIf, rps->m_used[j], 1);
        }

        prev = 0;
        for (j = rps->num_negative_pic; j < rps->num_negative_pic + rps->num_positive_pic; j++) {
            mpp_writer_put_ue(bitIf, rps->delta_poc[j] - prev - 1);
            prev = rps->delta_poc[j];
            mpp_writer_put_bits(bitIf, rps->m_used[j], 1);
        }
    }
}

RK_U8 find_matching_ltrp(H265eSlice* slice, RK_U32 *ltrpsIndex, RK_S32 ltrpPOC, RK_U32 usedFlag)
{
    RK_U32 lsb = ltrpPOC % (1 << slice->m_sps->m_bitsForPOC);
    RK_U32 k;
    for (k = 0; k < slice->m_sps->m_numLongTermRefPicSPS; k++) {
        if ((lsb == slice->m_sps->m_ltRefPicPocLsbSps[k]) && (usedFlag == slice->m_sps->m_usedByCurrPicLtSPSFlag[k])) {
            *ltrpsIndex = k;
            return 1;
        }
    }

    return 0;
}

RK_S32 get_num_rps_cur_templist(H265eReferencePictureSet* rps)
{
    RK_S32 numRpsCurrTempList = 0;
    RK_S32 i;
    for ( i = 0; i < rps->num_negative_pic + rps->num_positive_pic + rps->num_long_term_pic; i++) {
        if (rps->m_used[i]) {
            numRpsCurrTempList++;
        }
    }

    return numRpsCurrTempList;
}


void h265e_code_slice_header(H265eSlice *slice, MppWriteCtx *bitIf)
{
    RK_U32 i = 0;
    mpp_writer_put_bits(bitIf, 1, 1); //first_slice_segment_in_pic_flag
    mpp_writer_put_ue(bitIf, slice->m_ppsId);
    H265eReferencePictureSet* rps = slice->m_rps;
    slice->m_enableTMVPFlag = 0;
    if (!slice->m_dependentSliceSegmentFlag) {
        for (i = 0; i < (RK_U32)slice->m_pps->m_numExtraSliceHeaderBits; i++) {
            mpp_writer_put_bits(bitIf, (slice->slice_reserved_flag >> i) & 0x1, 1);
        }

        mpp_writer_put_ue(bitIf, slice->m_sliceType);

        if (slice->m_pps->m_outputFlagPresentFlag) {
            mpp_writer_put_bits(bitIf, slice->m_picOutputFlag ? 1 : 0, 1);
        }

        if (slice->m_sliceType != I_SLICE) { // skip frame can't iDR
            RK_S32 picOrderCntLSB = (slice->poc - slice->last_idr + (1 << slice->m_sps->m_bitsForPOC)) % (1 << slice->m_sps->m_bitsForPOC);
            mpp_writer_put_bits(bitIf, picOrderCntLSB, slice->m_sps->m_bitsForPOC);
            if (slice->m_bdIdx < 0) {
                mpp_writer_put_bits(bitIf, 0, 1);
                code_st_refpic_set(bitIf, rps, slice->m_sps->m_RPSList.m_numberOfReferencePictureSets);
            } else {
                mpp_writer_put_bits(bitIf, 1, 1);
                RK_S32 numBits = 0;
                while ((1 << numBits) < slice->m_sps->m_RPSList.m_numberOfReferencePictureSets) {
                    numBits++;
                }

                if (numBits > 0) {
                    mpp_writer_put_bits(bitIf, slice->m_bdIdx, numBits);
                }
            }
            if (slice->m_sps->m_bLongTermRefsPresent) {

                RK_S32 numLtrpInSH = rps->m_numberOfPictures;
                RK_S32 ltrpInSPS[MAX_REFS];
                RK_S32 numLtrpInSPS = 0;
                RK_U32 ltrpIndex;
                RK_S32 counter = 0;
                RK_S32 k;
                for (k = rps->m_numberOfPictures - 1; k > rps->m_numberOfPictures - rps->num_long_term_pic - 1; k--) {
                    if (find_matching_ltrp(slice, &ltrpIndex, rps->poc[k], rps->m_used[k])) {
                        ltrpInSPS[numLtrpInSPS] = ltrpIndex;
                        numLtrpInSPS++;
                    } else {
                        counter++;
                    }
                }

                numLtrpInSH -= numLtrpInSPS;

                RK_S32 bitsForLtrpInSPS = 0;
                while (slice->m_sps->m_numLongTermRefPicSPS > (RK_U32)(1 << bitsForLtrpInSPS)) {
                    bitsForLtrpInSPS++;
                }

                if (slice->m_sps->m_numLongTermRefPicSPS > 0) {
                    mpp_writer_put_ue(bitIf, numLtrpInSPS);
                }
                mpp_writer_put_ue(bitIf, numLtrpInSH);
                // Note that the LSBs of the LT ref. pic. POCs must be sorted before.
                // Not sorted here because LT ref indices will be used in setRefPicList()
                RK_S32 prevDeltaMSB = 0;
                RK_S32 offset = rps->num_negative_pic + rps->num_positive_pic;
                for ( k = rps->m_numberOfPictures - 1; k > offset - 1; k--) {
                    if (counter < numLtrpInSPS) {
                        if (bitsForLtrpInSPS > 0) {
                            mpp_writer_put_bits(bitIf, ltrpInSPS[counter], bitsForLtrpInSPS);
                        }
                    } else {
                        mpp_writer_put_bits(bitIf, rps->m_pocLSBLT[k], slice->m_sps->m_bitsForPOC);
                        mpp_writer_put_bits(bitIf, rps->m_used[k], 1);
                    }
                    mpp_writer_put_bits(bitIf, rps->m_deltaPocMSBPresentFlag[k], 1);

                    if (rps->m_deltaPocMSBPresentFlag[k]) {
                        RK_U32 deltaFlag = 0;
                        if ((k == rps->m_numberOfPictures - 1) || (k == rps->m_numberOfPictures - 1 - numLtrpInSPS)) {
                            deltaFlag = 1;
                        }
                        if (deltaFlag) {
                            mpp_writer_put_ue(bitIf, rps->m_deltaPOCMSBCycleLT[k]);
                        } else {
                            RK_S32 differenceInDeltaMSB = rps->m_deltaPOCMSBCycleLT[k] - prevDeltaMSB;
                            mpp_writer_put_ue(bitIf, differenceInDeltaMSB);
                        }
                        prevDeltaMSB = rps->m_deltaPOCMSBCycleLT[k];
                    }
                }
            }
            if (slice->m_sps->m_TMVPFlagsPresent) {
                mpp_writer_put_bits(bitIf, slice->m_enableTMVPFlag ? 1 : 0, 1);
            }
        }

        if (slice->m_sps->m_bUseSAO) { //skip frame close sao
            mpp_writer_put_bits(bitIf, 0, 1);
            mpp_writer_put_bits(bitIf, 0, 1);

        }
        //check if numrefidxes match the defaults. If not, override

        if (slice->m_sliceType != I_SLICE) {
            RK_U32 overrideFlag = (slice->m_numRefIdx[0] != (RK_S32)slice->m_pps->m_numRefIdxL0DefaultActive);
            mpp_writer_put_bits(bitIf, overrideFlag ? 1 : 0, 1);
            if (overrideFlag) {
                mpp_writer_put_ue(bitIf, slice->m_numRefIdx[0] - 1);
                slice->m_numRefIdx[1] = 0;
            }
        }

        if (slice->m_pps->m_listsModificationPresentFlag && get_num_rps_cur_templist(rps) > 1) {
            H265eRefPicListModification* refPicListModification = &slice->m_RefPicListModification;
            mpp_writer_put_bits(bitIf, refPicListModification->m_refPicListModificationFlagL0 ? 1 : 0, 1);
            if (refPicListModification->m_refPicListModificationFlagL0) {
                RK_S32 numRpsCurrTempList0 = get_num_rps_cur_templist(rps);
                if (numRpsCurrTempList0 > 1) {
                    RK_S32 length = 1;
                    numRpsCurrTempList0--;
                    while (numRpsCurrTempList0 >>= 1) {
                        length++;
                    }
                    for (i = 0; i < (RK_U32)slice->m_numRefIdx[0]; i++) {
                        mpp_writer_put_bits(bitIf, refPicListModification->m_RefPicSetIdxL0[i], length);
                    }
                }
            }
        }

        if (slice->m_pps->m_cabacInitPresentFlag) {
            mpp_writer_put_bits(bitIf, slice->m_cabacInitFlag, 1);
        }

        if (slice->m_enableTMVPFlag) {

            if (slice->m_sliceType != I_SLICE &&
                ((slice->m_colFromL0Flag == 1 && slice->m_numRefIdx[0] > 1) ||
                 (slice->m_colFromL0Flag == 0 && slice->m_numRefIdx[1] > 1))) {
                mpp_writer_put_ue(bitIf, slice->m_colRefIdx);
            }
        }

        if (slice->m_sliceType != I_SLICE) {
            RK_S32 flag = MRG_MAX_NUM_CANDS - slice->m_maxNumMergeCand;
            flag = flag == 5 ? 4 : flag;
            mpp_writer_put_ue(bitIf, flag);
        }
        RK_S32 code = slice->m_sliceQp - (slice->m_pps->m_picInitQPMinus26 + 26);
        mpp_writer_put_se(bitIf, code);
        if (slice->m_pps->m_bSliceChromaQpFlag) {
            code = slice->m_sliceQpDeltaCb;
            mpp_writer_put_se(bitIf, code);
            code = slice->m_sliceQpDeltaCr;
            mpp_writer_put_se(bitIf, code);
        }
        if (slice->m_pps->m_deblockingFilterControlPresentFlag) {
            if (slice->m_pps->m_deblockingFilterOverrideEnabledFlag) {
                mpp_writer_put_bits(bitIf, slice->m_deblockingFilterOverrideFlag, 1);
            }
            if (slice->m_deblockingFilterOverrideFlag) {
                mpp_writer_put_bits(bitIf, slice->m_deblockingFilterDisable, 1);
                if (!slice->m_deblockingFilterDisable) {
                    mpp_writer_put_se(bitIf, slice->m_deblockingFilterBetaOffsetDiv2);
                    mpp_writer_put_se(bitIf, slice->m_deblockingFilterTcOffsetDiv2);
                }
            }
        }
    }
    if (slice->m_pps->m_sliceHeaderExtensionPresentFlag) {
        mpp_writer_put_ue(bitIf, slice->slice_header_extension_length);
        for (i = 0; i < slice->slice_header_extension_length; i++) {
            mpp_writer_put_bits(bitIf, 0, 8);
        }
    }
    h265e_dbg_func("leave\n");
}

void code_skip_flag(H265eSlice *slice, RK_U32 abs_part_idx, DataCu *cu)
{
    // get context function is here
    H265eCabacCtx *cabac_ctx = &slice->m_cabac;
    H265eSps *sps = slice->m_sps;
    RK_U32  ctxSkip;
    RK_U32 tpelx = cu->pixelX + sps->raster2pelx[sps->zscan2raster[abs_part_idx]];
    RK_U32 tpely = cu->pixelY + sps->raster2pely[sps->zscan2raster[abs_part_idx]];
    //RK_U32 ctxSkip = cu->getCtxSkipFlag(abs_part_idx);

    h265e_dbg_skip("tpelx = %d", tpelx);
    if (cu->cur_addr == 0 ) {
        ctxSkip = 0;
    } else if ((tpely == 0) || (tpelx == 0)) {
        ctxSkip = 1;
    } else {
        ctxSkip = 2;
    }
    h265e_dbg_skip("ctxSkip = %d", ctxSkip);
    h265e_cabac_encodeBin(cabac_ctx, &slice->m_contextModels[OFF_SKIP_FLAG_CTX + ctxSkip], 1);
}

static void code_merge_index(H265eSlice *slice)
{
    H265eCabacCtx *cabac_ctx = &slice->m_cabac;

    h265e_cabac_encodeBin(cabac_ctx, &slice->m_contextModels[OFF_MERGE_IDX_EXT_CTX], 0);
}

static void code_split_flag(H265eSlice *slice, RK_U32 abs_part_idx, RK_U32 depth, DataCu *cu)
{
    H265eSps *sps = slice->m_sps;

    if (depth == slice->m_sps->m_maxCUDepth - slice->m_sps->m_addCUDepth)
        return;

    h265e_dbg_skip("depth %d cu->m_cuDepth %d", depth, cu->m_cuDepth[sps->zscan2raster[abs_part_idx]]);

    H265eCabacCtx *cabac_ctx = &slice->m_cabac;
    RK_U32 currSplitFlag = (cu->m_cuDepth[sps->zscan2raster[abs_part_idx]] > depth) ? 1 : 0;

    h265e_cabac_encodeBin(cabac_ctx, &slice->m_contextModels[OFF_SPLIT_FLAG_CTX], currSplitFlag);
}

static void encode_cu(H265eSlice *slice, RK_U32 abs_part_idx, RK_U32 depth, DataCu *cu)
{
    H265eSps *sps = slice->m_sps;
    RK_U32 bBoundary = 0;
    RK_U32 lpelx = cu->pixelX + sps->raster2pelx[sps->zscan2raster[abs_part_idx]];
    RK_U32 rpelx = lpelx + (sps->m_maxCUSize >> depth) - 1;
    RK_U32 tpely = cu->pixelY + sps->raster2pely[sps->zscan2raster[abs_part_idx]];
    RK_U32 bpely = tpely + (sps->m_maxCUSize >> depth) - 1;

    h265e_dbg_skip("EncodeCU depth %d, abs_part_idx %d", depth, abs_part_idx);

    if ((rpelx < sps->m_picWidthInLumaSamples) && (bpely < sps->m_picHeightInLumaSamples)) {

        h265e_dbg_skip("code_split_flag in depth %d", depth);
        code_split_flag(slice, abs_part_idx, depth, cu);
    } else {
        h265e_dbg_skip("boundary flag found");
        bBoundary = 1;
    }

    h265e_dbg_skip("m_cuDepth[%d] = %d maxCUDepth %d, m_addCUDepth %d", abs_part_idx, cu->m_cuDepth[sps->zscan2raster[abs_part_idx]], sps->m_maxCUDepth, sps->m_addCUDepth);

    if ((depth < cu->m_cuDepth[sps->zscan2raster[abs_part_idx]] && (depth < (sps->m_maxCUDepth - sps->m_addCUDepth))) || bBoundary) {
        RK_U32 qNumParts = (256 >> (depth << 1)) >> 2;
        RK_U32 partUnitIdx = 0;

        for (partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++, abs_part_idx += qNumParts) {
            h265e_dbg_skip("depth %d partUnitIdx = %d, qNumParts %d, abs_part_idx %d", depth, partUnitIdx, qNumParts, abs_part_idx);
            lpelx = cu->pixelX + sps->raster2pelx[sps->zscan2raster[abs_part_idx]];
            tpely = cu->pixelY + sps->raster2pely[sps->zscan2raster[abs_part_idx]];
            if ((lpelx < sps->m_picWidthInLumaSamples) && (tpely < sps->m_picHeightInLumaSamples)) {
                encode_cu(slice, abs_part_idx, depth + 1, cu);
            }
        }
        return;
    }

    h265e_dbg_skip("code_skip_flag in depth %d", depth);
    code_skip_flag(slice, abs_part_idx, cu);
    h265e_dbg_skip("code_merge_index in depth %d", depth);
    code_merge_index(slice);
    return;
}

static void proc_cu8(DataCu *cu, RK_U32 pos_x, RK_U32 pos_y)
{
    RK_S32 nSize = 8;
    RK_S32 nSubPart = nSize * nSize / 4 / 4;
    RK_S32 puIdx = pos_x / 8 + pos_y / 8 * 8;

    h265e_dbg_skip("8 ctu puIdx %d no need split", puIdx);

    memset(cu->m_cuDepth + puIdx * nSubPart, 3, nSubPart);
}

static void proc_cu16(H265eSlice *slice, DataCu *cu, RK_U32 pos_x, RK_U32 pos_y)
{
    RK_U32 m;
    H265eSps *sps = slice->m_sps;
    RK_S32 nSize = 16;
    RK_S32 nSubPart = nSize * nSize / 4 / 4;
    RK_S32 puIdx = pos_x / 16 + pos_y / 16 * 4;
    RK_U32 cu_x_1, cu_y_1;

    h265e_dbg_skip("cu 16 pos_x %d pos_y %d", pos_x, pos_y);

    if ((cu->pixelX + pos_x + 15 < sps->m_picWidthInLumaSamples) &&
        (cu->pixelY + pos_y + 15 < sps->m_picHeightInLumaSamples)) {
        h265e_dbg_skip("16 ctu puIdx %d no need split", puIdx);
        memset(cu->m_cuDepth + puIdx * nSubPart, 2, nSubPart);
        return;
    } else if ((cu->pixelX + pos_x >=  sps->m_picWidthInLumaSamples) ||
               (cu->pixelY + pos_y  >= sps->m_picHeightInLumaSamples)) {
        h265e_dbg_skip("16 ctu puIdx %d out of pic", puIdx);
        memset(cu->m_cuDepth + puIdx * nSubPart, 2, nSubPart);
        return;
    }

    for (m = 0; m < 4; m ++) {
        cu_x_1 = pos_x + (m & 1) * (nSize >> 1);
        cu_y_1 = pos_y + (m >> 1) * (nSize >> 1);

        proc_cu8(cu, cu_x_1, cu_y_1);
    }
}


static void proc_cu32(H265eSlice *slice, DataCu *cu, RK_U32 pos_x, RK_U32 pos_y)
{
    RK_U32 m;
    H265eSps *sps = slice->m_sps;
    RK_S32 nSize = 32;
    RK_S32 nSubPart = nSize * nSize / 4 / 4;
    RK_S32 puIdx = pos_x / 32 + pos_y / 32 * 2;
    RK_U32 cu_x_1, cu_y_1;

    h265e_dbg_skip("cu 32 pos_x %d pos_y %d", pos_x, pos_y);

    if ((cu->pixelX + pos_x + 31 < sps->m_picWidthInLumaSamples) &&
        (cu->pixelY + pos_y + 31 < sps->m_picHeightInLumaSamples)) {
        h265e_dbg_skip("32 ctu puIdx %d no need split", puIdx);
        memset(cu->m_cuDepth + puIdx * nSubPart, 1, nSubPart);
        return;
    } else if ((cu->pixelX + pos_x >=  sps->m_picWidthInLumaSamples) ||
               (cu->pixelY + pos_y  >= sps->m_picHeightInLumaSamples)) {
        h265e_dbg_skip("32 ctu puIdx %d out of pic", puIdx);
        memset(cu->m_cuDepth + puIdx * nSubPart, 1, nSubPart);
        return;
    }

    for (m = 0; m < 4; m ++) {
        cu_x_1 = pos_x + (m & 1) * (nSize >> 1);
        cu_y_1 = pos_y + (m >> 1) * (nSize >> 1);

        proc_cu16(slice, cu, cu_x_1, cu_y_1);
    }
}

static void proc_ctu(H265eSlice *slice, DataCu *cu)
{
    H265eSps *sps = slice->m_sps;
    RK_U32 k, m;
    RK_U32 cu_x_1, cu_y_1, m_nCtuSize = 64;
    RK_U32 lpelx = cu->pixelX;
    RK_U32 rpelx = lpelx + 63;
    RK_U32 tpely = cu->pixelY;
    RK_U32 bpely = tpely + 63;

    for (k = 0; k < 256; k++) {
        cu->m_cuDepth[k] = 0;
        cu->m_cuSize[k] = 64;
    }
    if ((rpelx < sps->m_picWidthInLumaSamples) && (bpely < sps->m_picHeightInLumaSamples))
        return;

    for (m = 0; m < 4; m ++) {
        cu_x_1 = (m & 1) * (m_nCtuSize >> 1);
        cu_y_1 = (m >> 1) * (m_nCtuSize >> 1);
        proc_cu32(slice, cu, cu_x_1, cu_y_1);
    }

    for (k = 0; k < 256; k++) {
        switch (cu->m_cuDepth[k]) {
        case 0: cu->m_cuSize[k] = 64; break;
        case 1: cu->m_cuSize[k] = 32; break;
        case 2: cu->m_cuSize[k] = 16; break;
        case 3: cu->m_cuSize[k] = 8;  break;
        }
    }
}

static void h265e_write_nal(MppWriteCtx *bitIf)
{
    h265e_dbg_func("enter\n");

    mpp_writer_put_raw_bits(bitIf, 0x0, 24);
    mpp_writer_put_raw_bits(bitIf, 0x01, 8);
    mpp_writer_put_bits(bitIf, 0, 1);   // forbidden_zero_bit
    mpp_writer_put_bits(bitIf, 1, 6);   // nal_unit_type
    mpp_writer_put_bits(bitIf, 0, 6);   // nuh_reserved_zero_6bits
    mpp_writer_put_bits(bitIf, 1, 3);   // nuh_temporal_id_plus1

    h265e_dbg_func("leave\n");
}

static void h265e_write_algin(MppWriteCtx *bitIf)
{
    h265e_dbg_func("enter\n");
    mpp_writer_put_bits(bitIf, 1, 1);
    mpp_writer_align_zero(bitIf);
    h265e_dbg_func("leave\n");
}


RK_S32 h265e_code_slice_skip_frame(void *ctx, H265eSlice *slice, RK_U8 *buf, RK_S32 len)
{

    MppWriteCtx bitIf;
    H265eCtx *p = (H265eCtx *)ctx;
    H265eSps *sps = &p->sps;
    H265eCabacCtx *cabac_ctx = &slice->m_cabac;
    h265e_dbg_func("enter\n");
    RK_U32 mb_wd = ((sps->m_picWidthInLumaSamples + 63) >> 6);
    RK_U32 mb_h = ((sps->m_picHeightInLumaSamples + 63) >> 6);
    RK_U32 i = 0, j = 0, cu_cnt = 0;
    if (!buf || !len) {
        mpp_err("buf or size no set");
        return MPP_NOK;
    }
    mpp_writer_init(&bitIf, buf, len);
    h265e_write_nal(&bitIf);
    h265e_code_slice_header(slice, &bitIf);
    h265e_write_algin(&bitIf);
    h265e_reset_enctropy((void*)slice);
    h265e_cabac_init(cabac_ctx, &bitIf);
    DataCu cu;
    cu.mb_w = mb_wd;
    cu.mb_h = mb_h;
    slice->is_referenced = 0;
    for (i = 0; i < mb_h; i++) {
        for ( j = 0; j < mb_wd; j++) {
            cu.pixelX = j * 64;
            cu.pixelY = i * 64;
            cu.cur_addr = cu_cnt;
            proc_ctu(slice, &cu);
            encode_cu(slice, 0, 0, &cu);
            h265e_cabac_encodeBinTrm(cabac_ctx, 0);
            cu_cnt++;
        }
    }

    h265e_cabac_finish(cabac_ctx);
    h265e_write_algin(&bitIf);
    h265e_dbg_func("leave\n");
    return mpp_writer_bytes(&bitIf);
}


