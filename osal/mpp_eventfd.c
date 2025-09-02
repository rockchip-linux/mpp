/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/eventfd.h>

#include "mpp_eventfd.h"

RK_S32 mpp_eventfd_get(RK_U32 init)
{
    RK_S32 fd = eventfd(init, 0);

    if (fd < 0)
        fd = errno;

    return fd;
}

RK_S32 mpp_eventfd_put(RK_S32 fd)
{
    if (fd >= 0)
        close(fd);

    return 0;
}

RK_S32 mpp_eventfd_read(RK_S32 fd, RK_U64 *val, RK_S64 timeout)
{
    struct pollfd nfds;
    RK_U64 tmp = 0;
    RK_S32 ret = 0;

    if (NULL == val)
        val = &tmp;

    nfds.fd = fd;
    nfds.events = POLLIN;

    ret = poll(&nfds, 1, timeout);
    if (ret == 1 && (nfds.revents & POLLIN) &&
        sizeof(RK_U64) == read(fd, val, sizeof(RK_U64))) {
        ret = 0;
    } else
        ret = errno;

    return ret;
}

RK_S32 mpp_eventfd_write(RK_S32 fd, RK_U64 val)
{
    RK_S32 ret = 0;

    if (sizeof(RK_U64) != write(fd, &val, sizeof(val)))
        ret = errno;

    return ret;
}
