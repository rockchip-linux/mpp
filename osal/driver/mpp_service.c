/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_serivce"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "osal_2str.h"

#include "mpp_device_debug.h"
#include "mpp_service_api.h"
#include "mpp_service_impl.h"
#include "mpp_server.h"

typedef struct MppServiceQueryCfg_t {
    RK_U32      cmd_butt;
    const char  *name;
} MppServiceQueryCfg;

static const MppServiceQueryCfg query_cfg[] = {
    {   MPP_CMD_QUERY_BASE,     "query_cmd",    },
    {   MPP_CMD_INIT_BASE,      "init_cmd",     },
    {   MPP_CMD_SEND_BASE,      "send_cmd",     },
    {   MPP_CMD_POLL_BASE,      "poll_cmd",     },
    {   MPP_CMD_CONTROL_BASE,   "control_cmd",  },
};

static const RK_U32 query_count = MPP_ARRAY_ELEMS(query_cfg);

const char *mpp_get_mpp_service_name(void)
{
    static const char *mpp_service_name = NULL;
    static const char *mpp_service_dev[] = {
        "/dev/mpp_service",
        "/dev/mpp-service",
    };

    if (mpp_service_name)
        return mpp_service_name;

    if (!access(mpp_service_dev[0], F_OK | R_OK | W_OK)) {
        mpp_service_name = mpp_service_dev[0];
    } else if (!access(mpp_service_dev[1], F_OK | R_OK | W_OK))
        mpp_service_name = mpp_service_dev[1];

    return mpp_service_name;
}

RK_S32 mpp_service_ioctl(RK_S32 fd, RK_U32 cmd, RK_U32 size, void *param)
{
    MppReqV1 mpp_req;

    memset(&mpp_req, 0, sizeof(mpp_req));

    mpp_req.cmd = cmd;
    mpp_req.flag = 0;
    mpp_req.size = size;
    mpp_req.offset = 0;
    mpp_req.data_ptr = REQ_DATA_PTR(param);

    return (RK_S32)ioctl(fd, MPP_IOC_CFG_V1, &mpp_req);
}

RK_S32 mpp_service_ioctl_request(RK_S32 fd, MppReqV1 *req)
{
    return (RK_S32)ioctl(fd, MPP_IOC_CFG_V1, req);
}

MPP_RET mpp_service_check_cmd_valid(RK_U32 cmd, const MppServiceCmdCap *cap)
{
    RK_U32 found = 0;

    if (cap->support_cmd > 0) {
        found = (cmd < cap->query_cmd) ? 1 : 0;
        found = (cmd >= MPP_CMD_INIT_BASE && cmd < cap->init_cmd)    ? 1 : found;
        found = (cmd >= MPP_CMD_SEND_BASE && cmd < cap->send_cmd)    ? 1 : found;
        found = (cmd >= MPP_CMD_POLL_BASE && cmd < cap->poll_cmd)    ? 1 : found;
        found = (cmd >= MPP_CMD_CONTROL_BASE && cmd < cap->ctrl_cmd) ? 1 : found;
    } else {
        /* old kernel before support_cmd query is valid */
        found = (cmd >= MPP_CMD_INIT_BASE && cmd <= MPP_CMD_INIT_TRANS_TABLE)    ? 1 : found;
        found = (cmd >= MPP_CMD_SEND_BASE && cmd <= MPP_CMD_SET_REG_ADDR_OFFSET) ? 1 : found;
        found = (cmd >= MPP_CMD_POLL_BASE && cmd <= MPP_CMD_POLL_HW_FINISH)      ? 1 : found;
        found = (cmd >= MPP_CMD_CONTROL_BASE && cmd <= MPP_CMD_RELEASE_FD)       ? 1 : found;
    }

    return found ? MPP_OK : MPP_NOK;
}

