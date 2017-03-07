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

#include <string.h>
#include "mpp_common.h"
#include "hal_h264d_common.h"

RK_U32 vdpu_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

RK_U32 vdpu_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static MPP_RET get_info_input(H264dVdpuPriv_t *priv,
                              DXVA_Slice_H264_Long *p_long,
                              DXVA_PicParams_H264_MVC  *pp)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    memset(priv->new_dpb, 0, sizeof(priv->new_dpb));
    memset(priv->refinfo, 0, sizeof(priv->refinfo));
    //!< change dpb_info syntax
    {
        RK_U32 i = 0;
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
            if (pp->RefFrameList[i].bPicEntry != 0xff) {
                priv->new_dpb[i].valid = 1;
                priv->new_dpb[i].is_long_term = pp->RefFrameList[i].AssociatedFlag;
                priv->new_dpb[i].slot_index = pp->RefFrameList[i].Index7Bits;
                priv->new_dpb[i].top_poc = pp->FieldOrderCntList[i][0];
                priv->new_dpb[i].bot_poc = pp->FieldOrderCntList[i][1];
                if (priv->new_dpb[i].is_long_term) {
                    priv->new_dpb[i].long_term_frame_idx = pp->FrameNumList[i];
                } else {
                    priv->new_dpb[i].frame_num = pp->FrameNumList[i];
                }
                priv->new_dpb[i].long_term_pic_num = pp->LongTermPicNumList[i];
                priv->new_dpb[i].top_used = ((pp->UsedForReferenceFlags
                                              >> (2 * i + 0)) & 0x1) ? 1 : 0;
                priv->new_dpb[i].bot_used = ((pp->UsedForReferenceFlags
                                              >> (2 * i + 1)) & 0x1) ? 1 : 0;
            }
        }
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->ViewIDList); i++) {
            priv->new_dpb[i].view_id = pp->ViewIDList[i];
        }
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
            priv->new_dpb[i].colmv_is_used = ((pp->RefPicColmvUsedFlags >> i) & 0x1) ? 1 : 0;
            priv->new_dpb[i].field_flag = ((pp->RefPicFiledFlags >> i) & 0x1) ? 1 : 0;
            priv->new_dpb[i].is_ilt_flag = ((pp->UsedForInTerviewflags >> i) & 0x1) ? 1 : 0;
        }
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefPicLayerIdList); i++) {
            priv->new_dpb[i].voidx = pp->RefPicLayerIdList[i];
        }
    }
    //!< change ref_pic_info syntax
    {
        RK_U32 i = 0, j = 0;
        //!< list P B0 B1
        for (j = 0; j < 3; j++) {
            for (i = 0; i < MPP_ARRAY_ELEMS(p_long->RefPicList[j]); i++) {
                if (p_long->RefPicList[j][i].bPicEntry != 0xff) {
                    priv->refinfo[j][i].valid = 1;
                    priv->refinfo[j][i].dpb_idx = p_long->RefPicList[j][i].Index7Bits;
                    priv->refinfo[j][i].bottom_flag = p_long->RefPicList[j][i].AssociatedFlag;
                }
            }
        }
    }
    return ret = MPP_OK;
}

static MPP_RET fill_picture_entry(DXVA_PicEntry_H264 *pic, RK_U32 index,
                                  RK_U32 flag)
{
    ASSERT((index & 0x7f) == index && (flag & 0x01) == flag);
    pic->bPicEntry = index | (flag << 7);

    return MPP_OK;
}

static MPP_RET refill_info_input(H264dVdpuPriv_t *priv,
                                 DXVA_Slice_H264_Long *p_long,
                                 DXVA_PicParams_H264_MVC  *pp)

