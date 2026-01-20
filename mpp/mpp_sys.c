/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_sys"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_2str.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_singleton.h"

#include "mpp_sys.h"
#include "mpp_cfg_io.h"

#define SYS_DBG_EN                  (0x00000001)
#define SYS_DBG_FLOW                (0x00000002)

#define SYS_DBG_CMD                 (0x00000010)
#define SYS_DBG_ARGS                (0x00000020)
#define SYS_DBG_SPLIT               (0x00000040)

#define mpp_sys_dbg(flag, fmt, ...) mpp_dbg(mpp_sys_debug, flag, fmt, ## __VA_ARGS__)

#define sys_dbg_flow(fmt, ...)      mpp_sys_dbg(SYS_DBG_FLOW, fmt, ## __VA_ARGS__)
#define sys_dbg_cmd(fmt, ...)       mpp_sys_dbg(SYS_DBG_CMD, fmt, ## __VA_ARGS__)
#define sys_dbg_args(fmt, ...)      mpp_sys_dbg(SYS_DBG_ARGS, fmt, ## __VA_ARGS__)
#define sys_dbg_split(fmt, ...)     mpp_sys_dbg(SYS_DBG_SPLIT, fmt, ## __VA_ARGS__)

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

typedef enum MppSysCmdType_e {
    MPP_CMD_SYS,
    MPP_CMD_VENC,
    MPP_CMD_VDEC,
} MppSysCmdType;

typedef struct MppSysSrv_t {
    struct list_head list;
    MppSThd thd;
    rk_s32 pid;
    rk_s32 ctx_id;
    rk_s32 ctx_cnt;
    rk_s32 exit_efd;
} MppSysSrv;

typedef struct MppSysCmdInfo_t MppSysCmdInfo;

struct MppSysCmdInfo_t {
    const char *name;
    const char *desc;

    rk_s32 (*cmd)(MppSysSrv *srv, const char **argv, void *ctx);
    MppSysCmdInfo *sub_cmd[];
};

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
    ctx->ctx_id = srv->ctx_id++;
    if (srv->ctx_id < 0)
        srv->ctx_id = 0;
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

static rk_s32 split_line(char *buf, rk_s32 buf_len, const char *argv[], rk_s32 maxargv)
{
    rk_s32 argc = 0;
    rk_s32 idx  = 0;
    rk_s32 end  = buf_len;
    rk_s32 i;

    sys_dbg_split("==== split_line: buf_len %d  hex_dump:\n", end);
    for (i = 0; i < end; i++)
        sys_dbg_split("  [%02d] %02x '%c'\n", i, (unsigned char)buf[i],
                      isprint(buf[i]) ? buf[i] : '?');

    while (argc < maxargv && idx < end) {
        sys_dbg_split("---- new token loop -> idx %d\n", idx);

        while (idx < end && isspace((unsigned char)buf[idx])) {
            sys_dbg_split("  skip space @%d  char %02x",
                          idx, (unsigned char)buf[idx]);
            idx++;
        }

        if (idx >= end)
            break;

        bool    in_quote    = false;
        char    quote_char  = 0;
        rk_s32  token_start = -1;
        rk_s32  dst         = idx;

        sys_dbg_split("  token body start: idx %2d  char %02x\n",
                      idx, (unsigned char)buf[idx]);

        while (idx < end) {
            char ch = buf[idx];

            sys_dbg_split("    idx %2d  raw %02x  ch '%c'  in_quote %d",
                          idx, (unsigned char)ch, isprint(ch) ? ch : '?', in_quote);

            if (!in_quote && isspace((unsigned char)ch)) {
                if (token_start == -1) {
                    idx++;
                    continue;
                }
                sys_dbg_split("    break by space @%d", idx);
                break;
            }

            if (ch == '"' || ch == '\'') {
                if (!in_quote) {
                    in_quote = true;
                    quote_char = ch;
                    idx++;
                    continue;
                }
                if (ch == quote_char) {
                    in_quote = false;
                    idx++;
                    continue;
                }
            } else if (ch == '\\' && in_quote) {
                idx++;
                if (idx >= end)
                    break;

                ch = buf[idx];
                sys_dbg_split("    escape  next %02x", (unsigned char)ch);

                if (token_start == -1)
                    token_start = dst;

                buf[dst++] = ch;
                idx++;
                continue;
            }

            if (in_quote || !isspace((unsigned char)ch)) {
                if (token_start == -1)
                    token_start = dst;
                buf[dst++] = ch;
                idx++;
            } else {
                idx++;
            }
        }

        if (dst < end) {
            buf[dst] = '\0';
            idx++;
        }
        sys_dbg_split("  token_end %d  token_start %d", dst, token_start);

        if (token_start != -1 && token_start < end)
            argv[argc++] = &buf[token_start];

        sys_dbg_split("  argv[%d] = \"%s\"", argc - 1, &buf[token_start]);
    }

    sys_dbg_split("==== final argc %d\n", argc);

    return argc;
}

