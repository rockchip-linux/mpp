/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_SERVER_H__
#define __MPP_SERVER_H__

#include "mpp_device.h"

#ifdef  __cplusplus
extern "C" {
#endif

rk_s32 mpp_server_attach(MppDev ctx);
rk_s32 mpp_server_detach(MppDev ctx);

rk_s32 mpp_server_send_task(MppDev ctx);
rk_s32 mpp_server_wait_task(MppDev ctx, RK_S64 timeout);

#ifdef  __cplusplus
}
#endif

#endif /* __MPP_SERVER_H__ */
