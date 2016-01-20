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

#define MODULE_TAG "hal_h264d_vdpu_reg"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "vpu.h"

#include "h264d_log.h"
#include "hal_regdrv.h"
#include "hal_h264d_global.h"
#include "hal_h264d_api.h"
#include "hal_h264d_vdpu_pkt.h"
#include "hal_h264d_vdpu_reg.h"



static void print_single_regs(HalRegDrvCtx_t *p_drv, const HalRegDrv_t *pcur)
{
    RK_S32 val = 0;
    hal_get_regdrv(p_drv, pcur->syn_id, (RK_U32 *)&val);
    LogInfo((LogCtx_t *)p_drv->log, "name=[%24s], reg=[%.2d], bit=[%.2d], len=[%.2d], val=[%8d]",
            pcur->name, pcur->reg_id, pcur->bitpos, pcur->bitlen, val);
}


RK_U32 g_print_init_value = 1;

static MPP_RET vdpu_h264d_print_regs(H264dHalCtx_t *p_hal, HalRegDrvCtx_t *p_drv)
{
    RK_U32 i = 0;
    //RK_U32 val = 0;
    LogCtx_t *logctx = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, NULL == p_drv);
    logctx = (LogCtx_t *)p_drv->log;
    if (LogEnable(logctx, LOG_LEVEL_INFO)) {
#if 1
        if (g_print_init_value) {
            g_print_init_value = 0;
            LogInfo((LogCtx_t *)p_drv->log, "---------------- VDPU REG START ------------------ ");
            //!< ---------------- init register -------------
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_MODE         ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUT_ENDIAN   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_IN_ENDIAN    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_STRENDIAN_E  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUT_TILED_E  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_MAX_BURST    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_SCMD_DIS     ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_ADV_PRE_DIS  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_APF_THRESHOLD    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_LATENCY      ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_DATA_DISC_E  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUTSWAP32_E  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_INSWAP32_E   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_STRSWAP32_E  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_TIMEOUT_E    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_CLK_GATE_E   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_IRQ_DIS      ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_AXI_RD_ID    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_AXI_WR_ID    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PRED_BC_TAP_0_0  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PRED_BC_TAP_0_1  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PRED_BC_TAP_0_2  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_LATENCY      ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_CLK_GATE_E   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUT_TILED_E  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUT_ENDIAN   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_MAX_BURST    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_DATA_DISC_E  ]);
            //!< picture parameter
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PIC_MB_WIDTH     ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PIC_MB_HEIGHT_P  ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_QTABLE_BASE      ]);
        }
#endif

