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

#define MODULE_TAG "hal_avsd_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_mpi_cmd.h"
#include "mpp_debug.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_platform.h"
#include "mpp_device.h"

#include "mpp_dec_cb_param.h"
#include "hal_avsd_api.h"
#include "hal_avsd_base.h"
#include "hal_avsd_vdpu1.h"
#include "hal_avsd_vdpu2.h"
#include "hal_avsd_plus.h"

static MPP_RET init_hard_platform(AvsdHalCtx_t *p_hal, MppCodingType coding)
{
    MppHalApi *p_api = &p_hal->hal_api;
    RK_U32 vcodec_type = mpp_get_vcodec_type();
    MppClientType client_type = VPU_CLIENT_BUTT;
    MPP_RET ret = MPP_ERR_UNKNOW;

    if (coding == MPP_VIDEO_CodingAVSPLUS) {
        if (!(vcodec_type & HAVE_AVSDEC))
            mpp_err("coding %x vcodec_type %x do not found avs hw %x\n",
                    coding, vcodec_type, HAVE_AVSDEC);
    } else {
        RK_U32 hw_flag = HAVE_VDPU1 | HAVE_VDPU2 | HAVE_VDPU1_PP | HAVE_VDPU2_PP;

        if (!(vcodec_type & hw_flag ))
            mpp_err("coding %x vcodec_type %x do not found avs hw %x\n",
                    coding, vcodec_type, hw_flag);
    }

    if ((coding == MPP_VIDEO_CodingAVSPLUS) &&
        (vcodec_type & HAVE_AVSDEC)) {
        p_api->init    = hal_avsd_plus_init;
        p_api->deinit  = hal_avsd_plus_deinit;
        p_api->reg_gen = hal_avsd_plus_gen_regs;
        p_api->start   = hal_avsd_plus_start;
        p_api->wait    = hal_avsd_plus_wait;
        p_api->reset   = hal_avsd_plus_reset;
        p_api->flush   = hal_avsd_plus_flush;
        p_api->control = hal_avsd_plus_control;
        client_type    = VPU_CLIENT_AVSPLUS_DEC;
    } else if ((coding == MPP_VIDEO_CodingAVS) &&
               (vcodec_type & (HAVE_VDPU1 | HAVE_VDPU1_PP))) {
        p_api->init    = hal_avsd_vdpu1_init;
        p_api->deinit  = hal_avsd_vdpu1_deinit;
        p_api->reg_gen = hal_avsd_vdpu1_gen_regs;
        p_api->start   = hal_avsd_vdpu1_start;
        p_api->wait    = hal_avsd_vdpu1_wait;
        p_api->reset   = hal_avsd_vdpu1_reset;
        p_api->flush   = hal_avsd_vdpu1_flush;
        p_api->control = hal_avsd_vdpu1_control;
        client_type    = VPU_CLIENT_VDPU1;
    } else if ((coding == MPP_VIDEO_CodingAVS) &&
               (vcodec_type & (HAVE_VDPU2 | HAVE_VDPU2_PP))) {
        p_api->init    = hal_avsd_vdpu2_init;
        p_api->deinit  = hal_avsd_vdpu2_deinit;
        p_api->reg_gen = hal_avsd_vdpu2_gen_regs;
        p_api->start   = hal_avsd_vdpu2_start;
        p_api->wait    = hal_avsd_vdpu2_wait;
        p_api->reset   = hal_avsd_vdpu2_reset;
        p_api->flush   = hal_avsd_vdpu2_flush;
        p_api->control = hal_avsd_vdpu2_control;
        client_type    = VPU_CLIENT_VDPU2;
    } else {
        ret = MPP_NOK;
        goto __FAILED;
    }
    p_hal->coding = coding;
    AVSD_HAL_DBG(AVSD_DBG_HARD_MODE, "hw_spt %08x, coding %d\n", vcodec_type, coding);

    ret = mpp_dev_init(&p_hal->dev, client_type);
    if (ret) {
        mpp_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;

    }
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
MPP_RET hal_avsd_deinit(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == decoder);

    FUN_CHECK(ret = p_hal->hal_api.deinit(decoder));
    //!< mpp_dev_init
    if (p_hal->dev) {
        ret = mpp_dev_deinit(p_hal->dev);
        if (ret)
            mpp_err("mpp_dev_deinit failed. ret: %d\n", ret);
    }

    if (p_hal->buf_group) {
        FUN_CHECK(ret = mpp_buffer_group_put(p_hal->buf_group));
    }
__RETURN:
    AVSD_HAL_TRACE("Out.");

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
MPP_RET hal_avsd_init(void *decoder, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == decoder);

    memset(p_hal, 0, sizeof(AvsdHalCtx_t));
    p_hal->frame_slots = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;

    //!< callback function to parser module
    p_hal->dec_cb = cfg->dec_cb;
    mpp_env_get_u32("avsd_debug", &avsd_hal_debug, 0);
    //< get buffer group
    FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_ION));

    FUN_CHECK(ret = init_hard_platform(p_hal, cfg->coding));
    cfg->dev = p_hal->dev;
    p_hal->dec_cfg = cfg->cfg;

    //!< run init funtion
    FUN_CHECK(ret = p_hal->hal_api.init(decoder, cfg));

