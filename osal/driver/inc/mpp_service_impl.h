/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#ifndef __MPP_SERVICE_IMPL_H__
#define __MPP_SERVICE_IMPL_H__

#include "mpp_device.h"
#include "mpp_service.h"

#define MAX_REG_OFFSET          32
#define MAX_RCB_OFFSET          32
#define MAX_INFO_COUNT          16

typedef struct FdTransInfo_t {
    RK_U32          reg_idx;
    RK_U32          offset;
} RegOffsetInfo;

typedef struct RcbInfo_t {
    RK_U32          reg_idx;
    RK_U32          size;
} RcbInfo;

typedef struct MppDevMppService_t {
    RK_S32          client_type;
    RK_S32          client;
    RK_S32          server;
    void            *serv_ctx;
    RK_S32          batch_io;
    MppCbCtx        *dev_cb;

    RK_S32          req_cnt;
    RK_S32          reg_offset_count;
    MppReqV1        reqs[MAX_REQ_NUM];
    RegOffsetInfo   reg_offset_info[MAX_REG_OFFSET];
    RK_S32          rcb_count;
    RcbInfo         rcb_info[MAX_RCB_OFFSET];

    RK_S32          info_count;
    MppDevInfoCfg   info[MAX_INFO_COUNT];

    /* support max cmd buttom  */
    const MppServiceCmdCap *cap;
    RK_U32          support_set_info;
    RK_U32          support_set_rcb_info;
} MppDevMppService;

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 mpp_service_ioctl(RK_S32 fd, RK_U32 cmd, RK_U32 size, void *param);
RK_S32 mpp_service_ioctl_request(RK_S32 fd, MppReqV1 *req);
MPP_RET mpp_service_check_cmd_valid(RK_U32 cmd, const MppServiceCmdCap *cap);

#ifdef  __cplusplus
}
#endif

#endif /* __MPP_SERVICE_IMPL_H__ */
