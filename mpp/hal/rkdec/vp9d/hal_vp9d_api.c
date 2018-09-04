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


#define MODULE_TAG "hal_vp9d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_dec.h"

#include "hal_vp9d_api.h"
#include "hal_vp9d_reg.h"
#include "mpp_device.h"
#include "mpp_bitput.h"
#include "vp9d_syntax.h"
#include "hal_vp9d_table.h"

#define PROBE_SIZE   4864
#define COUNT_SIZE   13208

/*nCtuX*nCtuY*8*8/2
 * MaxnCtuX = 4096/64
 * MaxnCtuY = 2304/64
*/
#define MAX_SEGMAP_SIZE  73728
/*

*/
#define MAX_INTER_CMD_SIZE  147456

RK_U32 vp9h_debug = 0;
//#define dump
#ifdef dump
static FILE *vp9_fp = NULL;
static FILE *vp9_fp1 = NULL;
static FILE *vp9_fp_yuv = NULL;

static RK_S32 count = 0;
static RK_S32 frame_num = 0;
#endif
typedef struct vp9_dec_last_info {
    RK_S32 abs_delta_last;
    RK_S8 last_ref_deltas[4];
    RK_S8 last_mode_deltas[2];
    RK_U8 segmentation_enable_flag_last;
    RK_U8 last_show_frame;
    RK_U8 last_intra_only;
    RK_U32 last_width;
    RK_U32 last_height;
    SHORT feature_data[8][4];
    UCHAR feature_mask[8];
} vp9_dec_last_info_t;

#define MAX_GEN_REG 3
typedef struct vp9d_reg_buf {
    RK_S32    use_flag;
    MppBuffer probe_base;
    MppBuffer count_base;
    MppBuffer segid_cur_base;
    MppBuffer segid_last_base;
    void*     hw_regs;
} vp9d_reg_buf_t;

typedef struct hal_vp9_context {
    MppBufSlots     slots;
    MppBufSlots     packet_slots;
    MppDevCtx       dev_ctx;
    MppBufferGroup group;
    vp9d_reg_buf_t g_buf[MAX_GEN_REG];
    MppBuffer probe_base;
    MppBuffer count_base;
    MppBuffer segid_cur_base;
    MppBuffer segid_last_base;
    void*     hw_regs;
    IOInterruptCB int_cb;
    RK_S32 mv_base_addr;
    RK_S32 pre_mv_base_addr;
    vp9_dec_last_info_t ls_info;
    /*swap between segid_cur_base & segid_last_base
        0  used segid_cur_base as last
        1  used segid_last_base as
    */
    RK_U32    last_segid_flag;
    RK_U32    fast_mode;
} hal_vp9_context_t;

static RK_U32 vp9_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 64);
}

static RK_U32 vp9_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 256) | 256;
}

static MPP_RET hal_vp9d_alloc_res(hal_vp9_context_t *reg_cxt)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;

    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            reg_cxt->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(VP9_REGS));
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].probe_base, PROBE_SIZE);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].count_base, COUNT_SIZE);
            if (ret) {
                mpp_err("vp9 count_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].segid_cur_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_cur_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].segid_last_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_last_base get buffer failed\n");
                return ret;
            }
        }
    } else {
        reg_cxt->hw_regs = mpp_calloc_size(void, sizeof(VP9_REGS));
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->probe_base, PROBE_SIZE);
        if (ret) {
            mpp_err("vp9 probe_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->count_base, COUNT_SIZE);
        if (ret) {
            mpp_err("vp9 count_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->segid_cur_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_cur_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->segid_last_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_last_base get buffer failed\n");
            return ret;
        }
    }
    return MPP_OK;
}

