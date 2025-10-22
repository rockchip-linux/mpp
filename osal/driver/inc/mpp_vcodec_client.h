/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_VCODEC_CLIENT_H
#define MPP_VCODEC_CLIENT_H

#include "rk_type.h"
#include "mpp_err.h"

#define VCODEC_ID_BASE_COMMON       (0x00000000)
#define VCODEC_ID_BASE_STATE        (0x00000100)
#define VCODEC_ID_BASE_FLOW         (0x00000200)

#define VCODEC_ID_BASE_INPUT        (0x00000400)
#define VCODEC_ID_BASE_INPUT_ACK    (0x00000500)

#define VCODEC_ID_BASE_OUTPUT       (0x00000600)
#define VCODEC_ID_BASE_OUTPUT_ACK   (0x00000700)

enum vcodec_event_id {
    /* channel comment event */
    VCODEC_CHAN_CREATE          = VCODEC_ID_BASE_COMMON,
    VCODEC_CHAN_DESTROY,
    VCODEC_CHAN_RESET,
    VCODEC_CHAN_CONTROL,

    /* channel state change event */
    VCODEC_CHAN_START           = VCODEC_ID_BASE_STATE,
    VCODEC_CHAN_STOP,
    VCODEC_CHAN_PAUSE,
    VCODEC_CHAN_RESUME,

    /* channel data flow event */
    VCODEC_CHAN_BIND            = VCODEC_ID_BASE_FLOW,
    VCODEC_CHAN_UNBIND,

    /* channel input side io event from external module */
    VCODEC_CHAN_IN_FRM_RDY      = VCODEC_ID_BASE_INPUT,
    VCODEC_CHAN_IN_FRM_START,
    VCODEC_CHAN_IN_FRM_EARLY_END,
    VCODEC_CHAN_IN_FRM_END,

    /* channel input side ack event from vcodec module */
    VCODEC_CHAN_IN_BLOCK        = VCODEC_ID_BASE_INPUT_ACK,

    /* channel output side io event from vcodec module */
    VCODEC_CHAN_OUT_STRM_Q_FULL = VCODEC_ID_BASE_OUTPUT,
    VCODEC_CHAN_OUT_STRM_BUF_RDY,
    VCODEC_CHAN_OUT_STRM_END,
    /* new get packet interface */
    VCODEC_CHAN_OUT_PKT_RDY,

    /* channel input side ack event from external module */
    VCODEC_CHAN_OUT_BLOCK       = VCODEC_ID_BASE_OUTPUT_ACK,

};

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 mpp_vcodec_open(void);
MPP_RET mpp_vcodec_ioctl(RK_S32 fd, RK_U32 cmd, RK_U32 ctrl_cmd, RK_U32 size, void *param);
MPP_RET mpp_vcodec_close(RK_S32 fd);

#ifdef  __cplusplus
}
#endif

#endif
