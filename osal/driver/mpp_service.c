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

#define MODULE_TAG "mpp_serivce"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_env.h"
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
    {   MPP_CMD_SEND_BASE,      "query_cmd",    },
    {   MPP_CMD_POLL_BASE,      "init_cmd",     },
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
    if (MPP_OK == mpp_service_check_cmd_valid(MPP_CMD_SET_RCB_INFO, p->cap))
        p->support_set_rcb_info = 1;

    /* default server fd is the opened client fd */
    p->server = p->client;
    p->batch_io = 0;
    p->serv_ctx = NULL;
    p->dev_cb   = NULL;

    return ret;
}

MPP_RET mpp_service_deinit(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (p->batch_io)
        mpp_server_detach(p);

    if (p->client)
        close(p->client);

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

MPP_RET mpp_service_set_cb_ctx(void *ctx, MppCbCtx *cb_ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    p->dev_cb = cb_ctx;

    return MPP_OK;
}

MPP_RET mpp_service_reg_wr(void *ctx, MppDevRegWrCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (!p->req_cnt)
        memset(p->reqs, 0, sizeof(p->reqs));

    MppReqV1 *mpp_req = &p->reqs[p->req_cnt];

    mpp_req->cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req->flag = 0;
    mpp_req->size = cfg->size;
    mpp_req->offset = cfg->offset;
    mpp_req->data_ptr = REQ_DATA_PTR(cfg->reg);
    p->req_cnt++;

    return MPP_OK;
}

MPP_RET mpp_service_reg_rd(void *ctx, MppDevRegRdCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (!p->req_cnt)
        memset(p->reqs, 0, sizeof(p->reqs));

    MppReqV1 *mpp_req = &p->reqs[p->req_cnt];

    mpp_req->cmd = MPP_CMD_SET_REG_READ;
    mpp_req->flag = 0;
    mpp_req->size = cfg->size;
    mpp_req->offset = cfg->offset;
    mpp_req->data_ptr = REQ_DATA_PTR(cfg->reg);
    p->req_cnt++;

    return MPP_OK;
}

MPP_RET mpp_service_reg_offset(void *ctx, MppDevRegOffsetCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    RK_S32 i;
    RegOffsetInfo *info;

    if (!cfg->offset)
        return MPP_OK;

    if (p->reg_offset_count >= MAX_REG_OFFSET) {
        mpp_err_f("reach max offset definition\n", MAX_REG_OFFSET);
        return MPP_NOK;
    }

    for (i = 0; i < p->reg_offset_count; i++) {
        info = &p->reg_offset_info[i];

        if (info->reg_idx == cfg->reg_idx) {
            mpp_err_f("reg[%d] offset has been set, cover old %d -> %d\n",
                      info->reg_idx, info->offset, cfg->offset);
            info->offset = cfg->offset;
            return MPP_OK;
        }
    }

    info = &p->reg_offset_info[p->reg_offset_count++];

    info->reg_idx = cfg->reg_idx;
    info->offset = cfg->offset;

    return MPP_OK;
}

MPP_RET mpp_service_rcb_info(void *ctx, MppDevRcbInfoCfg *cfg)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    RK_U32 rcb_info_disable = 0;

    mpp_env_get_u32("disable_rcb_info", &rcb_info_disable, 0);
    if (rcb_info_disable)
        return MPP_OK;

    if (!p->support_set_rcb_info)
        return MPP_OK;

    if (p->rcb_count >= MAX_RCB_OFFSET) {
        mpp_err_f("reach max offset definition\n", MAX_RCB_OFFSET);
        return MPP_NOK;
    }

    RcbInfo *info = &p->rcb_info[p->rcb_count++];

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

MPP_RET mpp_service_cmd_send(void *ctx)
{
    MPP_RET ret = MPP_OK;
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (p->req_cnt <= 0 || p->req_cnt > MAX_REQ_NUM) {
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
            ret = mpp_service_ioctl_request(p->server, &mpp_req);
            if (ret)
                p->support_set_info = 0;
        }
        p->info_count = 0;
    }

    /* set fd trans info if needed */
    if (p->reg_offset_count) {
        MppReqV1 *mpp_req = &p->reqs[p->req_cnt];

        mpp_req->cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
        mpp_req->flag = MPP_FLAGS_REG_OFFSET_ALONE;
        mpp_req->size = p->reg_offset_count * sizeof(p->reg_offset_info[0]);
        mpp_req->offset = 0;
        mpp_req->data_ptr = REQ_DATA_PTR(&p->reg_offset_info[0]);
        p->req_cnt++;
    }

    /* set rcb offst info if needed */
    if (p->rcb_count) {
        MppReqV1 *mpp_req = &p->reqs[p->req_cnt];

        mpp_req->cmd = MPP_CMD_SET_RCB_INFO;
        mpp_req->flag = 0;
        mpp_req->size = p->rcb_count * sizeof(p->rcb_info[0]);
        mpp_req->offset = 0;
        mpp_req->data_ptr = REQ_DATA_PTR(&p->rcb_info[0]);
        p->req_cnt++;
    }

    /* setup flag for multi message request */
    if (p->req_cnt > 1) {
        RK_S32 i;

        for (i = 0; i < p->req_cnt; i++)
            p->reqs[i].flag |= MPP_FLAGS_MULTI_MSG;
    }
    p->reqs[p->req_cnt - 1].flag |=  MPP_FLAGS_LAST_MSG;

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
    p->rcb_count = 0;
    return ret;
}

MPP_RET mpp_service_cmd_poll(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MppReqV1 dev_req;
    MPP_RET ret = MPP_OK;

    if (p->batch_io) {
        ret = mpp_server_wait_task(ctx, 0);
    } else {
        memset(&dev_req, 0, sizeof(dev_req));
        dev_req.cmd = MPP_CMD_POLL_HW_FINISH;
        dev_req.flag |= MPP_FLAGS_LAST_MSG;

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
    mpp_service_set_cb_ctx,
    mpp_service_reg_wr,
    mpp_service_reg_rd,
    mpp_service_reg_offset,
    mpp_service_rcb_info,
    mpp_service_set_info,
    mpp_service_cmd_send,
    mpp_service_cmd_poll,
};