#if 1
        {
            LogInfo(logctx, "[SET_REG] g_framecnt=%d, ---- DECODE BEING -------", p_hal->iDecodedNum);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUT_DIS      ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_RLC_MODE_E       ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_INIT_QP          ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_REFIDX0_ACTIVE   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_REF_FRAMES       ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_FRAMENUM_LEN     ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_FRAMENUM         ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_CONST_INTRA_E    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_FILT_CTRL_PRES   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_RDPIC_CNT_PRES   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_REFPIC_MK_LEN    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_IDR_PIC_E        ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_IDR_PIC_ID       ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PPS_ID           ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_POC_LENGTH       ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_REFER_LTERM_E    ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_REFER_VALID_E    ]);
            for (i = 0; i < 16; i++) {
                if (p_hal->pp->RefFrameList[i].bPicEntry != 0xff) { //!< valid
                    print_single_regs(p_drv, &p_drv->p_emt[g_refPicNum[i]]);
                }
            }
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PICORD_COUNT_E   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_CABAC_E          ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_START_CODE_E     ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_STRM_START_BIT   ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_RLC_VLC_BASE     ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_STREAM_LEN       ]);
            /* P lists */
            for (i = 0; i < MPP_ARRAY_ELEMS(g_refPicListP); i++) {
                print_single_regs(p_drv, &p_drv->p_emt[g_refPicListP[i]]);
            }
            /* B lists */
            for (i = 0; i < MPP_ARRAY_ELEMS(g_refPicList0); i++) {
                print_single_regs(p_drv, &p_drv->p_emt[g_refPicList0[i]]);
                print_single_regs(p_drv, &p_drv->p_emt[g_refPicList1[i]]);
            }
            /* reference based address */
            for (i = 0; i < MPP_ARRAY_ELEMS(g_refBase); i++) {
                print_single_regs(p_drv, &p_drv->p_emt[g_refBase[i]]);
            }
            /* inter-view reference picture */
            if (p_hal->pp->curr_layer_id && p_hal->pp->inter_view_flag) {

                print_single_regs(p_drv, &p_drv->p_emt[VDPU_INTER_VIEW_BASE]);
                print_single_regs(p_drv, &p_drv->p_emt[VDPU_REFER_VALID_E]);
            }

            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PIC_FIXED_QUANT]); //!< VDPU_MVC_E
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_FILTERING_DIS]);   //!< filterDisable = 0;
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUT_BASE]);    //!< outPhyAddr, pp->CurrPic.Index7Bits

            print_single_regs(p_drv, &p_drv->p_emt[VDPU_CH_QP_OFFSET]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_CH_QP_OFFSET2]);

            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DIR_MV_BASE]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_WRITE_MVS_E]); //!< defalut set 1
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DIR_8X8_INFER_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_WEIGHT_PRED_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_WEIGHT_BIPR_IDC]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_REFIDX1_ACTIVE]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_FIELDPIC_FLAG_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PIC_INTERLACE_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PIC_FIELDMODE_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_PIC_TOPFIELD_E]); //!< bottomFieldFlag
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_SEQ_MBAFF_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_8X8TRANS_FLAG_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_BLACKWHITE_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_TYPE1_QUANT_E]);

            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_OUT_DIS]); //!< set defalut 0
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_CH_8PIX_ILEAV_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_E]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_QTABLE_BASE]);
            LogInfo((LogCtx_t *)p_drv->log, "--------- [FLUSH_CNT] flush framecnt=%d -------- \n", p_hal->iDecodedNum);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_IRQ_STAT]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_IRQ]);
            print_single_regs(p_drv, &p_drv->p_emt[VDPU_DEC_PIC_INF]);
        }
#endif
    }
    (void)i;
    (void)p_hal;
__RETURN:
    return ret = MPP_OK;
}


static MPP_RET vdpu_set_device_regs(HalRegDrvCtx_t *p_drv)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 apfTmpThreshold = 0;
    INP_CHECK(ret, NULL == p_drv);

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_MODE,        DEC_X170_MODE_H264));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUT_ENDIAN,  DEC_X170_OUTPUT_PICTURE_ENDIAN));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_IN_ENDIAN,   DEC_X170_INPUT_DATA_ENDIAN));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_STRENDIAN_E, DEC_X170_INPUT_STREAM_ENDIAN));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUT_TILED_E, DEC_X170_OUTPUT_FORMAT));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_MAX_BURST,   DEC_X170_BUS_BURST_LENGTH));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_SCMD_DIS,    DEC_X170_SCMD_DISABLE));
    //!< set APF_THRESHOLD
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_ADV_PRE_DIS, DEC_X170_APF_DISABLE));
    if ((DEC_X170_APF_DISABLE) == 0) {
        if (DEC_X170_REFBU_SEQ) {
            apfTmpThreshold = DEC_X170_REFBU_NONSEQ / DEC_X170_REFBU_SEQ;
        } else {
            apfTmpThreshold = DEC_X170_REFBU_NONSEQ;
            if (apfTmpThreshold > 63) {
                apfTmpThreshold = 63;
            }
        }
    }
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_APF_THRESHOLD,   apfTmpThreshold));
    //!< set device configuration register
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_LATENCY,     DEC_X170_LATENCY_COMPENSATION));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_DATA_DISC_E, DEC_X170_DATA_DISCARD_ENABLE));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUTSWAP32_E, DEC_X170_OUTPUT_SWAP_32_ENABLE));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_INSWAP32_E,  DEC_X170_INPUT_DATA_SWAP_32_ENABLE));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_STRSWAP32_E, DEC_X170_INPUT_STREAM_SWAP_32_ENABLE));

