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

#define MODULE_TAG "hal_avsd_reg"

#include "mpp_log.h"
#include "mpp_common.h"

#include "avsd_syntax.h"
#include "hal_avsd_api.h"
#include "hal_avsd_reg.h"



static RK_S32 get_queue_pic(AvsdHalCtx_t *p_hal)
{
    RK_U32 i = 0;
    RK_S32 ret_idx = -1;

    for (i = 0; i < MPP_ARRAY_ELEMS(p_hal->pic); i++) {
        if (!p_hal->pic[i].valid) {
            ret_idx = i;
            p_hal->pic[i].valid = 1;
            break;
        }
    }

    return ret_idx;
}

static RK_S32 get_packet_fd(AvsdHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->packet_slots, idx, SLOT_BUFFER, &mbuffer);
    ret_fd =  mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}
static RK_S32 get_frame_fd(AvsdHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->frame_slots, idx, SLOT_BUFFER, &mbuffer);
    ret_fd = mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}
/*!
***********************************************************************
* \brief
*    init decoder parameters
***********************************************************************
*/
//extern "C"
MPP_RET set_defalut_parameters(AvsdHalCtx_t *p_hal)
{
    AvsdRegs_t *p_regs = (AvsdRegs_t *)p_hal->p_regs;

    p_regs->sw02.dec_out_endian = 1;
    p_regs->sw02.dec_in_endian = 0;
    p_regs->sw02.dec_strendian_e = 1;
    p_regs->sw02.dec_max_burst = 16;
    p_regs->sw02.dec_scmd_dis = 0;

    p_regs->sw02.dec_adv_pre_dis = 0;
    p_regs->sw55.apf_threshold = 8;

    p_regs->sw02.dec_latency = 0;
    p_regs->sw02.dec_data_disc_e = 0;
    p_regs->sw02.dec_outswap32_e = 1;
    p_regs->sw02.dec_inswap32_e = 1;
    p_regs->sw02.dec_strswap32_e = 1;

    p_regs->sw02.dec_timeout_e = 0;
    p_regs->sw02.dec_clk_gate_e = 1;
    p_regs->sw01.dec_irq_dis = 0;

    p_regs->sw58.serv_merge_dis = 0;
    p_regs->sw02.dec_axi_rd_id = 0xFF;
    p_regs->sw03.dec_axi_wr_id = 0;

    p_regs->sw49.pred_bc_tap_0_0 = -1;
    p_regs->sw49.pred_bc_tap_0_1 = 5;
    p_regs->sw49.pred_bc_tap_0_2 = 5;
    p_regs->sw34.pred_bc_tap_0_3 = -1;

    p_regs->sw34.pred_bc_tap_1_0 = 1;
    p_regs->sw34.pred_bc_tap_1_1 = 7;
    p_regs->sw35.pred_bc_tap_1_2 = 7;
    p_regs->sw35.pred_bc_tap_1_3 = 1;

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*    generate register parameters
***********************************************************************
*/
//extern "C"
MPP_RET set_regs_parameters(AvsdHalCtx_t *p_hal, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AvsdSyntax_t *p_syn = &p_hal->syn;
    AvsdRegs_t *p_regs = (AvsdRegs_t *)p_hal->p_regs;

    //!< set wrok_out pic info
    if (p_hal->work_out < 0) {
        p_hal->work_out = get_queue_pic(p_hal);
        if (p_hal->work_out < 0) {
            ret = MPP_NOK;
            mpp_err_f("cannot get a pic_info buffer.\n");
            goto __FAILED;
        }
    }
    {
        AvsdHalPic_t *p_work_out = &p_hal->pic[p_hal->work_out];

        p_work_out->slot_idx = task->output;
        p_work_out->pic_code_type = p_syn->pp.picCodingType;
        p_work_out->picture_distance = p_syn->pp.pictureDistance;
    }
    //!< set register
    p_regs->sw04.pic_mb_width = (p_syn->pp.horizontalSize + 15) >> 4;
    p_regs->sw03.dec_mode = 11; //!< DEC_MODE_AVS

    if (p_syn->pp.pictureStructure == FRAMEPICTURE) {
        p_regs->sw03.pic_interlace_e = 0;
        p_regs->sw03.pic_fieldmode_e = 0;
        p_regs->sw03.pic_topfiled_e = 0;
    } else {
        p_regs->sw03.pic_interlace_e = 1;
        p_regs->sw03.pic_fieldmode_e = 1;
        if (p_syn->pp.topFieldFirst) {
            p_regs->sw03.pic_topfiled_e = p_hal->first_field;
        } else {
            p_regs->sw03.pic_topfiled_e = !p_hal->first_field;
        }
    }

    p_regs->sw04.pic_mb_height_p = (p_syn->pp.verticalSize + 15) >> 4;
    p_regs->sw07.avs_h264_h_ext = (p_syn->pp.verticalSize + 15) >> 12;

    if (p_syn->pp.picCodingType == BFRAME) {
        p_regs->sw03.pic_b_e = 1;
    } else {
        p_regs->sw03.pic_b_e = 0;
    }
    p_regs->sw03.pic_inter_e = (p_syn->pp.picCodingType != IFRAME) ? 1 : 0;

    p_regs->sw05.strm_start_bit = 8 * (p_hal->data_offset & 0x7);
    p_hal->data_offset = (p_hal->data_offset & ~0x7);
    p_regs->sw12.rlc_vlc_base = get_packet_fd(p_hal, task->input) | (p_hal->data_offset << 10);
    p_regs->sw06.stream_len = p_syn->bitstream_size - p_hal->data_offset;
    p_regs->sw03.pic_fixed_quant = p_syn->pp.fixedPictureQp;
    p_regs->sw06.init_qp = p_syn->pp.pictureQp;
    //!< AVS Plus stuff
    if (p_syn->pp.profileId == 0x48) {
        p_regs->sw44.dec_avsp_ena = 1;
    } else {
        p_regs->sw44.dec_avsp_ena = 0;
    }
    if (p_regs->sw44.dec_avsp_ena) {
        p_regs->sw44.weight_qp_e = p_syn->pp.weightingQuantFlag;
        p_regs->sw44.avs_aec_e = p_syn->pp.aecEnable;
        p_regs->sw44.no_fwd_ref_e = p_syn->pp.noForwardReferenceFlag;
        p_regs->sw44.pb_field_enhance_e = p_syn->pp.pbFieldEnhancedFlag;

        if (p_syn->pp.weightingQuantFlag
            && !p_syn->pp.chromaQuantParamDisable) {
            p_regs->sw44.qp_delta_cb = p_syn->pp.chromaQuantParamDeltaCb;
            p_regs->sw44.qp_delta_cr = p_syn->pp.chromaQuantParamDeltaCr;
        } else {
            p_regs->sw44.qp_delta_cb = 0;
            p_regs->sw44.qp_delta_cr = 0;
        }
        if (p_syn->pp.weightingQuantFlag) {
            p_regs->sw44.weight_qp_model = p_syn->pp.weightingQuantModel;
            p_regs->sw44.weight_qp_0 = p_syn->pp.weightingQuantParam[0];
            p_regs->sw42.weight_qp_1 = p_syn->pp.weightingQuantParam[1];
            p_regs->sw43.weight_qp_2 = p_syn->pp.weightingQuantParam[2];
            p_regs->sw43.weight_qp_3 = p_syn->pp.weightingQuantParam[3];
            p_regs->sw43.weight_qp_4 = p_syn->pp.weightingQuantParam[4];
            p_regs->sw43.weight_qp_5 = p_syn->pp.weightingQuantParam[5];
        }
    }
    //!< AVS Plus end
    p_regs->sw13.dpb_ilace_mode = 0;
    if (p_syn->pp.pictureStructure == FRAMEPICTURE || p_hal->first_field) {
        p_regs->sw13.dec_out_base = get_frame_fd(p_hal, task->output);
    } else {
        //!< start of bottom field line
        RK_U32 stride = p_syn->pp.horizontalSize;
        p_regs->sw13.dec_out_base = get_frame_fd(p_hal, task->output) | (stride << 10);
    }
    {
        RK_S32 tmp_fwd = -1;
        RK_S32 refer0 = -1;
        RK_S32 refer1 = -1;

        tmp_fwd = (p_hal->work1 < 0) ? p_hal->work0 : p_hal->work1;
        tmp_fwd = (tmp_fwd < 0) ? p_hal->work_out : tmp_fwd;

        refer0 = (task->refer[0] < 0) ? task->output : task->refer[0];
        refer1 = (task->refer[1] < 0) ? refer0 : task->refer[1];
        if (!p_hal->first_field
            && p_syn->pp.pictureStructure == FIELDPICTURE
            && p_syn->pp.picCodingType != BFRAME) {
            p_regs->sw14.refer0_base = get_frame_fd(p_hal, task->output);
            p_regs->sw15.refer1_base = get_frame_fd(p_hal, refer0);
            p_regs->sw16.refer2_base = get_frame_fd(p_hal, refer0);
            p_regs->sw17.refer3_base = get_frame_fd(p_hal, refer1);
        } else {
            p_regs->sw14.refer0_base = get_frame_fd(p_hal, refer0);
            p_regs->sw15.refer1_base = get_frame_fd(p_hal, refer0);
            p_regs->sw16.refer2_base = get_frame_fd(p_hal, refer1);
            p_regs->sw17.refer3_base = get_frame_fd(p_hal, refer1);
        }
    }
    //!< block distances
    if (p_syn->pp.pictureStructure == FRAMEPICTURE) {
        if (p_syn->pp.picCodingType == BFRAME) {
            RK_S32 tmp = 0;
            //!< current to future anchor
            if (p_hal->work0 >= 0) {
                tmp = (2 * p_hal->pic[p_hal->work0].picture_distance -
                       2 * p_syn->pp.pictureDistance + 512) & 0x1FF;
            }
            //!< prevent division by zero
            if (!tmp) tmp = 2;
            p_regs->sw31.ref_dist_cur_2 = tmp;
            p_regs->sw31.ref_dist_cur_3 = tmp;
            p_regs->sw29.ref_invd_cur_2 = 512 / tmp;
            p_regs->sw29.ref_invd_cur_3 = 512 / tmp;

            //!< current to past anchor
            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            p_regs->sw30.ref_dist_cur_0 = tmp;
            p_regs->sw30.ref_dist_cur_1 = tmp;
            p_regs->sw28.ref_invd_cur_0 = 512 / tmp;
            p_regs->sw28.ref_invd_cur_1 = 512 / tmp;
            //!< future anchor to past anchor
            if (p_hal->work0 >= 0 && p_hal->work1 >= 0) {
                tmp = (2 * p_hal->pic[p_hal->work0].picture_distance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            tmp = 16384 / tmp;
            p_regs->sw32.ref_invd_col_0 = tmp;
            p_regs->sw32.ref_invd_col_1 = tmp;
            //!< future anchor to previous past anchor
            tmp = p_hal->future2prev_past_dist;
            tmp = 16384 / tmp;
            p_regs->sw33.ref_invd_col_2 = tmp;
            p_regs->sw33.ref_invd_col_3 = tmp;
        } else {
            RK_S32 tmp = 0;
            //!< current to past anchor
            if (p_hal->work0 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work0].picture_distance - 512) & 0x1FF;
            }
            if (!tmp) tmp = 2;
            p_regs->sw30.ref_dist_cur_0 = tmp;
            p_regs->sw30.ref_dist_cur_1 = tmp;
            p_regs->sw28.ref_invd_cur_0 = 512 / tmp;
            p_regs->sw28.ref_invd_cur_1 = 512 / tmp;
            //!< current to previous past anchor
            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            //!< this will become "future to previous past" for next B
            p_hal->future2prev_past_dist = tmp;

            p_regs->sw31.ref_dist_cur_2 = tmp;
            p_regs->sw31.ref_dist_cur_3 = tmp;
            p_regs->sw29.ref_invd_cur_2 = 512 / tmp;
            p_regs->sw29.ref_invd_cur_3 = 512 / tmp;

            p_regs->sw32.ref_invd_col_0 = 0;
            p_regs->sw32.ref_invd_col_1 = 0;
            p_regs->sw33.ref_invd_col_2 = 0;
            p_regs->sw33.ref_invd_col_3 = 0;
        }
    } else {
        //!< field interlaced
        if (p_syn->pp.picCodingType == BFRAME) {
            RK_S32 tmp = 0;
            //!< block distances
            if (p_hal->work0 >= 0) {
                tmp = (2 * p_hal->pic[p_hal->work0].picture_distance -
                       2 * p_syn->pp.pictureDistance + 512) & 0x1FF;
            }
            //!< prevent division by zero
            if (!tmp) tmp = 2;

            if (p_hal->first_field) {
                p_regs->sw31.ref_dist_cur_2 = tmp;
                p_regs->sw31.ref_dist_cur_3 = tmp + 1;
                p_regs->sw29.ref_invd_cur_2 = 512 / tmp;
                p_regs->sw29.ref_invd_cur_3 = 512 / (tmp + 1);
            } else {
                p_regs->sw31.ref_dist_cur_2 = tmp - 1;
                p_regs->sw31.ref_dist_cur_3 = tmp;
                p_regs->sw29.ref_invd_cur_2 = 512 / (tmp - 1);
                p_regs->sw29.ref_invd_cur_3 = 512 / tmp;
            }

            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            if (p_hal->first_field) {
                p_regs->sw30.ref_dist_cur_0 = (tmp - 1);
                p_regs->sw30.ref_dist_cur_1 = tmp;
                p_regs->sw28.ref_invd_cur_0 = 512 / (tmp - 1);
                p_regs->sw28.ref_invd_cur_1 = 512 / tmp;
            } else {
                p_regs->sw30.ref_dist_cur_0 = tmp;
                p_regs->sw30.ref_dist_cur_1 = tmp + 1;
                p_regs->sw28.ref_invd_cur_0 = 512 / tmp;
                p_regs->sw28.ref_invd_cur_1 = 512 / (tmp + 1);
            }

            if (p_hal->work0 >= 0 && p_hal->work1 >= 0) {
                tmp = (2 * p_hal->pic[p_hal->work0].picture_distance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            //!< AVS Plus stuff
            if (p_syn->pp.pbFieldEnhancedFlag && !p_hal->first_field) {
                //!< in this case, BlockDistanceRef is different with before, the mvRef points to top field
                p_regs->sw32.ref_invd_col_0 = 16384 / (tmp - 1);
                p_regs->sw32.ref_invd_col_1 = 16384 / tmp;

                //!< future anchor to previous past anchor
                tmp = p_hal->future2prev_past_dist;

                p_regs->sw33.ref_invd_col_2 = 16384 / (tmp - 1);
                p_regs->sw33.ref_invd_col_3 = 16384 / tmp;
            } else {
                if (p_hal->first_field) {
                    p_regs->sw32.ref_invd_col_0 = 16384 / (tmp - 1);
                    p_regs->sw32.ref_invd_col_1 = 16384 / tmp;
                } else {
                    p_regs->sw32.ref_invd_col_0 = 16384;
                    p_regs->sw32.ref_invd_col_1 = 16384 / tmp;
                    p_regs->sw33.ref_invd_col_2 = 16384 / (tmp + 1);
                }

                //!< future anchor to previous past anchor
                tmp = p_hal->future2prev_past_dist;

                if (p_hal->first_field) {
                    p_regs->sw33.ref_invd_col_2 = 16384 / (tmp - 1);
                    p_regs->sw33.ref_invd_col_3 = 16384 / tmp;
                } else {
                    p_regs->sw33.ref_invd_col_3 = 16384 / tmp;
                }
            }
        } else {
            RK_S32 tmp = 0;
            if (p_hal->work0 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work0].picture_distance - 512) & 0x1FF;
            }
            if (!tmp) tmp = 2;

            if (!p_hal->first_field) {
                p_regs->sw30.ref_dist_cur_0 = 1;
                p_regs->sw31.ref_dist_cur_2 = tmp + 1;

                p_regs->sw28.ref_invd_cur_0 = 512;
                p_regs->sw29.ref_invd_cur_2 = 512 / (tmp + 1);
            } else {
                p_regs->sw30.ref_dist_cur_0 = tmp - 1;
                p_regs->sw28.ref_invd_cur_0 = 512 / (tmp - 1);
            }
            p_regs->sw30.ref_dist_cur_1 = tmp;
            p_regs->sw28.ref_invd_cur_1 = 512 / tmp;

            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            //!< this will become "future to previous past" for next B
            p_hal->future2prev_past_dist = tmp;
            if (p_hal->first_field) {
                p_regs->sw31.ref_dist_cur_2 = tmp - 1;
                p_regs->sw31.ref_dist_cur_3 = tmp;

                p_regs->sw29.ref_invd_cur_2 = 512 / (tmp - 1);
                p_regs->sw29.ref_invd_cur_3 = 512 / tmp;
            } else {
                p_regs->sw31.ref_dist_cur_3 = tmp;
                p_regs->sw29.ref_invd_cur_3 = 512 / tmp;
            }

            p_regs->sw32.ref_invd_col_0 = 0;
            p_regs->sw32.ref_invd_col_1 = 0;
            p_regs->sw33.ref_invd_col_2 = 0;
            p_regs->sw33.ref_invd_col_3 = 0;
        }
    }
    //!< AVS Plus stuff
    if (p_regs->sw44.dec_avsp_ena) {
        p_regs->sw42.ref_delta_col_0 = 0;
        p_regs->sw42.ref_delta_col_1 = 0;
        p_regs->sw42.ref_delta_col_2 = 0;
        p_regs->sw42.ref_delta_col_3 = 0;
        p_regs->sw42.ref_delta_cur_0 = 0;
        p_regs->sw42.ref_delta_cur_1 = 0;
        p_regs->sw42.ref_delta_cur_2 = 0;
        p_regs->sw42.ref_delta_cur_3 = 0;
        if (p_syn->pp.pictureStructure == FIELDPICTURE
            && p_syn->pp.picCodingType == BFRAME) {
            //!< 1 means delta=2, 3 means delta=-2, 0 means delta=0
            //!< delta1
            p_regs->sw42.ref_delta_col_0 = 2;
            p_regs->sw42.ref_delta_col_1 = 0;
            p_regs->sw42.ref_delta_col_2 = 2;
            p_regs->sw42.ref_delta_col_3 = 0;
            if (p_hal->first_field) {
                //!< deltaFw
                p_regs->sw42.ref_delta_cur_0 = 2;
                p_regs->sw42.ref_delta_cur_1 = 0;
                //!< deltaBw
                p_regs->sw42.ref_delta_cur_2 = 0;
                p_regs->sw42.ref_delta_cur_3 = 0;
            } else {
                //!< deltaFw
                p_regs->sw42.ref_delta_cur_0 = 0;
                p_regs->sw42.ref_delta_cur_1 = 0;
                //!< deltaBw
                p_regs->sw42.ref_delta_cur_2 = 6; //!< (RK_U32)-2
                p_regs->sw42.ref_delta_cur_3 = 6; //!< (RK_U32)-2
            }
        }
    }
    //!< AVS Plus end

    p_regs->sw48.startmb_x = 0;
    p_regs->sw48.startmb_y = 0;

    p_regs->sw03.filtering_dis = p_syn->pp.loopFilterDisable;
    p_regs->sw05.alpha_offset = p_syn->pp.alphaOffset;
    p_regs->sw05.beta_offset = p_syn->pp.betaOffset;
    p_regs->sw03.skip_mode = p_syn->pp.skipModeFlag;
    p_regs->sw04.pic_refer_flag = p_syn->pp.pictureReferenceFlag;

    //!< AVS Plus change
    p_regs->sw03.write_mvs_e = 0;
    if (p_regs->sw44.dec_avsp_ena) {
        if (p_syn->pp.picCodingType == PFRAME
            || p_syn->pp.picCodingType == IFRAME) {
            p_regs->sw03.write_mvs_e = 1;
        }
    } else {
        if (p_syn->pp.picCodingType == PFRAME
            || (p_syn->pp.picCodingType == IFRAME && !p_hal->first_field)) {
            p_regs->sw03.write_mvs_e = 1;
        }
    }
    //!< AVS Plus end
    //!< set mv base
    if (p_hal->first_field ||
        (p_syn->pp.picCodingType == BFRAME && p_hal->prev_pic_structure)) {
        p_regs->sw41.dir_mv_base = mpp_buffer_get_fd(p_hal->mv_buf);
    } else {
        RK_U32 offset = MPP_ALIGN(p_syn->pp.horizontalSize, 16)
                        * MPP_ALIGN(p_syn->pp.verticalSize, 16) / 32;
        p_regs->sw41.dir_mv_base = mpp_buffer_get_fd(p_hal->mv_buf) | (offset << 10);
    }
    //!< AVS Plus stuff
    if (p_regs->sw44.dec_avsp_ena) {
        p_regs->sw45.dir_mv_base2 = mpp_buffer_get_fd(p_hal->mv_buf);
    }
    //!< AVS Plus end
    {
        RK_U32 pic_type = 0;
        RK_U32 prev_anc_type = 0;
        if (p_hal->work0 >= 0) {
            pic_type = p_hal->pic[p_hal->work0].pic_type;
        }
        prev_anc_type = !pic_type || (!p_hal->first_field && !p_hal->prev_pic_structure);
        p_regs->sw18.prev_anc_type = prev_anc_type;
    }
    //!< b-picture needs to know if future reference is field or frame coded
    p_regs->sw16.refer2_field_e = (!p_hal->prev_pic_structure) ? 1 : 0;
    p_regs->sw17.refer3_field_e = (!p_hal->prev_pic_structure) ? 1 : 0;

    p_regs->sw03.dec_out_dis = 0;
    p_regs->sw01.dec_e = 1;

    return MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*    init decoder parameters
***********************************************************************
*/
//extern "C"
MPP_RET update_parameters(AvsdHalCtx_t *p_hal)
{
    AvsdSyntax_t *p_syn = &p_hal->syn;

    if (p_syn->pp.pictureStructure == FRAMEPICTURE || !p_hal->first_field) {
        p_hal->first_field = 1;
        if (p_syn->pp.picCodingType != BFRAME) {
            RK_S32 temp = p_hal->work1;

            p_hal->work1 =  p_hal->work0;
            p_hal->work0 =  p_hal->work_out;

            p_hal->pic[p_hal->work_out].pic_type = p_syn->pp.picCodingType == IFRAME;
            p_hal->work_out = temp;
            p_hal->prev_pic_structure = p_syn->pp.pictureStructure;
        }
        p_hal->prev_pic_code_type = p_syn->pp.picCodingType;
    } else {
        p_hal->first_field = 0;
    }

    return MPP_OK;
}