void check_mpp_service_cap(RK_U32 *codec_type, RK_U32 *hw_ids, MppServiceCmdCap *cap)
{
    MppReqV1 mpp_req;
    RK_S32 fd = -1;
    RK_S32 ret = 0;
    RK_U32 *cmd_butt = &cap->query_cmd;;
    RK_U32 hw_support = 0;
    RK_U32 val;
    RK_U32 i;

    /* for device check on startup */
    mpp_env_get_u32("mpp_device_debug", &mpp_device_debug, 0);

    *codec_type = 0;
    memset(hw_ids, 0, sizeof(RK_U32) * 32);

    /* check hw_support flag for valid client type */
    fd = open(mpp_get_mpp_service_name(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        mpp_err("open mpp_service to check cmd capability failed\n");
        memset(cap, 0, sizeof(*cap));
        return ;
    }
    ret = mpp_service_ioctl(fd, MPP_CMD_PROBE_HW_SUPPORT, 0, &hw_support);
    if (!ret) {
        mpp_dev_dbg_probe("vcodec_support %08x\n", hw_support);
        *codec_type = hw_support;
    }
    cap->support_cmd = !access("/proc/mpp_service/supports-cmd", F_OK) ||
                       !access("/proc/mpp_service/support_cmd", F_OK);
    if (cap->support_cmd) {
        for (i = 0; i < query_count; i++, cmd_butt++) {
            const MppServiceQueryCfg *cfg = &query_cfg[i];

            memset(&mpp_req, 0, sizeof(mpp_req));

            val = cfg->cmd_butt;
            mpp_req.cmd = MPP_CMD_QUERY_CMD_SUPPORT;
            mpp_req.data_ptr = REQ_DATA_PTR(&val);

            ret = (RK_S32)ioctl(fd, MPP_IOC_CFG_V1, &mpp_req);
            if (ret)
                mpp_err_f("query %-11s support error %s.\n", cfg->name, strerror(errno));
            else {
                *cmd_butt = val;
                mpp_dev_dbg_probe("query %-11s support %04x\n", cfg->name, val);
            }
        }
    }
    close(fd);

    /* check each valid client type for hw_id */
    /* kernel need to set client type then get hw_id */
    for (i = 0; i < 32; i++) {
        if (hw_support & (1 << i)) {
            val = i;

            fd = open(mpp_get_mpp_service_name(), O_RDWR | O_CLOEXEC);
            if (fd < 0) {
                mpp_err("open mpp_service to check cmd capability failed\n");
                break;
            }
            /* set client type first */
            ret = mpp_service_ioctl(fd, MPP_CMD_INIT_CLIENT_TYPE, sizeof(val), &val);
            if (ret) {
                mpp_err("check valid client type %d failed\n", i);
            } else {
                /* then get hw_id */
                ret = mpp_service_ioctl(fd, MPP_CMD_QUERY_HW_ID, sizeof(val), &val);
                if (!ret) {
                    mpp_dev_dbg_probe("client %-10s hw_id %08x\n",
                                      strof_client_type((MppClientType)i), val);
                    hw_ids[i] = val;
                } else
                    mpp_err("check valid client %-10s for hw_id failed\n",
                            strof_client_type((MppClientType)i));
            }
            close(fd);
        }
    }
}

MppReqV1 *mpp_service_next_req(MppDevMppService *p)
{
    MppReqV1 *mpp_req = NULL;

    if (p->req_cnt >= p->req_max) {
        mpp_dev_dbg_msg("enlarge request count %d -> %d\n",
                        p->req_max, p->req_max * 2);
        p->reqs = mpp_realloc(p->reqs, MppReqV1, p->req_max * 2);
        if (NULL == p->reqs) {
            mpp_err_f("failed to enlarge request buffer\n");
            return NULL;
        }

        p->req_max *= 2;
    }

    mpp_req = &p->reqs[p->req_cnt++];

    return mpp_req;
}

RegOffsetInfo *mpp_service_next_reg_offset(MppDevMppService *p)
{
    RegOffsetInfo *info = NULL;

    if (p->reg_offset_count + p->reg_offset_pos >= p->reg_offset_max) {
        RegOffsetInfo *orig = p->reg_offset_info;

        mpp_dev_dbg_msg("enlarge reg offset count %d -> %d\n",
                        p->reg_offset_max, p->reg_offset_max * 2);
        p->reg_offset_info = mpp_realloc(p->reg_offset_info, RegOffsetInfo, p->reg_offset_max * 2);
        if (NULL == p->reg_offset_info) {
            mpp_err_f("failed to enlarge request buffer\n");
            return NULL;
        }
        if (orig != p->reg_offset_info)
            mpp_logw_f("enlarge reg offset buffer and get different pointer\n");

        p->reg_offset_max *= 2;
    }

    info = &p->reg_offset_info[p->reg_offset_count + p->reg_offset_pos];
    mpp_dev_dbg_msg("reg offset %d : %d\n", p->reg_offset_pos, p->reg_offset_count);
    p->reg_offset_count++;

    return info;
}


RcbInfo *mpp_service_next_rcb_info(MppDevMppService *p)
{
    RcbInfo *info = NULL;

    if (p->rcb_count + p->rcb_pos >= p->rcb_max) {
        mpp_dev_dbg_msg("enlarge rcb info count %d -> %d\n",
                        p->rcb_max, p->rcb_max * 2);
        p->rcb_info = mpp_realloc(p->rcb_info, RcbInfo, p->rcb_max * 2);
        if (NULL == p->rcb_info) {
            mpp_err_f("failed to enlarge request buffer\n");
            return NULL;
        }

        p->rcb_max *= 2;
    }

    info = &p->rcb_info[p->rcb_count + p->rcb_pos];
    mpp_dev_dbg_msg("rcb info %d : %d\n", p->rcb_pos, p->rcb_count);
    p->rcb_count++;

    return info;
}

static MPP_RET mpp_service_ioc_attach_fd(MppDevBufMapNode *node)
{
    MppReqV1 mpp_req;
    RK_S32 fd = node->buf_fd;
    MPP_RET ret;

    mpp_req.cmd = MPP_CMD_TRANS_FD_TO_IOVA;
    mpp_req.flag = MPP_FLAGS_LAST_MSG;
    mpp_req.size = sizeof(RK_U32);
    mpp_req.offset = 0;
    mpp_req.data_ptr = REQ_DATA_PTR(&fd);

    ret = mpp_service_ioctl_request(node->dev_fd, &mpp_req);
    if (ret) {
        mpp_err_f("failed ret %d errno %d %s\n", ret, errno, strerror(errno));
        node->iova = (RK_U32)(-1);
    } else {
        node->iova = (RK_U32)fd;
    }

    return ret;
}

static MPP_RET mpp_service_ioc_detach_fd(MppDevBufMapNode *node)
{
    RK_S32 fd = node->buf_fd;
    MppReqV1 mpp_req;
    MPP_RET ret;

    mpp_req.cmd = MPP_CMD_RELEASE_FD;
    mpp_req.flag = MPP_FLAGS_LAST_MSG;
    mpp_req.size = sizeof(RK_U32);
    mpp_req.offset = 0;
    mpp_req.data_ptr = REQ_DATA_PTR(&fd);

    ret = mpp_service_ioctl_request(node->dev_fd, &mpp_req);
    if (ret) {
        mpp_err_f("failed ret %d errno %d %s\n", ret, errno, strerror(errno));
    }
    node->iova = (RK_U32)(-1);
    return ret;
}

MPP_RET mpp_service_init(void *ctx, MppClientType type)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MPP_RET ret = MPP_NOK;

    p->cap = mpp_get_mpp_service_cmd_cap();
    p->client = open(mpp_get_mpp_service_name(), O_RDWR | O_CLOEXEC);
    if (p->client < 0) {
        mpp_err("open mpp_service failed\n");
        return ret;
    }

    /* set client type first */
    ret = mpp_service_ioctl(p->client, MPP_CMD_INIT_CLIENT_TYPE, sizeof(type), &type);
    if (ret)
        mpp_err("set client type %d failed\n", type);

    mpp_assert(p->cap);
    if (MPP_OK == mpp_service_check_cmd_valid(MPP_CMD_SEND_CODEC_INFO, p->cap))
        p->support_set_info = 1;
    if (MPP_OK == mpp_service_check_cmd_valid(MPP_CMD_SET_RCB_INFO, p->cap)) {
        RK_U32 disable_rcb_info = 0;

        mpp_env_get_u32("disable_rcb_info", &disable_rcb_info, 0);
        p->support_set_rcb_info = !disable_rcb_info;
    }
    if (MPP_OK == mpp_service_check_cmd_valid(MPP_CMD_POLL_HW_IRQ, p->cap))
        p->support_hw_irq = 1;

    /* default server fd is the opened client fd */
    p->client_type = type;
    p->server = p->client;
    p->batch_io = 0;
    p->serv_ctx = NULL;
    p->dev_cb   = NULL;

    p->bat_cmd.flag = 0;
    p->bat_cmd.client = p->client;
    p->bat_cmd.ret = 0;

    p->req_max = MAX_REQ_NUM;
    p->reqs = mpp_malloc(MppReqV1, p->req_max);
    if (NULL == p->reqs) {
        mpp_err("create request buffer failed\n");
        ret = MPP_ERR_MALLOC;
    }

    p->reg_offset_max = MAX_REG_OFFSET;
    p->reg_offset_info = mpp_malloc(RegOffsetInfo, p->reg_offset_max);
    if (NULL == p->reg_offset_info) {
        mpp_err("create register offset buffer failed\n");
        ret = MPP_ERR_MALLOC;
    }
    p->reg_offset_pos = 0;
    p->reg_offset_count = 0;

    p->rcb_max = MAX_RCB_OFFSET;
    p->rcb_info = mpp_malloc(RcbInfo, p->rcb_max);
    if (NULL == p->rcb_info) {
        mpp_err("create rcb info buffer failed\n");
        ret = MPP_ERR_MALLOC;
    }
    p->rcb_pos = 0;
    p->rcb_count = 0;

    INIT_LIST_HEAD(&p->list_bufs);
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&p->lock_bufs, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    return ret;
}

