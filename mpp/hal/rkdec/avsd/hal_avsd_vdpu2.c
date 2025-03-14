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

#define MODULE_TAG "hal_avsd_vdpu2"

#include <string.h>

#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_device.h"
#include "mpp_dec_cb_param.h"

#include "hal_avsd_base.h"
#include "hal_avsd_vdpu2.h"
#include "hal_avsd_vdpu2_reg.h"

static MPP_RET set_defalut_parameters(AvsdHalCtx_t *p_hal)
{
    AvsdVdpu2Regs_t *p_regs = (AvsdVdpu2Regs_t *)p_hal->p_regs;

    p_regs->sw54.dec_out_endian = 1;
    p_regs->sw54.dec_in_endian = 0;
    p_regs->sw54.dec_strendian_e = 1;
    p_regs->sw56.dec_max_burlen = 16;
    p_regs->sw50.dec_ascmd0_dis = 0;

    p_regs->sw50.adv_pref_dis = 0;
    p_regs->sw52.adv_pref_thrd = 8;
    p_regs->sw50.adtion_latency = 0;

    p_regs->sw56.dec_data_discd_en = 0;
    p_regs->sw54.dec_out_wordsp = 1;
    p_regs->sw54.dec_in_wordsp = 1;
    p_regs->sw54.dec_strm_wordsp = 1;
    p_regs->sw55.timeout_det_sts = 0;
    p_regs->sw57.dec_clkgate_en = 1;
    p_regs->sw55.dec_irq_dis = 0;

    p_regs->sw56.dec_axi_id_rd = 0xFF;
    p_regs->sw56.dec_axi_id_wr = 0;

    p_regs->sw59.pred_bc_tap_0_0 = 0x3FF;
    p_regs->sw59.pred_bc_tap_0_1 = 5;
    p_regs->sw59.pred_bc_tap_0_2 = 5;
    p_regs->sw153.pred_bc_tap_0_3 = 0x3FF;
    p_regs->sw153.pred_bc_tap_1_0 = 1;
    p_regs->sw153.pred_bc_tap_1_1 = 7;
    p_regs->sw154.pred_bc_tap_1_2 = 7;
    p_regs->sw154.pred_bc_tap_1_3 = 1;

    p_regs->sw50.dec_tiled_lsb = 0;

    return MPP_OK;
}

