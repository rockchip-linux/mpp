/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HAL_H265E_STREAM_AMEND_H__
#define __HAL_H265E_STREAM_AMEND_H__

#include "hal_enc_task.h"
#include "h265e_syntax_new.h"

MPP_RET hal_h265e_amend_temporal_id(HalEncTask *enc_task, RK_U32 strm_size);

#endif /* __HAL_H265E_STREAM_AMEND_H__ */