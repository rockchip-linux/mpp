/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_DEC_CFG_H
#define MPP_DEC_CFG_H

#include "mpp_bit.h"
#include "mpp_frame.h"
#include "rk_vdec_cmd.h"

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

typedef struct MppDecCbCfg_t {
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
    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;
    RK_S32              buf_size;

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
