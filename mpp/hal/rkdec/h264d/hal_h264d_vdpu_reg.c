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

static MPP_RET vdpu_h264d_print_regs(H264dHalCtx_t *p_hal, HalRegDrvCtx_t *p_drv)
{
    RK_U32 i = 0;
    //RK_U32 val = 0;
    LogCtx_t *logctx = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    //const HalRegDrv_t *pcur = NULL;

    INP_CHECK(ret, NULL == p_drv);

    logctx = (LogCtx_t *)p_drv->log;
    if (LogEnable(logctx, LOG_LEVEL_INFO)) {
#if 0
        LogInfo(logctx, "---------------- VDPU REG START ------------------ ");
        for (i = 0; i < MPP_ARRAY_ELEMS(g_vdpu_drv); i++) {
            pcur = &g_vdpu_drv[i];
            if (p_drv->syn_size == pcur->syn_id) {
                break;
            }
            FUN_CHECK(ret = hal_get_regdrv(p_drv, pcur->syn_id, &val));
            LogInfo(logctx, "[SET_REG]: name=[%24s], id=[%.3d], reg=[%.2d], bit=[%.2d], len=[%.2d], val=[%8d] ",
                    pcur->name, pcur->syn_id, pcur->reg_id, pcur->bitpos, pcur->bitlen, val);
        }
#else

#if 1
        LogInfo(logctx, "[SET_REG] ---- DECODE BEING -------");
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUT_DIS      ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_RLC_MODE_E       ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_INIT_QP          ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_REFIDX0_ACTIVE   ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_REF_FRAMES       ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_FRAMENUM_LEN     ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_FRAMENUM         ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_CONST_INTRA_E    ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_FILT_CTRL_PRES   ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_RDPIC_CNT_PRES   ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_REFPIC_MK_LEN    ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_IDR_PIC_E        ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_IDR_PIC_ID       ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_PPS_ID           ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_POC_LENGTH       ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_REFER_LTERM_E    ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_REFER_VALID_E    ]);
        for (i = 0; i < 16; i++) {
            if (p_hal->pp->RefFrameList[i].bPicEntry != 0xff) { //!< valid
                print_single_regs(p_drv, &p_drv->p_syn[g_refPicNum[i]]);
            }
        }
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_PICORD_COUNT_E   ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_CABAC_E          ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_START_CODE_E     ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_STRM_START_BIT   ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_RLC_VLC_BASE     ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_STREAM_LEN       ]);
        /* P lists */
        for (i = 0; i < MPP_ARRAY_ELEMS(g_refPicListP); i++) {
            print_single_regs(p_drv, &p_drv->p_syn[g_refPicListP[i]]);
        }
        /* B lists */
        for (i = 0; i < MPP_ARRAY_ELEMS(g_refPicList0); i++) {
            print_single_regs(p_drv, &p_drv->p_syn[g_refPicList0[i]]);
            print_single_regs(p_drv, &p_drv->p_syn[g_refPicList1[i]]);
        }
        /* reference based address */
        for (i = 0; i < MPP_ARRAY_ELEMS(g_refBase); i++) {
            print_single_regs(p_drv, &p_drv->p_syn[g_refBase[i]]);
        }
        /* inter-view reference picture */
        if (p_hal->pp->curr_layer_id && p_hal->pp->inter_view_flag) {

            print_single_regs(p_drv, &p_drv->p_syn[VDPU_INTER_VIEW_BASE]);
            print_single_regs(p_drv, &p_drv->p_syn[VDPU_REFER_VALID_E]);
        }

        print_single_regs(p_drv, &p_drv->p_syn[VDPU_PIC_FIXED_QUANT]); //!< VDPU_MVC_E
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_FILTERING_DIS]);   //!< filterDisable = 0;
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUT_BASE]);    //!< outPhyAddr, pp->CurrPic.Index7Bits

        print_single_regs(p_drv, &p_drv->p_syn[VDPU_CH_QP_OFFSET]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_CH_QP_OFFSET2]);

        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DIR_MV_BASE]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_WRITE_MVS_E]); //!< defalut set 1
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DIR_8X8_INFER_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_WEIGHT_PRED_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_WEIGHT_BIPR_IDC]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_REFIDX1_ACTIVE]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_FIELDPIC_FLAG_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_PIC_INTERLACE_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_PIC_FIELDMODE_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_PIC_TOPFIELD_E]); //!< bottomFieldFlag
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_SEQ_MBAFF_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_8X8TRANS_FLAG_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_BLACKWHITE_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_TYPE1_QUANT_E]);

        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUT_DIS]); //!< set defalut 0
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_CH_8PIX_ILEAV_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_E]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_QTABLE_BASE]);


        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_IRQ_STAT]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_IRQ]);
        print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_PIC_INF]);
#endif



