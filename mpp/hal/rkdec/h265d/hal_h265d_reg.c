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

/*
 * @file       hal_h265d_reg.c
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#define MODULE_TAG "H265HAL"

#include <stdio.h>
#include <string.h>

#include "hal_h265d_reg.h"
#include "mpp_buffer.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "cabac.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_dec.h"
#include "mpp_device.h"
#include "mpp_buffer.h"
#include "mpp_env.h"
#include "mpp_bitput.h"
#include "hal_h265d_api.h"
#include "h265d_syntax.h"

/* #define dump */
#ifdef dump
static FILE *fp = NULL;
#endif

#define MAX_GEN_REG 3
RK_U32 h265h_debug = 0;
typedef struct h265d_reg_buf {
    RK_S32    use_flag;
    MppBuffer scaling_list_data;
    MppBuffer pps_data;
    MppBuffer rps_data;
    void*     hw_regs;
} h265d_reg_buf_t;
typedef struct h265d_reg_context {
    MppBufSlots     slots;
    MppBufSlots     packet_slots;
    MppBufferGroup group;
    MppBuffer cabac_table_data;
    MppBuffer scaling_list_data;
    MppBuffer pps_data;
    MppBuffer rps_data;
    void*     hw_regs;
    h265d_reg_buf_t g_buf[MAX_GEN_REG];
    RK_U32 fast_mode;
    IOInterruptCB int_cb;
    MppDevCtx dev_ctx;
    RK_U32 fast_mode_err_found;
    void *scaling_rk;
    void *scaling_qm;
} h265d_reg_context_t;

typedef struct ScalingList {
    /* This is a little wasteful, since sizeID 0 only needs 8 coeffs,
     * and size ID 3 only has 2 arrays, not 6. */
    RK_U8 sl[4][6][64];
    RK_U8 sl_dc[2][6];
} scalingList_t;

typedef struct ScalingFactor_Model {
    RK_U8 scalingfactor0[1248];
    RK_U8 scalingfactor1[96];     /*4X4 TU Rotate, total 16X4*/
    RK_U8 scalingdc[12];          /*N1005 Vienna Meeting*/
    RK_U8 reserverd[4];           /*16Bytes align*/
} scalingFactor_t;

static RK_U8 hal_hevc_diag_scan4x4_x[16] = {
    0, 0, 1, 0,
    1, 2, 0, 1,
    2, 3, 1, 2,
    3, 2, 3, 3,
};

static RK_U8 hal_hevc_diag_scan4x4_y[16] = {
    0, 1, 0, 2,
    1, 0, 3, 2,
    1, 0, 3, 2,
    1, 3, 2, 3,
};

static RK_U8 hal_hevc_diag_scan8x8_x[64] = {
    0, 0, 1, 0,
    1, 2, 0, 1,
    2, 3, 0, 1,
    2, 3, 4, 0,
    1, 2, 3, 4,
    5, 0, 1, 2,
    3, 4, 5, 6,
    0, 1, 2, 3,
    4, 5, 6, 7,
    1, 2, 3, 4,
    5, 6, 7, 2,
    3, 4, 5, 6,
    7, 3, 4, 5,
    6, 7, 4, 5,
    6, 7, 5, 6,
    7, 6, 7, 7,
};

static RK_U8 hal_hevc_diag_scan8x8_y[64] = {
    0, 1, 0, 2,
    1, 0, 3, 2,
    1, 0, 4, 3,
    2, 1, 0, 5,
    4, 3, 2, 1,
    0, 6, 5, 4,
    3, 2, 1, 0,
    7, 6, 5, 4,
    3, 2, 1, 0,
    7, 6, 5, 4,
    3, 2, 1, 7,
    6, 5, 4, 3,
    2, 7, 6, 5,
    4, 3, 7, 6,
    5, 4, 7, 6,
    5, 7, 6, 7,
};

typedef struct slice_ref_map {
    RK_U8 dbp_index;
    RK_U8 is_long_term;
} slice_ref_map_t;

#define SCALING_LIST_SIZE  81 * 1360
#define PPS_SIZE  80 * 64
#define RPS_SIZE   600 * 32
#define SCALING_LIST_SIZE_NUM 4

#define IS_IDR(nal_type) (nal_type == 19 || nal_type == 20)
#define IS_BLA(nal_type) (nal_type == 17 || nal_type == 16 || \
                   nal_type == 18)

#define IS_IRAP(nal_type) (nal_type >= 16 && nal_type <= 23)

#define L0 0
#define L1 1
#define MAX_REFS 16

enum RPSType {
    ST_CURR_BEF = 0,
    ST_CURR_AFT,
    ST_FOLL,
    LT_CURR,
    LT_FOLL,
    NB_RPS_TYPE,
};

enum SliceType {
    B_SLICE = 0,
    P_SLICE = 1,
    I_SLICE = 2,
};

typedef struct ShortTermRPS {
    RK_U32 num_negative_pics;
    RK_S32 num_delta_pocs;
    RK_S32 delta_poc[32];
    RK_U8  used[32];
} ShortTermRPS;

typedef struct LongTermRPS {
    RK_S32  poc[32];
    RK_U8   used[32];
    RK_U8   nb_refs;
} LongTermRPS;

typedef struct RefPicList {
    RK_U32  dpb_index[MAX_REFS];
    RK_U32 nb_refs;
} RefPicList_t;

typedef struct RefPicListTab {
    RefPicList_t refPicList[2];
} RefPicListTab_t;


typedef struct SliceHeader {
    RK_U32 pps_id;

    ///< address (in raster order) of the first block in the current slice segment
    RK_U32   slice_segment_addr;
    ///< address (in raster order) of the first block in the current slice
    RK_U32   slice_addr;

    enum SliceType slice_type;

    RK_S32 pic_order_cnt_lsb;

    RK_U8 first_slice_in_pic_flag;
    RK_U8 dependent_slice_segment_flag;
    RK_U8 pic_output_flag;
    RK_U8 colour_plane_id;

    ///< RPS coded in the slice header itself is stored here
    ShortTermRPS slice_rps;
    const ShortTermRPS *short_term_rps;
    LongTermRPS long_term_rps;
    RK_U32 list_entry_lx[2][32];

    RK_U8 rpl_modification_flag[2];
    RK_U8 no_output_of_prior_pics_flag;
    RK_U8 slice_temporal_mvp_enabled_flag;

    RK_U32 nb_refs[2];

    RK_U8 slice_sample_adaptive_offset_flag[3];
    RK_U8 mvd_l1_zero_flag;

    RK_U8 cabac_init_flag;
    RK_U8 disable_deblocking_filter_flag; ///< slice_header_disable_deblocking_filter_flag
    RK_U8 slice_loop_filter_across_slices_enabled_flag;
    RK_U8 collocated_list;

    RK_U32 collocated_ref_idx;

    RK_S32 slice_qp_delta;
    RK_S32 slice_cb_qp_offset;
    RK_S32 slice_cr_qp_offset;

    RK_S32 beta_offset;    ///< beta_offset_div2 * 2
    RK_S32 tc_offset;      ///< tc_offset_div2 * 2

    RK_U32 max_num_merge_cand; ///< 5 - 5_minus_max_num_merge_cand

    RK_S32 *entry_point_offset;
    RK_S32 * offset;
    RK_S32 * size;
    RK_S32 num_entry_point_offsets;

    RK_S8 slice_qp;

    RK_U8 luma_log2_weight_denom;
    RK_S16 chroma_log2_weight_denom;

    RK_S32 slice_ctb_addr_rs;
} SliceHeader_t;

static RK_U32 hevc_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 8);
}

#ifdef SOFIA_3GR_LINUX
static RK_U32 hevc_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 64);
}
#else
static RK_U32 hevc_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 256) | 256;
}
#endif


