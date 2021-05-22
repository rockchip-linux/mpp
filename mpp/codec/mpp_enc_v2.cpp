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

#define  MODULE_TAG "mpp_enc"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_info.h"
#include "mpp_common.h"
#include "mpp_2str.h"

#include "mpp.h"
#include "mpp_enc_debug.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_enc_impl.h"

RK_U32 mpp_enc_debug = 0;

MPP_RET mpp_enc_init_v2(MppEnc *enc, MppEncInitCfg *cfg)
{
    MPP_RET ret;
    MppCodingType coding = cfg->coding;
    EncImpl impl = NULL;
    MppEncImpl *p = NULL;
    MppEncHal enc_hal = NULL;
    MppEncHalCfg enc_hal_cfg;
    EncImplCfg ctrl_cfg;

    mpp_env_get_u32("mpp_enc_debug", &mpp_enc_debug, 0);

    if (NULL == enc) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    *enc = NULL;

    p = mpp_calloc(MppEncImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_MALLOC;
    }

    ret = mpp_enc_refs_init(&p->refs);
    if (ret) {
        mpp_err_f("could not init enc refs\n");
        goto ERR_RET;
    }

    // H.264 encoder use mpp_enc_hal path
    // create hal first
    enc_hal_cfg.coding = coding;
    enc_hal_cfg.cfg = &p->cfg;
    enc_hal_cfg.type = VPU_CLIENT_BUTT;
    enc_hal_cfg.dev = NULL;

    ctrl_cfg.coding = coding;
    ctrl_cfg.type = VPU_CLIENT_BUTT;
    ctrl_cfg.cfg = &p->cfg;
    ctrl_cfg.refs = p->refs;
    ctrl_cfg.task_count = 2;

    ret = mpp_enc_hal_init(&enc_hal, &enc_hal_cfg);
    if (ret) {
        mpp_err_f("could not init enc hal\n");
        goto ERR_RET;
    }

    ctrl_cfg.type = enc_hal_cfg.type;
    ctrl_cfg.task_count = -1;

    ret = enc_impl_init(&impl, &ctrl_cfg);
    if (ret) {
        mpp_err_f("could not init impl\n");
        goto ERR_RET;
    }

    ret = hal_info_init(&p->hal_info, MPP_CTX_ENC, coding);
    if (ret) {
        mpp_err_f("could not init hal info\n");
        goto ERR_RET;
    }

    p->coding   = coding;
    p->impl     = impl;
    p->enc_hal  = enc_hal;
    p->dev      = enc_hal_cfg.dev;
    p->mpp      = cfg->mpp;
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_SEQ;
    p->version_info = get_mpp_version();
    p->version_length = strlen(p->version_info);
    p->rc_cfg_size = SZ_1K;
    p->rc_cfg_info = mpp_calloc_size(char, p->rc_cfg_size);

    {
        // create header packet storage
        size_t size = SZ_1K;
        p->hdr_buf = mpp_calloc_size(void, size);

        mpp_packet_init(&p->hdr_pkt, p->hdr_buf, size);
        mpp_packet_set_length(p->hdr_pkt, 0);
    }
    {
        Mpp *mpp = (Mpp *)p->mpp;

        p->input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
        p->output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    }

    /* NOTE: setup configure coding for check */
    p->cfg.codec.coding = coding;
    p->cfg.plt_cfg.plt = &p->cfg.plt_data;
    mpp_enc_ref_cfg_init(&p->cfg.ref_cfg);
    ret = mpp_enc_ref_cfg_copy(p->cfg.ref_cfg, mpp_enc_ref_default());
    ret = mpp_enc_refs_set_cfg(p->refs, mpp_enc_ref_default());

    sem_init(&p->enc_reset, 0, 0);
    sem_init(&p->cmd_start, 0, 0);
    sem_init(&p->cmd_done, 0, 0);

    *enc = p;
    return ret;
ERR_RET:
    mpp_enc_deinit_v2(p);
    return ret;
}

MPP_RET mpp_enc_deinit_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    if (NULL == enc) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (enc->hal_info) {
        hal_info_deinit(enc->hal_info);
        enc->hal_info = NULL;
    }

    if (enc->impl) {
        enc_impl_deinit(enc->impl);
        enc->impl = NULL;
    }

    if (enc->enc_hal) {
        mpp_enc_hal_deinit(enc->enc_hal);
        enc->enc_hal = NULL;
    }

    if (enc->hdr_pkt)
        mpp_packet_deinit(&enc->hdr_pkt);

    MPP_FREE(enc->hdr_buf);

    if (enc->cfg.ref_cfg) {
        mpp_enc_ref_cfg_deinit(&enc->cfg.ref_cfg);
        enc->cfg.ref_cfg = NULL;
    }

    if (enc->refs) {
        mpp_enc_refs_deinit(&enc->refs);
        enc->refs = NULL;
    }

    if (enc->rc_ctx) {
        rc_deinit(enc->rc_ctx);
        enc->rc_ctx = NULL;
    }

    MPP_FREE(enc->rc_cfg_info);
    enc->rc_cfg_size = 0;
    enc->rc_cfg_length = 0;

    sem_destroy(&enc->enc_reset);
    sem_destroy(&enc->cmd_start);
    sem_destroy(&enc->cmd_done);

    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_start_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;
    char name[16];

    enc_dbg_func("%p in\n", enc);

    snprintf(name, sizeof(name) - 1, "mpp_%se_%d",
             strof_coding_type(enc->coding), getpid());

    enc->thread_enc = new MppThread(mpp_enc_thread, enc->mpp, name);
    enc->thread_enc->start();

    enc_dbg_func("%p out\n", enc);

    return MPP_OK;
}