#if( DEC_X170_HW_TIMEOUT_INT_ENA  != 0)
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_TIMEOUT_E, 1));
#else
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_TIMEOUT_E, 0));
#endif

#if( DEC_X170_INTERNAL_CLOCK_GATING != 0)
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_CLK_GATE_E, 1));
#else
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_CLK_GATE_E, 0));
#endif

#if( DEC_X170_USING_IRQ  == 0)
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_IRQ_DIS, 1));
#else
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_IRQ_DIS, 0));
#endif
    //!< set AXI RW IDs
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_AXI_RD_ID,   (DEC_X170_AXI_ID_R & 0xFFU)));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_AXI_WR_ID,   (DEC_X170_AXI_ID_W & 0xFFU)));

    hal_get_regdrv(p_drv, VDPU_DEC_AXI_WR_ID, &apfTmpThreshold);
    //mpp_log_f("[SET_REG]:DEC_X170_AXI_ID_W=%d, set_value=%d \n", DEC_X170_AXI_ID_W, apfTmpThreshold);


    ///!< Set prediction filter taps
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PRED_BC_TAP_0_0, 1));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PRED_BC_TAP_0_1, (RK_U32)(-5)));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_PRED_BC_TAP_0_2, 20));


    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_LATENCY,      DEC_X170_LATENCY_COMPENSATION));
    //!< clock_gating  0:clock always on, 1: clock gating module control the key(turn off when decoder free)
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_CLK_GATE_E,   DEC_X170_INTERNAL_CLOCK_GATING));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUT_TILED_E,  DEC_X170_OUTPUT_FORMAT));
    //!<output_picture_endian = 1 ( 1:little endian; 0:big endian )
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_OUT_ENDIAN,   DEC_X170_OUTPUT_PICTURE_ENDIAN));
    //!< bus_burst_length = 16, bus burst
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_MAX_BURST,    DEC_X170_BUS_BURST_LENGTH));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_DATA_DISC_E,  DEC_X170_DATA_DISCARD_ENABLE));

__RETURN:
    return MPP_OK;
__FAILED:
    return ret;
}