static MPP_RET hal_h265d_alloc_res(void *hal)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;
    h265d_reg_context_t *reg_cxt = (h265d_reg_context_t *)hal;
    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            reg_cxt->g_buf[i].hw_regs =
                mpp_calloc_size(void, sizeof(H265d_REGS_t));
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].scaling_list_data,
                                 SCALING_LIST_SIZE);
            if (ret) {
                mpp_err("h265d scaling_list_data get buffer failed\n");
                return ret;
            }

            ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->g_buf[i].pps_data,
                                 PPS_SIZE);
            if (ret) {
                mpp_err("h265d pps_data get buffer failed\n");
                return ret;
            }

            ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->g_buf[i].rps_data,
                                 RPS_SIZE);
            if (ret) {
                mpp_err("h265d rps_data get buffer failed\n");
                return ret;
            }
        }
    } else {
        reg_cxt->hw_regs = mpp_calloc_size(void, sizeof(H265d_REGS_t));
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->scaling_list_data,
                             SCALING_LIST_SIZE);
        if (ret) {
            mpp_err("h265d scaling_list_data get buffer failed\n");
            return ret;
        }

        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->pps_data, PPS_SIZE);
        if (ret) {
            mpp_err("h265d pps_data get buffer failed\n");
            return ret;
        }

        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->rps_data, RPS_SIZE);
        if (ret) {
            mpp_err("h265d rps_data get buffer failed\n");
            return ret;
        }

    }
    return MPP_OK;
}

