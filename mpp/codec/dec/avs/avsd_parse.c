/*
*
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

#define MODULE_TAG "avsd_parse"

#include <string.h>
#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_packet_impl.h"
#include "hal_task.h"

#include "avsd_api.h"
#include "avsd_parse.h"


static MPP_RET add_nalu_header(AvsdCtx_t *p_dec, RK_U32 header)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AvsdStreamBuf_t *p_buf = p_dec->p_header;
    RK_U32 add_size = sizeof(AvsdNalu_t);

    if ((p_buf->len + add_size) > p_buf->size) {
        mpp_err_f("buffer is larger than %d", p_buf->size);
        goto __FAILED;
    }
    p_dec->nal = (AvsdNalu_t *)&p_buf->pbuf[p_buf->len];
    p_dec->nal->eof = 0;
    p_dec->nal->header = header;
    p_dec->nal->pdata = NULL;
    p_dec->nal->length = 0;
    p_dec->nal->start_pos = 4;
    p_buf->len += add_size;

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static MPP_RET store_cur_nalu(AvsdCtx_t *p_dec, RK_U8 *p_start, RK_U32 nalu_len)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdStreamBuf_t *p_buf = NULL;
    AvsdNalu_t *p_nalu = p_dec->nal;

    if (p_nalu->header >= SLICE_MIN_START_CODE
        && p_nalu->header <= SLICE_MAX_START_CODE) {
        p_buf = p_dec->p_stream;
    } else {
        p_buf = p_dec->p_header;
    }
    if ((p_buf->len + nalu_len) > p_buf->size) {
        mpp_err_f("buffer is larger than %d", p_buf->size);
        goto __FAILED;
    }
    p_nalu->length += nalu_len;
    p_nalu->pdata = &p_buf->pbuf[p_buf->len];
    memcpy(p_nalu->pdata, p_start, nalu_len);
    p_buf->len += nalu_len;

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static MPP_RET get_sequence_header(BitReadCtx_t *bitctx, AvsdSeqHeader_t *vsh)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 val_temp = 0;

    READ_BITS(bitctx, 8, &vsh->profile_id);
    //!< check profile_id
    if (vsh->profile_id != 0x20 && vsh->profile_id != 0x48) {
        ret = MPP_NOK;
        mpp_err_f("profile_id 0x%02x is not supported.\n", vsh->profile_id);
        goto __FAILED;
    }
    READ_BITS(bitctx, 8, &vsh->level_id);
    if (vsh->level_id > 0x42) {
        ret = MPP_NOK;
        mpp_err_f("level_id 0x%02x is not supported.\n", vsh->level_id);
        goto __FAILED;
    }
    READ_ONEBIT(bitctx, &vsh->progressive_sequence);
    READ_BITS(bitctx, 14, &vsh->horizontal_size);
    READ_BITS(bitctx, 14, &vsh->vertical_size);
    READ_BITS(bitctx, 2,  &vsh->chroma_format);
    READ_BITS(bitctx, 3, &vsh->sample_precision);
    READ_BITS(bitctx, 4, &vsh->aspect_ratio);
    READ_BITS(bitctx, 4, &vsh->frame_rate_code);
    READ_BITS(bitctx, 18, &val_temp); //!< bit_rate_high_18
    vsh->bit_rate = val_temp << 12;
    SKIP_BITS(bitctx, 1);
    READ_BITS(bitctx, 12, &val_temp); //!< bit_rate_low_12
    vsh->bit_rate += val_temp;
    READ_ONEBIT(bitctx, &vsh->low_delay);
    SKIP_BITS(bitctx, 1);
    READ_BITS(bitctx, 18, &vsh->bbv_buffer_size);
    READ_BITS(bitctx, 3, &val_temp); //!< reserve 3bits 000
    if (val_temp) {
        AVSD_DBG(AVSD_DBG_WARNNING, "reserver bits error.\n");
    }
    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

static MPP_RET gen_weight_quant_param(AvsdPicHeader_t *ph)
{
    RK_U32 i = 0;
    RK_U32 *wqp = (RK_U32 *)ph->weighting_quant_param;

    RK_U32 weighting_quant_param_default[] = { 128, 98, 106, 116, 116, 128 };
    RK_U32 weighting_quant_param_base1[] = { 135, 143, 143, 160, 160, 213 };
    RK_U32 weighting_quant_param_base2[] = { 128, 98, 106, 116, 116, 128 };

    if (ph->weighting_quant_flag == 0) {
        //!< needn't generate this param
        for (i = 0; i < 6; i++) {
            wqp[i] = 128;
        }
        return MPP_OK;
    }

    if (ph->weighting_quant_param_index == 0x0) {
        for (i = 0; i < 6; i++) {
            wqp[i] = weighting_quant_param_default[i];
        }
    } else if (ph->weighting_quant_param_index == 0x1) {
        for (i = 0; i < 6; i++) {
            wqp[i] = weighting_quant_param_base1[i] +
                     ph->weighting_quant_param_delta1[i];
        }
    } else if (ph->weighting_quant_param_index == 0x2) {
        for (i = 0; i < 6; i++) {
            wqp[i] = weighting_quant_param_base2[i] +
                     ph->weighting_quant_param_delta2[i];
        }
    } else {
        //!< shouldn't happen
        AVSD_DBG(AVSD_DBG_WARNNING, "Something went wrong.\n");
        for (i = 0; i < 6; i++) {
            wqp[i] = 128;
        }
    }

    return MPP_OK;
}

static MPP_RET get_extend_header(BitReadCtx_t *bitctx, AvsdSeqHeader_t *vsh, AvsdPicHeader_t *ph)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 i = 0;

    READ_ONEBIT(bitctx, &ph->loop_filter_disable);
    if (!ph->loop_filter_disable) {
        READ_ONEBIT(bitctx, &ph->loop_filter_parameter_flag);
        if (ph->loop_filter_parameter_flag) {
            READ_SE(bitctx, &ph->alpha_c_offset);
            READ_SE(bitctx, &ph->beta_offset);
        }
    }
    ph->chroma_quant_param_delta_cb = 0;
    ph->chroma_quant_param_delta_cr = 0;
    for (i = 0; i < 6; i++) {
        ph->weighting_quant_param_delta1[i] = 0;
        ph->weighting_quant_param_delta2[i] = 0;
    }
    if (vsh->profile_id == 0x48) {
        READ_ONEBIT(bitctx, &ph->weighting_quant_flag);
        if (ph->weighting_quant_flag) {
            SKIP_BITS(bitctx, 1);
            READ_ONEBIT(bitctx, &ph->chroma_quant_param_disable);
            if (!ph->chroma_quant_param_disable) {
                READ_SE(bitctx, &ph->chroma_quant_param_delta_cb);
                READ_SE(bitctx, &ph->chroma_quant_param_delta_cr);
            }
            READ_BITS(bitctx, 2, &ph->weighting_quant_param_index);
            READ_BITS(bitctx, 2, &ph->weighting_quant_model);
            if (ph->weighting_quant_param_index == 1) {
                for (i = 0; i < 6; i++) {
                    READ_SE(bitctx, &ph->weighting_quant_param_delta1[i]);
                }
            } else if (ph->weighting_quant_param_index == 2) {
                for (i = 0; i < 6; i++) {
                    READ_SE(bitctx, &ph->weighting_quant_param_delta2[i]);
                }
            }
        }
        gen_weight_quant_param(ph); //!< generate wqP[m][6]

        READ_ONEBIT(bitctx, &ph->aec_enable);
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = bitctx->ret;
}

static MPP_RET get_seq_dispay_ext_header(BitReadCtx_t *bitctx, AvsdSeqExtHeader_t *ext)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 val_temp = 0;

    READ_BITS(bitctx, 3, &ext->video_format);
    READ_ONEBIT(bitctx, &ext->sample_range);
    READ_ONEBIT(bitctx, &ext->color_description);

    if (ext->color_description) {
        READ_BITS(bitctx, 8, &ext->color_primaries);
        READ_BITS(bitctx, 8, &ext->transfer_characteristics);
        READ_BITS(bitctx, 8, &ext->matrix_coefficients);
    }
    READ_BITS(bitctx, 14, &ext->display_horizontalSize);
    SKIP_BITS(bitctx, 1); //!< marker bit
    READ_BITS(bitctx, 14, &ext->display_verticalSize);

    READ_BITS(bitctx, 2, &val_temp); //!< reserve 2 bits
    if (val_temp) {
        AVSD_DBG(AVSD_DBG_WARNNING, "reserve bits not equal to zeros.\n");
    }
    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = bitctx->ret;
}
static MPP_RET get_extension_header(BitReadCtx_t *bitctx, AvsdSeqExtHeader_t *ext)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 val_temp = 0;

    READ_BITS(bitctx, 4, &val_temp); //!< extension_start_code
    switch (val_temp) {
    case SEQUENCE_DISPLAY_EXTENTION:
        FUN_CHECK(ret = get_seq_dispay_ext_header(bitctx, ext));
        break;

    default:
        break;
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

static MPP_RET get_i_picture_header(BitReadCtx_t *bitctx, AvsdSeqHeader_t *vsh, AvsdPicHeader_t *ph)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 val_temp = 0;
    ph->picture_coding_type = I_PICTURE;

    READ_BITS(bitctx, 16, &ph->bbv_delay);
    if (vsh->profile_id == 0x48) {
        SKIP_BITS(bitctx, 1);
        READ_BITS(bitctx, 7, &ph->bbv_delay_extension);
    }
    READ_ONEBIT(bitctx, &ph->time_code_flag);
    if (ph->time_code_flag) {
        READ_BITS(bitctx, 24, &ph->time_code);
    }
    SKIP_BITS(bitctx, 1);
    READ_BITS(bitctx, 8, &ph->picture_distance);
    if (vsh->low_delay) {
        READ_UE(bitctx, &ph->bbv_check_times);
    }
    READ_ONEBIT(bitctx, &ph->progressive_frame);
    if (!ph->progressive_frame) {
        READ_ONEBIT(bitctx, &ph->picture_structure);
    } else {
        ph->picture_structure = 1; //!< frame picture
    }
    READ_ONEBIT(bitctx, &ph->top_field_first);
    READ_ONEBIT(bitctx, &ph->repeat_first_field);
    READ_ONEBIT(bitctx, &ph->fixed_picture_qp);
    READ_BITS(bitctx, 6, &ph->picture_qp);
    if (!ph->progressive_frame && !ph->picture_structure) {
        READ_ONEBIT(bitctx, &ph->skip_mode_flag);
    } else {
        ph->skip_mode_flag = 0;
    }
    READ_BITS(bitctx, 4, &val_temp); //!< reserve 4 bits
    if (val_temp) {
        AVSD_DBG(AVSD_DBG_WARNNING, "reserve bits not equal to zeros.\n");
    }
    ph->no_forward_reference_flag = 0;
    ph->pb_field_enhanced_flag = 0;
    ph->weighting_quant_flag = 0;
    ph->aec_enable = 0;

    FUN_CHECK(ret = get_extend_header(bitctx, vsh, ph));

    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

static MPP_RET get_pb_picture_header(BitReadCtx_t *bitctx, AvsdSeqHeader_t *vsh, AvsdPicHeader_t *ph)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 val_temp = 0;

    READ_BITS(bitctx, 16, &ph->bbv_delay);
    if (vsh->profile_id == 0x48) {
        SKIP_BITS(bitctx, 1);
        READ_BITS(bitctx, 7, &ph->bbv_delay_extension);
    }
    READ_BITS(bitctx, 2, &ph->picture_coding_type);
    READ_BITS(bitctx, 8, &ph->picture_distance);
    if (vsh->low_delay) {
        READ_UE(bitctx, &ph->bbv_check_times);
    }
    READ_ONEBIT(bitctx, &ph->progressive_frame);

    if (!ph->progressive_frame) {
        READ_ONEBIT(bitctx, &ph->picture_structure);
        if (!ph->picture_structure) {
            READ_ONEBIT(bitctx, &ph->advanced_pred_mode_disable);
        }
    } else {
        ph->picture_structure = 1; //!< frame picture
    }
    READ_ONEBIT(bitctx, &ph->top_field_first);
    READ_ONEBIT(bitctx, &ph->repeat_first_field);
    READ_ONEBIT(bitctx, &ph->fixed_picture_qp);
    READ_BITS(bitctx, 6, &ph->picture_qp);
    if (!(ph->picture_coding_type == B_PICTURE && ph->picture_structure == P_PICTURE)) {
        READ_ONEBIT(bitctx, &ph->picture_reference_flag);
    }
    READ_ONEBIT(bitctx, &ph->no_forward_reference_flag);
    READ_ONEBIT(bitctx, &ph->pb_field_enhanced_flag);
    if (vsh->profile_id != 0x48) {
        ph->no_forward_reference_flag = 0;
        ph->pb_field_enhanced_flag = 0;
    }
    ph->weighting_quant_flag = 0;
    ph->aec_enable = 0;

    READ_BITS(bitctx, 2, &val_temp); //!< reserve bits
    if (val_temp) {
        AVSD_DBG(AVSD_DBG_WARNNING, "reserve bits not equal to zeros.\n");
    }
    READ_ONEBIT(bitctx, &ph->skip_mode_flag);

    FUN_CHECK(ret = get_extend_header(bitctx, vsh, ph));

    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

static void reset_one_save(AvsdFrame_t *p)
{
    if (p) {
        memset(p, 0, sizeof(AvsdFrame_t));
        p->slot_idx = -1;
    }
}

static AvsdFrame_t *get_one_save(AvsdCtx_t *p_dec, HalDecTask *task)
{
    RK_U32 i = 0;
    AvsdFrame_t *p_cur = NULL;

    for (i = 0; i < MPP_ARRAY_ELEMS(p_dec->mem->save); i++) {
        if (!p_dec->mem->save[i].valid) {
            p_dec->mem->save[i].valid = 1;
            p_cur = &p_dec->mem->save[i];
            break;
        }
    }
    if (!p_cur) {
        AVSD_DBG(AVSD_DBG_WARNNING, "error, mem_save dpb has not get.\n");
        goto __FAILED;
    }
    (void)task;
    return p_cur;
__FAILED:
    reset_one_save(p_cur);

    return NULL;
}

static MPP_RET set_frame_unref(AvsdCtx_t *pdec, AvsdFrame_t *p)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    if (p && p->slot_idx >= 0) {
        mpp_buf_slot_clr_flag(pdec->frame_slots, p->slot_idx, SLOT_CODEC_USE);
        memset(p, 0, sizeof(AvsdFrame_t));
        p->slot_idx = -1;
    }

    return ret = MPP_OK;
}


static MPP_RET set_frame_output(AvsdCtx_t *p_dec, AvsdFrame_t *p)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    if (p && p->slot_idx >= 0 && !p->had_display) {
        mpp_buf_slot_set_flag(p_dec->frame_slots, p->slot_idx, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(p_dec->frame_slots, p->slot_idx, QUEUE_DISPLAY);
        p->had_display = 1;
    }

    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*    reset decoder parameters
***********************************************************************
*/
MPP_RET avsd_reset_parameters(AvsdCtx_t *p_dec)
{
    RK_U32 i = 0;

    set_frame_output(p_dec, p_dec->dpb[1]);
    set_frame_output(p_dec, p_dec->dpb[0]);
    set_frame_output(p_dec, p_dec->cur);
    set_frame_unref(p_dec, p_dec->dpb[1]);
    set_frame_unref(p_dec, p_dec->dpb[0]);
    set_frame_unref(p_dec, p_dec->cur);

    p_dec->cur = NULL;
    p_dec->dpb[0] = NULL;
    p_dec->dpb[1] = NULL;
    p_dec->has_get_eos = 0;

    for (i = 0; i < MPP_ARRAY_ELEMS(p_dec->mem->save); i++) {
        memset(&p_dec->mem->save[i], 0, sizeof(AvsdFrame_t));
        p_dec->mem->save[i].slot_idx = -1;
    }

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*    set refer
***********************************************************************
*/
MPP_RET avsd_set_dpb(AvsdCtx_t *p_dec, HalDecTask *task)
{
    MppFrame mframe = NULL;
    RK_S32 slot_idx = -1;
    AvsdFrame_t *p_cur = p_dec->cur;

    //!< set current dpb for decode
    mpp_buf_slot_get_unused(p_dec->frame_slots, &slot_idx);
    if (slot_idx < 0) {
        AVSD_DBG(AVSD_DBG_WARNNING, "error, buf_slot has not get.\n");
        goto __FAILED;
    }
    //!< init current frame data
    p_cur->slot_idx = slot_idx;
    p_cur->pic_type = p_dec->ph.picture_coding_type;
    p_cur->width = p_dec->vsh.horizontal_size;
    p_cur->height = p_dec->vsh.vertical_size;
    p_cur->hor_stride = MPP_ALIGN(p_dec->vsh.horizontal_size, 16);
    p_cur->ver_stride = MPP_ALIGN(p_dec->vsh.vertical_size, 16);
    p_cur->pts = mpp_packet_get_pts(task->input_packet);
    p_cur->dts = mpp_packet_get_dts(task->input_packet);
    //!< set frame info
    mpp_frame_init(&mframe);
    mpp_frame_set_fmt(mframe, MPP_FMT_YUV420SP);
    mpp_frame_set_hor_stride(mframe, p_cur->hor_stride);  // before crop
    mpp_frame_set_ver_stride(mframe, p_cur->ver_stride);
    mpp_frame_set_width(mframe, p_cur->width);  // after crop
    mpp_frame_set_height(mframe, p_cur->height);
    mpp_frame_set_pts(mframe, p_cur->pts);
    mpp_frame_set_dts(mframe, p_cur->dts);
    if (p_dec->ph.picture_structure) { //!< data combine 2 field
        p_cur->frame_mode = MPP_FRAME_FLAG_PAIRED_FIELD;
        if (p_dec->ph.top_field_first) {
            p_cur->frame_mode |= MPP_FRAME_FLAG_TOP_FIRST;
        } else {
            p_cur->frame_mode |= MPP_FRAME_FLAG_BOT_FIRST;
        }
    } else { //!< frame picture
        p_cur->frame_mode = MPP_FRAME_FLAG_FRAME;
    }
    mpp_frame_set_mode(mframe, p_cur->frame_mode);
    mpp_buf_slot_set_prop(p_dec->frame_slots, slot_idx, SLOT_FRAME, mframe);
    mpp_frame_deinit(&mframe);

    mpp_buf_slot_set_flag(p_dec->frame_slots, p_cur->slot_idx, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(p_dec->frame_slots, p_cur->slot_idx, SLOT_HAL_OUTPUT);

    //!< set task
    task->output = p_dec->cur->slot_idx;
    //!< set task refers
    if (p_dec->dpb[0] && p_dec->dpb[0]->slot_idx >= 0 &&
        (p_dec->dpb[0]->slot_idx != p_dec->cur->slot_idx)) {
        mpp_buf_slot_set_flag(p_dec->frame_slots, p_dec->dpb[0]->slot_idx, SLOT_HAL_INPUT);
        if (p_dec->ph.picture_coding_type == B_PICTURE) {
            task->refer[1] = p_dec->dpb[0]->slot_idx;
        } else {
            task->refer[0] = p_dec->dpb[0]->slot_idx;
        }
    }
    if (p_dec->dpb[1] && p_dec->dpb[1]->slot_idx >= 0 &&
        (p_dec->dpb[1]->slot_idx != p_dec->cur->slot_idx)) {
        mpp_buf_slot_set_flag(p_dec->frame_slots, p_dec->dpb[1]->slot_idx, SLOT_HAL_INPUT);
        if (p_dec->ph.picture_coding_type == B_PICTURE) {
            task->refer[0] = p_dec->dpb[1]->slot_idx;
        } else {
            task->refer[1] = p_dec->dpb[1]->slot_idx;
        }
    }

    //!< set ref flag and mark error
    if (p_dec->ph.picture_coding_type == I_PICTURE) {
        task->flags.used_for_ref = 1;
        task->flags.ref_err = 0;
    } else if (p_dec->ph.picture_coding_type == P_PICTURE) {
        task->flags.used_for_ref = 1;
        if (task->refer[0] >= 0) {
            mpp_buf_slot_get_prop(p_dec->frame_slots, task->refer[0], SLOT_FRAME_PTR, &mframe);
            if (mframe)
                task->flags.ref_err |= mpp_frame_get_errinfo(mframe);
        }
    } else if (p_dec->ph.picture_coding_type == B_PICTURE) {
        task->flags.used_for_ref = 0;
        if (task->refer[0] >= 0) {
            mpp_buf_slot_get_prop(p_dec->frame_slots, task->refer[0], SLOT_FRAME_PTR, &mframe);
            if (mframe)
                task->flags.ref_err |= mpp_frame_get_errinfo(mframe);
        }
        if (task->refer[1] >= 0) {
            mpp_buf_slot_get_prop(p_dec->frame_slots, task->refer[1], SLOT_FRAME_PTR, &mframe);
            if (mframe)
                task->flags.ref_err |= mpp_frame_get_errinfo(mframe);
        }
    }

    return MPP_OK;
__FAILED:
    return MPP_NOK;
}

/*!
***********************************************************************
* \brief
*    commit buffer to hal
***********************************************************************
*/
MPP_RET avsd_commit_syntaxs(AvsdSyntax_t *syn, HalDecTask *task)
{
    task->syntax.number = 1;
    task->syntax.data = syn;

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*    commit buffer to hal
***********************************************************************
*/
MPP_RET avsd_update_dpb(AvsdCtx_t *p_dec)
{
    if (p_dec->ph.picture_coding_type != B_PICTURE) {
        set_frame_output(p_dec, p_dec->dpb[0]);
        set_frame_unref(p_dec, p_dec->dpb[1]);
        p_dec->dpb[1] = p_dec->dpb[0];
        p_dec->dpb[0] = p_dec->cur;
        p_dec->cur = NULL;
    } else {
        set_frame_output(p_dec, p_dec->cur);
        set_frame_unref(p_dec, p_dec->cur);
        p_dec->cur = NULL;
    }

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*    fill parameters
***********************************************************************
*/
MPP_RET avsd_fill_parameters(AvsdCtx_t *p_dec, AvsdSyntax_t *syn)
{
    RK_S32 i = 0;
    PicParams_Avsd *pp = &syn->pp;

    //!< sequence header
    pp->profileId           = p_dec->vsh.profile_id;
    pp->levelId             = p_dec->vsh.level_id;
    pp->progressiveSequence = p_dec->vsh.progressive_sequence;
    pp->horizontalSize      = p_dec->vsh.horizontal_size;
    pp->verticalSize        = p_dec->vsh.vertical_size;
    pp->chromaFormat        = p_dec->vsh.chroma_format;
    pp->aspectRatio         = p_dec->vsh.aspect_ratio;
    pp->frameRateCode       = p_dec->vsh.frame_rate_code;
    pp->bitRateValue        = p_dec->vsh.bit_rate;
    pp->lowDelay            = p_dec->vsh.low_delay;
    pp->bbvBufferSize       = p_dec->vsh.bbv_buffer_size;

    //!< sequence display extension header
    pp->videoFormat             = p_dec->ext.video_format;
    pp->sampleRange             = p_dec->ext.sample_range;
    pp->colorDescription        = p_dec->ext.color_description;
    pp->colorPrimaries          = p_dec->ext.color_primaries;
    pp->transferCharacteristics = p_dec->ext.transfer_characteristics;
    pp->matrixCoefficients      = p_dec->ext.matrix_coefficients;
    pp->displayHorizontalSize   = p_dec->ext.display_horizontalSize;
    pp->displayVerticalSize     = p_dec->ext.display_verticalSize;

    //!< picture header
    pp->picCodingType           = p_dec->ph.picture_coding_type;
    pp->bbvDelay                = p_dec->ph.bbv_delay;
    pp->bbvDelayExtension       = p_dec->ph.bbv_delay_extension;
    pp->timeCodeFlag            = p_dec->ph.time_code_flag;
    pp->timeCode                = p_dec->ph.time_code;

    pp->pictureDistance         = p_dec->ph.picture_distance;
    pp->progressiveFrame        = p_dec->ph.progressive_frame;
    pp->pictureStructure        = p_dec->ph.picture_structure;
    pp->advancedPredModeDisable = p_dec->ph.advanced_pred_mode_disable;
    pp->topFieldFirst           = p_dec->ph.top_field_first;
    pp->repeatFirstField        = p_dec->ph.repeat_first_field;
    pp->fixedPictureQp          = p_dec->ph.fixed_picture_qp;
    pp->pictureQp               = p_dec->ph.picture_qp;
    pp->pictureReferenceFlag    = p_dec->ph.picture_reference_flag;
    pp->skipModeFlag            = p_dec->ph.skip_mode_flag;
    pp->loopFilterDisable       = p_dec->ph.loop_filter_disable;
    pp->alphaOffset             = p_dec->ph.alpha_c_offset;
    pp->betaOffset              = p_dec->ph.beta_offset;

    //!< weighting quant, AVS Plus stuff
    pp->weightingQuantFlag = p_dec->ph.weighting_quant_flag;
    pp->chromaQuantParamDisable = p_dec->ph.chroma_quant_param_disable;
    pp->chromaQuantParamDeltaCb = p_dec->ph.chroma_quant_param_delta_cb;
    pp->chromaQuantParamDeltaCr = p_dec->ph.chroma_quant_param_delta_cr;
    pp->weightingQuantParamIndex = p_dec->ph.weighting_quant_param_index;
    pp->weightingQuantModel = p_dec->ph.weighting_quant_model;
    for (i = 0; i < 6; i++) {
        pp->weightingQuantParamDelta1[i] = p_dec->ph.weighting_quant_param_delta1[i];
        pp->weightingQuantParamDelta2[i] = p_dec->ph.weighting_quant_param_delta2[i];
        pp->weightingQuantParam[i] = p_dec->ph.weighting_quant_param[i];
    }
    //!< advance entropy coding
    pp->aecEnable = p_dec->ph.aec_enable;

    //!< picture enhance
    pp->noForwardReferenceFlag = p_dec->ph.no_forward_reference_flag;
    pp->pbFieldEnhancedFlag = p_dec->ph.pb_field_enhanced_flag;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    prepare function for parser
***********************************************************************
*/
MPP_RET avsd_parse_prepare(AvsdCtx_t *p_dec, MppPacket *pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8  *p_curdata = NULL;
    RK_U8  *p_start = NULL;  //!< store nalu start
    RK_U32 nalu_len = 0;
    RK_U8  got_frame_flag = 0;
    RK_U8  got_nalu_flag = 0;
    RK_U32 pkt_length = 0;

    RK_U32 prefix = 0xFFFFFFFF;

    AVSD_PARSE_TRACE("In.");
    //!< check input
    if (mpp_packet_get_length(pkt) < 4) {
        AVSD_DBG(AVSD_DBG_WARNNING, "input have no stream.");
        mpp_packet_set_length(pkt, 0);
        goto __RETURN;
    }

    pkt_length = (RK_U32)mpp_packet_get_length(pkt);
    p_curdata = p_start = (RK_U8 *)mpp_packet_get_pos(pkt);

    while (pkt_length > 0) {
        //!<  found next nalu start code
        if ((prefix & 0xFFFFFF00) == 0x00000100) {
            if (got_nalu_flag)  {
                nalu_len = (RK_U32)(p_curdata - p_start) - 4;
                FUN_CHECK(ret = store_cur_nalu(p_dec, p_start, nalu_len));
            }
            FUN_CHECK(ret = add_nalu_header(p_dec, prefix));
            p_start = p_curdata - 4;
            got_nalu_flag = 1;
        }
        //!< found next picture start code
        if (prefix == I_PICUTRE_START_CODE || prefix == PB_PICUTRE_START_CODE) {
            task->valid = 1;
            if (got_frame_flag) {
                p_dec->nal->eof = 1;
                pkt_length += 4;
                break;
            }
            got_frame_flag = 1;
        }

        prefix = (prefix << 8) | (*p_curdata);
        p_curdata++;
        pkt_length--;
    }
    //!< reach the packet end
    if (!pkt_length) {
        nalu_len = (RK_U32)(p_curdata - p_start);
        FUN_CHECK(ret = store_cur_nalu(p_dec, p_start, nalu_len));
        if (task->valid) {
            FUN_CHECK(ret = add_nalu_header(p_dec, 0));
            p_dec->nal->eof = 1;
        }
    }
    //!< reset position
    pkt_length = (RK_U32)mpp_packet_get_length(pkt) - pkt_length;
    mpp_packet_set_pos(pkt, (RK_U8 *)mpp_packet_get_pos(pkt) + pkt_length);

__RETURN:
    AVSD_PARSE_TRACE("Out.");

    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    parse stream which ha function for parser
***********************************************************************
*/
MPP_RET avsd_parse_stream(AvsdCtx_t *p_dec, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    RK_U8 pic_type = 0;
    AvsdNalu_t *p_nalu = (AvsdNalu_t *)p_dec->p_header->pbuf;

    task->valid = 0;
    while (!p_nalu->eof) {
        RK_U32 startcode = p_nalu->header;

        if (startcode >= SLICE_MIN_START_CODE && startcode <= SLICE_MAX_START_CODE) {
            p_nalu++;
        } else {
            memset(&p_dec->bitctx, 0, sizeof(BitReadCtx_t));
            mpp_set_bitread_ctx(&p_dec->bitctx, p_nalu->pdata + p_nalu->start_pos, p_nalu->length);
            p_nalu = (AvsdNalu_t *)(p_nalu->pdata + p_nalu->length);
        }

        if (startcode != VIDEO_SEQUENCE_START_CODE && !p_dec->got_vsh) {
            // when has not got sequence header, then do nothing
            continue;
        }
        switch (startcode) {
        case VIDEO_SEQUENCE_START_CODE:
            ret = get_sequence_header(&p_dec->bitctx, &p_dec->vsh);
            if (ret == MPP_OK) {
                p_dec->got_vsh = 1;
            }
            break;
        case VIDEO_SEQUENCE_END_CODE:
            break;
        case USER_DATA_CODE:
            break;
        case VIDEO_EDIT_CODE:
            p_dec->vec_flag = 0;
            break;
        case I_PICUTRE_START_CODE:
            if (!p_dec->got_keyframe) {
                avsd_reset_parameters(p_dec);
                p_dec->got_keyframe = 1;
            }
            ret = get_i_picture_header(&p_dec->bitctx, &p_dec->vsh, &p_dec->ph);
            if (ret == MPP_OK) {
                p_dec->cur = get_one_save(p_dec, task);
            }
            p_dec->cur->pic_type = pic_type = I_PICTURE;
            p_dec->vec_flag++;
            break;
        case EXTENSION_START_CODE:
            ret = get_extension_header(&p_dec->bitctx, &p_dec->ext);
            break;
        case PB_PICUTRE_START_CODE:
            if (!p_dec->got_keyframe) {
                avsd_reset_parameters(p_dec);
                break;
            }
            ret = get_pb_picture_header(&p_dec->bitctx, &p_dec->vsh, &p_dec->ph);
            if (ret == MPP_OK) {
                p_dec->cur = get_one_save(p_dec, task);
            }
            p_dec->cur->pic_type = pic_type = p_dec->ph.picture_coding_type;
            p_dec->vec_flag += (p_dec->vec_flag == 1 && pic_type == P_PICTURE);
            break;
        default:
            if (!p_dec->cur
                || (pic_type == P_PICTURE && !p_dec->dpb[0])
                || (pic_type == B_PICTURE && !p_dec->dpb[0])
                || (pic_type == B_PICTURE && !p_dec->dpb[1] && !p_dec->vsh.low_delay)
                || (pic_type == P_PICTURE && p_dec->vec_flag < 1)
                || (pic_type == B_PICTURE && p_dec->vec_flag < 2)) {
                mpp_err_f("missing refer frame.\n");
                ret = MPP_NOK;
                goto __FAILED;
            }
            if (startcode >= SLICE_MIN_START_CODE
                && startcode <= SLICE_MAX_START_CODE) {
                task->valid = 1;
            }

            break;
        }
    }

    return ret = MPP_OK;
__FAILED:
    reset_one_save(p_dec->cur);

    return ret;
}