static MPP_RET set_regs_parameters(AvsdHalCtx_t *p_hal, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AvsdSyntax_t *p_syn = &p_hal->syn;
    AvsdVdpu2Regs_t *p_regs = (AvsdVdpu2Regs_t *)p_hal->p_regs;

    set_defalut_parameters(p_hal);

    p_regs->sw57.timeout_sts_en = 1;
    p_regs->sw57.dec_clkgate_en = 1;
    p_regs->sw55.dec_irq_dis = 0;

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
    p_regs->sw120.pic_mb_width = (p_syn->pp.horizontalSize + 15) >> 4;
    p_regs->sw53.dec_fmt_sel = 11; //!< DEC_MODE_AVS

    if (p_syn->pp.pictureStructure == FRAMEPICTURE) {
        p_regs->sw57.pic_interlace_e = 0; //!< interlace_e
        p_regs->sw57.pic_fieldmode_e = 0; //!< fieldmode_e
        p_regs->sw57.pic_topfield_e = 0; //!< topfield_e
    } else {
        p_regs->sw57.pic_interlace_e = 1;
        p_regs->sw57.pic_fieldmode_e = 1;
        p_regs->sw57.pic_topfield_e = p_hal->first_field;
    }

    p_regs->sw120.pic_mb_height_p = (p_syn->pp.verticalSize + 15) >> 4;
    p_regs->sw121.avs_h_ext = (p_syn->pp.verticalSize + 15) >> 12;
    p_regs->sw57.pic_b_e = (p_syn->pp.picCodingType == BFRAME);
    p_regs->sw57.pic_inter_e = (p_syn->pp.picCodingType != IFRAME) ? 1 : 0;

    p_regs->sw122.strm_start_bit = 8 * (p_hal->data_offset & 0x7);
    p_hal->data_offset = (p_hal->data_offset & ~0x7);
    p_regs->sw64.rlc_vlc_st_adr = get_packet_fd(p_hal, task->input);
    mpp_dev_set_reg_offset(p_hal->dev, 64, p_hal->data_offset);
    p_regs->sw51.stream_len = p_syn->bitstream_size - p_hal->data_offset;
    p_regs->sw50.dec_fixed_quant = p_syn->pp.fixedPictureQp;
    p_regs->sw51.init_qp = p_syn->pp.pictureQp;

    p_regs->sw63.dec_out_st_adr = get_frame_fd(p_hal, task->output);
    if (p_syn->pp.pictureStructure == FIELDPICTURE && !p_hal->first_field) {
        //!< start of bottom field line
        mpp_dev_set_reg_offset(p_hal->dev, 63, p_syn->pp.horizontalSize);
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
            p_regs->sw131.refer0_base = get_frame_fd(p_hal, task->output);
            p_regs->sw148.refer1_base = get_frame_fd(p_hal, refer0);
            p_regs->sw134.refer2_base = get_frame_fd(p_hal, refer0);
            p_regs->sw135.refer3_base = get_frame_fd(p_hal, refer1);
        } else {
            p_regs->sw131.refer0_base = get_frame_fd(p_hal, refer0);
            p_regs->sw148.refer1_base = get_frame_fd(p_hal, refer0);
            p_regs->sw134.refer2_base = get_frame_fd(p_hal, refer1);
            p_regs->sw135.refer3_base = get_frame_fd(p_hal, refer1);
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
            p_regs->sw133.ref_dist_cur_2 = tmp;
            p_regs->sw133.ref_dist_cur_3 = tmp;
            p_regs->sw147.ref_invd_cur_2 = 512 / tmp;
            p_regs->sw147.ref_invd_cur_3 = 512 / tmp;

            //!< current to past anchor
            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            p_regs->sw132.ref_dist_cur_0 = tmp;
            p_regs->sw132.ref_dist_cur_1 = tmp;
            p_regs->sw146.ref_invd_cur_0 = 512 / tmp;
            p_regs->sw146.ref_invd_cur_1 = 512 / tmp;
            //!< future anchor to past anchor
            if (p_hal->work0 >= 0 && p_hal->work1 >= 0) {
                tmp = (2 * p_hal->pic[p_hal->work0].picture_distance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            tmp = 16384 / tmp;
            p_regs->sw129.ref_invd_col_0 = tmp;
            p_regs->sw129.ref_invd_col_1 = tmp;
            //!< future anchor to previous past anchor
            tmp = p_hal->future2prev_past_dist;
            tmp = 16384 / tmp;
            p_regs->sw130.ref_invd_col_2 = tmp;
            p_regs->sw130.ref_invd_col_3 = tmp;
        } else {
            RK_S32 tmp = 0;
            //!< current to past anchor
            if (p_hal->work0 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work0].picture_distance - 512) & 0x1FF;
            }
            if (!tmp) tmp = 2;
            p_regs->sw132.ref_dist_cur_0 = tmp;
            p_regs->sw132.ref_dist_cur_1 = tmp;
            p_regs->sw146.ref_invd_cur_0 = 512 / tmp;
            p_regs->sw146.ref_invd_cur_1 = 512 / tmp;
            //!< current to previous past anchor
            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            //!< this will become "future to previous past" for next B
            p_hal->future2prev_past_dist = tmp;

            p_regs->sw133.ref_dist_cur_2 = tmp;
            p_regs->sw133.ref_dist_cur_3 = tmp;
            p_regs->sw147.ref_invd_cur_2 = 512 / tmp;
            p_regs->sw147.ref_invd_cur_3 = 512 / tmp;

            p_regs->sw129.ref_invd_col_0 = 0;
            p_regs->sw129.ref_invd_col_1 = 0;
            p_regs->sw130.ref_invd_col_2 = 0;
            p_regs->sw130.ref_invd_col_3 = 0;
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
                p_regs->sw133.ref_dist_cur_2 = tmp;
                p_regs->sw133.ref_dist_cur_3 = tmp + 1;
                p_regs->sw147.ref_invd_cur_2 = 512 / tmp;
                p_regs->sw147.ref_invd_cur_3 = 512 / (tmp + 1);
            } else {
                p_regs->sw133.ref_dist_cur_2 = tmp - 1;
                p_regs->sw133.ref_dist_cur_3 = tmp;
                p_regs->sw147.ref_invd_cur_2 = 512 / (tmp - 1);
                p_regs->sw147.ref_invd_cur_3 = 512 / tmp;
            }

            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            if (p_hal->first_field) {
                p_regs->sw132.ref_dist_cur_0 = (tmp - 1);
                p_regs->sw132.ref_dist_cur_1 = tmp;
                p_regs->sw146.ref_invd_cur_0 = 512 / (tmp - 1);
                p_regs->sw146.ref_invd_cur_1 = 512 / tmp;
            } else {
                p_regs->sw132.ref_dist_cur_0 = tmp;
                p_regs->sw132.ref_dist_cur_1 = tmp + 1;
                p_regs->sw146.ref_invd_cur_0 = 512 / tmp;
                p_regs->sw146.ref_invd_cur_1 = 512 / (tmp + 1);
            }

            if (p_hal->work0 >= 0 && p_hal->work1 >= 0) {
                tmp = (2 * p_hal->pic[p_hal->work0].picture_distance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }

            if (p_syn->pp.pbFieldEnhancedFlag && !p_hal->first_field) {
                //!< in this case, BlockDistanceRef is different with before, the mvRef points to top field
                p_regs->sw129.ref_invd_col_0 = 16384 / (tmp - 1);
                p_regs->sw129.ref_invd_col_1 = 16384 / tmp;

                //!< future anchor to previous past anchor
                tmp = p_hal->future2prev_past_dist;

                p_regs->sw130.ref_invd_col_2 = 16384 / (tmp - 1);
                p_regs->sw130.ref_invd_col_3 = 16384 / tmp;
            } else {
                if (p_hal->first_field) {
                    p_regs->sw129.ref_invd_col_0 = 16384 / (tmp - 1);
                    p_regs->sw129.ref_invd_col_1 = 16384 / tmp;
                } else {
                    p_regs->sw129.ref_invd_col_0 = 16384;
                    p_regs->sw129.ref_invd_col_1 = 16384 / tmp;
                    p_regs->sw130.ref_invd_col_2 = 16384 / (tmp + 1);
                }

                //!< future anchor to previous past anchor
                tmp = p_hal->future2prev_past_dist;

                if (p_hal->first_field) {
                    p_regs->sw130.ref_invd_col_2 = 16384 / (tmp - 1);
                    p_regs->sw130.ref_invd_col_3 = 16384 / tmp;
                } else {
                    p_regs->sw130.ref_invd_col_3 = 16384 / tmp;
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
                p_regs->sw132.ref_dist_cur_0 = 1;
                p_regs->sw133.ref_dist_cur_2 = tmp + 1;

                p_regs->sw146.ref_invd_cur_0 = 512;
                p_regs->sw147.ref_invd_cur_2 = 512 / (tmp + 1);
            } else {
                p_regs->sw132.ref_dist_cur_0 = tmp - 1;
                p_regs->sw146.ref_invd_cur_0 = 512 / (tmp - 1);
            }
            p_regs->sw132.ref_dist_cur_1 = tmp;
            p_regs->sw146.ref_invd_cur_1 = 512 / tmp;

            if (p_hal->work1 >= 0) {
                tmp = (2 * p_syn->pp.pictureDistance -
                       2 * p_hal->pic[p_hal->work1].picture_distance - 512) & 0x1FF;
                if (!tmp) tmp = 2;
            }
            //!< this will become "future to previous past" for next B
            p_hal->future2prev_past_dist = tmp;
            if (p_hal->first_field) {
                p_regs->sw133.ref_dist_cur_2 = tmp - 1;
                p_regs->sw133.ref_dist_cur_3 = tmp;

                p_regs->sw147.ref_invd_cur_2 = 512 / (tmp - 1);
                p_regs->sw147.ref_invd_cur_3 = 512 / tmp;
            } else {
                p_regs->sw133.ref_dist_cur_3 = tmp;
                p_regs->sw147.ref_invd_cur_3 = 512 / tmp;
            }

            p_regs->sw129.ref_invd_col_0 = 0;
            p_regs->sw129.ref_invd_col_1 = 0;
            p_regs->sw130.ref_invd_col_2 = 0;
            p_regs->sw130.ref_invd_col_3 = 0;
        }
    }

    p_regs->sw52.startmb_x = 0;
    p_regs->sw52.startmb_y = 0;

    p_regs->sw50.filtering_dis = p_syn->pp.loopFilterDisable;
    p_regs->sw122.alpha_offset = p_syn->pp.alphaOffset;
    p_regs->sw122.beta_offset = p_syn->pp.betaOffset;
    p_regs->sw50.skip_mode = p_syn->pp.skipModeFlag;
    p_regs->sw120.pic_refer_flag = p_syn->pp.pictureReferenceFlag;

    if (p_syn->pp.picCodingType == PFRAME
        || (p_syn->pp.picCodingType == IFRAME && !p_hal->first_field)) {
        p_regs->sw57.dmmv_wr_en = 1;
    } else {
        p_regs->sw57.dmmv_wr_en = 0;
    }

    //!< set mv base
    p_regs->sw62.dmmv_st_adr = mpp_buffer_get_fd(p_hal->mv_buf);
    if (p_hal->first_field ||
        (p_syn->pp.picCodingType == BFRAME && p_hal->prev_pic_structure)) {
    } else {
        RK_U32 frame_width = 0, frame_height = 0, offset = 0;
        frame_width = (p_syn->pp.horizontalSize + 15) >> 4;
        if (p_syn->pp.progressiveFrame)
            frame_height = (p_syn->pp.verticalSize + 15) >> 4;
        else
            frame_height = 2 * ((p_syn->pp.verticalSize + 31) >> 5);
        offset = MPP_ALIGN(frame_width * frame_height / 2, 2) * 16;
        mpp_dev_set_reg_offset(p_hal->dev, 62, offset);
    }
    {
        RK_U32 pic_type = 0;
        RK_U32 prev_anc_type = 0;
        if (p_hal->work0 >= 0) {
            pic_type = p_hal->pic[p_hal->work0].pic_type;
        }
        prev_anc_type = !pic_type || (!p_hal->first_field && !p_hal->prev_pic_structure);
        p_regs->sw136.prev_anc_type = prev_anc_type;
    }
    //!< b-picture needs to know if future reference is field or frame coded
    // p_regs->sw134.refer2_field_e = (!p_hal->prev_pic_structure) ? 1 : 0;
    // p_regs->sw135.refer3_field_e = (!p_hal->prev_pic_structure) ? 1 : 0;
    if (!p_hal->prev_pic_structure) {
        mpp_dev_set_reg_offset(p_hal->dev, 134, 2);
        mpp_dev_set_reg_offset(p_hal->dev, 135, 3);
    }

    p_regs->sw57.dec_out_dis = 0;
    p_regs->sw57.dec_e = 1;

    return MPP_OK;
__FAILED:
    return ret;
}