static MPP_RET hal_h265d_release_res(void *hal)
{
    RK_S32 ret = 0;
    h265d_reg_context_t *reg_cxt = ( h265d_reg_context_t *)hal;
    RK_S32 i = 0;
    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (reg_cxt->g_buf[i].scaling_list_data) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].scaling_list_data);
                if (ret) {
                    mpp_err("h265d scaling_list_data free buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].pps_data) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].pps_data);
                if (ret) {
                    mpp_err("h265d pps_data free buffer failed\n");
                    return ret;
                }
            }

            if (reg_cxt->g_buf[i].rps_data) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].rps_data);
                if (ret) {
                    mpp_err("h265d rps_data free buffer failed\n");
                    return ret;
                }
            }

            if (reg_cxt->g_buf[i].hw_regs) {
                mpp_free(reg_cxt->g_buf[i].hw_regs);
                reg_cxt->g_buf[i].hw_regs = NULL;
            }
        }
    } else {
        if (reg_cxt->scaling_list_data) {
            ret = mpp_buffer_put(reg_cxt->scaling_list_data);
            if (ret) {
                mpp_err("h265d scaling_list_data free buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->pps_data) {
            ret = mpp_buffer_put(reg_cxt->pps_data);
            if (ret) {
                mpp_err("h265d pps_data free buffer failed\n");
                return ret;
            }
        }

        if (reg_cxt->rps_data) {
            ret = mpp_buffer_put(reg_cxt->rps_data);
            if (ret) {
                mpp_err("h265d rps_data free buffer failed\n");
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

MPP_RET hal_h265d_init(void *hal, MppHalCfg *cfg)
{

    RK_S32 ret = 0;
    h265d_reg_context_t *reg_cxt = (h265d_reg_context_t *)hal;

    if (NULL == reg_cxt) {
        mpp_err("hal instan no alloc");
        return MPP_ERR_UNKNOW;
    }

    reg_cxt->slots = cfg->frame_slots;
    reg_cxt->int_cb = cfg->hal_int_cb;
    reg_cxt->fast_mode = cfg->fast_mode;

    mpp_slots_set_prop(reg_cxt->slots, SLOTS_HOR_ALIGN, hevc_hor_align);
    mpp_slots_set_prop(reg_cxt->slots, SLOTS_VER_ALIGN, hevc_ver_align);

    reg_cxt->scaling_qm = mpp_calloc(DXVA_Qmatrix_HEVC, 1);
    if (reg_cxt->scaling_qm == NULL) {
        mpp_err("scaling_org alloc fail");
        return MPP_ERR_MALLOC;
    }

    reg_cxt->scaling_rk = mpp_calloc(scalingFactor_t, 1);
    if (reg_cxt->scaling_rk == NULL) {
        mpp_err("scaling_rk alloc fail");
        return MPP_ERR_MALLOC;
    }
    reg_cxt->packet_slots = cfg->packet_slots;

    ///<- mpp_device_init
    MppDevCfg dev_cfg = {
        .type = MPP_CTX_DEC,               /* type */
        .coding = MPP_VIDEO_CodingHEVC,    /* coding */
        .platform = 0,                     /* platform */
        .pp_enable = 0,                    /* pp_enable */
    };

    ret = mpp_device_init(&reg_cxt->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }

    if (reg_cxt->group == NULL) {
        ret = mpp_buffer_group_get_internal(&reg_cxt->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->cabac_table_data, sizeof(cabac_table));
    if (ret) {
        mpp_err("h265d cabac_table get buffer failed\n");
        return ret;
    }

    ret = mpp_buffer_write(reg_cxt->cabac_table_data, 0, (void*)cabac_table, sizeof(cabac_table));
    if (ret) {
        mpp_err("h265d write cabac_table data failed\n");
        return ret;
    }

    ret = hal_h265d_alloc_res(hal);
    if (ret) {
        mpp_err("hal_h265d_alloc_res failed\n");
        return ret;
    }
    mpp_env_get_u32("h265h_debug", &h265h_debug, 0);

#ifdef dump
    fp = fopen("/data/hal.bin", "wb");
#endif
    return MPP_OK;
}

MPP_RET hal_h265d_deinit(void *hal)
{

    RK_S32 ret = 0;
    h265d_reg_context_t *reg_cxt = (h265d_reg_context_t *)hal;

    ///<- mpp_device_init
    if (reg_cxt->dev_ctx) {
        ret = mpp_device_deinit(reg_cxt->dev_ctx);
        if (ret)
            mpp_err("mpp_device_deinit failed. ret: %d\n", ret);
    }

    ret = mpp_buffer_put(reg_cxt->cabac_table_data);
    if (ret) {
        mpp_err("h265d cabac_table free buffer failed\n");
        return ret;
    }

    if (reg_cxt->scaling_qm) {
        mpp_free(reg_cxt->scaling_qm);
    }

    if (reg_cxt->scaling_rk) {
        mpp_free(reg_cxt->scaling_rk);
    }

    hal_h265d_release_res(hal);

    if (reg_cxt->group) {
        ret = mpp_buffer_group_put(reg_cxt->group);
        if (ret) {
            mpp_err("h265d group free buffer failed\n");
            return ret;
        }
    }
    return MPP_OK;
}

static void
hal_record_scaling_list(scalingFactor_t *pScalingFactor_out,
                        scalingList_t *pScalingList)
{
    RK_S32 i;
    RK_U32 g_scalingListNum_model[SCALING_LIST_SIZE_NUM] = {6, 6, 6, 2}; // from C Model
    RK_U32 nIndex = 0;
    RK_U32 sizeId, matrixId, listId;
    RK_U8 *p = pScalingFactor_out->scalingfactor0;
    RK_U8 tmpBuf[8 * 8];

    //output non-default scalingFactor Table (1248 BYTES)
    for (sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++) {
        for (listId = 0; listId < g_scalingListNum_model[sizeId]; listId++) {
            if (sizeId < 3) {
                for (i = 0; i < (sizeId == 0 ? 16 : 64); i++) {
                    pScalingFactor_out->scalingfactor0[nIndex++] = (RK_U8)pScalingList->sl[sizeId][listId][i];
                }
            } else {
                for (i = 0; i < 64; i ++) {
                    pScalingFactor_out->scalingfactor0[nIndex++] = (RK_U8)pScalingList->sl[sizeId][listId][i];
                }
                for (i = 0; i < 128; i ++) {
                    pScalingFactor_out->scalingfactor0[nIndex++] = 0;
                }
            }
        }
    }
    //output non-default scalingFactor Table Rotation(96 Bytes)
    nIndex = 0;
    for (listId = 0; listId < g_scalingListNum_model[0]; listId++) {
        RK_U8 temp16[16] = {0};
        for (i = 0; i < 16; i ++) {
            temp16[i] = (RK_U8)pScalingList->sl[0][listId][i];
        }
        for (i = 0; i < 4; i ++) {
            pScalingFactor_out->scalingfactor1[nIndex++] = temp16[i];
            pScalingFactor_out->scalingfactor1[nIndex++] = temp16[i + 4];
            pScalingFactor_out->scalingfactor1[nIndex++] = temp16[i + 8];
            pScalingFactor_out->scalingfactor1[nIndex++] = temp16[i + 12];
        }
    }
    //output non-default ScalingList_DC_Coeff (12 BYTES)
    nIndex = 0;
    for (listId = 0; listId < g_scalingListNum_model[2]; listId++) { //sizeId = 2
        pScalingFactor_out->scalingdc[nIndex++] = (RK_U8)pScalingList->sl_dc[0][listId];// zrh warning: sl_dc differed from scalingList->getScalingListDC
    }
    for (listId = 0; listId < g_scalingListNum_model[3]; listId++) { //sizeId = 3
        pScalingFactor_out->scalingdc[nIndex++] = (RK_U8)pScalingList->sl_dc[1][listId];// zrh warning: sl_dc differed from scalingList->getScalingListDC
        pScalingFactor_out->scalingdc[nIndex++] = 0;
        pScalingFactor_out->scalingdc[nIndex++] = 0;
    }

    //align 16X address
    nIndex = 0;
    for (i = 0; i < 4; i ++) {
        pScalingFactor_out->reserverd[nIndex++] = 0;
    }

    //----------------------All above code show the normal store way in HM--------------------------
    //--------from now on, the scalingfactor0 is rotated 90', the scalingfactor1 is also rotated 90'

    //sizeId == 0
    for (matrixId = 0; matrixId < 6; matrixId++) {
        p = pScalingFactor_out->scalingfactor0 + matrixId * 16;

        for (i = 0; i < 4; i++) {
            tmpBuf[4 * 0 + i] = p[i * 4 + 0];
            tmpBuf[4 * 1 + i] = p[i * 4 + 1];
            tmpBuf[4 * 2 + i] = p[i * 4 + 2];
            tmpBuf[4 * 3 + i] = p[i * 4 + 3];
        }
        memcpy(p, tmpBuf, 4 * 4 * sizeof(RK_U8));
    }
    //sizeId == 1
    for (matrixId = 0; matrixId < 6; matrixId++) {
        p = pScalingFactor_out->scalingfactor0 + 6 * 16 + matrixId * 64;

        for (i = 0; i < 8; i++) {
            tmpBuf[8 * 0 + i] = p[i * 8 + 0];
            tmpBuf[8 * 1 + i] = p[i * 8 + 1];
            tmpBuf[8 * 2 + i] = p[i * 8 + 2];
            tmpBuf[8 * 3 + i] = p[i * 8 + 3];
            tmpBuf[8 * 4 + i] = p[i * 8 + 4];
            tmpBuf[8 * 5 + i] = p[i * 8 + 5];
            tmpBuf[8 * 6 + i] = p[i * 8 + 6];
            tmpBuf[8 * 7 + i] = p[i * 8 + 7];
        }
        memcpy(p, tmpBuf, 8 * 8 * sizeof(RK_U8));
    }
    //sizeId == 2
    for (matrixId = 0; matrixId < 6; matrixId++) {
        p = pScalingFactor_out->scalingfactor0 + 6 * 16 + 6 * 64 + matrixId * 64;

        for (i = 0; i < 8; i++) {
            tmpBuf[8 * 0 + i] = p[i * 8 + 0];
            tmpBuf[8 * 1 + i] = p[i * 8 + 1];
            tmpBuf[8 * 2 + i] = p[i * 8 + 2];
            tmpBuf[8 * 3 + i] = p[i * 8 + 3];
            tmpBuf[8 * 4 + i] = p[i * 8 + 4];
            tmpBuf[8 * 5 + i] = p[i * 8 + 5];
            tmpBuf[8 * 6 + i] = p[i * 8 + 6];
            tmpBuf[8 * 7 + i] = p[i * 8 + 7];
        }
        memcpy(p, tmpBuf, 8 * 8 * sizeof(RK_U8));
    }
    //sizeId == 3
    for (matrixId = 0; matrixId < 6; matrixId++) {
        p = pScalingFactor_out->scalingfactor0 + 6 * 16 + 6 * 64 + 6 * 64 + matrixId * 64;

        for (i = 0; i < 8; i++) {
            tmpBuf[8 * 0 + i] = p[i * 8 + 0];
            tmpBuf[8 * 1 + i] = p[i * 8 + 1];
            tmpBuf[8 * 2 + i] = p[i * 8 + 2];
            tmpBuf[8 * 3 + i] = p[i * 8 + 3];
            tmpBuf[8 * 4 + i] = p[i * 8 + 4];
            tmpBuf[8 * 5 + i] = p[i * 8 + 5];
            tmpBuf[8 * 6 + i] = p[i * 8 + 6];
            tmpBuf[8 * 7 + i] = p[i * 8 + 7];
        }
        memcpy(p, tmpBuf, 8 * 8 * sizeof(RK_U8));
    }

    //sizeId == 0
    for (matrixId = 0; matrixId < 6; matrixId++) {
        p = pScalingFactor_out->scalingfactor1 + matrixId * 16;

        for (i = 0; i < 4; i++) {
            tmpBuf[4 * 0 + i] = p[i * 4 + 0];
            tmpBuf[4 * 1 + i] = p[i * 4 + 1];
            tmpBuf[4 * 2 + i] = p[i * 4 + 2];
            tmpBuf[4 * 3 + i] = p[i * 4 + 3];
        }
        memcpy(p, tmpBuf, 4 * 4 * sizeof(RK_U8));
    }
}

static int hal_h265d_slice_rpl(void *dxva, SliceHeader_t *sh, RefPicListTab_t *ref)
{
    RK_U8 nb_list = sh->slice_type == B_SLICE ? 2 : 1;
    RK_U8 list_idx;
    RK_U32 i, j;
    RK_U8 bef_nb_refs = 0, aft_nb_refs = 0, lt_cur_nb_refs = 0;
    h265d_dxva2_picture_context_t *dxva_cxt = NULL;
    RK_S32 cand_lists[3];

    memset(ref, 0, sizeof(RefPicListTab_t));
    dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;

    for (i = 0; i < 8; i++ ) {
        if (dxva_cxt->pp.RefPicSetStCurrBefore[i] != 0xff) {
            bef_nb_refs ++;
        }

        if (dxva_cxt->pp.RefPicSetStCurrAfter[i] != 0xff) {
            aft_nb_refs ++;
        }

        if (dxva_cxt->pp.RefPicSetLtCurr[i] != 0xff) {
            lt_cur_nb_refs ++;
        }
    }

    if (!(bef_nb_refs + aft_nb_refs +
          lt_cur_nb_refs)) {
        mpp_err( "Zero refs in the frame RPS.\n");
        return  MPP_ERR_STREAM;
    }

    for (list_idx = 0; list_idx < nb_list; list_idx++) {
        RefPicList_t  rpl_tmp;
        RefPicList_t *rpl     = &ref->refPicList[list_idx];
        memset(&rpl_tmp, 0, sizeof(RefPicList_t));

        /* The order of the elements is
         * ST_CURR_BEF - ST_CURR_AFT - LT_CURR for the L0 and
         * ST_CURR_AFT - ST_CURR_BEF - LT_CURR for the L1 */

        cand_lists[0] = list_idx ? ST_CURR_AFT : ST_CURR_BEF;
        cand_lists[1] = list_idx ? ST_CURR_BEF : ST_CURR_AFT;
        cand_lists[2] = LT_CURR;
        /* concatenate the candidate lists for the current frame */
        while ((RK_U32)rpl_tmp.nb_refs < sh->nb_refs[list_idx]) {
            for (i = 0; i < MPP_ARRAY_ELEMS(cand_lists); i++) {
                RK_U8 *rps = NULL;
                RK_U32 nb_refs = 0;
                if (cand_lists[i] == ST_CURR_BEF) {
                    rps = &dxva_cxt->pp.RefPicSetStCurrBefore[0];
                    nb_refs = bef_nb_refs;
                } else if (cand_lists[i] == ST_CURR_AFT) {
                    rps = &dxva_cxt->pp.RefPicSetStCurrAfter[0];
                    nb_refs = aft_nb_refs;
                } else {
                    rps = &dxva_cxt->pp.RefPicSetLtCurr[0];
                    nb_refs = lt_cur_nb_refs;
                }
                for (j = 0; j < nb_refs && rpl_tmp.nb_refs < MAX_REFS; j++) {
                    rpl_tmp.dpb_index[rpl_tmp.nb_refs]       = rps[j];
                    rpl_tmp.nb_refs++;
                }
            }
        }

        /* reorder the references if necessary */
        if (sh->rpl_modification_flag[list_idx]) {
            for (i = 0; i < sh->nb_refs[list_idx]; i++) {
                int idx = sh->list_entry_lx[list_idx][i];
                rpl->dpb_index[i]        = rpl_tmp.dpb_index[idx];
                rpl->nb_refs++;
            }
        } else {
            memcpy(rpl, &rpl_tmp, sizeof(*rpl));
            rpl->nb_refs = MPP_MIN((RK_U32)rpl->nb_refs, sh->nb_refs[list_idx]);
        }
    }
    return 0;
}

static RK_S32 hal_h265d_slice_output_rps(void *dxva, void *rps_buf)
{

    RK_U32 i, j, k;
    RK_S32 value;
    RK_U32 nal_type;
    RK_S32 slice_idx = 0;
    BitReadCtx_t gb_cxt, *gb;
    SliceHeader_t sh;
    RK_U8     rps_bit_offset[600];
    RK_U8     rps_bit_offset_st[600];
    RK_U8     slice_nb_rps_poc[600];
    RK_U8     lowdelay_flag[600];
    slice_ref_map_t rps_pic_info[600][2][15];
    RK_U32    nb_refs = 0;
    RK_S32    bit_begin;
    h265d_dxva2_picture_context_t *dxva_cxt = NULL;

    memset(&rps_pic_info,   0, sizeof(rps_pic_info));
    memset(&slice_nb_rps_poc, 0, sizeof(slice_nb_rps_poc));
    memset(&rps_bit_offset, 0, sizeof(rps_bit_offset));
    memset(&rps_bit_offset_st, 0, sizeof(rps_bit_offset_st));
    memset(&lowdelay_flag, 0, sizeof(lowdelay_flag));

    dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    for (k = 0; k < dxva_cxt->slice_count; k++) {
        RefPicListTab_t ref;
        memset(&sh, 0, sizeof(SliceHeader_t));
        // mpp_err("data[%d]= 0x%x,size[%d] = %d \n",
        //   k,dxva_cxt->slice_short[k].BSNALunitDataLocation, k,dxva_cxt->slice_short[k].SliceBytesInBuffer);
        mpp_set_bitread_ctx(&gb_cxt, (RK_U8*)(dxva_cxt->bitstream + dxva_cxt->slice_short[k].BSNALunitDataLocation),
                            dxva_cxt->slice_short[k].SliceBytesInBuffer);

        mpp_set_pre_detection(&gb_cxt);
        gb = &gb_cxt;

        READ_ONEBIT(gb, &value);

        if ( value != 0)
            return  MPP_ERR_STREAM;

        READ_BITS(gb, 6, &nal_type);

        if (nal_type > 23) {
            continue;
        }

        SKIP_BITS(gb, 9);

        READ_ONEBIT(gb, &sh.first_slice_in_pic_flag);

        if (nal_type >= 16 && nal_type <= 23)
            READ_ONEBIT(gb, &sh.no_output_of_prior_pics_flag);

        READ_UE(gb, &sh.pps_id);

        if (sh.pps_id >= 64 ) {
            mpp_err( "PPS id out of range: %d\n", sh.pps_id);
            return  MPP_ERR_STREAM;
        }

        sh.dependent_slice_segment_flag = 0;
        if (!sh.first_slice_in_pic_flag) {
            RK_S32 slice_address_length;
            RK_S32 width, height, ctb_width, ctb_height;
            RK_S32 log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;
            RK_S32 log2_ctb_size = log2_min_cb_size + dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size;

            width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
            height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

            ctb_width  = (width  + (1 << log2_ctb_size) - 1) >> log2_ctb_size;
            ctb_height = (height + (1 << log2_ctb_size) - 1) >> log2_ctb_size;

            if (dxva_cxt->pp.dependent_slice_segments_enabled_flag)
                READ_ONEBIT(gb, &sh.dependent_slice_segment_flag);

            slice_address_length = mpp_ceil_log2(ctb_width * ctb_height);

            READ_BITS(gb, slice_address_length, &sh.slice_segment_addr);

            if (sh.slice_segment_addr >= (RK_U32)(ctb_width * ctb_height)) {
                mpp_err(
                    "Invalid slice segment address: %u.\n",
                    sh.slice_segment_addr);
                return  MPP_ERR_STREAM;
            }

            if (!sh.dependent_slice_segment_flag) {
                sh.slice_addr = sh.slice_segment_addr;
                slice_idx++;
            }
        } else {
            sh.slice_segment_addr = sh.slice_addr = 0;
            slice_idx           = 0;
        }

        if (!sh.dependent_slice_segment_flag) {
            for (i = 0; i < dxva_cxt->pp.num_extra_slice_header_bits; i++)
                SKIP_BITS(gb, 1);

            READ_UE(gb, &sh.slice_type);
            if (!(sh.slice_type == I_SLICE ||
                  sh.slice_type == P_SLICE ||
                  sh.slice_type == B_SLICE)) {
                mpp_err( "Unknown slice type: %d.\n",
                         sh.slice_type);
                return  MPP_ERR_STREAM;
            }
            /*     if (!s->decoder_id && IS_IRAP(nal_type) && sh->slice_type != I_SLICE) {
                     mpp_err( "Inter slices in an IRAP frame.\n");
                     return  MPP_ERR_STREAM;
                 }*/

            if (dxva_cxt->pp.output_flag_present_flag)
                READ_ONEBIT(gb, &sh.pic_output_flag);

            if (dxva_cxt->pp.separate_colour_plane_flag)
                READ_BITS(gb, 2, &sh.colour_plane_id );

            if (!IS_IDR(nal_type)) {
                int short_term_ref_pic_set_sps_flag;

                READ_BITS(gb, (dxva_cxt->pp.log2_max_pic_order_cnt_lsb_minus4 + 4), &sh.pic_order_cnt_lsb);

                READ_ONEBIT(gb, &short_term_ref_pic_set_sps_flag);

                bit_begin = gb->used_bits;

                if (!short_term_ref_pic_set_sps_flag) {
                    SKIP_BITS(gb, dxva_cxt->pp.wNumBitsForShortTermRPSInSlice);
                } else {
                    RK_S32 numbits, rps_idx;
                    if (!dxva_cxt->pp.num_short_term_ref_pic_sets) {
                        mpp_err( "No ref lists in the SPS.\n");
                        return  MPP_ERR_STREAM;
                    }
                    numbits = mpp_ceil_log2(dxva_cxt->pp.num_short_term_ref_pic_sets);
                    rps_idx = 0;
                    if (numbits > 0)
                        READ_BITS(gb, numbits, &rps_idx);
                }

                rps_bit_offset_st[slice_idx] = gb->used_bits - bit_begin;
                rps_bit_offset[slice_idx] = rps_bit_offset_st[slice_idx];
                if (dxva_cxt->pp.long_term_ref_pics_present_flag) {

//                    RK_S32 max_poc_lsb    = 1 << (dxva_cxt->pp.log2_max_pic_order_cnt_lsb_minus4 + 4);
                    RK_U32 nb_sps = 0, nb_sh;

                    bit_begin = gb->used_bits;
                    if (dxva_cxt->pp.num_long_term_ref_pics_sps > 0)
                        READ_UE(gb, &nb_sps);

                    READ_UE(gb, &nb_sh);

                    nb_refs = nb_sh + nb_sps;

                    for (i = 0; i < nb_refs; i++) {
                        RK_U8 delta_poc_msb_present;

                        if ((RK_U32)i < nb_sps) {
                            RK_U8 lt_idx_sps = 0;

                            if (dxva_cxt->pp.num_long_term_ref_pics_sps > 1)
                                READ_BITS(gb, mpp_ceil_log2(dxva_cxt->pp.num_long_term_ref_pics_sps), &lt_idx_sps);
                        } else {
                            SKIP_BITS(gb, (dxva_cxt->pp.log2_max_pic_order_cnt_lsb_minus4 + 4));
                            SKIP_BITS(gb, 1);
                        }

                        READ_ONEBIT(gb, &delta_poc_msb_present);
                        if (delta_poc_msb_present) {
                            RK_S32 delta = 0;
                            READ_UE(gb, &delta);
                        }
                    }
                    rps_bit_offset[slice_idx] += (gb->used_bits - bit_begin);

                }

                if (dxva_cxt->pp.sps_temporal_mvp_enabled_flag)
                    READ_ONEBIT(gb, &sh.slice_temporal_mvp_enabled_flag);
                else
                    sh.slice_temporal_mvp_enabled_flag = 0;
            } else {
                sh.short_term_rps = NULL;
            }

            if (dxva_cxt->pp.sample_adaptive_offset_enabled_flag) {
                READ_ONEBIT(gb, &sh.slice_sample_adaptive_offset_flag[0]);
                READ_ONEBIT(gb, &sh.slice_sample_adaptive_offset_flag[1]);
                sh.slice_sample_adaptive_offset_flag[2] =
                    sh.slice_sample_adaptive_offset_flag[1];
            } else {
                sh.slice_sample_adaptive_offset_flag[0] = 0;
                sh.slice_sample_adaptive_offset_flag[1] = 0;
                sh.slice_sample_adaptive_offset_flag[2] = 0;
            }

            sh.nb_refs[L0] = sh.nb_refs[L1] = 0;
            if (sh.slice_type == P_SLICE || sh.slice_type == B_SLICE) {

                sh.nb_refs[L0] = dxva_cxt->pp.num_ref_idx_l0_default_active_minus1 + 1;
                if (sh.slice_type == B_SLICE)
                    sh.nb_refs[L1] =  dxva_cxt->pp.num_ref_idx_l1_default_active_minus1 + 1;

                READ_ONEBIT(gb, &value);

                if (value) { // num_ref_idx_active_override_flag
                    READ_UE(gb, &sh.nb_refs[L0]);
                    sh.nb_refs[L0] += 1;
                    if (sh.slice_type == B_SLICE) {
                        READ_UE(gb, &sh.nb_refs[L1]);
                        sh.nb_refs[L1] += 1;
                    }
                }
                if (sh.nb_refs[L0] > MAX_REFS || sh.nb_refs[L1] > MAX_REFS) {
                    mpp_err( "Too many refs: %d/%d.\n",
                             sh.nb_refs[L0], sh.nb_refs[L1]);
                    return  MPP_ERR_STREAM;
                }

                sh.rpl_modification_flag[0] = 0;
                sh.rpl_modification_flag[1] = 0;
                nb_refs = 0;
                for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_cxt->pp.RefPicList); i++) {
                    if (dxva_cxt->pp.RefPicList[i].bPicEntry != 0xff) {
                        h265h_dbg(H265H_DBG_RPS, "dxva_cxt->pp.RefPicList[i].bPicEntry = %d", dxva_cxt->pp.RefPicList[i].bPicEntry);
                        nb_refs++;
                    }
                }

                if (!nb_refs) {
                    mpp_err( "Zero refs for a frame with P or B slices.\n");
                    return  MPP_ERR_STREAM;
                }

                if (dxva_cxt->pp.lists_modification_present_flag && nb_refs > 1) {
                    READ_ONEBIT(gb, &sh.rpl_modification_flag[0]);
                    if (sh.rpl_modification_flag[0]) {
                        for (i = 0; (RK_U32)i < sh.nb_refs[L0]; i++)
                            READ_BITS(gb, mpp_ceil_log2(nb_refs), &sh.list_entry_lx[0][i]);
                    }

                    if (sh.slice_type == B_SLICE) {
                        READ_ONEBIT(gb, &sh.rpl_modification_flag[1]);
                        if (sh.rpl_modification_flag[1] == 1)
                            for (i = 0; (RK_U32)i < sh.nb_refs[L1]; i++)
                                READ_BITS(gb, mpp_ceil_log2(nb_refs), &sh.list_entry_lx[1][i]);
                    }
                }
            }
        }

        if (!sh.dependent_slice_segment_flag &&
            sh.slice_type != I_SLICE) {
            hal_h265d_slice_rpl(dxva, &sh, &ref);
            lowdelay_flag[slice_idx]  =  1;
            if (ref.refPicList) {
                RK_U32 nb_list = I_SLICE - sh.slice_type;
                for (j = 0; j < nb_list; j++) {
                    for (i = 0; i < ref.refPicList[j].nb_refs; i++) {
                        RK_U8 index = 0;
                        index = ref.refPicList[j].dpb_index[i];
                        // mpp_err("slice_idx = %d index = %d,j = %d i = %d",slice_idx,index,j,i);
                        if (index != 0xff) {
                            rps_pic_info[slice_idx][j][i].dbp_index = index;
                            rps_pic_info[slice_idx][j][i].is_long_term
                                = dxva_cxt->pp.RefPicList[index].AssociatedFlag;
                            if (dxva_cxt->pp.PicOrderCntValList[index] > dxva_cxt->pp.CurrPicOrderCntVal)
                                lowdelay_flag[slice_idx] = 0;
                        }
                    }
                }
            }
            if (sh.slice_type == I_SLICE)
                slice_nb_rps_poc[slice_idx] = 0;
            else
                slice_nb_rps_poc[slice_idx] = nb_refs;

        }
    }
//out put for rk format
    {
        RK_S32  nb_slice = slice_idx + 1;
        RK_S32  fifo_len   = nb_slice * 4 + 1;//size of rps_packet alloc more 1 64 bit invoid buffer no enought
        RK_U64 *rps_packet = mpp_malloc(RK_U64, fifo_len);
        BitputCtx_t bp;
        mpp_set_bitput_ctx(&bp, rps_packet, fifo_len);
        for (k = 0; k < (RK_U32)nb_slice; k++) {
            for (j = 0; j < 2; j++) {
                for (i = 0; i < 15; i++) {
                    mpp_put_bits(&bp, rps_pic_info[k][j][i].is_long_term, 1);
                    if (j == 1 && i == 4) {
                        mpp_put_align (&bp, 64, 0xf);
                    }
                    mpp_put_bits(&bp, rps_pic_info[k][j][i].dbp_index,    4);
                }
            }
            mpp_put_bits(&bp, lowdelay_flag      [k], 1);
            h265h_dbg(H265H_DBG_RPS, "lowdelay_flag = %d \n", lowdelay_flag[k]);
            mpp_put_bits(&bp, rps_bit_offset     [k], 10);

            h265h_dbg(H265H_DBG_RPS, "rps_bit_offset = %d \n", rps_bit_offset[k]);
            mpp_put_bits(&bp, rps_bit_offset_st  [k], 9);

            h265h_dbg(H265H_DBG_RPS, "rps_bit_offset_st = %d \n", rps_bit_offset_st[k]);
            mpp_put_bits(&bp, slice_nb_rps_poc   [k], 4);

            h265h_dbg(H265H_DBG_RPS, "slice_nb_rps_poc = %d \n", slice_nb_rps_poc[k]);
            mpp_put_align   (&bp, 64, 0xf);
        }
        if (rps_buf != NULL) {
            memcpy(rps_buf, rps_packet, nb_slice * 32);
        }
        mpp_free(rps_packet);
    }

    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static void hal_h265d_output_scalinglist_packet(void *hal, void *ptr, void *dxva)
{
    scalingList_t sl;
    RK_U32 i, j, pos;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    h265d_reg_context_t *reg_cxt = ( h265d_reg_context_t *)hal;
    if (memcmp((void*)&dxva_cxt->qm, reg_cxt->scaling_qm, sizeof(DXVA_Qmatrix_HEVC))) {
        memset(&sl, 0, sizeof(scalingList_t));
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 16; j++) {
                pos = 4 * hal_hevc_diag_scan4x4_y[j] + hal_hevc_diag_scan4x4_x[j];
                sl.sl[0][i][pos] = dxva_cxt->qm.ucScalingLists0[i][j];
            }

            for (j = 0; j < 64; j++) {
                pos = 8 * hal_hevc_diag_scan8x8_y[j] + hal_hevc_diag_scan8x8_x[j];
                sl.sl[1][i][pos] =  dxva_cxt->qm.ucScalingLists1[i][j];
                sl.sl[2][i][pos] =  dxva_cxt->qm.ucScalingLists2[i][j];

                if (i < 2)
                    sl.sl[3][i][pos] =  dxva_cxt->qm.ucScalingLists3[i][j];
            }

            sl.sl_dc[0][i] =  dxva_cxt->qm.ucScalingListDCCoefSizeID2[i];
            if (i < 2)
                sl.sl_dc[1][i] =  dxva_cxt->qm.ucScalingListDCCoefSizeID3[i];
        }
        hal_record_scaling_list((scalingFactor_t *)reg_cxt->scaling_rk, &sl);
    }
    memcpy(ptr, reg_cxt->scaling_rk, sizeof(scalingFactor_t));
}


static RK_S32 hal_h265d_output_pps_packet(void *hal, void *dxva)
{
    RK_S32 fifo_len = 10;
    RK_S32 i, j;
    RK_U32 addr;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height;
    h265d_reg_context_t *reg_cxt = ( h265d_reg_context_t *)hal;
    h265d_dxva2_picture_context_t *dxva_cxt = (h265d_dxva2_picture_context_t*)dxva;
    BitputCtx_t bp;
    RK_U64 *pps_packet = mpp_calloc(RK_U64, fifo_len + 1);

    if (NULL == reg_cxt || dxva_cxt == NULL) {
        mpp_err("%s:%s:%d reg_cxt or dxva_cxt is NULL",
                __FILE__, __FUNCTION__, __LINE__);
        MPP_FREE(pps_packet);
        return MPP_ERR_NULL_PTR;
    }

    void *pps_ptr = mpp_buffer_get_ptr(reg_cxt->pps_data);
    if (NULL == pps_ptr) {
        mpp_err("pps_data get ptr error");
        return MPP_ERR_NOMEM;
    }
    memset(pps_ptr, 0, 80 * 64);
    // pps_packet = (RK_U64 *)(pps_ptr + dxva_cxt->pp.pps_id * 80);

    for (i = 0; i < 10; i++) pps_packet[i] = 0;

    mpp_set_bitput_ctx(&bp, pps_packet, fifo_len);

    // SPS
    mpp_put_bits(&bp, dxva_cxt->pp.vps_id                            , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.sps_id                            , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.chroma_format_idc                 , 2);

    log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;
    width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

    mpp_put_bits(&bp, width                                          , 13);
    mpp_put_bits(&bp, height                                         , 13);
    mpp_put_bits(&bp, dxva_cxt->pp.bit_depth_luma_minus8 + 8         , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.bit_depth_chroma_minus8 + 8       , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.log2_max_pic_order_cnt_lsb_minus4 + 4      , 5);
    mpp_put_bits(&bp, dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size   , 2); //log2_maxa_coding_block_depth
    mpp_put_bits(&bp, dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3 , 3);
    mpp_put_bits(&bp, dxva_cxt->pp.log2_min_transform_block_size_minus2 + 2   , 3);
    ///<-zrh comment ^  57 bit above
    mpp_put_bits(&bp, dxva_cxt->pp.log2_diff_max_min_transform_block_size     , 2);
    mpp_put_bits(&bp, dxva_cxt->pp.max_transform_hierarchy_depth_inter        , 3);
    mpp_put_bits(&bp, dxva_cxt->pp.max_transform_hierarchy_depth_intra        , 3);
    mpp_put_bits(&bp, dxva_cxt->pp.scaling_list_enabled_flag                  , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.amp_enabled_flag                           , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.sample_adaptive_offset_enabled_flag        , 1);
    ///<-zrh comment ^  68 bit above
    mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag                           , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag ? (dxva_cxt->pp.pcm_sample_bit_depth_luma_minus1 + 1) : 0  , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag ? (dxva_cxt->pp.pcm_sample_bit_depth_chroma_minus1 + 1) : 0 , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.pcm_loop_filter_disabled_flag                                               , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.log2_diff_max_min_pcm_luma_coding_block_size                                , 3);
    mpp_put_bits(&bp, dxva_cxt->pp.pcm_enabled_flag ? (dxva_cxt->pp.log2_min_pcm_luma_coding_block_size_minus3 + 3) : 0, 3);

    mpp_put_bits(&bp, dxva_cxt->pp.num_short_term_ref_pic_sets             , 7);
    mpp_put_bits(&bp, dxva_cxt->pp.long_term_ref_pics_present_flag         , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.num_long_term_ref_pics_sps              , 6);
    mpp_put_bits(&bp, dxva_cxt->pp.sps_temporal_mvp_enabled_flag           , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.strong_intra_smoothing_enabled_flag     , 1);
    ///<-zrh comment ^ 100 bit above

    mpp_put_bits(&bp, 0                                                    , 7 );
    mpp_put_align(&bp                                                         , 32, 0xf);

    // PPS
    mpp_put_bits(&bp, dxva_cxt->pp.pps_id                                    , 6 );
    mpp_put_bits(&bp, dxva_cxt->pp.sps_id                                    , 4 );
    mpp_put_bits(&bp, dxva_cxt->pp.dependent_slice_segments_enabled_flag     , 1 );
    mpp_put_bits(&bp, dxva_cxt->pp.output_flag_present_flag                  , 1 );
    mpp_put_bits(&bp, dxva_cxt->pp.num_extra_slice_header_bits               , 13);
    mpp_put_bits(&bp, dxva_cxt->pp.sign_data_hiding_enabled_flag , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.cabac_init_present_flag                   , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.num_ref_idx_l0_default_active_minus1 + 1  , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.num_ref_idx_l1_default_active_minus1 + 1  , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.init_qp_minus26                           , 7);
    mpp_put_bits(&bp, dxva_cxt->pp.constrained_intra_pred_flag               , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.transform_skip_enabled_flag               , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.cu_qp_delta_enabled_flag                  , 1);

    mpp_put_bits(&bp, log2_min_cb_size +
                 dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size -
                 dxva_cxt->pp.diff_cu_qp_delta_depth                             , 3);

    h265h_dbg(H265H_DBG_PPS, "log2_min_cb_size %d %d %d \n", log2_min_cb_size,
              dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size, dxva_cxt->pp.diff_cu_qp_delta_depth );

    mpp_put_bits(&bp, dxva_cxt->pp.pps_cb_qp_offset                            , 5);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_cr_qp_offset                            , 5);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_slice_chroma_qp_offsets_present_flag    , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.weighted_pred_flag                          , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.weighted_bipred_flag                        , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.transquant_bypass_enabled_flag              , 1 );
    mpp_put_bits(&bp, dxva_cxt->pp.tiles_enabled_flag                          , 1 );
    mpp_put_bits(&bp, dxva_cxt->pp.entropy_coding_sync_enabled_flag            , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_loop_filter_across_slices_enabled_flag  , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.loop_filter_across_tiles_enabled_flag       , 1);

    mpp_put_bits(&bp, dxva_cxt->pp.deblocking_filter_override_enabled_flag     , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_deblocking_filter_disabled_flag         , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_beta_offset_div2                        , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.pps_tc_offset_div2                          , 4);
    mpp_put_bits(&bp, dxva_cxt->pp.lists_modification_present_flag             , 1);
    mpp_put_bits(&bp, dxva_cxt->pp.log2_parallel_merge_level_minus2 + 2        , 3);
    mpp_put_bits(&bp, dxva_cxt->pp.slice_segment_header_extension_present_flag , 1);
    mpp_put_bits(&bp, 0                                                        , 3);
    mpp_put_bits(&bp, dxva_cxt->pp.num_tile_columns_minus1 + 1, 5);
    mpp_put_bits(&bp, dxva_cxt->pp.num_tile_rows_minus1 + 1 , 5 );
    mpp_put_bits(&bp, 3, 2); //mSps_Pps[i]->mMode
    mpp_put_align(&bp, 64, 0xf);

    {
        /// tiles info begin
        RK_U16 column_width[20];
        RK_U16 row_height[22];

        memset(column_width, 0, sizeof(column_width));
        memset(row_height, 0, sizeof(row_height));

        if (dxva_cxt->pp.tiles_enabled_flag) {
            if (dxva_cxt->pp.uniform_spacing_flag == 0) {
                RK_S32 maxcuwidth = dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size + log2_min_cb_size;
                RK_S32 ctu_width_in_pic = (width +
                                           (1 << maxcuwidth) - 1) / (1 << maxcuwidth) ;
                RK_S32 ctu_height_in_pic = (height +
                                            (1 << maxcuwidth) - 1) / (1 << maxcuwidth) ;
                RK_S32 sum = 0;
                for (i = 0; i < dxva_cxt->pp.num_tile_columns_minus1; i++) {
                    column_width[i] = dxva_cxt->pp.column_width_minus1[i] + 1;
                    sum += column_width[i]  ;
                }
                column_width[i] = ctu_width_in_pic - sum;

                sum = 0;
                for (i = 0; i < dxva_cxt->pp.num_tile_rows_minus1; i++) {
                    row_height[i] = dxva_cxt->pp.row_height_minus1[i] + 1;
                    sum += row_height[i];
                }
                row_height[i] = ctu_height_in_pic - sum;
            } // end of (pps->uniform_spacing_flag == 0)
            else {

                RK_S32    pic_in_cts_width = (width +
                                              (1 << (log2_min_cb_size +
                                                     dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size)) - 1)
                                             / (1 << (log2_min_cb_size +
                                                      dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size));
                RK_S32 pic_in_cts_height = (height +
                                            (1 << (log2_min_cb_size +
                                                   dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size)) - 1)
                                           / (1 << (log2_min_cb_size +
                                                    dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size));

                for (i = 0; i < dxva_cxt->pp.num_tile_columns_minus1 + 1; i++)
                    column_width[i] = ((i + 1) * pic_in_cts_width) / (dxva_cxt->pp.num_tile_columns_minus1 + 1) -
                                      (i * pic_in_cts_width) / (dxva_cxt->pp.num_tile_columns_minus1 + 1);

                for (i = 0; i < dxva_cxt->pp.num_tile_rows_minus1 + 1; i++)
                    row_height[i] = ((i + 1) * pic_in_cts_height) / (dxva_cxt->pp.num_tile_rows_minus1 + 1) -
                                    (i * pic_in_cts_height) / (dxva_cxt->pp.num_tile_rows_minus1 + 1);
            }
        } // pps->tiles_enabled_flag
        else {
            RK_S32 MaxCUWidth = (1 << (dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size + log2_min_cb_size));
            column_width[0] = (width  + MaxCUWidth - 1) / MaxCUWidth;
            row_height[0]   = (height + MaxCUWidth - 1) / MaxCUWidth;
        }

        for (j = 0; j < 20; j++) {
            if (column_width[j] > 0)
                column_width[j]--;
            mpp_put_bits(&bp, column_width[j], 8);
        }

        for (j = 0; j < 22; j++) {
            if (row_height[j] > 0)
                row_height[j]--;
            mpp_put_bits(&bp, row_height[j], 8);
        }
    }

    {
        RK_U8 *ptr_scaling = (RK_U8 *)mpp_buffer_get_ptr(reg_cxt->scaling_list_data);
        if (dxva_cxt->pp.scaling_list_data_present_flag) {
            addr = (dxva_cxt->pp.pps_id + 16) * 1360;
        } else if (dxva_cxt->pp.scaling_list_enabled_flag) {
            addr = dxva_cxt->pp.sps_id * 1360;
        } else {
            addr = 80 * 1360;
        }

        hal_h265d_output_scalinglist_packet(hal, ptr_scaling + addr, dxva);

        RK_U32 fd = mpp_buffer_get_fd(reg_cxt->scaling_list_data);
        /* need to config addr */
        addr = fd | (addr << 10);

        mpp_put_bits(&bp, addr, 32);
        mpp_put_align(&bp, 64, 0xf);
    }

    for (i = 0; i < 64; i++)
        memcpy(pps_ptr + i * 80, pps_packet, 80);

#ifdef dump
    fwrite(pps_ptr, 1, 80 * 64, fp);
    fflush(fp);
#endif

    MPP_FREE(pps_packet);
    return 0;
}