static rk_s32 mpp_sys_help(MppSysSrv *srv, const char **argv, void *ctx);
static rk_s32 mpp_sys_show(MppSysSrv *srv, const char **argv, void *ctx);
static rk_s32 mpp_sys_venc(MppSysSrv *srv, const char **argv, void *ctx);
static rk_s32 mpp_sys_venc_dump_cfg(MppSysSrv *srv, const char **argv, void *ctx);
static rk_s32 mpp_sys_venc_show_cfg(MppSysSrv *srv, const char **argv, void *ctx);

static MppSysCmdInfo venc_dump_cfg = {
    .name = "dump_cfg",
    .desc = "dump venc currnt config to file",
    .cmd = mpp_sys_venc_dump_cfg,
    .sub_cmd = {
        NULL,
    },
};

static MppSysCmdInfo venc_show_cfg = {
    .name = "show_cfg",
    .desc = "show all / enc / dec mpp instances",
    .cmd = mpp_sys_venc_show_cfg,
    .sub_cmd = {
        NULL,
    },
};

static MppSysCmdInfo sys_help = {
    .name = "help",
    .desc = "show all commands",
    .cmd = mpp_sys_help,
    .sub_cmd = {
        NULL,
    },
};

static MppSysCmdInfo sys_show = {
    .name = "show",
    .desc = "show all / enc / dec mpp instances",
    .cmd = mpp_sys_show,
    .sub_cmd = {
        NULL,
    },
};

static MppSysCmdInfo venc_cmds = {
    .name = "venc",
    .desc = "venc command: [all / id] [cmd]",
    .cmd = mpp_sys_venc,
    .sub_cmd = {
        &venc_dump_cfg,
        &venc_show_cfg,
        NULL,
    },
};

static MppSysCmdInfo *sys_cmds[] = {
    &sys_help,
    &sys_show,
    &venc_cmds,
    NULL,
};

static rk_s32 mpp_sys_help(MppSysSrv *srv, const char *argv[], void *ctx)
{
    MppSysCmdInfo **cmds = &sys_cmds[0];
    rk_s32 idx = 0;
    (void)srv;
    (void)argv;
    (void)ctx;

    mpp_logi("mpp system command list:");

    while (*cmds) {
        MppSysCmdInfo *info = *cmds;
        MppSysCmdInfo **subs = &info->sub_cmd[0];
        rk_s32 sub_idx = 0;

        mpp_logi("%2d | %-8s | : %s", idx, info->name, info->desc);

        while (*subs) {
            MppSysCmdInfo *sub = *subs;

            mpp_logi(" |- %2d | %-8s | : %s", sub_idx, sub->name, sub->desc);
            subs++;
            sub_idx++;
        }

        cmds++;
        idx++;
    }

    return 0;
}

static rk_s32 mpp_sys_show(MppSysSrv *srv, const char *argv[], void *ctx)
{
    MpiImpl *impl;
    MpiImpl *tmp;
    rk_u32 show_mask = 0;
    rk_s32 next = 0;
    (void)ctx;

    if (argv[1]) {
        rk_s32 i;

        static const char *filter[] = {
            "dec",
            "enc",
            "all",
        };
        static rk_u32 filter_mask[] = {
            MPP_BIT(MPP_CTX_DEC),
            MPP_BIT(MPP_CTX_ENC),
            MPP_BIT32_OR(MPP_CTX_ENC, MPP_CTX_DEC),
        };

        for (i = 0; i < MPP_ARRAY_ELEMS_S(filter); i++) {
            if (!strcmp(argv[1], filter[i])) {
                show_mask = filter_mask[i];
                next++;

                mpp_logi("show %s contexts info:\n", filter[i]);
                break;
            }
        }
    }

    if (show_mask == 0) {
        show_mask = MPP_BIT32_OR(MPP_CTX_ENC, MPP_CTX_DEC);
        mpp_logi("show all contexts info:\n");
    }


    mpp_sthd_lock(srv->thd);

    list_for_each_entry_safe(impl, tmp, &srv->list, MpiImpl, list) {
        rk_u32 mask = MPP_BIT(impl->type);

        if ((show_mask & mask) == 0)
            continue;

        switch (impl->type) {
        case MPP_CTX_DEC : {
            Mpp *mpp = impl->ctx;
            MppDec dec = mpp->mDec;
            MppEncCfg cfg = mpp_dec_to_cfg(dec);
            rk_s32 width = 0;
            rk_s32 height = 0;

            mpp_dec_cfg_get_s32(cfg, "status:width", &width);
            mpp_dec_cfg_get_s32(cfg, "status:height", &height);

            mpp_logi("ctx %4d - dec %-6s - %4dx%-4d\n", impl->ctx_id,
                     strof_coding_type(impl->coding), width, height);
        } break;
        case MPP_CTX_ENC : {
            Mpp *mpp = impl->ctx;
            MppEnc enc = mpp->mEnc;
            MppEncCfg cfg = mpp_enc_to_cfg(enc);
            rk_s32 width = 0;
            rk_s32 height = 0;

            mpp_enc_cfg_get_s32(cfg, "prep:width", &width);
            mpp_enc_cfg_get_s32(cfg, "prep:height", &height);

            mpp_logi("ctx %4d - enc %-6s - %4dx%-4d\n", impl->ctx_id,
                     strof_coding_type(impl->coding), width, height);
        } break;
        default : {
        } break;
        }
    }

    mpp_sthd_unlock(srv->thd);

    return next;
}

