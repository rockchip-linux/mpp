/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#include "iniparser.h"

#include "mpp_mem.h"
#include "mpp_cfg_io.h"
#include "mpp_singleton.h"

#include "mpi_enc_utils.h"
#include "mpp_enc_args.h"

static rk_s32 mpp_enc_args_impl_init(void *entry, KmppObj obj, const char *caller)
{
    MpiEncTestArgs *args = (MpiEncTestArgs *)entry;

    args->nthreads = 1;
    args->frm_step = 1;
    args->rc_mode = MPP_ENC_RC_MODE_BUTT;

    (void) obj;
    (void) caller;

    return rk_ok;
}

static rk_s32 mpp_enc_args_impl_deinit(void *entry, KmppObj obj, const char *caller)
{
    MpiEncTestArgs *args = (MpiEncTestArgs *)entry;

    MPP_FREE(args->file_input);
    MPP_FREE(args->file_output);
    MPP_FREE(args->file_cfg);
    MPP_FREE(args->file_slt);

    (void) obj;
    (void) caller;

    return rk_ok;
}

static rk_s32 mpp_enc_args_impl_dump(void *entry)
{
    MpiEncTestArgs *args = (MpiEncTestArgs *)entry;

    mpp_logi("cmd parse result:\n");
    mpp_logi("input  file name: %s\n", args->file_input);
    mpp_logi("output file name: %s\n", args->file_output);
    mpp_logi("width      : %d\n", args->width);
    mpp_logi("height     : %d\n", args->height);
    mpp_logi("format     : %d\n", args->format);
    mpp_logi("type       : %d\n", args->type);
    if (args->file_slt) {
        mpp_logi("verify     : %s\n", args->file_slt);
        mpp_logi("frame step : %d\n", args->frm_step);
    }

    return rk_ok;
}

#define MPP_ENC_ARGS_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    CFG_DEF_START() \
    ENTRY(prefix, s32,  rk_s32,     type_src,         FLAG_NONE,       type_src) \
    ENTRY(prefix, s32,  rk_s32,     frame_num,        FLAG_NONE,       frame_num) \
    ENTRY(prefix, s32,  rk_s32,     loop_cnt,         FLAG_NONE,       loop_cnt) \
    ENTRY(prefix, s32,  rk_s32,     nthreads,         FLAG_NONE,       nthreads) \
    ENTRY(prefix, s32,  rk_s32,     frm_step,         FLAG_NONE,       frm_step) \
    ENTRY(prefix, s32,  rk_s32,     gop_mode,         FLAG_NONE,       gop_mode) \
    ENTRY(prefix, s32,  rk_s32,     vi_len,           FLAG_NONE,       vi_len) \
    ENTRY(prefix, s32,  rk_s32,     quiet,            FLAG_NONE,       quiet) \
    ENTRY(prefix, s32,  rk_s32,     trace_fps,        FLAG_NONE,       trace_fps) \
    ENTRY(prefix, u32,  rk_u32,     kmpp_en,          FLAG_NONE,       kmpp_en) \
    ENTRY(prefix, u32,  rk_u32,     osd_enable,       FLAG_NONE,       osd_enable) \
    ENTRY(prefix, u32,  rk_u32,     osd_mode,         FLAG_NONE,       osd_mode) \
    ENTRY(prefix, u32,  rk_u32,     user_data_enable, FLAG_NONE,       user_data_enable) \
    ENTRY(prefix, u32,  rk_u32,     roi_enable,       FLAG_NONE,       roi_enable) \
    ENTRY(prefix, u32,  rk_u32,     roi_jpeg_enable,  FLAG_NONE,       roi_jpeg_enable) \
    ENTRY(prefix, u32,  rk_u32,     jpeg_osd_case,    FLAG_NONE,       jpeg_osd_case) \
    ENTRY(prefix, u32,  rk_u32,     constraint_set,   FLAG_NONE,       constraint_set) \
    ENTRY(prefix, u32,  rk_u32,     sei_mode,         FLAG_NONE,       sei_mode) \
    CFG_DEF_END()

#define KMPP_OBJ_NAME               mpp_enc_args
#define KMPP_OBJ_INTF_TYPE          MppEncArgs
#define KMPP_OBJ_IMPL_TYPE          MpiEncTestArgs
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_ENC_ARGS
#define KMPP_OBJ_FUNC_INIT          mpp_enc_args_impl_init
#define KMPP_OBJ_FUNC_DEINIT        mpp_enc_args_impl_deinit
#define KMPP_OBJ_FUNC_DUMP          mpp_enc_args_impl_dump
#define KMPP_OBJ_ENTRY_TABLE        MPP_ENC_ARGS_ENTRY_TABLE
#define KMPP_OBJ_ACCESS_DISABLE
#define KMPP_OBJ_HIERARCHY_ENABLE
#include "kmpp_obj_helper.h"

rk_s32 mpp_enc_args_extract(MppEncArgs cmd_obj, MppCfgStrFmt fmt, char **buf)
{
    MpiEncTestArgs *cmd = kmpp_obj_to_entry(cmd_obj);
    MppCfgObj obj = NULL;
    MppCfgObj root = NULL;

    root = kmpp_objdef_get_cfg_root(mpp_enc_args_def);

    mpp_cfg_from_struct(&obj, root, cmd);
    if (obj) {
        mpp_cfg_to_string(obj, fmt, buf);
        mpp_cfg_put_all(obj);
    }

    return rk_ok;
}

rk_s32 mpp_enc_args_apply(MppEncArgs cmd_obj, MppCfgStrFmt fmt, char *buf)
{
    MpiEncTestArgs *cmd = kmpp_obj_to_entry(cmd_obj);
    MppCfgObj obj = NULL;
    MppCfgObj root = NULL;

    root = kmpp_objdef_get_cfg_root(mpp_enc_args_def);

    mpp_cfg_from_string(&obj, fmt, buf);
    if (obj) {
        mpp_cfg_to_struct(obj, root, cmd);
        mpp_cfg_put_all(obj);
    }

    return rk_ok;
}