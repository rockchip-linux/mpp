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

#ifndef __MPP_SERVICE_H__
#define __MPP_SERVICE_H__

#include "rk_type.h"
#include <asm/ioctl.h>

/* Use 'v' as magic number */
#define MPP_IOC_MAGIC                       'v'
#define MPP_IOC_CFG_V1                      _IOW(MPP_IOC_MAGIC, 1, unsigned int)
#define MAX_REQ_NUM                         16

#if __SIZEOF_POINTER__ == 4
#define REQ_DATA_PTR(ptr) ((RK_U32)ptr)
#elif __SIZEOF_POINTER__ == 8
#define REQ_DATA_PTR(ptr) ((RK_U64)ptr)
#endif

/* define flags for mpp_request */
#define MPP_FLAGS_MULTI_MSG         (0x00000001)
#define MPP_FLAGS_LAST_MSG          (0x00000002)
#define MPP_FLAGS_REG_FD_NO_TRANS   (0x00000004)
#define MPP_FLAGS_SCL_FD_NO_TRANS   (0x00000008)
#define MPP_FLAGS_LINK_MODE_FIX     (0x00000010)
#define MPP_FLAGS_LINK_MODE_UPDATE  (0x00000020)
#define MPP_FLAGS_SECURE_MODE       (0x00010000)

/* mpp service capability description */
typedef enum MppDevCmd_e {
    MPP_DEV_GET_START               = 0,
    MPP_DEV_GET_MAX_WIDTH,
    MPP_DEV_GET_MAX_HEIGHT,
    MPP_DEV_GET_MIN_WIDTH,
    MPP_DEV_GET_MIN_HEIGHT,
    MPP_DEV_GET_MMU_STATUS,

    MPP_DEV_SET_START               = 0x01000000,
    MPP_DEV_SET_HARD_PLATFORM,      // set paltform by user
    MPP_DEV_ENABLE_POSTPROCCESS,

    MPP_DEV_PROP_BUTT,
} MppDevCmd;

typedef enum MppServiceCmdType_e {
    MPP_CMD_QUERY_BASE              = 0,
    MPP_CMD_PROBE_HW_SUPPORT        = MPP_CMD_QUERY_BASE + 0,
    MPP_CMD_QUERY_HW_ID             = MPP_CMD_QUERY_BASE + 1,
    MPP_CMD_QUERY_CMD_SUPPORT       = MPP_CMD_QUERY_BASE + 2,
    MPP_CMD_QUERY_BUTT,

    MPP_CMD_INIT_BASE               = 0x100,
    MPP_CMD_INIT_CLIENT_TYPE        = MPP_CMD_INIT_BASE + 0,
    MPP_CMD_INIT_DRIVER_DATA        = MPP_CMD_INIT_BASE + 1,
    MPP_CMD_INIT_TRANS_TABLE        = MPP_CMD_INIT_BASE + 2,
    MPP_CMD_INIT_BUTT,

    MPP_CMD_SEND_BASE               = 0x200,
    MPP_CMD_SET_REG_WRITE           = MPP_CMD_SEND_BASE + 0,
    MPP_CMD_SET_REG_READ            = MPP_CMD_SEND_BASE + 1,
    MPP_CMD_SET_REG_ADDR_OFFSET     = MPP_CMD_SEND_BASE + 2,
    MPP_CMD_SET_RCB_INFO            = MPP_CMD_SEND_BASE + 3,
    MPP_CMD_SEND_BUTT,

    MPP_CMD_POLL_BASE               = 0x300,
    MPP_CMD_POLL_HW_FINISH          = MPP_CMD_POLL_BASE + 0,
    MPP_CMD_POLL_BUTT,

    MPP_CMD_CONTROL_BASE            = 0x400,
    MPP_CMD_RESET_SESSION           = MPP_CMD_CONTROL_BASE + 0,
    MPP_CMD_TRANS_FD_TO_IOVA        = MPP_CMD_CONTROL_BASE + 1,
    MPP_CMD_RELEASE_FD              = MPP_CMD_CONTROL_BASE + 2,
    MPP_CMD_SEND_CODEC_INFO         = MPP_CMD_CONTROL_BASE + 3,
    MPP_CMD_CONTROL_BUTT,

    MPP_CMD_BUTT,
} MppServiceCmdType;

typedef struct mppReqV1_t {
    RK_U32 cmd;
    RK_U32 flag;
    RK_U32 size;
    RK_U32 offset;
    RK_U64 data_ptr;
} MppReqV1;

typedef struct MppServiceCmdCap_t {
    RK_U32 support_cmd;
    RK_U32 query_cmd;
    RK_U32 init_cmd;
    RK_U32 send_cmd;
    RK_U32 poll_cmd;
    RK_U32 ctrl_cmd;
} MppServiceCmdCap;

#ifdef  __cplusplus
extern "C" {
#endif

void check_mpp_service_cap(RK_U32 *codec_type, RK_U32 *hw_ids, MppServiceCmdCap *cap);
const MppServiceCmdCap *mpp_get_mpp_service_cmd_cap(void);
const char *mpp_get_mpp_service_name(void);

#ifdef  __cplusplus
}
#endif

#endif /* __MPP_SERVICE_H__ */
