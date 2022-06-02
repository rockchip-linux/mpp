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

#ifndef __MPP_DEVICE_H__
#define __MPP_DEVICE_H__

#include "mpp_err.h"
#include "mpp_dev_defs.h"
#include "mpp_callback.h"

typedef enum MppDevIoctlCmd_e {
    /* device batch mode config */
    MPP_DEV_BATCH_ON,
    MPP_DEV_BATCH_OFF,
    MPP_DEV_DELIMIT,
    MPP_DEV_SET_CB_CTX,

    /* hardware operation setup config */
    MPP_DEV_REG_WR,
    MPP_DEV_REG_RD,
    MPP_DEV_REG_OFFSET,
    MPP_DEV_REG_OFFS,
    MPP_DEV_RCB_INFO,
    MPP_DEV_SET_INFO,

    MPP_DEV_CMD_SEND,
    MPP_DEV_CMD_POLL,

    MPP_DEV_IOCTL_CMD_BUTT,
} MppDevIoctlCmd;

/* for MPP_DEV_REG_WR */
typedef struct MppDevRegWrCfg_t {
    void    *reg;
    RK_U32  size;
    RK_U32  offset;
} MppDevRegWrCfg;

/* for MPP_DEV_REG_RD */
typedef struct MppDevRegRdCfg_t {
    void    *reg;
    RK_U32  size;
    RK_U32  offset;
} MppDevRegRdCfg;

/* for MPP_DEV_REG_OFFSET */
typedef struct MppDevRegOffsetCfg_t {
    RK_U32  reg_idx;
    RK_U32  offset;
} MppDevRegOffsetCfg;

/* for multi MPP_DEV_REG_OFFSET */
typedef struct MppDevRegOffsCfg_t {
    RK_S32  size;
    RK_S32  count;
    MppDevRegOffsetCfg cfgs[];
} MppDevRegOffCfgs;

/* for MPP_DEV_RCB_INFO */
typedef struct MppDevRcbInfoCfg_t {
    RK_U32  reg_idx;
    RK_U32  size;
} MppDevRcbInfoCfg;

/* for MPP_DEV_SET_INFO */
typedef struct MppDevSetInfoCfg_t {
    RK_U32  type;
    RK_U32  flag;
    RK_U64  data;
} MppDevInfoCfg;

typedef union MppDevPollEncSliceInfo_u {
    RK_U32  val;
    struct {
        RK_U32  length  : 31;
        RK_U32  last    : 1;
    };
} MppDevPollEncSliceInfo;

/* for MPP_DEV_POLL */
typedef struct MppDevPollCfg_t {
    RK_S32  poll_type;
    RK_S32  poll_ret;
    RK_S32  count_max;
    RK_S32  count_ret;
    MppDevPollEncSliceInfo slice_info[];
} MppDevPollCfg;

typedef struct MppDevApi_t {
    const char  *name;
    RK_U32      ctx_size;
    MPP_RET     (*init)(void *ctx, MppClientType type);
    MPP_RET     (*deinit)(void *ctx);

    /* bat mode function */
    MPP_RET     (*attach)(void *ctx);
    MPP_RET     (*detach)(void *ctx);
    MPP_RET     (*delimit)(void *ctx);
    MPP_RET     (*set_cb_ctx)(void *ctx, MppCbCtx *cb);

    /* config the cmd on preparing */
    MPP_RET     (*reg_wr)(void *ctx, MppDevRegWrCfg *cfg);
    MPP_RET     (*reg_rd)(void *ctx, MppDevRegRdCfg *cfg);
    MPP_RET     (*reg_offset)(void *ctx, MppDevRegOffsetCfg *cfg);
    MPP_RET     (*reg_offs)(void *ctx, MppDevRegOffCfgs *cfg);
    MPP_RET     (*rcb_info)(void *ctx, MppDevRcbInfoCfg *cfg);
    MPP_RET     (*set_info)(void *ctx, MppDevInfoCfg *cfg);

    /* send cmd to hardware */
    MPP_RET     (*cmd_send)(void *ctx);

    /* poll cmd from hardware */
    MPP_RET     (*cmd_poll)(void *ctx, MppDevPollCfg *cfg);
} MppDevApi;

typedef void* MppDev;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_dev_init(MppDev *ctx, MppClientType type);
MPP_RET mpp_dev_deinit(MppDev ctx);

MPP_RET mpp_dev_ioctl(MppDev ctx, RK_S32 cmd, void *param);

/* special helper function for large address offset config */
MPP_RET mpp_dev_set_reg_offset(MppDev dev, RK_S32 index, RK_U32 offset);

/* register offset multi config */
MPP_RET mpp_dev_multi_offset_init(MppDevRegOffCfgs **cfgs, RK_S32 size);
MPP_RET mpp_dev_multi_offset_deinit(MppDevRegOffCfgs *cfgs);

MPP_RET mpp_dev_multi_offset_reset(MppDevRegOffCfgs *cfgs);
MPP_RET mpp_dev_multi_offset_update(MppDevRegOffCfgs *cfgs, RK_S32 index, RK_U32 offset);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_DEVICE_H__ */
