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


#define MODULE_TAG "hal_h264d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "vpu.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"

#include "dxva_syntax.h"
#include "h264d_syntax.h"
#include "h264d_log.h"

#include "hal_h264d_global.h"
#include "hal_h264d_api.h"

#include "hal_h264d_rkv_reg.h"
#include "hal_h264d_vdpu_reg.h"




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

    INP_CHECK(ret, !pflag->write_en);
    set_log_outpath(env);
    //!< runlog file
    if (GetBitVal(env->ctrl, LOG_DEBUG)) {
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

static void explain_input_buffer(void *hal, HalDecTask *task)
{
    RK_U32 i = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    DXVA2_DecodeBufferDesc *pdes = (DXVA2_DecodeBufferDesc *)task->syntax.data;
    for (i = 0; i < task->syntax.number; i++) {
        switch (pdes[i].CompressedBufferType) {
        case DXVA2_PictureParametersBufferType:
            p_hal->pp = (DXVA_PicParams_H264_MVC *)pdes[i].pvPVPState;
            break;
        case DXVA2_InverseQuantizationMatrixBufferType:
            p_hal->qm = (DXVA_Qmatrix_H264 *)pdes[i].pvPVPState;
            break;
        case DXVA2_SliceControlBufferType:
            p_hal->slice_num = pdes[i].DataSize / sizeof(DXVA_Slice_H264_Long);
            p_hal->slice_long = (DXVA_Slice_H264_Long *)pdes[i].pvPVPState;
            break;
        case DXVA2_BitStreamDateBufferType:
            p_hal->bitstream = (RK_U8 *)pdes[i].pvPVPState;
            p_hal->strm_len = pdes[i].DataSize;
            break;
        default:
            break;
        }
    }
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
    MppHalApi *p_api = NULL;

    INP_CHECK(ret, NULL == p_hal);
    memset(p_hal, 0, sizeof(H264dHalCtx_t));

    p_api = &p_hal->hal_api;

    p_hal->frame_slots  = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;
    //!< choose hard mode
    switch (cfg->device_id) {
    case HAL_RKVDEC:
        p_api->init    = rkv_h264d_init;
        p_api->deinit  = rkv_h264d_deinit;
        p_api->reg_gen = rkv_h264d_gen_regs;
        p_api->start   = rkv_h264d_start;
        p_api->wait    = rkv_h264d_wait;
        p_api->reset   = rkv_h264d_reset;
        p_api->flush   = rkv_h264d_flush;
        p_api->control = rkv_h264d_control;
        break;
    case HAL_VDPU:
        p_api->init    = vdpu_h264d_init;
        p_api->deinit  = vdpu_h264d_deinit;
        p_api->reg_gen = vdpu_h264d_gen_regs;
        p_api->start   = vdpu_h264d_start;
        p_api->wait    = vdpu_h264d_wait;
        p_api->reset   = vdpu_h264d_reset;
        p_api->flush   = vdpu_h264d_flush;
        p_api->control = vdpu_h264d_control;
    default:
        break;
    }
    //!< init logctx
    FUN_CHECK(ret = logctx_init(&p_hal->logctx, p_hal->logctxbuf));
    //!< VPUClientInit
#ifdef ANDROID
    if (p_hal->vpu_socket <= 0) {
        p_hal->vpu_socket = VPUClientInit(VPU_DEC);
        if (p_hal->vpu_socket <= 0) {
            mpp_err("p_hal->vpu_socket <= 0\n");
            ret = MPP_ERR_UNKNOW;
            goto __FAILED;
        }
    }
#endif
    //< get buffer group
    if (p_hal->buf_group == NULL) {
#ifdef ANDROID
        mpp_log_f("mpp_buffer_group_get_internal used ion In");
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_ION));
#else
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_NORMAL));
#endif
    }

    //!< run init funtion
    FUN_CHECK(ret = p_api->init(hal, cfg));

__RETURN:
    return MPP_OK;
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
MPP_RET hal_h264d_deinit(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    FUN_CHECK(ret = p_hal->hal_api.deinit(hal));
    FUN_CHECK(ret = logctx_deinit(&p_hal->logctx));
    //!< VPUClientInit
#ifdef ANDROID
    if (p_hal->vpu_socket >= 0) {
        VPUClientRelease(p_hal->vpu_socket);
    }
#endif
    if (p_hal->buf_group) {
        FUN_CHECK(ret = mpp_buffer_group_put(p_hal->buf_group));
    }

    return MPP_OK;
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
MPP_RET hal_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    //RK_U32 i = 0, j = 0;
    //RK_U8 *pdata = NULL;

    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    //const RK_U32 *ptr = (RK_U32 *)H264_RKV_Cabac_table;
    //for (i = 0; i< 155; i++) {
    //  for (j = 0; j< 6;j++) {
    //      pdata = (RK_U8 *)&ptr[j + i*6];
    //      FPRINT(g_debug_file1, "0x%02x%02x%02x%02x, ", pdata[3], pdata[2], pdata[1], pdata[0]);
    //  }
    //  FPRINT(g_debug_file1, "\n");
    //}
    //FPRINT(g_debug_file1, "\n");
    explain_input_buffer(hal, &task->dec);

    return p_hal->hal_api.reg_gen(hal, task);
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_start(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.start(hal, task);
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_wait(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.wait(hal, task);
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
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.reset(hal);
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
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.flush(hal);
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
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.control(hal, cmd_type, param);
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