static RK_U32 vdpu_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static RK_U32 vdpu_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}
#if 0
static MPP_RET vdpu_adjust_input(H264dHalCtx_t *p_hal)
{
    RK_U32 i = 0, j = 0;
    RK_U32 use_flag = 0, find_flag = 0;
    H264dHalOldDXVA_t *priv = (H264dHalOldDXVA_t *)p_hal->priv;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    H264dVdpuDpb_t *new_dpb = priv->new_dpb;
    H264dVdpuDpb_t *old_dpb = priv->old_dpb[pp->curr_layer_id];
    H264dVdpuDpb_t *ilt_dpb = priv->ilt_dpb;


    assert(pp->curr_layer_id == 0);

    //!< change input dxva into pdb structure
    priv->new_dpb_cnt = 0;
    priv->ilt_dpb_cnt = 0;
    memset(new_dpb, 0, 16 * sizeof(H264dVdpuDpb_t));
    memset(ilt_dpb, 0, 16 * sizeof(H264dVdpuDpb_t));
    for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
        if (pp->RefFrameList[i].bPicEntry != 0xff) {
            new_dpb[i].is_long_term = pp->RefFrameList[i].AssociatedFlag;
            new_dpb[i].slot_index = pp->RefFrameList[i].Index7Bits;

            new_dpb[i].TOP_POC = pp->FieldOrderCntList[i][0];
            new_dpb[i].BOT_POC = pp->FieldOrderCntList[i][1];
            new_dpb[i].non_exist_frame = (pp->NonExistingFrameFlags >> i) & 0x1;
            new_dpb[i].is_ilt = (pp->UsedForInTerviewflags >> i) & 0x1;

            new_dpb[i].frame_num = pp->FrameNumList[i];
            new_dpb[i].LongTermPicNum = pp->LongTermPicNumList[i];

            use_flag = pp->UsedForReferenceFlags >> (2 * i);
            if (use_flag & 0x1) {
                new_dpb[i].top_used = 1;
            }
            if (use_flag & 0x2) {
                new_dpb[i].bot_used = 1;
            }
            //!< inter-view frame flag
            if (new_dpb[i].is_ilt) {
                assert(0);
                ilt_dpb[priv->ilt_dpb_cnt] = new_dpb[i];
                ilt_dpb[priv->ilt_dpb_cnt].valid = 1;
                priv->ilt_dpb_cnt++;
            } else {
                new_dpb[i].valid = 1;
                priv->new_dpb_cnt++;
            }
        }
    }
    if (p_hal->iDecodedNum == 3) {
        i = i;
    }

    //for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
    //  if (new_dpb[i].valid) {
    //      FPRINT(g_debug_file0, "i=%2d, picnum=%d, framenum=%2d, ", i, new_dpb[i].frame_num,  new_dpb[i].frame_num);
    //      FPRINT(g_debug_file0, " dbp_idx=%d, longterm=%d, poc=%d \n", new_dpb[i].slot_index, new_dpb[i].is_long_term,
    //          new_dpb[i].TOP_POC);
    //  }
    //}
    //FPRINT(g_debug_file0, "--------- [new DPB] -------- \n");
    //for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
    //  if (old_dpb[i].valid) {
    //      FPRINT(g_debug_file0, "i=%2d, picnum=%d, framenum=%2d, ", i, old_dpb[i].frame_num,  old_dpb[i].frame_num);
    //      FPRINT(g_debug_file0, " dbp_idx=%d, longterm=%d, poc=%d \n", old_dpb[i].slot_index, old_dpb[i].is_long_term,
    //          old_dpb[i].TOP_POC);
    //  }
    //}
    //FPRINT(g_debug_file0, "--------- [Old DPB] -------- \n");

    //!< delete old dpb
    for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
        find_flag = 0;

        if (old_dpb[i].valid) {
            for (j = 0; j < MPP_ARRAY_ELEMS(pp->RefFrameList); j++) {
                if (new_dpb[j].valid) {
                    find_flag = (old_dpb[i].frame_num == new_dpb[j].frame_num) ? 1 : 0;
                    if (new_dpb[j].top_used) {
                        find_flag = (old_dpb[i].TOP_POC == new_dpb[j].TOP_POC) ? find_flag : 0;
                    }
                    if (new_dpb[j].bot_used) {
                        find_flag = (old_dpb[i].BOT_POC == new_dpb[j].BOT_POC) ? find_flag : 0;
                    }
                    if (find_flag) { //!< found
                        new_dpb[j].have_same = 1;
                        break;
                    }
                }
            }
        }
        //!< not found
        if (find_flag == 0) {
            memset(&old_dpb[i], 0, sizeof(H264dVdpuDpb_t));
        }
    }
    //!< add new dpb
    for (j = 0; j < MPP_ARRAY_ELEMS(pp->RefFrameList); j++) {
        if ((new_dpb[j].valid == 0) || new_dpb[j].have_same) {
            continue;
        }
        for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
            if (old_dpb[i].valid == 0) {
                old_dpb[i] = new_dpb[j];
                break;
            }
        }
    }
    //!< re-fill reference dxva syntax
    pp->UsedForReferenceFlags = 0;
    pp->NonExistingFrameFlags = 0;
    memset(pp->RefFrameList, 0xff, sizeof(pp->RefFrameList));
    memset(pp->FieldOrderCntList, 0, sizeof(pp->FieldOrderCntList));
    memset(pp->LongTermPicNumList, 0, sizeof(pp->LongTermPicNumList));
    memset(pp->FrameNumList, 0, sizeof(pp->FrameNumList));

    for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
        if (old_dpb[i].valid) {
            pp->RefFrameList[i].Index7Bits = old_dpb[i].slot_index;
            pp->RefFrameList[i].AssociatedFlag = old_dpb[i].is_long_term;

            pp->FieldOrderCntList[i][0] = old_dpb[i].TOP_POC;
            pp->FieldOrderCntList[i][1] = old_dpb[i].BOT_POC;
            pp->FrameNumList[i] = old_dpb[i].frame_num;
            pp->LongTermPicNumList[i] = old_dpb[i].LongTermPicNum;
            pp->NonExistingFrameFlags |= old_dpb[i].non_exist_frame ? (1 << i) : 0;
            if (old_dpb[i].top_used) { //!< top_field
                pp->UsedForReferenceFlags |= 1 << (2 * i + 0);
            }
            //if (old_dpb[i].bot_used) { //!< bot_field
            //    pp->UsedForReferenceFlags |= 1 << (2 * i + 1);
            //}
        } else {
            pp->RefFrameList[i].bPicEntry = 0xff;
            pp->FieldOrderCntList[i][0] = 0;
            pp->FieldOrderCntList[i][1] = 0;
            pp->FrameNumList[i] = 0;
        }
    }

    //for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
    //  if (old_dpb[i].valid) {
    //      FPRINT(g_debug_file0, "i=%2d, picnum=%d, framenum=%2d, ", i, old_dpb[i].frame_num,  old_dpb[i].frame_num);
    //      FPRINT(g_debug_file0, " dbp_idx=%d, longterm=%d, poc=%d \n", old_dpb[i].slot_index, old_dpb[i].is_long_term,
    //          old_dpb[i].TOP_POC);
    //  }
    //}


    return MPP_OK;
}
#endif
/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_init(void *hal, MppHalCfg *cfg)
{
    RK_U32 i = 0;
    RK_U32 cabac_size = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    HalRegDrvCtx_t *p_drv = NULL;

    INP_CHECK(ret, NULL == hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    MEM_CHECK(ret, p_hal->priv = mpp_calloc_size(void, sizeof(H264dHalOldDXVA_t)));
    //!< malloc init registers
    MEM_CHECK(ret, p_hal->regs = mpp_calloc_size(void, sizeof(HalRegDrvCtx_t)));
    p_drv = (HalRegDrvCtx_t *)p_hal->regs;
    p_drv->type     = cfg->type;
    p_drv->coding   = cfg->coding;
    p_drv->emt_size = VDPU_MAX_SIZE;
    p_drv->log      = (void *)p_hal->logctx.parr[LOG_WRITE_REG];

    MEM_CHECK(ret, p_drv->p_emt = mpp_calloc(HalRegDrv_t, p_drv->emt_size + 1));
    for (i = 0; i < MPP_ARRAY_ELEMS(g_vdpu_drv); i++) {

        p_drv->p_emt[g_vdpu_drv[i].syn_id] = g_vdpu_drv[i];
        if (VDPU_MAX_SIZE == g_vdpu_drv[i].syn_id) {
            break;
        }
    }
    p_drv->reg_size = p_drv->p_emt[VDPU_MAX_SIZE].reg_id;
    MEM_CHECK(ret, p_drv->p_reg = mpp_calloc(RK_U32, p_drv->reg_size));
    //!< malloc cabac+scanlis + packets + poc_buf
    cabac_size = VDPU_CABAC_TAB_SIZE + VDPU_SCALING_LIST_SIZE + VDPU_POC_BUF_SIZE;
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &p_hal->cabac_buf, cabac_size));
    //!< copy cabac table bytes
    FUN_CHECK(ret = mpp_buffer_write(p_hal->cabac_buf, 0, (void *)H264_VDPU_Cabac_table, sizeof(H264_VDPU_Cabac_table)));
    //!< setting device register
    vdpu_set_device_regs(p_drv);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, vdpu_hor_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, vdpu_ver_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, NULL);

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)cfg;
__RETURN:
    return MPP_OK;