MPP_RET mpp_service_deinit(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MppDevBufMapNode *pos, *n;

    pthread_mutex_lock(&p->lock_bufs);
    list_for_each_entry_safe(pos, n, &p->list_bufs, MppDevBufMapNode, list_dev) {
        pthread_mutex_t *lock_buf = pos->lock_buf;

        mpp_assert(pos->lock_buf && pos->lock_dev);
        mpp_assert(pos->lock_dev == &p->lock_bufs);

        pthread_mutex_lock(lock_buf);

        list_del_init(&pos->list_dev);
        list_del_init(&pos->list_buf);
        pos->lock_buf = NULL;
        pos->lock_dev = NULL;
        mpp_service_ioc_detach_fd(pos);
        mpp_mem_pool_put_f(pos->pool, pos);

        pthread_mutex_unlock(lock_buf);
    }
    pthread_mutex_unlock(&p->lock_bufs);
    pthread_mutex_destroy(&p->lock_bufs);

    if (p->batch_io)
        mpp_server_detach(p);

    if (p->client)
        close(p->client);

    MPP_FREE(p->reqs);
    MPP_FREE(p->reg_offset_info);
    MPP_FREE(p->rcb_info);

    return MPP_OK;
}

MPP_RET mpp_service_attach(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (p->req_cnt) {
        mpp_err_f("can not switch on bat mode when service working\n");
        return MPP_NOK;
    }

    if (!p->batch_io)
        mpp_server_attach(p);

    return MPP_OK;
}

