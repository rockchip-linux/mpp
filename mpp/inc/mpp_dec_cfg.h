/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_DEC_CFG_H
#define MPP_DEC_CFG_H

#include "mpp_bit.h"
#include "mpp_frame.h"
#include "rk_vdec_cmd.h"

typedef enum MppDecCfgChange_e {
    MPP_DEC_CFG_CHANGE_TYPE              = MPP_BIT(0),
    MPP_DEC_CFG_CHANGE_CODING            = MPP_BIT(1),
    MPP_DEC_CFG_CHANGE_HW_TYPE           = MPP_BIT(2),
    MPP_DEC_CFG_CHANGE_BATCH_MODE        = MPP_BIT(3),

    MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT     = MPP_BIT(8),
    MPP_DEC_CFG_CHANGE_FAST_OUT          = MPP_BIT(9),
    MPP_DEC_CFG_CHANGE_FAST_PARSE        = MPP_BIT(10),
    MPP_DEC_CFG_CHANGE_SPLIT_PARSE       = MPP_BIT(11),
    MPP_DEC_CFG_CHANGE_INTERNAL_PTS      = MPP_BIT(12),
    MPP_DEC_CFG_CHANGE_SORT_PTS          = MPP_BIT(13),
    MPP_DEC_CFG_CHANGE_DISABLE_ERROR     = MPP_BIT(14),
    MPP_DEC_CFG_CHANGE_ENABLE_VPROC      = MPP_BIT(15),
    MPP_DEC_CFG_CHANGE_ENABLE_FAST_PLAY  = MPP_BIT(16),
    MPP_DEC_CFG_CHANGE_ENABLE_HDR_META   = MPP_BIT(17),
    MPP_DEC_CFG_CHANGE_ENABLE_THUMBNAIL  = MPP_BIT(18),
    MPP_DEC_CFG_CHANGE_ENABLE_MVC        = MPP_BIT(19),
    /* disable dpb discontinuous check */
    MPP_DEC_CFG_CHANGE_DISABLE_DPB_CHECK = MPP_BIT(20),
    /* reserve high bit for global config */
    MPP_DEC_CFG_CHANGE_DISABLE_THREAD    = MPP_BIT(28),
    MPP_DEC_CFG_CHANGE_CODEC_MODE        = MPP_BIT(29),
    MPP_DEC_CFG_CHANGE_DIS_ERR_CLR_MARK  = MPP_BIT(30),

    MPP_DEC_CFG_CHANGE_ALL               = (0xFFFFFFFFU),
} MppDecCfgChange;

typedef enum MppVprocMode_e {
    MPP_VPROC_MODE_NONE                  = 0,
    /*
     * Deinterlacing interlaced video stream only.
     * If video is marked as progressive, it won't be deinterlaced.
     */
    MPP_VPROC_MODE_DEINTELACE            = (1U << 0),
    /*
     * Both interlaced and progressive video will be sending to Vproc for Detection.
     * - For progressive vide, output directly during detection and adjust later
     *       according to IEP result.
     * - For interlaced video, interlaceing directly and adjust later according to
     *       IEP result.
     */
    MPP_VPROC_MODE_DETECTION             = (1U << 1),
    MPP_VPROC_MODE_ALL                   = (0xFFFFFFFFU),
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
    MPP_DEC_CB_CFG_CHANGE_PKT_RDY       = MPP_BIT(0),
    MPP_DEC_CB_CFG_CHANGE_FRM_RDY       = MPP_BIT(1),

    MPP_DEC_CB_CFG_CHANGE_ALL           = (0xFFFFFFFFU),
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

rk_s32 mpp_dec_cfg_set_default(void *entry, KmppObj obj, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* MPP_DEC_CFG_H */