static MPP_RET update_parameters(AvsdHalCtx_t *p_hal)
{
    AvsdSyntax_t *p_syn = &p_hal->syn;

    if (p_syn->pp.pictureStructure == FRAMEPICTURE || !p_hal->first_field) {
        p_hal->first_field = 1;
        if (p_syn->pp.picCodingType != BFRAME) {
            RK_S32 temp = p_hal->work1;

            p_hal->work1 =  p_hal->work0;
            p_hal->work0 =  p_hal->work_out;

            if (p_hal->work_out >= 0)
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

static MPP_RET repeat_other_field(AvsdHalCtx_t *p_hal, HalTaskInfo *task)
{
    RK_U8 i = 0;
    RK_U8 *pdata = NULL;
    MppBuffer mbuffer = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdVdpu2Regs_t *p_regs = (AvsdVdpu2Regs_t *)p_hal->p_regs;

    //!< re-find start code and calculate offset
    p_hal->data_offset = p_regs->sw64.rlc_vlc_st_adr >> 10;
    p_hal->data_offset += p_hal->syn.bitstream_offset;
    p_hal->data_offset -= MPP_MIN(p_hal->data_offset, 8);

    mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
    pdata = (RK_U8 *)mpp_buffer_get_ptr(mbuffer) + p_hal->data_offset;

    while (i < 16) {
        if (pdata[i] == 0 && pdata[i + 1] == 0 && pdata[i + 2] == 1) {
            p_hal->data_offset += i;
            break;
        }
        i++;
    }

    AVSD_HAL_DBG(AVSD_HAL_DBG_OFFSET, "frame_no %d idx %d offset %x\n",
                 p_hal->frame_no, i, p_hal->data_offset);

    //!< re-generate register
    FUN_CHECK(ret = set_regs_parameters(p_hal, &task->dec));
    hal_avsd_vdpu2_start((void *)p_hal, task);
    hal_avsd_vdpu2_wait((void *)p_hal, task);

    return ret = MPP_OK;
__FAILED:
    return ret;
}


/*!
 ***********************************************************************
 * \brief
 *    init
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_init(void *decoder, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 buf_size = 0;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("AVS_vdpu2 In.");

    buf_size = (1920 * 1088) * 2;
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &p_hal->mv_buf, buf_size));

    p_hal->p_regs = mpp_calloc_size(RK_U32, sizeof(AvsdVdpu2Regs_t));
    MEM_CHECK(ret, p_hal->p_regs);

    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, avsd_hor_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, avsd_ver_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, avsd_len_align);

    p_hal->regs_num = 159;
    //!< initial for control
    p_hal->first_field = 1;
    p_hal->prev_pic_structure = 0; //!< field

    memset(p_hal->pic, 0, sizeof(p_hal->pic));
    p_hal->work_out = -1;
    p_hal->work0 = -1;
    p_hal->work1 = -1;

    AVSD_HAL_TRACE("Out.");
    (void)cfg;
    return ret = MPP_OK;
__FAILED:
    return ret;

}
/*!
 ***********************************************************************
 * \brief
 *    deinit
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_deinit(void *decoder)
{
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");

    if (p_hal->mv_buf) {
        mpp_buffer_put(p_hal->mv_buf);
        p_hal->mv_buf = NULL;
    }
    MPP_FREE(p_hal->p_regs);

    AVSD_HAL_TRACE("Out.");

    return MPP_OK;
}
/*!
 ***********************************************************************
 * \brief
 *    generate register
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_gen_regs(void *decoder, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");
    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->dec_cfg->base.disable_error) {
        goto __RETURN;
    }
    p_hal->data_offset = p_hal->syn.bitstream_offset;

    FUN_CHECK(ret = set_regs_parameters(p_hal, &task->dec));
__RETURN:
    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
__FAILED:
    return ret;

}
/*!
 ***********************************************************************
 * \brief h
 *    start hard
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_start(void *decoder, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->dec_cfg->base.disable_error) {
        goto __RETURN;
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = 159 * sizeof(RK_U32);

        wr_cfg.reg = p_hal->p_regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = p_hal->p_regs;
        rd_cfg.size = reg_size;
        rd_cfg.offset = 0;

        ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

__RETURN:
    AVSD_HAL_TRACE("Out.");
    return ret = MPP_OK;
}
/*!
 ***********************************************************************
 * \brief
 *    wait hard
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_wait(void *decoder, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->dec_cfg->base.disable_error) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

__SKIP_HARD:
    if (p_hal->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)p_hal->p_regs;

        if (!((AvsdVdpu2Regs_t *)p_hal->p_regs)->sw55.dec_rdy_sts) {
            param.hard_err = 1;
        } else
            param.hard_err = 0;

        mpp_callback(p_hal->dec_cb, &param);
        AVSD_HAL_DBG(AVSD_HAL_DBG_WAIT, "reg[55]=%08x, ref=%d, dpberr=%d, harderr=%d\n",
                     p_hal->p_regs[55], task->dec.flags.used_for_ref, task->dec.flags.ref_err, param.hard_err);
    }

    update_parameters(p_hal);
    memset(&p_hal->p_regs[55], 0, sizeof(RK_U32));
    if (!p_hal->first_field && p_hal->syn.pp.pictureStructure == FIELDPICTURE &&
        ((!task->dec.flags.parse_err && !task->dec.flags.ref_err) ||
         p_hal->dec_cfg->base.disable_error)) {
        repeat_other_field(p_hal, task);
    }
    p_hal->frame_no++;

    AVSD_HAL_TRACE("Out.");
    return ret = MPP_OK;
}
/*!
 ***********************************************************************
 * \brief
 *    reset
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_reset(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");

    p_hal->first_field = 1;
    p_hal->prev_pic_structure = 0; //!< field

    memset(p_hal->pic, 0, sizeof(p_hal->pic));
    p_hal->work_out = -1;
    p_hal->work0 = -1;
    p_hal->work1 = -1;

    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
}
/*!
 ***********************************************************************
 * \brief
 *    flush
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_flush(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)decoder;

    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
}
/*!
 ***********************************************************************
 * \brief
 *    control
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_vdpu2_control(void *decoder, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)decoder;
    (void)cmd_type;
    (void)param;

    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
}
