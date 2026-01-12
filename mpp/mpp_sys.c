/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_sys"

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_singleton.h"

#include "mpp_sys.h"

#define SYS_DBG_EN                  (0x00000001)
#define SYS_DBG_CMD                 (0x00000002)
#define SYS_DBG_ARGS                (0x00000004)

#define mpp_sys_dbg(flag, fmt, ...) mpp_dbg(mpp_sys_debug, flag, fmt, ## __VA_ARGS__)

#define sys_dbg_cmd(fmt, ...)       mpp_sys_dbg(SYS_DBG_CMD, fmt, ## __VA_ARGS__)
#define sys_dbg_args(fmt, ...)      mpp_sys_dbg(SYS_DBG_ARGS, fmt, ## __VA_ARGS__)

#define get_srv_sys() \
    ({ \
        MppSysSrv *__tmp; \
        if (srv_sys) { \
            __tmp = srv_sys; \
        } else { \
            mpp_err("mpp sys srv not init at %s\n", __FUNCTION__); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

typedef struct MppSysSrv_t {
    struct list_head list;
    MppSThd thd;
    rk_s32 ctx_cnt;
    rk_s32 exit_efd;
} MppSysSrv;

static MppSysSrv *srv_sys = NULL;
static rk_u32 mpp_sys_debug = 0;

rk_s32 mpp_sys_attach(MpiImpl *ctx)
{
    MppSysSrv *srv = get_srv_sys();

    if (!srv || !srv->thd)
        return rk_nok;

    INIT_LIST_HEAD(&ctx->list);
    mpp_sthd_lock(srv->thd);
    list_add_tail(&ctx->list, &srv->list);
    srv->ctx_cnt++;
    mpp_sthd_unlock(srv->thd);

    return rk_ok;
}

rk_s32 mpp_sys_detach(MpiImpl *ctx)
{
    MppSysSrv *srv = get_srv_sys();

    if (!srv || !srv->thd)
        return rk_nok;

    mpp_sthd_lock(srv->thd);
    list_del_init(&ctx->list);
    srv->ctx_cnt--;
    mpp_sthd_unlock(srv->thd);

    return rk_ok;
}

static rk_s32 split_line(char *line, char *argv[], rk_s32 maxargv)
{
    rk_s32 argc = 0;
    char *p = line;

    while (argc < maxargv) {
        while (*p == ' ' || *p == '\t')
            p++;

        if (*p == '\0')
            break;

        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;

        if (*p)
            *p++ = '\0';
    }

    argv[argc] = NULL;

    return argc;
}

static rk_s32 mpp_sys_ctrl(rk_s32 pid, char *cmd)
{
    MppSysSrv *srv = get_srv_sys();
    char *argv[32];
    RK_S32 argc;
    RK_S32 i;

    sys_dbg_cmd("mpp-%d [cmd]: %s", pid, cmd);

    if (!srv || !srv->thd)
        return rk_nok;

    argc = split_line(cmd, argv, 32);

    sys_dbg_args("argc %d", argc);
    for (i = 0; i < argc; i++) {
        sys_dbg_args("argv[%d]: %s", i, argv[i]);
    }

    return 0;
}

static void *mpp_sys_srv(MppSThdCtx *data)
{
    MppSThd thd = data->thd;
    MppSysSrv *ctx = (MppSysSrv *)data->ctx;
    struct epoll_event ev = { .events = EPOLLIN };
    char cmd_buf[256];
    char name[128];
    MppSThdStatus status;
    rk_s32 nf;
    rk_s32 exit_efd = ctx->exit_efd;
    rk_s32 fifo_fd = -1;
    rk_s32 epoll_fd = -1;
    rk_s32 str_len = 0;
    rk_s32 pid = getpid();
    rk_s32 len;

    len = snprintf(name, sizeof(name) - 1, "/tmp/mpp-%d-cmd", pid);
    do {
        if (0 != mkfifo(name, 0600))
            break;

        fifo_fd = open(name, O_RDWR | O_CLOEXEC);
        if (fifo_fd < 0)
            break;

        /* add epoll and exit efd */
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd < 0)
            break;

        mpp_logi("pid %d create sys cmd fifo %s fd %d\n", pid, name, fifo_fd);

        /* add exit event fd */
        ev.data.fd = exit_efd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, exit_efd, &ev);

        ev.data.fd = fifo_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fifo_fd, &ev);

        while (1) {
            mpp_sthd_lock(thd);
            status = mpp_sthd_get_status(thd);
            mpp_sthd_unlock(thd);

            if (status != MPP_STHD_RUNNING)
                break;

            nf = epoll_wait(epoll_fd, &ev, 1, -1);
            if (nf <= 0)
                continue;

            // exit event
            if (ev.data.fd == exit_efd) {
                rk_u64 u;
                rk_s32 rd = read(exit_efd, &u, 8);

                if (rd == 8)
                    break;
            }

            // normal event
            if (ev.data.fd == fifo_fd) {
                rk_s32 n = read(fifo_fd, cmd_buf, sizeof(cmd_buf) - 1);

                if (n > 0) {
                    cmd_buf[n] = 0;
                    mpp_sys_ctrl(pid, cmd_buf);
                }
            }
        }
    } while (0);

    /* clean up all fds */
    if (epoll_fd > 0)
        close(epoll_fd);

    if (fifo_fd > 0) {
        close(fifo_fd);
        unlink(name);
    }

    mpp_logi("pid %d sys srv finish\n", pid);

    return NULL;
}

static void mpp_sys_init(void)
{
    MppSysSrv *srv = srv_sys;

    mpp_env_get_u32("mpp_sys_debug", &mpp_sys_debug, 0);

    if (srv)
        return;

    srv = mpp_calloc(MppSysSrv, 1);
    if (!srv) {
        mpp_err_f("alloc mpp sys srv failed\n");
        return;
    }

    srv_sys = srv;

    INIT_LIST_HEAD(&srv->list);

    /* setup external command fifo and system service thread */
    if (mpp_sys_debug != 0) {
        MppSThd sys_thd = NULL;
        char name[64];

        mpp_sys_debug |= SYS_DBG_EN;

        /* create system service thread and its exit event fd */
        srv->exit_efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

        snprintf(name, sizeof(name) - 1, "mpp-%d-sys-srv", getpid());
        sys_thd = mpp_sthd_get(name);
        if (sys_thd) {
            mpp_sthd_setup(sys_thd, mpp_sys_srv, srv);
            mpp_sthd_start(sys_thd);
            srv->thd = sys_thd;
        }
    }
}

static void mpp_sys_deinit(void)
{
    MppSysSrv *srv = srv_sys;

    if (!srv)
        return;

    srv_sys = NULL;

    if (srv->thd != NULL) {
        /* notify system service to quit */
        rk_u64 exit_val = 1;
        rk_s32 ret;

        mpp_sthd_stop(srv->thd);
        ret = write(srv->exit_efd, &exit_val, sizeof(exit_val));

        /* wait for system service to quit */
        mpp_sthd_stop_sync(srv->thd);
        mpp_sthd_put(srv->thd);
        srv->thd = NULL;
    }

    mpp_free(srv);
}

MPP_SINGLETON(MPP_SGLN_SYS, mpp_sys, mpp_sys_init, mpp_sys_deinit)