MPP_RET mpp_service_detach(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (p->req_cnt) {
        mpp_err_f("can not switch off bat mode when service working\n");
        return MPP_NOK;
    }

    if (p->batch_io)
        mpp_server_detach(p);

    return MPP_OK;
}

MPP_RET mpp_service_delimit(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MppReqV1 *mpp_req = NULL;

    /* set fd trans info if needed */
    if (p->reg_offset_count) {
        mpp_req = mpp_service_next_req(p);

        mpp_req->cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
        mpp_req->flag = MPP_FLAGS_REG_OFFSET_ALONE;
        mpp_req->size = (p->reg_offset_count) * sizeof(RegOffsetInfo);
        mpp_req->offset = 0;
        mpp_req->data_ptr = REQ_DATA_PTR(&p->reg_offset_info[p->reg_offset_pos]);
        p->reg_offset_pos += p->reg_offset_count;
        p->reg_offset_count = 0;
    }

    /* set rcb offst info if needed */
    if (p->rcb_count) {
        mpp_req = mpp_service_next_req(p);

        mpp_req->cmd = MPP_CMD_SET_RCB_INFO;
        mpp_req->flag = 0;
        mpp_req->size = p->rcb_count * sizeof(RcbInfo);
        mpp_req->offset = 0;
        mpp_req->data_ptr = REQ_DATA_PTR(&p->rcb_info[p->rcb_pos]);
        p->rcb_pos += p->rcb_count;
        p->rcb_count = 0;
    }

    mpp_req = mpp_service_next_req(p);
    mpp_req->cmd = MPP_CMD_SET_SESSION_FD;
    mpp_req->flag = MPP_FLAGS_MULTI_MSG;
    mpp_req->offset = 0;
    mpp_req->size = sizeof(p->bat_cmd);
    mpp_req->data_ptr = REQ_DATA_PTR(&p->bat_cmd);

    return MPP_OK;
}

