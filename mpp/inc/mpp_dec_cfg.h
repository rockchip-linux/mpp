/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __MPP_DEC_CFG_H__
#define __MPP_DEC_CFG_H__

#include "mpp_frame.h"
#include "rk_vdec_cmd.h"

typedef enum MppDecCfgChange_e {
    MPP_DEC_CFG_CHANGE_TYPE              = (1 << 0),
    MPP_DEC_CFG_CHANGE_CODING            = (1 << 1),
    MPP_DEC_CFG_CHANGE_HW_TYPE           = (1 << 2),
    MPP_DEC_CFG_CHANGE_BATCH_MODE        = (1 << 3),

    MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT     = (1 << 8),
    MPP_DEC_CFG_CHANGE_FAST_OUT          = (1 << 9),
    MPP_DEC_CFG_CHANGE_FAST_PARSE        = (1 << 10),
    MPP_DEC_CFG_CHANGE_SPLIT_PARSE       = (1 << 11),
    MPP_DEC_CFG_CHANGE_INTERNAL_PTS      = (1 << 12),
    MPP_DEC_CFG_CHANGE_SORT_PTS          = (1 << 13),
    MPP_DEC_CFG_CHANGE_DISABLE_ERROR     = (1 << 14),
    MPP_DEC_CFG_CHANGE_ENABLE_VPROC      = (1 << 15),
    MPP_DEC_CFG_CHANGE_ENABLE_FAST_PLAY  = (1 << 16),
    MPP_DEC_CFG_CHANGE_ENABLE_HDR_META   = (1 << 17),
    MPP_DEC_CFG_CHANGE_ENABLE_THUMBNAIL  = (1 << 18),
    MPP_DEC_CFG_CHANGE_ENABLE_MVC        = (1 << 19),
    /* disable dpb discontinuous check */
    MPP_DEC_CFG_CHANGE_DISABLE_DPB_CHECK = (1 << 20),
    /* reserve high bit for global config */
    MPP_DEC_CFG_CHANGE_DISABLE_THREAD    = (1 << 28),
    MPP_DEC_CFG_CHANGE_CODEC_MODE        = (1 << 29),
    MPP_DEC_CFG_CHANGE_DIS_ERR_CLR_MARK  = (1 << 30),

    MPP_DEC_CFG_CHANGE_ALL               = (0xFFFFFFFF),
} MppDecCfgChange;

typedef enum MppVprocMode_e {
    MPP_VPROC_MODE_NONE                  = 0,
    /*
     * Deinterlacing interlaced video stream only.
     * If video is marked as progressive, it won't be deinterlaced.
     */
    MPP_VPROC_MODE_DEINTELACE            = (1 << 0),
    /*
     * Both interlaced and progressive video will be sending to Vproc for Detection.
     * - For progressive vide, output directly during detection and adjust later
     *       according to IEP result.
     * - For interlaced video, interlaceing directly and adjust later according to
     *       IEP result.
     */
    MPP_VPROC_MODE_DETECTION             = (1 << 1),
    MPP_VPROC_MODE_ALL                   = (0xFFFFFFFF),
} MppVprocMode;

typedef enum FastPlayMode_e {
    MPP_DISABLE_FAST_PLAY,
    MPP_ENABLE_FAST_PLAY,
    // first gop fast play when poc include negative value, otherwise enable fast play all time
    MPP_ENABLE_FAST_PLAY_ONCE,
} FastPlayMode;

typedef struct MppDecBaseCfg_t {
    RK_U64              change;

    MppCtxType          type;
    MppCodingType       coding;
    RK_S32              hw_type;
    RK_U32              batch_mode;

    MppFrameFormat      out_fmt;
    RK_U32              fast_out;
    RK_U32              fast_parse;
    RK_U32              split_parse;
    RK_U32              internal_pts;
    RK_U32              sort_pts;
    RK_U32              disable_error;
    RK_U32              enable_vproc;   /* MppVprocMode */
    RK_U32              enable_fast_play;
    RK_U32              enable_hdr_meta;
    RK_U32              enable_thumbnail;
    RK_U32              enable_mvc;
    RK_U32              disable_dpb_chk;
    RK_U32              disable_thread;
    RK_U32              codec_mode;
    RK_U32              dis_err_clr_mark;
} MppDecBaseCfg;

typedef enum MppDecCbCfgChange_e {
    MPP_DEC_CB_CFG_CHANGE_PKT_RDY       = (1 << 0),
    MPP_DEC_CB_CFG_CHANGE_FRM_RDY       = (1 << 1),

    MPP_DEC_CB_CFG_CHANGE_ALL           = (0xFFFFFFFF),
} MppDecCbCfgChange;

typedef struct MppDecCbCfg_t {
    RK_U64              change;

    /* notify packet process done and can accept new packet */
    MppExtCbFunc        pkt_rdy_cb;
    MppExtCbCtx         pkt_rdy_ctx;
    RK_S32              pkt_rdy_cmd;

    /* notify frame ready for output */
    MppExtCbFunc        frm_rdy_cb;
    MppExtCbCtx         frm_rdy_ctx;
    RK_S32              frm_rdy_cmd;
} MppDecCbCfg;

typedef struct MppDecStatusCfg_t {
    RK_U32              hal_support_fast_mode;
    RK_U32              hal_task_count;
    RK_U32              vproc_task_count;
} MppDecStatusCfg;

typedef struct MppDecCfgSet_t {
    MppDecBaseCfg       base;
    MppDecStatusCfg     status;
    MppDecCbCfg         cb;
} MppDecCfgSet;

#ifdef __cplusplus
extern "C" {
#endif

void mpp_dec_cfg_set_default(MppDecCfgSet *cfg);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_DEC_CFG_H__ */