static rk_s32 safe_atoi(const char *s, rk_s32 *out)
{
    char *e = NULL;
    rk_long v;

    errno = 0;
    v = strtol(s, &e, 10);

    if (e == s || *e != '\0')
        return -1;

    if (errno == ERANGE || v < INT_MIN || v > INT_MAX)
        return -2;

    *out = (int)v;

    return rk_ok;
}

static rk_s32 mpp_sys_venc(MppSysSrv *srv, const char **argv, void *ctx)
{
    const char *cmd = NULL;
    MppSysCmdInfo *sub = NULL;
    MpiImpl *impl;
    MpiImpl *tmp;
    rk_s32 next = 0;
    rk_s32 ctx_id = -1;
    (void)ctx;

    if (argv[1]) {
        rk_s32 ret = safe_atoi(argv[1], &ctx_id);

        if (ret == 0 && ctx_id >= 0)
            next++;
        else
            ctx_id = -1;
    }

    if (argv[next + 1]) {
        MppSysCmdInfo **cmds = &venc_cmds.sub_cmd[0];
        rk_s32 j = 0;

        cmd = argv[next + 1];

        while (*cmds) {
            MppSysCmdInfo *t = *cmds;
            bool match = !strcmp(cmd, t->name);

            sys_dbg_args("match %d %-8s -> %s", j,
                         t->name, match ? "yes" : "no");

            if (match) {
                sub = t;
                break;
            }

            cmds++;
        }

        if (sub != NULL)
            next++;
    }

    if (sub == NULL)
        return next;

    mpp_sthd_lock(srv->thd);

    list_for_each_entry_safe(impl, tmp, &srv->list, MpiImpl, list) {
        if (impl->type != MPP_CTX_ENC)
            continue;

        if (ctx_id >= 0 && impl->ctx_id != ctx_id)
            continue;

        sub->cmd(srv, &argv[next + 1], impl);
    }

    mpp_sthd_unlock(srv->thd);

    return next;
}

static rk_s32 mpp_sys_venc_dump_cfg(MppSysSrv *srv, const char **argv, void *ctx)
{
    MpiImpl *impl = (MpiImpl *)ctx;
    Mpp *mpp = impl->ctx;
    MppEncCfg cfg;
    FILE *fp;

    if (NULL == mpp->mEnc) {
        mpp_loge("venc %d dump cfg but not init", impl->ctx_id);
        return 0;
    }

    cfg = mpp_enc_to_cfg(mpp->mEnc);
    if (NULL == cfg) {
        mpp_loge("venc %d dump cfg but get cfg failed", impl->ctx_id);
        return 0;
    }

    {
        char file_name[128];
        char *buf = NULL;

        snprintf(file_name, sizeof(file_name) - 1, "%s/mpp-%d-%d.json",
                 argv[0], srv->pid, impl->ctx_id);

        fp = fopen(file_name, "w+b");
        if (NULL == fp) {
            mpp_loge("venc %d dump cfg but open file %s failed %s\n",
                     impl->ctx_id, file_name, strerror(errno));
            return 0;
        }

        mpp_enc_cfg_extract(cfg, MPP_CFG_STR_FMT_JSON, &buf);
        if (buf) {
            rk_s32 len = strlen(buf);
            rk_s32 wr_nr = 0;

            wr_nr = fwrite(buf, 1, len, fp);
            if (wr_nr != len) {
                mpp_loge("venc %d write cfg to file %s failed, written %d/%d\n",
                         impl->ctx_id, file_name, wr_nr, len);
            }

            mpp_free(buf);
        }

        fclose(fp);
    }

    return 1;
}