#endif
    }

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
#if 1
    LogInfo((LogCtx_t *)p_drv->log, "---------------- VDPU REG START ------------------ ");
    //!< ---------------- init register -------------
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_MODE         ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUT_ENDIAN   ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_IN_ENDIAN    ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_STRENDIAN_E  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUT_TILED_E  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_MAX_BURST    ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_SCMD_DIS     ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_ADV_PRE_DIS  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_APF_THRESHOLD    ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_LATENCY      ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_DATA_DISC_E  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUTSWAP32_E  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_INSWAP32_E   ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_STRSWAP32_E  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_TIMEOUT_E    ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_CLK_GATE_E   ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_IRQ_DIS      ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_AXI_RD_ID    ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_AXI_WR_ID    ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_PRED_BC_TAP_0_0  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_PRED_BC_TAP_0_1  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_PRED_BC_TAP_0_2  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_LATENCY      ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_CLK_GATE_E   ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUT_TILED_E  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_OUT_ENDIAN   ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_MAX_BURST    ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_DEC_DATA_DISC_E  ]);
    //!< picture parameter
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_PIC_MB_WIDTH     ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_PIC_MB_HEIGHT_P  ]);
    print_single_regs(p_drv, &p_drv->p_syn[VDPU_QTABLE_BASE      ]);
#endif
__RETURN:
    return MPP_OK;
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
MPP_RET vdpu_h264d_init(void *hal, MppHalCfg *cfg)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    HalRegDrvCtx_t *p_drv = NULL;
    H264dVdpuPkt_t *pkts  = NULL;

    INP_CHECK(ret, NULL == hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    //!< malloc init registers
    MEM_CHECK(ret, p_hal->regs = mpp_calloc_size(void, sizeof(HalRegDrvCtx_t)));
    p_drv = (HalRegDrvCtx_t *)p_hal->regs;
    p_drv->type     = cfg->type;
    p_drv->coding   = cfg->coding;
    p_drv->syn_size = VDPU_MAX_SIZE;
    p_drv->log      = (void *)p_hal->logctx.parr[LOG_WRITE_REG];

    MEM_CHECK(ret, p_drv->p_syn = mpp_calloc(HalRegDrv_t, p_drv->syn_size));
    for (i = 0; i < MPP_ARRAY_ELEMS(g_vdpu_drv); i++) {
        p_drv->p_syn[g_vdpu_drv[i].syn_id] = g_vdpu_drv[i];
        if (p_drv->syn_size == g_vdpu_drv[i].syn_id) {
            break;
        }
    }
    p_drv->reg_size = p_drv->p_syn[p_drv->syn_size].reg_id;
    MEM_CHECK(ret, p_drv->p_reg = mpp_calloc(RK_U32, p_drv->reg_size));
    //!< malloc init packets
    MEM_CHECK(ret, p_hal->pkts = mpp_calloc_size(void, sizeof(H264dVdpuPkt_t)));
    pkts = (H264dVdpuPkt_t *)p_hal->pkts;
    FUN_CHECK(ret = vdpu_alloc_packet(&p_hal->logctx, pkts));
    //!< setting device register
    vdpu_set_device_regs(p_drv);
    //!< copy cabac table bytes
    memcpy(pkts->cabac_buf, H264_VDPU_Cabac_table, VDPU_CABAC_TAB_SIZE);

    FUN_CHECK(ret = hal_set_regdrv(p_drv, VDPU_QTABLE_BASE,  0/*(RK_U32)pkts->cabac_buf*/));

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
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    if (p_hal->regs) {
        MPP_FREE(((HalRegDrvCtx_t *)p_hal->regs)->p_syn);
        MPP_FREE(((HalRegDrvCtx_t *)p_hal->regs)->p_reg);
        MPP_FREE(p_hal->regs);
    }
    if (p_hal->pkts) {
        vdpu_free_packet((H264dVdpuPkt_t *)p_hal->pkts);
        MPP_FREE(p_hal->pkts);
    }

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
__RETURN:
    return ret = MPP_OK;
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
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    FUN_CHECK(ret = vdpu_set_pic_regs(hal, p_drv));
    FUN_CHECK(ret = vdpu_set_vlc_regs(hal, p_drv));
    FUN_CHECK(ret = vdpu_set_ref_pic_list_regs(hal, p_drv));
    FUN_CHECK(ret = vdpu_set_asic_regs(hal, p_drv));

    ((HalDecTask*)&task->dec)->valid = 0;
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

    FUN_CHECK(ret = vdpu_flush_regs(hal, p_drv));

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

    FUN_CHECK(ret = vdpu_wait_result(hal, p_drv));

    FUN_CHECK(ret = vdpu_h264d_print_regs(p_hal, p_drv));

    mpp_log("++++++++++ hal_h264_decoder, g_framecnt=%d \n", p_hal->g_framecnt++);

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

    INP_CHECK(ret, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    //memset(&p_hal->regs, 0, sizeof(H264dRkvRegs_t));
    //reset_fifo_packet(p_hal->pkts);

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