__FAILED:
    vdpu_h264d_deinit(hal);

    return ret;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_deinit(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    HalRegDrvCtx_t *p_drv = (HalRegDrvCtx_t *)p_hal->regs;
    INP_CHECK(ret, NULL == p_hal);

    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    MPP_FREE(p_hal->priv);
    if (p_hal->regs) {
        MPP_FREE(p_drv->p_reg);
        MPP_FREE(p_drv->p_emt);
        MPP_FREE(p_hal->regs);
    }
    if (p_hal->cabac_buf) {
        FUN_CHECK(ret = mpp_buffer_put(p_hal->cabac_buf));
    }


    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    HalRegDrvCtx_t *p_drv = (HalRegDrvCtx_t *)p_hal->regs;

    INP_CHECK(ret, NULL == p_hal);

    p_hal->in_task = &task->dec;
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    LogTrace(p_hal->logctx.parr[RUN_HAL], "[Generate register begin]");

    //FUN_CHECK(ret = vdpu_adjust_input(p_hal));
    FUN_CHECK(ret = vdpu_set_pic_regs(hal, p_drv));
    FUN_CHECK(ret = vdpu_set_vlc_regs(hal, p_drv));
    FUN_CHECK(ret = vdpu_set_ref_regs(hal, p_drv));
    FUN_CHECK(ret = vdpu_set_asic_regs(hal, p_drv));

    LogTrace(p_hal->logctx.parr[RUN_HAL], "[Generate register end]");
    p_hal->in_task->valid = 0;
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:

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
MPP_RET vdpu_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal  = (H264dHalCtx_t *)hal;
    HalRegDrvCtx_t *p_drv = (HalRegDrvCtx_t *)p_hal->regs;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_QTABLE_BASE,  mpp_buffer_get_fd(p_hal->cabac_buf))); //!< cabacInit.phy_addr
    p_drv->p_reg[57] |= 0xDE;

    FUN_CHECK(ret = vdpu_h264d_print_regs(p_hal, p_drv));