static MPP_RET hal_vp9d_release_res(hal_vp9_context_t *reg_cxt)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;

    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (reg_cxt->g_buf[i].probe_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].probe_base);
                if (ret) {
                    mpp_err("vp9 probe_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].count_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].count_base);
                if (ret) {
                    mpp_err("vp9 count_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].segid_cur_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].segid_cur_base);
                if (ret) {
                    mpp_err("vp9 segid_cur_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].segid_last_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].segid_last_base);
                if (ret) {
                    mpp_err("vp9 segid_last_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].hw_regs) {
                mpp_free(reg_cxt->g_buf[i].hw_regs);
                reg_cxt->g_buf[i].hw_regs = NULL;
            }
        }
    } else {
        if (reg_cxt->probe_base) {
            ret = mpp_buffer_put(reg_cxt->probe_base);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->count_base) {
            ret = mpp_buffer_put(reg_cxt->count_base);
            if (ret) {
                mpp_err("vp9 count_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->segid_cur_base) {
            ret = mpp_buffer_put(reg_cxt->segid_cur_base);
            if (ret) {
                mpp_err("vp9 segid_cur_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->segid_last_base) {
            ret = mpp_buffer_put(reg_cxt->segid_last_base);
            if (ret) {
                mpp_err("vp9 segid_last_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->hw_regs) {
            mpp_free(reg_cxt->hw_regs);
            reg_cxt->hw_regs = NULL;
        }
    }
    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t *)hal;

    if (NULL == reg_cxt) {
        mpp_err("hal instant no alloc");
        return MPP_ERR_UNKNOW;
    }
    mpp_log("hal_vp9d_init in");
    reg_cxt->slots = cfg->frame_slots;
    reg_cxt->int_cb = cfg->hal_int_cb;
    reg_cxt->mv_base_addr = -1;
    reg_cxt->pre_mv_base_addr = -1;
    reg_cxt->fast_mode = cfg->fast_mode;
    mpp_slots_set_prop(reg_cxt->slots, SLOTS_HOR_ALIGN, vp9_hor_align);
    mpp_slots_set_prop(reg_cxt->slots, SLOTS_VER_ALIGN, vp9_ver_align);
    reg_cxt->packet_slots = cfg->packet_slots;

    ///<- mpp_device_init
    MppDevCfg dev_cfg = {
        .type = MPP_CTX_DEC,              /* type */
        .coding = MPP_VIDEO_CodingVP9,    /* coding */
        .platform = 0,                    /* platform */
        .pp_enable = 0,                   /* pp_enable */
    };

    ret = mpp_device_init(&reg_cxt->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }

    if (reg_cxt->group == NULL) {
        ret = mpp_buffer_group_get_internal(&reg_cxt->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("vp9 mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    ret = hal_vp9d_alloc_res(reg_cxt);
    if (ret) {
        mpp_err("hal_vp9d_alloc_res failed\n");
        return ret;
    }

    mpp_env_get_u32("vp9h_debug", &vp9h_debug, 0);

    reg_cxt->last_segid_flag = 1;
#ifdef dump
    if (vp9_fp_yuv != NULL) {
        fclose(vp9_fp_yuv);
    }

    frame_num = 0;
    vp9_fp_yuv = fopen("data/vp9_out.yuv", "wb");
    count = 0;
#endif
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t *)hal;

    if (reg_cxt->dev_ctx) {
        ret = mpp_device_deinit(reg_cxt->dev_ctx);
        if (ret) {
            mpp_err("mpp_device_deinit failed. ret: %d\n", ret);
        }
    }

    hal_vp9d_release_res(reg_cxt);

    if (reg_cxt->group) {
        ret = mpp_buffer_group_put(reg_cxt->group);
        if (ret) {
            mpp_err("vp9d group free buffer failed\n");
            return ret;
        }
    }
    return ret = MPP_OK;
}

MPP_RET hal_vp9d_output_probe(void *hal, void *dxva)
{
    RK_S32 i, j, k, m, n;
    RK_S32 fifo_len = 304;
    RK_U64 *probe_packet = NULL;
    BitputCtx_t bp;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)dxva;
    RK_S32 intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    vp9_prob partition_probs[PARTITION_CONTEXTS][PARTITION_TYPES - 1];
    vp9_prob uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t*)hal;
    void *probe_ptr = mpp_buffer_get_ptr(reg_cxt->probe_base);
    if (NULL == probe_ptr) {
        mpp_err("probe_ptr get ptr error");
        return MPP_ERR_NOMEM;
    }

    memset(probe_ptr, 0, 304 * 8);

    if (intraFlag) {
        memcpy(partition_probs, vp9_kf_partition_probs, sizeof(partition_probs));
        memcpy(uv_mode_prob, vp9_kf_uv_mode_prob, sizeof(uv_mode_prob));
    } else {
        memcpy(partition_probs, pic_param->prob.partition, sizeof(partition_probs));
        memcpy(uv_mode_prob, pic_param->prob.uv_mode, sizeof(uv_mode_prob));
    }

    probe_packet = mpp_calloc(RK_U64, fifo_len + 1);
    mpp_set_bitput_ctx(&bp, probe_packet, fifo_len);
    //sb info  5 x 128 bit
    for (i = 0; i < PARTITION_CONTEXTS; i++) //kf_partition_prob
        for (j = 0; j < PARTITION_TYPES - 1; j++)
            mpp_put_bits(&bp, partition_probs[i][j], 8); //48

    for (i = 0; i < PREDICTION_PROBS; i++) //Segment_id_pred_prob //3
        mpp_put_bits(&bp, pic_param->stVP9Segments.pred_probs[i], 8);

    for (i = 0; i < SEG_TREE_PROBS; i++) //Segment_id_probs
        mpp_put_bits(&bp, pic_param->stVP9Segments.tree_probs[i], 8); //7

    for (i = 0; i < SKIP_CONTEXTS; i++) //Skip_flag_probs //3
        mpp_put_bits(&bp, pic_param->prob.skip[i], 8);

    for (i = 0; i < TX_SIZE_CONTEXTS; i++) //Tx_size_probs //6
        for (j = 0; j < TX_SIZES - 1; j++)
            mpp_put_bits(&bp, pic_param->prob.tx32p[i][j], 8);

    for (i = 0; i < TX_SIZE_CONTEXTS; i++) //Tx_size_probs //4
        for (j = 0; j < TX_SIZES - 2; j++)
            mpp_put_bits(&bp, pic_param->prob.tx16p[i][j], 8);

    for (i = 0; i < TX_SIZE_CONTEXTS; i++) //Tx_size_probs //2
        mpp_put_bits(&bp, pic_param->prob.tx8p[i], 8);

    for (i = 0; i < INTRA_INTER_CONTEXTS; i++) //Tx_size_probs //4
        mpp_put_bits(&bp, pic_param->prob.intra[i], 8);

    mpp_put_align(&bp, 128, 0);
    if (intraFlag) { //intra probs
        //intra only //149 x 128 bit ,aligned to 152 x 128 bit
        //coeff releated prob   64 x 128 bit
        for (i = 0; i < TX_SIZES; i++)
            for (j = 0; j < PLANE_TYPES; j++) {
                RK_S32 byte_count = 0;
                for (k = 0; k < COEF_BANDS; k++) {
                    for (m = 0; m < COEFF_CONTEXTS; m++)
                        for (n = 0; n < UNCONSTRAINED_NODES; n++) {
                            mpp_put_bits(&bp, pic_param->prob.coef[i][j][0][k][m][n], 8);

                            byte_count++;
                            if (byte_count == 27) {
                                mpp_put_align(&bp, 128, 0);
                                byte_count = 0;
                            }
                        }
                }
                mpp_put_align(&bp, 128, 0);
            }

        //intra mode prob  80 x 128 bit
        for (i = 0; i < INTRA_MODES; i++) { //vp9_kf_y_mode_prob
            RK_S32 byte_count = 0;
            for (j = 0; j < INTRA_MODES; j++)
                for (k = 0; k < INTRA_MODES - 1; k++) {
                    mpp_put_bits(&bp, vp9_kf_y_mode_prob[i][j][k], 8);
                    byte_count++;
                    if (byte_count == 27) {
                        byte_count = 0;
                        mpp_put_align(&bp, 128, 0);
                    }

                }
            if (i < 4) {
                for (m = 0; m < (i < 3 ? 23 : 21); m++)
                    mpp_put_bits(&bp, ((vp9_prob *)(&vp9_kf_uv_mode_prob[0][0]))[i * 23 + m], 8);
                for (; m < 23; m++)
                    mpp_put_bits(&bp, 0, 8);
            } else {
                for (m = 0; m < 23; m++)
                    mpp_put_bits(&bp, 0, 8);
            }
            mpp_put_align(&bp, 128, 0);
        }
        //align to 152 x 128 bit
        for (i = 0; i < INTER_PROB_SIZE_ALIGN_TO_128 - INTRA_PROB_SIZE_ALIGN_TO_128; i++) { //aligned to 153 x 256 bit
            mpp_put_bits(&bp, 0, 8);
            mpp_put_align(&bp, 128, 0);
        }
    } else {
        //inter probs
        //151 x 128 bit ,aligned to 152 x 128 bit
        //inter only

        //intra_y_mode & inter_block info   6 x 128 bit
        for (i = 0; i < BLOCK_SIZE_GROUPS; i++) //intra_y_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.y_mode[i][j], 8);

        for (i = 0; i < COMP_INTER_CONTEXTS; i++) //reference_mode prob
            mpp_put_bits(&bp, pic_param->prob.comp[i], 8);

        for (i = 0; i < REF_CONTEXTS; i++) //comp ref bit
            mpp_put_bits(&bp, pic_param->prob.comp_ref[i], 8);

        for (i = 0; i < REF_CONTEXTS; i++) //single ref bit
            for (j = 0; j < 2; j++)
                mpp_put_bits(&bp, pic_param->prob.single_ref[i][j], 8);

        for (i = 0; i < INTER_MODE_CONTEXTS; i++) //mv mode bit
            for (j = 0; j < INTER_MODES - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_mode[i][j], 8);


        for (i = 0; i < SWITCHABLE_FILTER_CONTEXTS; i++) //comp ref bit
            for (j = 0; j < SWITCHABLE_FILTERS - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.filter[i][j], 8);

        mpp_put_align(&bp, 128, 0);

        //128 x 128bit
        //coeff releated
        for (i = 0; i < TX_SIZES; i++)
            for (j = 0; j < PLANE_TYPES; j++) {
                RK_S32 byte_count = 0;
                for (k = 0; k < COEF_BANDS; k++) {
                    for (m = 0; m < COEFF_CONTEXTS; m++)
                        for (n = 0; n < UNCONSTRAINED_NODES; n++) {
                            mpp_put_bits(&bp, pic_param->prob.coef[i][j][0][k][m][n], 8);
                            byte_count++;
                            if (byte_count == 27) {
                                mpp_put_align(&bp, 128, 0);
                                byte_count = 0;
                            }
                        }
                }
                mpp_put_align(&bp, 128, 0);
            }
        for (i = 0; i < TX_SIZES; i++)
            for (j = 0; j < PLANE_TYPES; j++) {
                RK_S32 byte_count = 0;
                for (k = 0; k < COEF_BANDS; k++) {
                    for (m = 0; m < COEFF_CONTEXTS; m++) {
                        for (n = 0; n < UNCONSTRAINED_NODES; n++) {
                            mpp_put_bits(&bp, pic_param->prob.coef[i][j][1][k][m][n], 8);
                            byte_count++;
                            if (byte_count == 27) {
                                mpp_put_align(&bp, 128, 0);
                                byte_count = 0;
                            }
                        }

                    }
                }
                mpp_put_align(&bp, 128, 0);
            }

        //intra uv mode 6 x 128
        for (i = 0; i < 3; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);
        mpp_put_align(&bp, 128, 0);

        for (; i < 6; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);
        mpp_put_align(&bp, 128, 0);

        for (; i < 9; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);
        mpp_put_align(&bp, 128, 0);
        for (; i < INTRA_MODES; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);

        mpp_put_align(&bp, 128, 0);
        mpp_put_bits(&bp, 0, 8);
        mpp_put_align(&bp, 128, 0);

        //mv releated 6 x 128
        for (i = 0; i < MV_JOINTS - 1; i++) //mv_joint_type
            mpp_put_bits(&bp, pic_param->prob.mv_joint[i], 8);

        for (i = 0; i < 2; i++) { //sign bit
            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].sign, 8);
        }
        for (i = 0; i < 2; i++) { //classes bit
            for (j = 0; j < MV_CLASSES - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_comp[i].classes[j], 8);
        }
        for (i = 0; i < 2; i++) { //classe0 bit
            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].class0, 8);
        }
        for (i = 0; i < 2; i++) { // bits
            for (j = 0; j < MV_OFFSET_BITS; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_comp[i].bits[j], 8);
        }
        for (i = 0; i < 2; i++) { //class0_fp bit
            for (j = 0; j < CLASS0_SIZE; j++)
                for (k = 0; k < MV_FP_SIZE - 1; k++)
                    mpp_put_bits(&bp, pic_param->prob.mv_comp[i].class0_fp[j][k], 8);
        }
        for (i = 0; i < 2; i++) { //comp ref bit
            for (j = 0; j < MV_FP_SIZE - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_comp[i].fp[j], 8);
        }
        for (i = 0; i < 2; i++) { //class0_hp bit

            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].class0_hp, 8);
        }
        for (i = 0; i < 2; i++) { //hp bit
            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].hp, 8);
        }
        mpp_put_align(&bp, 128, 0);
    }

    memcpy(probe_ptr, probe_packet, 304 * 8);

#ifdef dump
    if (intraFlag) {
        fwrite(probe_packet, 1, 302 * 8, vp9_fp);
    } else {
        fwrite(probe_packet, 1, 304 * 8, vp9_fp);
    }
    fflush(vp9_fp);
#endif
    if (probe_packet != NULL)
        mpp_free(probe_packet);

    return 0;
}
void hal_vp9d_update_counts(void *hal, void *dxva)
{
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t*)hal;
    DXVA_PicParams_VP9 *s = (DXVA_PicParams_VP9*)dxva;
    RK_S32 i, j, m, n, k;
    RK_U32 *eob_coef;
    RK_S32 ref_type;
#ifdef dump
    RK_U32 count_length;
#endif
    RK_U32 com_len = 0;

    RK_U8 *counts_ptr = mpp_buffer_get_ptr(reg_cxt->count_base);
    if (NULL == counts_ptr) {
        mpp_err("counts_ptr get ptr error");
        return;
    }

#ifdef dump
    if (!(s->frame_type == 0 || s->intra_only)) //inter
        count_length = (213 * 64 + 576 * 5 * 32) / 8;
    else //intra
        count_length = (49 * 64 + 288 * 5 * 32) / 8;

    fwrite(counts_ptr, 1, count_length, vp9_fp1);
    fflush(vp9_fp1);
#endif
    if ((s->frame_type == 0 || s->intra_only)) {
        com_len = sizeof(s->counts.partition) + sizeof(s->counts.skip) + sizeof(s->counts.intra)
                  + sizeof(s->counts.tx32p) + sizeof(s->counts.tx16p) + sizeof(s->counts.tx8p);
    } else {
        com_len = sizeof(s->counts) - sizeof(s->counts.coef) - sizeof(s->counts.eob);
    }
    eob_coef = (RK_U32 *)(counts_ptr + com_len);
    memcpy(&s->counts, counts_ptr, com_len);
    ref_type = (!(s->frame_type == 0 || s->intra_only)) ? 2 : 1;
    if (ref_type == 1) {
        memset(s->counts.eob, 0, sizeof(s->counts.eob));
        memset(s->counts.coef, 0, sizeof(s->counts.coef));
    }
    for (i = 0; i < ref_type; i++) {
        for (j = 0; j < 4; j++) {
            for (m = 0; m < 2; m++) {
                for (n = 0; n < 6; n++) {
                    for (k = 0; k < 6; k++) {
                        s->counts.eob[j][m][i][n][k][0] = eob_coef[1];
                        s->counts.eob[j][m][i][n][k][1] = eob_coef[0] - eob_coef[1]; //ffmpeg need do  branch_ct[UNCONSTRAINED_NODES][2] =  { neob, eob_counts[i][j][k][l] - neob },
                        s->counts.coef[j][m][i][n][k][0] = eob_coef[2];
                        s->counts.coef[j][m][i][n][k][1] = eob_coef[3];
                        s->counts.coef[j][m][i][n][k][2] = eob_coef[4];
                        eob_coef += 5;
                    }
                }
            }
        }
    }
}