MPP_RET hal_h265d_gen_regs(void *hal,  HalTaskInfo *syn)
{
    RK_U32 uiMaxCUWidth, uiMaxCUHeight;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height, numCuInWidth;
    RK_S32 stride_y, stride_uv, virstrid_y, virstrid_yuv;
    RK_S32 i = 0;

    H265d_REGS_t *hw_regs;
    RK_S32 ret = MPP_SUCCESS;
    MppBuffer streambuf = NULL;
    RK_S32 aglin_offset = 0;
    RK_S32 valid_ref = -1;
    MppBuffer framebuf = NULL;

    if (syn->dec.flags.parse_err ||
        syn->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    h265d_dxva2_picture_context_t *dxva_cxt =
        (h265d_dxva2_picture_context_t *)syn->dec.syntax.data;
    h265d_reg_context_t *reg_cxt = ( h265d_reg_context_t *)hal;

    void *rps_ptr = NULL;
    if (reg_cxt ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_cxt->g_buf[i].use_flag) {
                syn->dec.reg_index = i;
                reg_cxt->rps_data = reg_cxt->g_buf[i].rps_data;
                reg_cxt->scaling_list_data =
                    reg_cxt->g_buf[i].scaling_list_data;
                reg_cxt->pps_data = reg_cxt->g_buf[i].pps_data;
                reg_cxt->hw_regs = reg_cxt->g_buf[i].hw_regs;
                reg_cxt->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("hevc rps buf all used");
            return MPP_ERR_NOMEM;
        }
    }
    rps_ptr = mpp_buffer_get_ptr(reg_cxt->rps_data);
    if (NULL == rps_ptr) {

        mpp_err("rps_data get ptr error");
        return MPP_ERR_NOMEM;
    }


    if (syn->dec.syntax.data == NULL) {
        mpp_err("%s:%s:%d dxva is NULL", __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

    /* output pps */
    hal_h265d_output_pps_packet(hal, syn->dec.syntax.data);

    if (NULL == reg_cxt->hw_regs) {
        return MPP_ERR_NULL_PTR;
    }

    hw_regs = (H265d_REGS_t*)reg_cxt->hw_regs;
    memset(hw_regs, 0, sizeof(H265d_REGS_t));

    uiMaxCUWidth = 1 << (dxva_cxt->pp.log2_diff_max_min_luma_coding_block_size
                         + dxva_cxt->pp.log2_min_luma_coding_block_size_minus3
                         + 3);
    uiMaxCUHeight = uiMaxCUWidth;

    log2_min_cb_size = dxva_cxt->pp.log2_min_luma_coding_block_size_minus3 + 3;

    width = (dxva_cxt->pp.PicWidthInMinCbsY << log2_min_cb_size);

    height = (dxva_cxt->pp.PicHeightInMinCbsY << log2_min_cb_size);

    numCuInWidth   = width / uiMaxCUWidth  + (width % uiMaxCUWidth != 0);

    stride_y = (((numCuInWidth * uiMaxCUWidth
                  * (dxva_cxt->pp.bit_depth_luma_minus8 + 8) + 7)
                 & (~7)) >> 3);
    stride_uv = (((numCuInWidth * uiMaxCUHeight
                   * (dxva_cxt->pp.bit_depth_chroma_minus8 + 8) + 7)
                  & (~7)) >> 3);

    stride_y = hevc_hor_align(stride_y);
    stride_uv = hevc_hor_align(stride_uv);
    virstrid_y = hevc_ver_align(height) * stride_y;
    virstrid_yuv  = virstrid_y + stride_uv * hevc_ver_align(height) / 2;

    hw_regs->sw_picparameter.sw_slice_num = dxva_cxt->slice_count;
    hw_regs->sw_picparameter.sw_y_hor_virstride
        = stride_y >> 4;
    hw_regs->sw_picparameter.sw_uv_hor_virstride
        = stride_uv >> 4;
    hw_regs->sw_y_virstride
        = virstrid_y >> 4;
    hw_regs->sw_yuv_virstride
        = virstrid_yuv >> 4;

    mpp_buf_slot_get_prop(reg_cxt->slots, dxva_cxt->pp.CurrPic.Index7Bits,
                          SLOT_BUFFER, &framebuf);
    hw_regs->sw_decout_base  = mpp_buffer_get_fd(framebuf); //just index need map

    /*if out_base is equal to zero it means this frame may error
    we return directly add by csy*/

    if (hw_regs->sw_decout_base == 0) {
        return 0;
    }

    hw_regs->sw_cur_poc = dxva_cxt->pp.CurrPicOrderCntVal;

    mpp_buf_slot_get_prop(reg_cxt->packet_slots, syn->dec.input, SLOT_BUFFER,
                          &streambuf);
    if ( dxva_cxt->bitstream == NULL) {
        dxva_cxt->bitstream = mpp_buffer_get_ptr(streambuf);
    }

    hal_h265d_slice_output_rps(syn->dec.syntax.data, rps_ptr);

    hw_regs->sw_cabactbl_base   =  mpp_buffer_get_fd(reg_cxt->cabac_table_data);
    hw_regs->sw_pps_base        =  mpp_buffer_get_fd(reg_cxt->pps_data);
    hw_regs->sw_rps_base        =  mpp_buffer_get_fd(reg_cxt->rps_data);
    hw_regs->sw_strm_rlc_base   =  mpp_buffer_get_fd(streambuf);
    hw_regs->sw_stream_len      = ((dxva_cxt->bitstream_size + 15)
                                   & (~15)) + 64;
    aglin_offset =  hw_regs->sw_stream_len - dxva_cxt->bitstream_size;
    if (aglin_offset > 0) {
        memset((void *)(dxva_cxt->bitstream + dxva_cxt->bitstream_size), 0,
               aglin_offset);
    }
    hw_regs->sw_interrupt.sw_dec_e         = 1;
    hw_regs->sw_interrupt.sw_dec_timeout_e = 1;
    hw_regs->sw_interrupt.sw_wr_ddr_align_en = dxva_cxt->pp.tiles_enabled_flag
                                               ? 0 : 1;


    ///find s->rps_model[i] position, and set register
    hw_regs->sw_ref_valid = 0;

    hw_regs->cabac_error_en = 0xfdfffffd;
    hw_regs->extern_error_en = 0x30000000;

    valid_ref = hw_regs->sw_decout_base;
    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_cxt->pp.RefPicList); i++) {
        if (dxva_cxt->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_cxt->pp.RefPicList[i].bPicEntry != 0x7f) {
            hw_regs->sw_refer_poc[i] = dxva_cxt->pp.PicOrderCntValList[i];
            mpp_buf_slot_get_prop(reg_cxt->slots,
                                  dxva_cxt->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);
            if (framebuf != NULL) {
                hw_regs->sw_refer_base[i] = mpp_buffer_get_fd(framebuf);
                valid_ref = hw_regs->sw_refer_base[i];
            } else {
                hw_regs->sw_refer_base[i] = valid_ref;
            }
            hw_regs->sw_ref_valid          |=   (1 << i);
        } else {
            hw_regs->sw_refer_base[i] = hw_regs->sw_decout_base;
        }
    }

    hw_regs->sw_refer_base[0] |= ((hw_regs->sw_ref_valid & 0xf) << 10);
    hw_regs->sw_refer_base[1] |= (((hw_regs->sw_ref_valid >> 4) & 0xf) << 10);
    hw_regs->sw_refer_base[2] |= (((hw_regs->sw_ref_valid >> 8) & 0xf) << 10);
    hw_regs->sw_refer_base[3] |= (((hw_regs->sw_ref_valid >> 12) & 0x7) << 10);

    return ret;
}