#ifdef ANDROID
    if (VPUClientSendReg(p_hal->vpu_socket, p_drv->p_reg, DEC_X170_REGISTERS)) {
        ret =  MPP_ERR_VPUHW;
        mpp_err_f("H264 VDPU FlushRegs fail. \n");
    } else {
        mpp_log("H264 VDPU FlushRegs success, pid=%d, hal_frame_no=%d. \n", getpid(), p_hal->iDecodedNum);
    }
#endif

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)task;
__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    HalRegDrvCtx_t *p_drv = (HalRegDrvCtx_t *)p_hal->regs;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
#ifdef ANDROID
    RK_S32 wait_ret = -1;
    RK_S32 ret_len = 0, cur_deat = 0;
    VPU_CMD_TYPE ret_cmd = VPU_CMD_BUTT;
    static struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    wait_ret = VPUClientWaitResult(p_hal->vpu_socket, p_drv->p_reg, DEC_X170_REGISTERS, &ret_cmd, &ret_len);
    gettimeofday(&tv2, NULL);
    cur_deat = (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000;
    p_hal->total_time += cur_deat;
    p_hal->iDecodedNum++;
    (void)wait_ret;
#endif

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_IRQ_STAT,    0));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_IRQ,         0));
    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_DEC_PIC_INF,     0));

    //FPRINT(g_debug_file1, "++++++++++ hal_h264_decoder, g_framecnt=%d \n", task->dec.g_framecnt);

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)task;
__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_reset(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    H264dHalOldDXVA_t *priv = NULL;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    priv = (H264dHalOldDXVA_t *)p_hal->priv;
    memset(priv->old_dpb, 0, sizeof(priv->old_dpb));

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_flush(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)hal;
    (void)cmd_type;
    (void)param;
__RETURN:
    return ret = MPP_OK;
}