/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32   i;
    RK_U8    bit_depth = 0;
    RK_U32   pic_h[3] = { 0 };
    RK_U32   ref_frame_width_y;
    RK_U32   ref_frame_height_y;
    RK_S32   stream_len = 0, aglin_offset = 0;
    RK_U32   y_hor_virstride, uv_hor_virstride, y_virstride, uv_virstride, yuv_virstride;
    RK_U8   *bitstream = NULL;
    MppBuffer streambuf = NULL;
    RK_U32 sw_y_hor_virstride;
    RK_U32 sw_uv_hor_virstride;
    RK_U32 sw_y_virstride;
    RK_U32 sw_uv_virstride;
    RK_U32 sw_yuv_virstride ;
    RK_U8  ref_idx = 0;
    RK_U32 *reg_ref_base = 0;
    RK_S32 intraFlag = 0;
    MppBuffer framebuf = NULL;

#ifdef dump
    char filename[20] = "data/probe";
    char filename1[20] = "data/count";

    if (vp9_fp != NULL) {
        fclose(vp9_fp);
    }
    if (vp9_fp1 != NULL) {
        fclose(vp9_fp1);
    }
    sprintf(&filename[10], "%d.bin", count);
    sprintf(&filename1[10], "%d.bin", count);
    mpp_log("filename %s", filename);
    mpp_log("filename %s", filename1);
    vp9_fp = fopen(filename, "wb");
    vp9_fp1 = fopen(filename1, "wb");
    count++;