MPP_RET mpp_service_set_cb_ctx(void *ctx, MppCbCtx *cb_ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    p->dev_cb = cb_ctx;

    return MPP_OK;
}

MPP_RET mpp_service_reg_wr(void *ctx, MppDevRegWrCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MppReqV1 *mpp_req = mpp_service_next_req(p);

    mpp_req->cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req->flag = 0;
    mpp_req->size = cfg->size;
    mpp_req->offset = cfg->offset;
    mpp_req->data_ptr = REQ_DATA_PTR(cfg->reg);

    return MPP_OK;
}

MPP_RET mpp_service_reg_rd(void *ctx, MppDevRegRdCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MppReqV1 *mpp_req = mpp_service_next_req(p);

    mpp_req->cmd = MPP_CMD_SET_REG_READ;
    mpp_req->flag = 0;
    mpp_req->size = cfg->size;
    mpp_req->offset = cfg->offset;
    mpp_req->data_ptr = REQ_DATA_PTR(cfg->reg);

    return MPP_OK;
}

MPP_RET mpp_service_reg_offset(void *ctx, MppDevRegOffsetCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    RegOffsetInfo *info;
    RK_S32 i;

    if (!cfg->offset)
        return MPP_OK;

    if (p->reg_offset_count >= MAX_REG_OFFSET) {
        mpp_err_f("reach max offset definition\n", MAX_REG_OFFSET);
        return MPP_NOK;
    }

    for (i = 0; i < p->reg_offset_count; i++) {
        info = &p->reg_offset_info[p->reg_offset_pos + i];

        if (info->reg_idx == cfg->reg_idx) {
            mpp_err_f("reg[%d] offset has been set, cover old %d -> %d\n",
                      info->reg_idx, info->offset, cfg->offset);
            info->offset = cfg->offset;
            return MPP_OK;
        }
    }

    info = mpp_service_next_reg_offset(p);
    info->reg_idx = cfg->reg_idx;
    info->offset = cfg->offset;

    return MPP_OK;
}

MPP_RET mpp_service_reg_offsets(void *ctx, MppDevRegOffCfgs *cfgs)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    RegOffsetInfo *info;
    RK_S32 i;

    if (cfgs->count <= 0)
        return MPP_OK;

    if (p->reg_offset_count >= MAX_REG_OFFSET ||
        p->reg_offset_count + cfgs->count >= MAX_REG_OFFSET) {
        mpp_err_f("reach max offset definition\n", MAX_REG_OFFSET);
        return MPP_NOK;
    }

    for (i = 0; i < cfgs->count; i++) {
        MppDevRegOffsetCfg *cfg = &cfgs->cfgs[i];
        RK_S32 j;

        for (j = 0; j < p->reg_offset_count; j++) {
            info = &p->reg_offset_info[p->reg_offset_pos + j];

            if (info->reg_idx == cfg->reg_idx) {
                mpp_err_f("reg[%d] offset has been set, cover old %d -> %d\n",
                          info->reg_idx, info->offset, cfg->offset);
                info->offset = cfg->offset;
                continue;
            }
        }

        info = mpp_service_next_reg_offset(p);;
        info->reg_idx = cfg->reg_idx;
        info->offset = cfg->offset;
    }

    return MPP_OK;
}

MPP_RET mpp_service_rcb_info(void *ctx, MppDevRcbInfoCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (!p->support_set_rcb_info)
        return MPP_OK;

    if (p->rcb_count >= MAX_RCB_OFFSET) {
        mpp_err_f("reach max offset definition\n", MAX_RCB_OFFSET);
        return MPP_NOK;
    }

    RcbInfo *info = mpp_service_next_rcb_info(p);

    info->reg_idx = cfg->reg_idx;
    info->size = cfg->size;

    return MPP_OK;
}

