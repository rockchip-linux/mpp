/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_ENC_ARGS_H
#define MPP_ENC_ARGS_H

#include "rk_mpp_cfg.h"

typedef void* MppEncArgs;

#define KMPP_OBJ_NAME               mpp_enc_args
#define KMPP_OBJ_INTF_TYPE          MppEncArgs
#include "kmpp_obj_func.h"

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_enc_args_extract(MppEncArgs cmd_obj, MppCfgStrFmt fmt, char **buf);
rk_s32 mpp_enc_args_apply(MppEncArgs cmd_obj, MppCfgStrFmt fmt, char *buf);

#ifdef __cplusplus
}
#endif

#endif /* MPP_ENC_ARGS_H */
