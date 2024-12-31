/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_vcodec"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_vcodec_client.h"

#define VOCDEC_IOC_MAGIC            'V'
#define VOCDEC_IOC_CFG              _IOW(VOCDEC_IOC_MAGIC, 1, unsigned int)

typedef struct vcodec_req_t {
    RK_U32 cmd;
    RK_U32 ctrl_cmd;
    RK_U32 size;
    RK_U64 data;
} vcodec_req;

#if __SIZEOF_POINTER__ == 4
#define REQ_DATA_PTR(ptr) ((RK_U32)ptr)
#elif __SIZEOF_POINTER__ == 8
#define REQ_DATA_PTR(ptr) ((RK_U64)ptr)
#endif

RK_S32 mpp_vcodec_open(void)
{
    RK_S32 fd = -1;

    fd = open("/dev/vcodec", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        mpp_err("open /dev/vcodec failed errno %d %s\n", errno, strerror(errno));
        return -1;
    }

    return fd;
}

MPP_RET mpp_vcodec_ioctl(RK_S32 fd, RK_U32 cmd, RK_U32 ctrl_cmd, RK_U32 size, void *param)
{
    vcodec_req req;
    RK_S32 ret = 0;

    memset(&req, 0, sizeof(req));
    req.cmd = cmd;
    req.ctrl_cmd = ctrl_cmd;
    req.size = size;
    req.data = REQ_DATA_PTR(param);

    ret = (RK_S32)ioctl(fd, VOCDEC_IOC_CFG, &req);
    if (ret) {
        mpp_err("ioctl fd %d failed ret %d errno %d %s\n",
                fd, ret, errno, strerror(errno));
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpp_vcodec_close(RK_S32 fd)
{
    if (fd)
        close(fd);

    return MPP_OK;
}