MPP_RET mpp_service_set_info(void *ctx, MppDevInfoCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (!p->support_set_info)
        return MPP_OK;

    if (!p->info_count)
        memset(p->info, 0, sizeof(p->info));

    memcpy(&p->info[p->info_count], cfg, sizeof(MppDevInfoCfg));
    p->info_count++;

    return MPP_OK;
}

MPP_RET mpp_service_set_err_ref_hack(void *ctx, RK_U32 *enable)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MppReqV1 mpp_req;

    mpp_req.cmd = MPP_CMD_SET_ERR_REF_HACK;
    mpp_req.flag = MPP_FLAGS_LAST_MSG;
    mpp_req.size = sizeof(RK_U32);
    mpp_req.offset = 0;
    mpp_req.data_ptr = REQ_DATA_PTR(enable);

    return mpp_service_ioctl_request(p->client, &mpp_req);
}

MPP_RET mpp_service_lock_map(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    mpp_dev_dbg_buf("dev %d lock mapping\n", p->client);
    pthread_mutex_lock(&p->lock_bufs);
    return MPP_OK;
}

MPP_RET mpp_service_unlock_map(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    mpp_dev_dbg_buf("dev %d unlock mapping\n", p->client);
    pthread_mutex_unlock(&p->lock_bufs);
    return MPP_OK;
}

MPP_RET mpp_service_attach_fd(void *ctx, MppDevBufMapNode *node)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MPP_RET ret;

    mpp_assert(node->buffer);
    mpp_assert(node->lock_buf);
    mpp_assert(node->buf_fd >= 0);

    node->lock_dev = &p->lock_bufs;
    node->dev_fd = p->client;
    ret = mpp_service_ioc_attach_fd(node);
    if (ret) {
        node->lock_dev = NULL;
        node->dev_fd = -1;
        list_del_init(&node->list_dev);
    } else {
        list_add_tail(&node->list_dev, &p->list_bufs);
    }

    mpp_dev_dbg_buf("node %p dev %d attach fd %d iova %x\n",
                    node, node->dev_fd, node->buf_fd, node->iova);

    return ret;
}

MPP_RET mpp_service_detach_fd(void *ctx, MppDevBufMapNode *node)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MPP_RET ret;

    mpp_assert(node->buffer);
    mpp_assert(node->lock_buf);
    mpp_assert(node->buf_fd >= 0);
    mpp_assert(node->dev_fd >= 0);
    mpp_assert(node->lock_dev == &p->lock_bufs);

    mpp_dev_dbg_buf("node %p dev %d detach fd %d iova %x\n",
                    node, node->dev_fd, node->buf_fd, node->iova);

    ret = mpp_service_ioc_detach_fd(node);
    node->dev = NULL;
    node->dev_fd = -1;
    node->lock_dev = NULL;
    list_del_init(&node->list_dev);

    return ret;
}