__RETURN:
    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
__FAILED:
    hal_avsd_deinit(decoder);
    return ret;
}

/*!
 ***********************************************************************
 * \brief
 *    generate register
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_gen_regs(void *decoder, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    MppCodingType coding = MPP_VIDEO_CodingUnused;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    memcpy(&p_hal->syn, task->dec.syntax.data, sizeof(AvsdSyntax_t));
    // check coding
    coding = (p_hal->syn.pp.profileId == 0x48) ? MPP_VIDEO_CodingAVSPLUS : p_hal->coding;
    if (coding != p_hal->coding) {
        if (p_hal->dev) {
            ret = mpp_dev_deinit(p_hal->dev);
            if (ret)
                mpp_err("mpp_dev_deinit failed. ret: %d\n", ret);

            p_hal->dev = NULL;
        }

        ret = p_hal->hal_api.deinit(decoder);
        if (ret) {
            mpp_err_f("deinit decoder failed, ret %d\n", ret);
            return ret;
        }

        ret = init_hard_platform(p_hal, coding);
        if (ret) {
            mpp_err_f("change paltform %x -> %x error\n", p_hal->coding, coding);
            return ret;
        }

        ret = p_hal->hal_api.init(decoder, p_hal->cfg);
        if (ret) {
            mpp_err_f("init decoder failed, ret %d\n", ret);
            return ret;
        }
    }

    p_hal->frame_no++;

    return p_hal->hal_api.reg_gen(decoder, task);
}
/*!
 ***********************************************************************
 * \brief h
 *    start hard
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_start(void *decoder, HalTaskInfo *task)
{
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    return p_hal->hal_api.start(decoder, task);
}
/*!
 ***********************************************************************
 * \brief
 *    wait hard
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_wait(void *decoder, HalTaskInfo *task)
{
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    return p_hal->hal_api.wait(decoder, task);
}
/*!
 ***********************************************************************
 * \brief
 *    reset
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_reset(void *decoder)
{
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    return p_hal->hal_api.reset(p_hal);
}
/*!
 ***********************************************************************
 * \brief
 *    flush
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_flush(void *decoder)
{
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    return p_hal->hal_api.flush(p_hal);
}
/*!
 ***********************************************************************
 * \brief
 *    control
 ***********************************************************************
 */
//extern "C"
MPP_RET hal_avsd_control(void *decoder, MpiCmd cmd_type, void *param)
{
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    return p_hal->hal_api.control(decoder, cmd_type, param);
}

const MppHalApi hal_api_avsd = {
    .name = "avsd_vdpu",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingAVS,
    .ctx_size = sizeof(AvsdHalCtx_t),
    .flag = 0,
    .init = hal_avsd_init,
    .deinit = hal_avsd_deinit,
    .reg_gen = hal_avsd_gen_regs,
    .start = hal_avsd_start,
    .wait = hal_avsd_wait,
    .reset = hal_avsd_reset,
    .flush = hal_avsd_flush,
    .control = hal_avsd_control,
};

const MppHalApi hal_api_avsd_plus = {
    .name = "avsd_plus",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingAVSPLUS,
    .ctx_size = sizeof(AvsdHalCtx_t),
    .flag = 0,
    .init = hal_avsd_init,
    .deinit = hal_avsd_deinit,
    .reg_gen = hal_avsd_gen_regs,
    .start =  hal_avsd_start,
    .wait = hal_avsd_wait,
    .reset = hal_avsd_reset,
    .flush = hal_avsd_flush,
    .control = hal_avsd_control,
};