MPP_RET hal_h265d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U8* p = NULL;
    H265d_REGS_t *hw_regs = NULL;
    h265d_reg_context_t *reg_cxt = (h265d_reg_context_t *)hal;
    RK_S32 index =  task->dec.reg_index;

    RK_S32 i;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    if (reg_cxt->fast_mode) {
        p = (RK_U8*)reg_cxt->g_buf[index].hw_regs;
        hw_regs = ( H265d_REGS_t *)reg_cxt->g_buf[index].hw_regs;
    } else {
        p = (RK_U8*)reg_cxt->hw_regs;
        hw_regs = ( H265d_REGS_t *)reg_cxt->hw_regs;
    }

    if (hw_regs == NULL) {
        mpp_err("hal_h265d_start hw_regs is NULL");
        return MPP_ERR_NULL_PTR;
    }
    for (i = 0; i < 68; i++) {
        h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                  i, *((RK_U32*)p));
        //mpp_log("RK_HEVC_DEC: regs[%02d]=%08X\n", i, *((RK_U32*)p));
        p += 4;
    }

    // 68 is the nb of uint32_t
    ret = mpp_device_send_reg(reg_cxt->dev_ctx, (RK_U32*)hw_regs, RKVDEC_HEVC_REGISTERS);
    if (ret) {
        mpp_err("RK_HEVC_DEC: ERROR: mpp_device_send_reg Failed!!!\n");
        return MPP_ERR_VPUHW;
    }

    return ret;
}