MPP_RET mpp_service_cmd_send(void *ctx)
{
    MPP_RET ret = MPP_OK;
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (p->req_cnt <= 0 || p->req_cnt > p->req_max) {
        mpp_err_f("ctx %p invalid request count %d\n", ctx, p->req_cnt);
        return MPP_ERR_VALUE;
    }

    if (p->info_count) {
        if (p->support_set_info) {
            MppReqV1 mpp_req;

            mpp_req.cmd = MPP_CMD_SEND_CODEC_INFO;
            mpp_req.flag = MPP_FLAGS_LAST_MSG;
            mpp_req.size = p->info_count * sizeof(p->info[0]);
            mpp_req.offset = 0;
            mpp_req.data_ptr = REQ_DATA_PTR(p->info);

            ret = mpp_service_ioctl_request(p->client, &mpp_req);
            if (ret)
                p->support_set_info = 0;
        }
        p->info_count = 0;
    }

    /* set fd trans info if needed */
    if (p->reg_offset_count) {
        MppReqV1 *mpp_req = mpp_service_next_req(p);

        mpp_req->cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
        mpp_req->flag = MPP_FLAGS_REG_OFFSET_ALONE;
        mpp_req->size = (p->reg_offset_count) * sizeof(RegOffsetInfo);
        mpp_req->offset = 0;
        mpp_req->data_ptr = REQ_DATA_PTR(&p->reg_offset_info[p->reg_offset_pos]);
        p->reg_offset_pos += p->reg_offset_count;
    }

    /* set rcb offst info if needed */
    if (p->rcb_count) {
        MppReqV1 *mpp_req = mpp_service_next_req(p);

        mpp_req->cmd = MPP_CMD_SET_RCB_INFO;
        mpp_req->flag = 0;
        mpp_req->size = p->rcb_count * sizeof(RcbInfo);
        mpp_req->offset = 0;
        mpp_req->data_ptr = REQ_DATA_PTR(&p->rcb_info[p->rcb_pos]);
        p->rcb_pos += p->rcb_count;
    }

    /* setup flag for multi message request */
    if (p->req_cnt > 1) {
        RK_S32 i;

        for (i = 0; i < p->req_cnt; i++)
            p->reqs[i].flag |= MPP_FLAGS_MULTI_MSG;
    }
    p->reqs[p->req_cnt - 1].flag |=  MPP_FLAGS_LAST_MSG | MPP_FLAGS_REG_OFFSET_ALONE;

    if (p->batch_io) {
        ret = mpp_server_send_task(ctx);
        if (ret)
            mpp_err_f("send task to server ret %d\n", ret);
    } else {
        ret = mpp_service_ioctl_request(p->server, &p->reqs[0]);
        if (ret) {
            mpp_err_f("ioctl MPP_IOC_CFG_V1 failed ret %d errno %d %s\n",
                      ret, errno, strerror(errno));
            ret = errno;
        }
    }

    p->req_cnt = 0;
    p->reg_offset_count = 0;
    p->reg_offset_pos = 0;
    p->rcb_count = 0;
    p->rcb_pos = 0;
    p->rcb_count = 0;
    return ret;
}

MPP_RET mpp_service_cmd_poll(void *ctx, MppDevPollCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MPP_RET ret = MPP_OK;

    if (p->batch_io) {
        ret = mpp_server_wait_task(ctx, 0);
    } else {
        MppReqV1 dev_req;

        memset(&dev_req, 0, sizeof(dev_req));

        if (p->support_hw_irq && cfg) {
            dev_req.cmd = MPP_CMD_POLL_HW_IRQ;
            dev_req.flag |= MPP_FLAGS_LAST_MSG | MPP_FLAGS_REG_OFFSET_ALONE;

            dev_req.size = sizeof(*cfg) + cfg->count_max * sizeof(cfg->slice_info[0]);
            dev_req.offset = 0;
            dev_req.data_ptr = REQ_DATA_PTR(cfg);
        } else {
            dev_req.cmd = MPP_CMD_POLL_HW_FINISH;
            dev_req.flag |= MPP_FLAGS_LAST_MSG | MPP_FLAGS_REG_OFFSET_ALONE;

            if (cfg) {
                mpp_assert(cfg->count_max);
                if (cfg->count_max) {
                    cfg->count_ret = 1;
                    cfg->slice_info[0].val = 0;
                    cfg->slice_info[0].last = 1;
                }
            }
        }

        ret = mpp_service_ioctl_request(p->server, &dev_req);
        if (ret) {
            mpp_err_f("ioctl MPP_IOC_CFG_V1 failed ret %d errno %d %s\n",
                      ret, errno, strerror(errno));
            ret = errno;
        }
    }

    return ret;
}

const MppDevApi mpp_service_api = {
    "mpp_service",
    sizeof(MppDevMppService),
    mpp_service_init,
    mpp_service_deinit,
    mpp_service_attach,
    mpp_service_detach,
    mpp_service_delimit,
    mpp_service_set_cb_ctx,
    mpp_service_reg_wr,
    mpp_service_reg_rd,
    mpp_service_reg_offset,
    mpp_service_reg_offsets,
    mpp_service_rcb_info,
    mpp_service_set_info,
    mpp_service_set_err_ref_hack,
    mpp_service_lock_map,
    mpp_service_unlock_map,
    mpp_service_attach_fd,
    mpp_service_detach_fd,
    mpp_service_cmd_send,
    mpp_service_cmd_poll,
};