{
    MPP_RET ret = MPP_ERR_UNKNOW;
    {
        RK_U32 i = 0;
        H264dVdpuDpbInfo_t *old_dpb = priv->old_dpb[priv->layed_id];
        //!< re-fill dpb_info
        pp->UsedForReferenceFlags = 0;
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
            if (old_dpb[i].valid) {
                fill_picture_entry(&pp->RefFrameList[i], old_dpb[i].slot_index,
                                   old_dpb[i].is_long_term);
                pp->FieldOrderCntList[i][0] = old_dpb[i].top_poc;
                pp->FieldOrderCntList[i][1] = old_dpb[i].bot_poc;
                pp->FrameNumList[i] = old_dpb[i].is_long_term ?
                                      old_dpb[i].long_term_frame_idx : old_dpb[i].frame_num;
                pp->LongTermPicNumList[i] = old_dpb[i].long_term_pic_num;
                if (old_dpb[i].top_used) { //!< top_field
                    pp->UsedForReferenceFlags |= 1 << (2 * i + 0);
                }
                if (old_dpb[i].bot_used) { //!< bot_field
                    pp->UsedForReferenceFlags |= 1 << (2 * i + 1);
                }
            } else {
                pp->RefFrameList[i].bPicEntry = 0xff;
                pp->FieldOrderCntList[i][0] = 0;
                pp->FieldOrderCntList[i][1] = 0;
                pp->FrameNumList[i] = 0;
                pp->LongTermPicNumList[i] = 0;
            }
        }
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->ViewIDList); i++) {
            pp->ViewIDList[i] = old_dpb[i].view_id;
        }
        pp->RefPicColmvUsedFlags = 0;
        pp->RefPicFiledFlags = 0;
        pp->UsedForInTerviewflags = 0;
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
            if (old_dpb[i].colmv_is_used) {
                pp->RefPicColmvUsedFlags |= 1 << i;
            }
            if (old_dpb[i].field_flag) {
                pp->RefPicFiledFlags |= 1 << i;
            }
            if (old_dpb[i].is_ilt_flag) {
                pp->UsedForInTerviewflags |= 1 << i;
            }
        }
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefPicLayerIdList); i++) {
            pp->RefPicLayerIdList[i] = old_dpb[i].voidx;
        }
    }
    //!< re-fill ref_info
    {
        RK_U32 i = 0, j = 0;
        for (j = 0; j < 3; j++) {
            H264dVdpuRefPicInfo_t *p = priv->refinfo[j];
            for (i = 0; i < MPP_ARRAY_ELEMS(p_long->RefPicList[j]); i++) {
                if (p[i].valid) {
                    fill_picture_entry(&p_long->RefPicList[j][i],
                                       p[i].dpb_idx, p[i].bottom_flag);
                } else {
                    p_long->RefPicList[j][i].bPicEntry = 0xff;
                }
            }
        }
    }
    return ret = MPP_OK;
}

MPP_RET adjust_input(H264dVdpuPriv_t *priv,
                     DXVA_Slice_H264_Long *p_long,
                     DXVA_PicParams_H264_MVC  *pp)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    get_info_input(priv, p_long, pp);
    //!< dpb mapping to new order
    {
        RK_U32 i = 0, j = 0;
        RK_U32 find_flag = 0;
        H264dVdpuDpbInfo_t *new_dpb = priv->new_dpb;
        H264dVdpuDpbInfo_t *old_dpb = priv->old_dpb[priv->layed_id];
        //!< delete old dpb
        for (i = 0; i < MPP_ARRAY_ELEMS(priv->old_dpb[priv->layed_id]); i++) {
            find_flag = 0;
            if (old_dpb[i].valid) {
                for (j = 0; j < MPP_ARRAY_ELEMS(priv->new_dpb); j++) {
                    if (new_dpb[j].valid) {
                        find_flag =
                            ((old_dpb[i].frame_num == new_dpb[j].frame_num)
                             ? 1 : 0);
                        find_flag = ((old_dpb[i].slot_index
                                      == new_dpb[j].slot_index)
                                     ? find_flag : 0);
                        if (new_dpb[j].top_used) {
                            find_flag =
                                ((old_dpb[i].top_poc == new_dpb[j].top_poc)
                                 ? find_flag : 0);
                        }
                        if (new_dpb[j].bot_used) {
                            find_flag  =
                                ((old_dpb[i].bot_poc == new_dpb[j].bot_poc)
                                 ? find_flag : 0);
                        }
                        if (find_flag) { //!< found
                            new_dpb[j].have_same = 1;
                            new_dpb[j].new_dpb_idx = i;
                            break;
                        }
                    }
                }
            }
            //!< not found
            if (find_flag == 0) {
                memset(&old_dpb[i], 0, sizeof(old_dpb[i]));
            }
        }
        //!< add new dpb
        for (j = 0; j < MPP_ARRAY_ELEMS(priv->new_dpb); j++) {
            if ((new_dpb[j].valid == 0) || new_dpb[j].have_same) {
                continue;
            }
            for (i = 0; i < MPP_ARRAY_ELEMS(priv->old_dpb[priv->layed_id]); i++) {
                if (old_dpb[i].valid == 0) {
                    old_dpb[i] = new_dpb[j];
                    new_dpb[j].new_dpb_idx = i;
                    break;
                }
            }
        }
        //!< inter-layer reference
        priv->ilt_dpb = NULL;
        if (priv->layed_id) {
            for (i = 0; i < MPP_ARRAY_ELEMS(priv->old_dpb[1]); i++) {
                if ((old_dpb[i].valid == 0) && old_dpb[i].is_ilt_flag) {
                    priv->ilt_dpb = &old_dpb[i];
                    break;
                }
            }
        }
    }
    //!< addjust ref_dpb
    {
        RK_U32 i = 0, j = 0;
        H264dVdpuDpbInfo_t *new_dpb = priv->new_dpb;

        for (j = 0; j < 3; j++) {
            H264dVdpuRefPicInfo_t *p = priv->refinfo[j];
            for (i = 0; i < MPP_ARRAY_ELEMS(priv->refinfo[j]); i++) {
                if (p[i].valid) {
                    p[i].dpb_idx = new_dpb[p[i].dpb_idx].new_dpb_idx;
                }
            }
        }
    }
    refill_info_input(priv, p_long, pp);

    return ret = MPP_OK;
}