#endif

    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t*)hal;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;

    if (reg_cxt ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_cxt->g_buf[i].use_flag) {
                task->dec.reg_index = i;
                reg_cxt->probe_base = reg_cxt->g_buf[i].probe_base;
                reg_cxt->count_base = reg_cxt->g_buf[i].count_base;
                reg_cxt->segid_cur_base = reg_cxt->g_buf[i].segid_cur_base;
                reg_cxt->segid_last_base = reg_cxt->g_buf[i].segid_last_base;
                reg_cxt->hw_regs = reg_cxt->g_buf[i].hw_regs;
                reg_cxt->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("vp9 fast mode buf all used\n");
            return MPP_ERR_NOMEM;
        }
    }
    VP9_REGS *vp9_hw_regs = (VP9_REGS*)reg_cxt->hw_regs;
    intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_output_probe(hal, task->dec.syntax.data);
    stream_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
    memset(reg_cxt->hw_regs, 0, sizeof(VP9_REGS));
    vp9_hw_regs->swreg2_sysctrl.sw_dec_mode = 2; //set as vp9 dec
    vp9_hw_regs->swreg5_stream_len = ((stream_len + 15) & (~15)) + 0x80;

    mpp_buf_slot_get_prop(reg_cxt->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
    bitstream = mpp_buffer_get_ptr(streambuf);
    aglin_offset = vp9_hw_regs->swreg5_stream_len - stream_len;
    if (aglin_offset > 0) {
        memset((void *)(bitstream + stream_len), 0, aglin_offset);
    }

    //--- caculate the yuv_frame_size and mv_size
    bit_depth = pic_param->BitDepthMinus8Luma + 8;
    pic_h[0] = vp9_ver_align(pic_param->height); //p_cm->height;
    pic_h[1] = vp9_ver_align(pic_param->height) / 2; //(p_cm->height + 1) / 2;
    pic_h[2] = pic_h[1];

    sw_y_hor_virstride = (vp9_hor_align((pic_param->width * bit_depth) >> 3) >> 4);
    sw_uv_hor_virstride = (vp9_hor_align((pic_param->width * bit_depth) >> 3) >> 4);
    sw_y_virstride = pic_h[0] * sw_y_hor_virstride;

    sw_uv_virstride = pic_h[1] * sw_uv_hor_virstride;
    sw_yuv_virstride = sw_y_virstride + sw_uv_virstride;

    vp9_hw_regs->swreg3_picpar.sw_y_hor_virstride = sw_y_hor_virstride;
    vp9_hw_regs->swreg3_picpar.sw_uv_hor_virstride = sw_uv_hor_virstride;
    vp9_hw_regs->swreg8_y_virstride.sw_y_virstride = sw_y_virstride;
    vp9_hw_regs->swreg9_yuv_virstride.sw_yuv_virstride = sw_yuv_virstride;

    if (!pic_param->intra_only && pic_param->frame_type && !pic_param->error_resilient_mode) {
        reg_cxt->pre_mv_base_addr = reg_cxt->mv_base_addr;
    }

    if (!(pic_param->stVP9Segments.enabled && !pic_param->stVP9Segments.update_map)) {
        if (!pic_param->intra_only && pic_param->frame_type && !pic_param->error_resilient_mode) {
            if (reg_cxt->last_segid_flag) {
                reg_cxt->last_segid_flag = 0;
            } else {
                reg_cxt->last_segid_flag = 1;
            }
        }
    }
    mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.output, SLOT_BUFFER, &framebuf);
    vp9_hw_regs->swreg7_decout_base =  mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->swreg4_strm_rlc_base = mpp_buffer_get_fd(streambuf);

    vp9_hw_regs->swreg6_cabactbl_prob_base = mpp_buffer_get_fd(reg_cxt->probe_base);
    vp9_hw_regs->swreg14_vp9_count_base  = mpp_buffer_get_fd(reg_cxt->count_base);

    if (reg_cxt->last_segid_flag) {
        vp9_hw_regs->swreg15_vp9_segidlast_base = mpp_buffer_get_fd(reg_cxt->segid_last_base);
        vp9_hw_regs->swreg16_vp9_segidcur_base = mpp_buffer_get_fd(reg_cxt->segid_cur_base);
    } else {
        vp9_hw_regs->swreg15_vp9_segidlast_base = mpp_buffer_get_fd(reg_cxt->segid_cur_base);
        vp9_hw_regs->swreg16_vp9_segidcur_base = mpp_buffer_get_fd(reg_cxt->segid_last_base);
    }

    reg_cxt->mv_base_addr = vp9_hw_regs->swreg7_decout_base | ((sw_yuv_virstride << 4) << 6);
    if (reg_cxt->pre_mv_base_addr < 0) {
        reg_cxt->pre_mv_base_addr = reg_cxt->mv_base_addr;
    }
    vp9_hw_regs->swreg52_vp9_refcolmv_base = reg_cxt->pre_mv_base_addr;

    vp9_hw_regs->swreg10_vp9_cprheader_offset.sw_vp9_cprheader_offset = 0; //no use now.
    reg_ref_base = &vp9_hw_regs->swreg11_vp9_referlast_base;
    for (i = 0; i < 3; i++) {
        ref_idx = pic_param->frame_refs[i].Index7Bits;
        ref_frame_width_y = pic_param->ref_frame_coded_width[ref_idx];
        ref_frame_height_y = pic_param->ref_frame_coded_height[ref_idx];
        pic_h[0] = vp9_ver_align(ref_frame_height_y);
        pic_h[1] = vp9_ver_align(ref_frame_height_y) / 2;
        y_hor_virstride = (vp9_hor_align((ref_frame_width_y * bit_depth) >> 3) >> 4);
        uv_hor_virstride = (vp9_hor_align((ref_frame_width_y * bit_depth) >> 3) >> 4);
        y_virstride = y_hor_virstride * pic_h[0];
        uv_virstride = uv_hor_virstride * pic_h[1];
        yuv_virstride = y_virstride + uv_virstride;

        if (pic_param->ref_frame_map[ref_idx].Index7Bits < 0x7f) {
            mpp_buf_slot_get_prop(reg_cxt->slots, pic_param->ref_frame_map[ref_idx].Index7Bits, SLOT_BUFFER, &framebuf);
        }

        if (pic_param->ref_frame_map[ref_idx].Index7Bits < 0x7f) {
            switch (i) {
            case 0: {

                vp9_hw_regs->swreg17_vp9_frame_size_last.sw_framewidth_last = ref_frame_width_y;
                vp9_hw_regs->swreg17_vp9_frame_size_last.sw_frameheight_last = ref_frame_height_y;
                vp9_hw_regs->swreg37_vp9_lastf_hor_virstride.sw_vp9_lastfy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->swreg37_vp9_lastf_hor_virstride.sw_vp9_lastfuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->swreg48_vp9_last_ystride.sw_vp9_lastfy_virstride = y_virstride;
                vp9_hw_regs->swreg51_vp9_lastref_yuvstride.sw_vp9_lastref_yuv_virstride = yuv_virstride;
                break;
            }
            case 1: {
                vp9_hw_regs->swreg18_vp9_frame_size_golden.sw_framewidth_golden = ref_frame_width_y;
                vp9_hw_regs->swreg18_vp9_frame_size_golden.sw_frameheight_golden = ref_frame_height_y;
                vp9_hw_regs->swreg38_vp9_goldenf_hor_virstride.sw_vp9_goldenfy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->swreg38_vp9_goldenf_hor_virstride.sw_vp9_goldenuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->swreg49_vp9_golden_ystride.sw_vp9_goldeny_virstride = y_virstride;
                break;
            }
            case 2: {
                vp9_hw_regs->swreg19_vp9_frame_size_altref.sw_framewidth_alfter = ref_frame_width_y;
                vp9_hw_regs->swreg19_vp9_frame_size_altref.sw_frameheight_alfter = ref_frame_height_y;
                vp9_hw_regs->swreg39_vp9_altreff_hor_virstride.sw_vp9_altreffy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->swreg39_vp9_altreff_hor_virstride.sw_vp9_altreffuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->swreg50_vp9_altrefy_ystride.sw_vp9_altrefy_virstride = y_virstride;
                break;
            }
            default:
                break;

            }

            /*0 map to 11*/
            /*1 map to 12*/
            /*2 map to 13*/
            if (framebuf != NULL) {
                reg_ref_base[i] = mpp_buffer_get_fd(framebuf);
            } else {
                mpp_log("ref buff address is no valid used out as base slot index 0x%x", pic_param->ref_frame_map[ref_idx].Index7Bits);
                reg_ref_base[i] = vp9_hw_regs->swreg7_decout_base; //set
            }
        } else {
            reg_ref_base[i] = vp9_hw_regs->swreg7_decout_base; //set
        }
    }

    for (i = 0; i < 8; i++) {
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_qp_delta_en              = (reg_cxt->ls_info.feature_mask[i]) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_qp_delta                 = reg_cxt->ls_info.feature_data[i][0];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_loopfitler_value_en      = (reg_cxt->ls_info.feature_mask[i] >> 1) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_loopfilter_value         = reg_cxt->ls_info.feature_data[i][1];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_referinfo_en                   = (reg_cxt->ls_info.feature_mask[i] >> 2) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_referinfo                      = reg_cxt->ls_info.feature_data[i][2];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_skip_en                  = (reg_cxt->ls_info.feature_mask[i] >> 3) & 0x1;
    }


    vp9_hw_regs->swreg20_27_vp9_segid_grp[0].sw_vp9segid_abs_delta                              = reg_cxt->ls_info.abs_delta_last;

    vp9_hw_regs->swreg28_vp9_cprheader_config.sw_vp9_tx_mode                                    = pic_param->txmode;

    vp9_hw_regs->swreg28_vp9_cprheader_config.sw_vp9_frame_reference_mode                   = pic_param->refmode;

    vp9_hw_regs->swreg32_vp9_ref_deltas_lastframe.sw_vp9_ref_deltas_lastframe               = 0;

    if (!intraFlag) {
        for (i = 0; i < 4; i++)
            vp9_hw_regs->swreg32_vp9_ref_deltas_lastframe.sw_vp9_ref_deltas_lastframe           |= (reg_cxt->ls_info.last_ref_deltas[i] & 0x7f) << (7 * i);

        for (i = 0; i < 2; i++)
            vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_mode_deltas_lastframe                |= (reg_cxt->ls_info.last_mode_deltas[i] & 0x7f) << (7 * i);


    } else {
        reg_cxt->ls_info.segmentation_enable_flag_last = 0;
        reg_cxt->ls_info.last_intra_only = 1;
    }

    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_mode_deltas_lastframe                        = 0;

    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_segmentation_enable_lstframe                  = reg_cxt->ls_info.segmentation_enable_flag_last;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_show_frame                          = reg_cxt->ls_info.last_show_frame;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_intra_only                          = reg_cxt->ls_info.last_intra_only;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_widthheight_eqcur                   = (pic_param->width == reg_cxt->ls_info.last_width) && (pic_param->height == reg_cxt->ls_info.last_height);

    vp9_hw_regs->swreg36_vp9_lasttile_size.sw_vp9_lasttile_size                             =  stream_len - pic_param->first_partition_size;


    if (!intraFlag) {
        vp9_hw_regs->swreg29_vp9_lref_scale.sw_vp9_lref_hor_scale = pic_param->mvscale[0][0];
        vp9_hw_regs->swreg29_vp9_lref_scale.sw_vp9_lref_ver_scale = pic_param->mvscale[0][1];
        vp9_hw_regs->swreg30_vp9_gref_scale.sw_vp9_gref_hor_scale = pic_param->mvscale[1][0];
        vp9_hw_regs->swreg30_vp9_gref_scale.sw_vp9_gref_ver_scale = pic_param->mvscale[1][1];
        vp9_hw_regs->swreg31_vp9_aref_scale.sw_vp9_aref_hor_scale = pic_param->mvscale[2][0];
        vp9_hw_regs->swreg31_vp9_aref_scale.sw_vp9_aref_ver_scale = pic_param->mvscale[2][1];
        // vp9_hw_regs.swreg33_vp9_info_lastframe.sw_vp9_color_space_lastkeyframe = p_cm->color_space_last;
    }


    //reuse reg64, and it will be written by hardware to show performance.
    vp9_hw_regs->swreg64_performance_cycle.sw_performance_cycle = 0;
    vp9_hw_regs->swreg64_performance_cycle.sw_performance_cycle |= pic_param->width;
    vp9_hw_regs->swreg64_performance_cycle.sw_performance_cycle |= pic_param->height << 16;

    vp9_hw_regs->swreg1_int.sw_dec_e         = 1;
    vp9_hw_regs->swreg1_int.sw_dec_timeout_e = 1;

    //last info  update
    reg_cxt->ls_info.abs_delta_last = pic_param->stVP9Segments.abs_delta;
    for (i = 0 ; i < 4; i ++) {
        reg_cxt->ls_info.last_ref_deltas[i] = pic_param->ref_deltas[i];
    }

    for (i = 0 ; i < 2; i ++) {
        reg_cxt->ls_info.last_mode_deltas[i] = pic_param->mode_deltas[i];
    }

    for (i = 0; i < 8; i++) {
        reg_cxt->ls_info.feature_data[i][0] = pic_param->stVP9Segments.feature_data[i][0];
        reg_cxt->ls_info.feature_data[i][1] = pic_param->stVP9Segments.feature_data[i][1];
        reg_cxt->ls_info.feature_data[i][2] = pic_param->stVP9Segments.feature_data[i][2];
        reg_cxt->ls_info.feature_data[i][3] = pic_param->stVP9Segments.feature_data[i][3];
        reg_cxt->ls_info.feature_mask[i]  = pic_param->stVP9Segments.feature_mask[i];
    }
    reg_cxt->ls_info.segmentation_enable_flag_last = pic_param->stVP9Segments.enabled;
    reg_cxt->ls_info.last_show_frame = pic_param->show_frame;
    reg_cxt->ls_info.last_width = pic_param->width;
    reg_cxt->ls_info.last_height = pic_param->height;
    reg_cxt->ls_info.last_intra_only = (!pic_param->frame_type || pic_param->intra_only);
    vp9h_dbg(VP9H_DBG_PAR, "stVP9Segments.enabled = %d show_frame %d  width %d  height = %d last_intra_only = %d", pic_param->stVP9Segments.enabled,
             pic_param->show_frame, pic_param->width, pic_param->height, reg_cxt->ls_info.last_intra_only);

    // whether need update counts
    if (pic_param->refresh_frame_context && !pic_param->parallelmode) {
        task->dec.flags.wait_done = 1;
    }

    return MPP_OK;
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_start(void *hal, HalTaskInfo *task)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_OK;
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t *)hal;
    VP9_REGS *hw_regs = ( VP9_REGS *)reg_cxt->hw_regs;

    if (reg_cxt->fast_mode) {
        RK_S32 index =  task->dec.reg_index;
        hw_regs = ( VP9_REGS *)reg_cxt->g_buf[index].hw_regs;
    } else {
        hw_regs = ( VP9_REGS *)reg_cxt->hw_regs;
    }

    if (hw_regs == NULL) {
        mpp_err("hal_vp9d_start hw_regs is NULL");
        return MPP_ERR_NULL_PTR;
    }

    RK_U8 *p = (RK_U8*)reg_cxt->hw_regs;
    for (i = 0; i < sizeof(VP9_REGS) / 4; i++) {
        //vp9h_dbg(VP9H_DBG_REG, "RK_VP9_DEC: regs[%02d]=%08X\n", i, *((RK_U32*)p));
        //mpp_log("RK_VP9_DEC: regs[%02d]=%08X\n", i, *((RK_U32*)p));
        p += 4;
    }

    ret = mpp_device_send_reg(reg_cxt->dev_ctx, (RK_U32*)hw_regs, sizeof(VP9_REGS) / 4); // 68 is the nb of uint32_t
    if (ret) {
        mpp_err("VP9H_DBG_REG: ERROR: mpp_device_send_reg Failed!!!\n");
        return MPP_ERR_VPUHW;
    }

    (void)task;
    return ret;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t *)hal;
    RK_U32 i;
    VP9_REGS *hw_regs = NULL;

    if (reg_cxt->fast_mode) {
        hw_regs = (VP9_REGS *)reg_cxt->g_buf[task->dec.reg_index].hw_regs;
    } else {
        hw_regs = (VP9_REGS *)reg_cxt->hw_regs;
    }

    ret = mpp_device_wait_reg(reg_cxt->dev_ctx, (RK_U32*)hw_regs, sizeof(VP9_REGS) / 4);

    RK_U32 *p = (RK_U32*)hw_regs;
    for (i = 0; i <  sizeof(VP9_REGS) / 4; i++) {
        if (i == 1) {
            vp9h_dbg(VP9H_DBG_REG, "RK_VP9_DEC: regs[%02d]=%08X\n", i, p[i]);
            // mpp_log("RK_VP9_DEC: regs[%02d]=%08X\n", i, *((RK_U32*)p));
        }
    }

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        !hw_regs->swreg1_int.sw_dec_rdy_sta) {
        MppFrame mframe = NULL;
        mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }

    if (reg_cxt->int_cb.callBack && task->dec.flags.wait_done) {
        DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
        hal_vp9d_update_counts(hal, task->dec.syntax.data);
        reg_cxt->int_cb.callBack(reg_cxt->int_cb.opaque, (void*)&pic_param->counts);
    }
    if (reg_cxt->fast_mode) {
        reg_cxt->g_buf[task->dec.reg_index].use_flag = 0;
    }

    (void)task;
    return ret;
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_reset(void *hal)
{
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t *)hal;
    mpp_log("hal_vp9d_reset in");
    memset(&reg_cxt->ls_info, 0, sizeof(reg_cxt->ls_info));
    reg_cxt->mv_base_addr = -1;
    reg_cxt->pre_mv_base_addr = -1;
    reg_cxt->last_segid_flag = 1;
    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_flush(void *hal)
{
    hal_vp9_context_t *reg_cxt = (hal_vp9_context_t *)hal;
    mpp_log("hal_vp9d_flush in");

    reg_cxt->mv_base_addr = -1;
    reg_cxt->pre_mv_base_addr = -1;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_control(void *hal, RK_S32 cmd_type, void *param)
{
    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO: {
        /* commit buffer stride */
        RK_U32 width = mpp_frame_get_width((MppFrame)param);
        RK_U32 height = mpp_frame_get_height((MppFrame)param);

        mpp_frame_set_hor_stride((MppFrame)param, vp9_hor_align(width));
        mpp_frame_set_ver_stride((MppFrame)param, vp9_ver_align(height));

        break;
    }
    default:
        break;
    }
    (void)hal;

    return MPP_OK;
}


const MppHalApi hal_api_vp9d = {
    .name = "vp9d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(hal_vp9_context_t),
    .flag = 0,
    .init = hal_vp9d_init,
    .deinit = hal_vp9d_deinit,
    .reg_gen = hal_vp9d_gen_regs,
    .start = hal_vp9d_start,
    .wait = hal_vp9d_wait,
    .reset = hal_vp9d_reset,
    .flush = hal_vp9d_flush,
    .control = hal_vp9d_control,
};