MPP_RET hal_h265d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_S32 index =  task->dec.reg_index;
    h265d_reg_context_t *reg_cxt = (h265d_reg_context_t *)hal;
    RK_U8* p = NULL;
    H265d_REGS_t *hw_regs = NULL;
    RK_S32 i;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        goto ERR_PROC;
    }

    if (reg_cxt->fast_mode) {
        hw_regs = ( H265d_REGS_t *)reg_cxt->g_buf[index].hw_regs;
    } else {
        hw_regs = ( H265d_REGS_t *)reg_cxt->hw_regs;
    }

    p = (RK_U8*)hw_regs;

    ret = mpp_device_wait_reg(reg_cxt->dev_ctx, (RK_U32*)hw_regs, RKVDEC_HEVC_REGISTERS);
ERR_PROC:
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        hw_regs->sw_interrupt.sw_dec_error_sta ||
        hw_regs->sw_interrupt.sw_dec_empty_sta) {
        if (!reg_cxt->fast_mode) {
            if (reg_cxt->int_cb.callBack)
                reg_cxt->int_cb.callBack(reg_cxt->int_cb.opaque, &task->dec);
        } else {
            MppFrame mframe = NULL;
            mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.output,
                                  SLOT_FRAME_PTR, &mframe);
            if (mframe) {
                reg_cxt->fast_mode_err_found = 1;
                mpp_frame_set_errinfo(mframe, 1);
            }
        }
    } else {
        if (reg_cxt->fast_mode && reg_cxt->fast_mode_err_found) {
            for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(task->dec.refer); i++) {
                if (task->dec.refer[i] >= 0) {
                    MppFrame frame_ref = NULL;

                    mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.refer[i],
                                          SLOT_FRAME_PTR, &frame_ref);
                    h265h_dbg(H265H_DBG_FAST_ERR, "refer[%d] %d frame %p\n",
                              i, task->dec.refer[i], frame_ref);
                    if (frame_ref && mpp_frame_get_errinfo(frame_ref)) {
                        MppFrame frame_out = NULL;
                        mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.output,
                                              SLOT_FRAME_PTR, &frame_out);
                        mpp_frame_set_errinfo(frame_out, 1);
                        break;
                    }
                }
            }
        }
    }

    for (i = 0; i < 68; i++) {
        if (i == 1) {
            h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                      i, *((RK_U32*)p));
        }

        if (i == 45) {
            h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                      i, *((RK_U32*)p));
        }
        p += 4;
    }

    if (reg_cxt->fast_mode) {
        reg_cxt->g_buf[index].use_flag = 0;
    }

    return ret;
}


MPP_RET hal_h265d_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    h265d_reg_context_t *p_hal = (h265d_reg_context_t *)hal;
    p_hal->fast_mode_err_found = 0;
    (void)hal;
    return ret;
}

MPP_RET hal_h265d_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

MPP_RET hal_h265d_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    (void)cmd_type;
    (void)param;
    return  ret;
}

const MppHalApi hal_api_h265d = {
    .name = "h265d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(h265d_reg_context_t),
    .flag = 0,
    .init = hal_h265d_init,
    .deinit = hal_h265d_deinit,
    .reg_gen = hal_h265d_gen_regs,
    .start = hal_h265d_start,
    .wait = hal_h265d_wait,
    .reset = hal_h265d_reset,
    .flush = hal_h265d_flush,
    .control = hal_h265d_control,
};


