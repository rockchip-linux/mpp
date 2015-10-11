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

#include "h264d_log.h"
#include "hal_h264d_global.h"
#include "hal_h264d_api.h"
#include "hal_h264d_vdpu_pkt.h"
#include "hal_h264d_vdpu_reg.h"




/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET vdpu_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, ctx, NULL == p_hal);
    ////!< init logctx
    //FUN_CHECK(ret = rkv_logctx_init(&p_hal->logctx, p_hal->logctxbuf));
    //FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    //p_hal->regs = mpp_calloc(H264_RkvRegs_t, 1);
    //p_hal->pkts = mpp_calloc(H264dRkvPkt_t, 1);
    //MEM_CHECK(ret, p_hal->regs && p_hal->pkts);
    //FUN_CHECK(ret = alloc_fifo_packet(&p_hal->logctx, p_hal->pkts));

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)cfg;
__RETURN:
    return MPP_OK;
//__FAILED:
//  vdpu_h264d_deinit(hal);

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

    INP_CHECK(ret, ctx, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    //free_fifo_packet(p_hal->pkts);
    //MPP_FREE(p_hal->regs);
    //MPP_FREE(p_hal->pkts);
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    //rkv_logctx_deinit(&p_hal->logctx);
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
    //H264dRkvPkt_t *pkts  = (H264dRkvPkt_t *)p_hal->pkts;
    INP_CHECK(ret, ctx, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    //explain_input_buffer(hal, &task->dec);
    //prepare_spspps_packet(hal, &pkts->spspps);
    //prepare_framerps_packet(hal, &pkts->rps);
    //prepare_scanlist_packet(hal, &pkts->scanlist);
    //prepare_stream_packet(hal, &pkts->strm);
    //generate_regs(p_hal, &pkts->reg);

    mpp_log("++++++++++ hal_h264_decoder, g_framecnt=%d \n", p_hal->g_framecnt++);
    ((HalDecTask*)&task->dec)->valid = 0;
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);

__RETURN:
    return ret = MPP_OK;
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
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, ctx, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)task;
__RETURN:
    return ret = MPP_OK;
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
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, ctx, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)task;
__RETURN:
    return ret = MPP_OK;
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

    INP_CHECK(ret, ctx, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    //memset(&p_hal->regs, 0, sizeof(H264_RkvRegs_t));
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

    INP_CHECK(ret, ctx, NULL == p_hal);
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

    INP_CHECK(ret, ctx, NULL == p_hal);
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);





    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)hal;
    (void)cmd_type;
    (void)param;
__RETURN:
    return ret = MPP_OK;
}