static rk_s32 mpp_sys_venc_show_cfg(MppSysSrv *srv, const char **argv, void *ctx)
{
    MpiImpl *impl = (MpiImpl *)ctx;
    Mpp *mpp = impl->ctx;
    (void)srv;
    (void)argv;

    mpp_logi("venc %d dump cfg:", impl->ctx_id);

    if (mpp->mEnc) {
        MppEncCfg cfg = mpp_enc_to_cfg(mpp->mEnc);

        if (cfg) {
            char *buf = NULL;

            mpp_enc_cfg_extract(cfg, MPP_CFG_STR_FMT_LOG, &buf);
            if (buf) {
                mpp_cfg_print_string(buf);
                mpp_free(buf);
            }
        }
    }

    return 0;
}


static rk_s32 mpp_sys_ctrl(rk_s32 pid, char *cmd, rk_s32 n)
{
    MppSysSrv *srv = get_srv_sys();
    MppSysCmdInfo **cmds = NULL;
    const char *argv[32];
    rk_s32 argc;
    rk_s32 i;

    sys_dbg_cmd("mpp-%d [cmd]: (%d) %s", pid, strlen(cmd), cmd);

    if (!srv || !srv->thd)
        return rk_nok;

    argc = split_line(cmd, n, argv, 32);

    sys_dbg_args("argc %d", argc);
    for (i = 0; i < argc; i++)
        sys_dbg_args("argv[%d]: %s", i, argv[i]);

    for (i = 0; i < argc; i++) {
        rk_s32 j = 0;

        if (!argv[i])
            break;

        sys_dbg_args("argv[%d]: %s", i, argv[i]);

        cmds = &sys_cmds[0];

        while (*cmds) {
            MppSysCmdInfo *info = *cmds;
            bool match = !strcmp(argv[i], info->name);

            sys_dbg_args("match %d %-8s -> %s", j,
                         info->name, match ? "yes" : "no");

            if (match) {
                i += info->cmd(srv, &argv[i], NULL);
                break;
            }

            cmds++;
            j++;
        }
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
    rk_s32 pid = ctx->pid;

    snprintf(name, sizeof(name) - 1, "/tmp/mpp-%d-cmd", pid);
    do {
        if (0 != mkfifo(name, 0600)) {
            sys_dbg_flow("pid %d mkfifo %s failed for %s\n",
                         pid, name, strerror(errno));
            break;
        }

        fifo_fd = open(name, O_RDWR | O_CLOEXEC);
        if (fifo_fd < 0) {
            sys_dbg_flow("pid %d open fifo %s failed for %s\n",
                         pid, name, strerror(errno));
            break;
        }

        /* add epoll and exit efd */
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd < 0) {
            sys_dbg_flow("pid %d create epoll failed for %s\n",
                         pid, strerror(errno));
            break;
        }

        sys_dbg_flow("pid %d create sys cmd fifo %s fd %d\n",
                     pid, name, fifo_fd);

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
                    cmd_buf[n] = '\0';
                    mpp_sys_ctrl(pid, cmd_buf, n);
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

    sys_dbg_flow("pid %d sys srv finish\n", pid);

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
    srv->pid = getpid();

    /* setup external command fifo and system service thread */
    if (mpp_sys_debug != 0) {
        MppSThd sys_thd = NULL;
        char name[64];
        rk_s32 exit_efd;
        rk_s32 pid = srv->pid;

        mpp_sys_debug |= SYS_DBG_EN;

        /* create system service thread and its exit event fd */
        exit_efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

        if (exit_efd < 0) {
            sys_dbg_flow("pid %d create exit event fd failed ret %d %s\n",
                         pid, exit_efd, strerror(errno));
            return;
        }

        srv->exit_efd = exit_efd;
        snprintf(name, sizeof(name) - 1, "mpp-%d-sys-srv", pid);
        sys_thd = mpp_sthd_get(name);
        if (sys_thd) {
            mpp_sthd_setup(sys_thd, mpp_sys_srv, srv);
            mpp_sthd_start(sys_thd);
            srv->thd = sys_thd;
        } else {
            sys_dbg_flow("pid %d create sys srv thread failed\n", pid);
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
        if (ret != sizeof(exit_val)) {
            mpp_loge("write to exit_efd failed, ret %zd expected %zu\n",
                     ret, sizeof(exit_val));
        }

        /* wait for system service to quit */
        mpp_sthd_stop_sync(srv->thd);
        mpp_sthd_put(srv->thd);
        srv->thd = NULL;
    }

    mpp_free(srv);
}

MPP_SINGLETON(MPP_SGLN_SYS, mpp_sys, mpp_sys_init, mpp_sys_deinit)
