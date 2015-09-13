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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_mem.h"

#include "dxva_syntax.h"
#include "h264d_syntax.h"
#include "h264d_log.h"

#include "hal_h264d_global.h"
#include "hal_h264d_reg.h"
#include "hal_h264d_packet.h"
#include "hal_h264d_api.h"



#define MODULE_TAG "hal_h264d_api"



static void close_log_files(LogEnv_t *env)
{
	FCLOSE(env->fp_driver);
	FCLOSE(env->fp_syn_hal);
	FCLOSE(env->fp_run_hal);
}

static MPP_RET open_log_files(LogEnv_t *env, LogFlag_t *pflag)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	char fname[128] = { 0 };

	INP_CHECK(ret, ctx, !pflag->write_en);
	//!< runlog file
	if (GetBitVal(env->ctrl, LOG_DEBUG_EN)) {
		sprintf(fname, "%s/h264d_hal_runlog.dat", env->outpath);
		FLE_CHECK(ret, env->fp_run_hal = fopen(fname, "wb"));
	}
	//!< fpga drive file
	if (GetBitVal(env->ctrl, LOG_FPGA)) {
		sprintf(fname, "%s/h264d_driver_data.dat", env->outpath);
		FLE_CHECK(ret, env->fp_driver = fopen(fname, "wb"));
	}
	//!< write syntax
	if (   GetBitVal(env->ctrl, LOG_WRITE_SPSPPS  )
		|| GetBitVal(env->ctrl, LOG_WRITE_RPS     )
		|| GetBitVal(env->ctrl, LOG_WRITE_SCANLIST)
		|| GetBitVal(env->ctrl, LOG_WRITE_STEAM   )
		|| GetBitVal(env->ctrl, LOG_WRITE_REG     ) ) {
			sprintf(fname, "%s/h264d_write_syntax.dat", env->outpath);
			FLE_CHECK(ret, env->fp_syn_hal = fopen(fname, "wb"));
	}

__RETURN:
	return MPP_OK;

__FAILED:
	return ret;
}

static MPP_RET logctx_deinit(H264dLogCtx_t *logctx)
{
	close_log_files(&logctx->env);

	return MPP_OK;
}

static MPP_RET logctx_init(H264dLogCtx_t *logctx, LogCtx_t *logbuf)
{
	RK_U8 i = 0;
	MPP_RET ret = MPP_ERR_UNKNOW;
	LogCtx_t *pcur = NULL;

	FUN_CHECK(ret = get_logenv(&logctx->env));

	FUN_CHECK(ret = explain_ctrl_flag(logctx->env.ctrl, &logctx->log_flag));
	if ( !logctx->log_flag.debug_en
		&& !logctx->log_flag.print_en && !logctx->log_flag.write_en ) {
			logctx->log_flag.debug_en = 0;
			goto __RETURN;
	}
	logctx->log_flag.level = (1 << logctx->env.level) - 1;
	//!< open file
	FUN_CHECK(ret = open_log_files(&logctx->env, &logctx->log_flag));
	//!< set logctx
	while (i < LOG_MAX) {
		if (GetBitVal(logctx->env.ctrl, i)) {
			pcur = logctx->parr[i] = &logbuf[i];
			pcur->tag = logctrl_name[i];
			pcur->flag = &logctx->log_flag;

			switch (i) {
			case LOG_FPGA:
				pcur->fp = logctx->env.fp_driver;
				break;
			case RUN_HAL:
				pcur->fp = logctx->env.fp_run_hal;
				break;
			case LOG_WRITE_SPSPPS:
			case LOG_WRITE_RPS:
			case LOG_WRITE_SCANLIST:
			case LOG_WRITE_STEAM:
			case LOG_WRITE_REG:
				pcur->fp = logctx->env.fp_syn_hal;
			default:
				break;
			}
		}
		i++;
	}
__RETURN:
	return ret = MPP_OK;
__FAILED:
	logctx->log_flag.debug_en = 0;
	logctx_deinit(logctx);

	return ret;
}


/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_init(void *hal, MppHalCfg *cfg)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

	INP_CHECK(ret, ctx, NULL == p_hal);
	memset(p_hal, 0, sizeof(H264dHalCtx_t));
	//!< init logctx
	FUN_CHECK(ret = logctx_init(&p_hal->logctx, p_hal->logctxbuf));
	FunctionIn(p_hal->logctx.parr[RUN_HAL]);
	p_hal->mem = mpp_calloc(H264dHalMem_t, 1);
	MEM_CHECK(ret, p_hal->mem);
	p_hal->regs       = &p_hal->mem->regs;
	p_hal->mmu_regs   = &p_hal->mem->mmu_regs;
	p_hal->cache_regs = &p_hal->mem->cache_regs;
	p_hal->pkts       = &p_hal->mem->pkts;
	FUN_CHECK(ret = alloc_fifo_packet(&p_hal->logctx, p_hal->pkts));

	FunctionOut(p_hal->logctx.parr[RUN_HAL]);
    (void)cfg;
__RETURN:
    return MPP_OK;
__FAILED:
	hal_h264d_deinit(hal);

	return ret;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_deinit(void *hal)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

	INP_CHECK(ret, ctx, NULL == p_hal);
	FunctionIn(p_hal->logctx.parr[RUN_HAL]);

	free_fifo_packet(p_hal->pkts);
	mpp_free(p_hal->mem);

	FunctionOut(p_hal->logctx.parr[RUN_HAL]);
	logctx_deinit(&p_hal->logctx);
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
MPP_RET hal_h264d_gen_regs(void *hal, HalTask *task)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

	INP_CHECK(ret, ctx, NULL == p_hal);
	FunctionIn(p_hal->logctx.parr[RUN_HAL]);

	explain_input_buffer(hal, &task->dec);
	prepare_spspps_packet(hal, &p_hal->pkts->spspps);
	prepare_framerps_packet(hal, &p_hal->pkts->rps);
	prepare_scanlist_packet(hal, &p_hal->pkts->scanlist);
	prepare_stream_packet(hal, &p_hal->pkts->strm);
	generate_regs(p_hal, &p_hal->pkts->reg);
	
	printf("++++++++++ hal_h264_decoder, g_framecnt=%d \n", p_hal->g_framecnt++);
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

MPP_RET hal_h264d_start(void *hal, HalTask *task)
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
MPP_RET hal_h264d_wait(void *hal, HalTask *task)
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
MPP_RET hal_h264d_reset(void *hal)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

	INP_CHECK(ret, ctx, NULL == p_hal);
	FunctionIn(p_hal->logctx.parr[RUN_HAL]);

	memset(&p_hal->regs, 0, sizeof(H264_REGS_t));
	memset(&p_hal->mmu_regs, 0, sizeof(H264_MMU_t));
	memset(&p_hal->cache_regs, 0, sizeof(H264_CACHE_t));
	reset_fifo_packet(p_hal->pkts);

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
MPP_RET hal_h264d_flush(void *hal)
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
MPP_RET hal_h264d_control(void *hal, RK_S32 cmd_type, void *param)
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

const MppHalApi hal_api_h264d = {
    "h264d_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingAVC,
    sizeof(H264dHalCtx_t),
    0,
    hal_h264d_init,
    hal_h264d_deinit,
    hal_h264d_gen_regs,
    hal_h264d_start,
    hal_h264d_wait,
    hal_h264d_reset,
    hal_h264d_flush,
    hal_h264d_control,
};