MPP_RET mpp_enc_stop_v2(MppEnc ctx)
{
    MPP_RET ret = MPP_OK;
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);

    if (enc->thread_enc) {
        enc->thread_enc->stop();
        delete enc->thread_enc;
        enc->thread_enc = NULL;
    }

    enc_dbg_func("%p out\n", enc);
    return ret;

}

MPP_RET mpp_enc_reset_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);
    if (NULL == enc) {
        mpp_err_f("found NULL input enc\n");
        return MPP_ERR_NULL_PTR;
    }

    MppThread *thd = enc->thread_enc;

    thd->lock(THREAD_CONTROL);
    enc->reset_flag = 1;
    mpp_enc_notify_v2(enc, MPP_ENC_RESET);
    thd->unlock(THREAD_CONTROL);
    sem_wait(&enc->enc_reset);
    mpp_assert(enc->reset_flag == 0);

    return MPP_OK;
}

MPP_RET mpp_enc_notify_v2(MppEnc ctx, RK_U32 flag)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in flag %08x\n", enc, flag);
    MppThread *thd  = enc->thread_enc;

    thd->lock();
    if (flag == MPP_ENC_CONTROL) {
        enc->notify_flag |= flag;
        enc_dbg_notify("%p status %08x notify control signal\n", enc,
                       enc->status_flag);
        thd->signal();
    } else {
        RK_U32 old_flag = enc->notify_flag;

        enc->notify_flag |= flag;
        if ((old_flag != enc->notify_flag) &&
            (enc->notify_flag & enc->status_flag)) {
            enc_dbg_notify("%p status %08x notify %08x signal\n", enc,
                           enc->status_flag, enc->notify_flag);
            thd->signal();
        }
    }
    thd->unlock();
    enc_dbg_func("%p out\n", enc);
    return MPP_OK;
}

/*
 * preprocess config and rate-control config is common config then they will
 * be done in mpp_enc layer
 *
 * codec related config will be set in each hal component
 */
MPP_RET mpp_enc_control_v2(MppEnc ctx, MpiCmd cmd, void *param)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    if (NULL == enc) {
        mpp_err_f("found NULL enc\n");
        return MPP_ERR_NULL_PTR;
    }

    if (NULL == param && cmd != MPP_ENC_SET_IDR_FRAME && cmd != MPP_ENC_SET_REF_CFG) {
        mpp_err_f("found NULL param enc %p cmd %x\n", enc, cmd);
        return MPP_ERR_NULL_PTR;
    }

    AutoMutex auto_lock(&enc->lock);
    MPP_RET ret = MPP_OK;

    enc_dbg_ctrl("sending cmd %d param %p\n", cmd, param);

    switch (cmd) {
    case MPP_ENC_GET_CFG : {
        MppEncCfgImpl *p = (MppEncCfgImpl *)param;

        enc_dbg_ctrl("get all config\n");
        memcpy(&p->cfg, &enc->cfg, sizeof(enc->cfg));
    } break;
    case MPP_ENC_GET_PREP_CFG : {
        enc_dbg_ctrl("get prep config\n");
        memcpy(param, &enc->cfg.prep, sizeof(enc->cfg.prep));
    } break;
    case MPP_ENC_GET_RC_CFG : {
        enc_dbg_ctrl("get rc config\n");
        memcpy(param, &enc->cfg.rc, sizeof(enc->cfg.rc));
    } break;
    case MPP_ENC_GET_CODEC_CFG : {
        enc_dbg_ctrl("get codec config\n");
        memcpy(param, &enc->cfg.codec, sizeof(enc->cfg.codec));
    } break;
    case MPP_ENC_GET_HEADER_MODE : {
        enc_dbg_ctrl("get header mode\n");
        memcpy(param, &enc->hdr_mode, sizeof(enc->hdr_mode));
    } break;
    case MPP_ENC_GET_OSD_PLT_CFG : {
        enc_dbg_ctrl("get osd plt cfg\n");
        memcpy(param, &enc->cfg.plt_cfg, sizeof(enc->cfg.plt_cfg));
    } break;
    default : {
        // Cmd which is not get configure will handle by enc_impl
        enc->cmd = cmd;
        enc->param = param;
        enc->cmd_ret = &ret;
        enc->cmd_send++;
        mpp_enc_notify_v2(ctx, MPP_ENC_CONTROL);
        sem_post(&enc->cmd_start);
        sem_wait(&enc->cmd_done);

        /* check the command is processed */
        mpp_assert(!enc->cmd);
        mpp_assert(!enc->param);
    } break;
    }

    enc_dbg_ctrl("sending cmd %d done\n", cmd);
    return ret;
}
