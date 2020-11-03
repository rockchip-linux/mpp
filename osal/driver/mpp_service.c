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

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp_service.h"
#include "mpp_service_api.h"

#define MPP_SERVICE_DBG_FUNC                (0x00000001)
#define MPP_SERVICE_DBG_PROBE               (0x00000002)

#define mpp_srv_dbg(flag, fmt, ...)         _mpp_dbg(mpp_service_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_srv_dbg_f(flag, fmt, ...)       _mpp_dbg_f(mpp_service_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_src_dbg_func(fmt, ...)          mpp_srv_dbg_f(MPP_SERVICE_DBG_FUNC, fmt, ## __VA_ARGS__)
#define mpp_src_dbg_probe(fmt, ...)         mpp_srv_dbg(MPP_SERVICE_DBG_PROBE, fmt, ## __VA_ARGS__)

static RK_U32 mpp_service_debug = 0;

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

static const char *mpp_service_hw_name[] = {
    /* 0 ~ 3 */
    /* VPU_CLIENT_VDPU1         */  "vdpu1",
    /* VPU_CLIENT_VDPU2         */  "vdpu2",
    /* VPU_CLIENT_VDPU1_PP      */  "vdpu1_pp",
    /* VPU_CLIENT_VDPU2_PP      */  "vdpu2_pp",
    /* 4 ~ 7 */
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* 8 ~ 11 */
    /* VPU_CLIENT_HEVC_DEC      */  "rkhevc",
    /* VPU_CLIENT_RKVDEC        */  "rkvdec",
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* 12 ~ 15 */
    /* VPU_CLIENT_AVSPLUS_DEC   */  "avsd",
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* 16 ~ 19 */
    /* VPU_CLIENT_RKVENC        */  "rkvenc",
    /* VPU_CLIENT_VEPU1         */  "vepu1",
    /* VPU_CLIENT_VEPU2         */  "vepu2",
    /* VPU_CLIENT_VEPU2_LITE    */  "vepu2_lite",
    /* 20 ~ 23 */
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* 24 ~ 27 */
    /* VPU_CLIENT_VEPU22        */  "vepu22",
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* 28 ~ 31 */
    /* IEP_CLIENT_TYPE          */  "iep",
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
    /* VPU_CLIENT_BUTT          */  NULL,
};

static const char *mpp_service_name = "/dev/mpp_service";

static RK_S32 mpp_service_ioctl(RK_S32 fd, RK_U32 cmd, RK_U32 size, void *param)
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

static RK_S32 mpp_service_ioctl_request(RK_S32 fd, MppReqV1 *req)
{
    return (RK_S32)ioctl(fd, MPP_IOC_CFG_V1, req);
}

void check_mpp_service_cap(RK_U32 *codec_type, RK_U32 *hw_ids, MppServiceCmdCap *cap)
{
    MppReqV1 mpp_req;
    RK_S32 fd = -1;
    RK_S32 ret = 0;
    RK_U32 *cmd_butt = &cap->query_cmd;;
    RK_U32 val;
    RK_U32 i;

    mpp_env_get_u32("mpp_service_debug", &mpp_service_debug, 0);

    *codec_type = 0;
    memset(hw_ids, 0, sizeof(RK_U32) * 32);

    fd = open(mpp_service_name, O_RDWR);
    if (fd < 0) {
        mpp_err("open mpp_service to check cmd capability failed\n");
        memset(cap, 0, sizeof(*cap));
        return ;
    }

    /* check hw_support flag for valid client type */
    ret = mpp_service_ioctl(fd, MPP_CMD_PROBE_HW_SUPPORT, 0, &val);
    if (!ret) {
        mpp_src_dbg_probe("vcodec_support %08x\n", val);
        *codec_type = val;
    }

    /* check each valid client type for hw_id */
    {
        RK_U32 hw_support = val;

        /* find first valid client type */
        for (i = 0; i < 32; i++)
            if (hw_support & (1 << i)) {
                val = i;
                break;
            }

        /* for compatible check hw_id read mode first */
        ret = mpp_service_ioctl(fd, MPP_CMD_QUERY_HW_ID, sizeof(val), &val);
        if (!ret) {
            /* kernel support hw_id check by input client type */
            for (i = 0; i < 32; i++) {
                if (hw_support & (1 << i)) {
                    val = i;

                    /* send client type and get hw_id */
                    ret = mpp_service_ioctl(fd, MPP_CMD_QUERY_HW_ID, sizeof(val), &val);
                    if (!ret) {
                        mpp_src_dbg_probe("client %-10s hw_id %08x\n", mpp_service_hw_name[i], val);
                        hw_ids[i] = val;
                    } else
                        mpp_err("check valid client %-10s for hw_id failed\n",
                                mpp_service_hw_name[i]);
                }
            }
        } else {
            /* kernel need to set client type then get hw_id */
            for (i = 0; i < 32; i++) {
                if (hw_support & (1 << i)) {
                    val = i;

                    /* set client type first */
                    ret = mpp_service_ioctl(fd, MPP_CMD_INIT_CLIENT_TYPE, sizeof(val), &val);
                    if (ret) {
                        mpp_err("check valid client type %d failed\n", i);
                        continue;
                    }

                    /* then get hw_id */
                    ret = mpp_service_ioctl(fd, MPP_CMD_QUERY_HW_ID, sizeof(val), &val);
                    if (!ret) {
                        mpp_src_dbg_probe("client %-10s hw_id %08x\n", mpp_service_hw_name[i], val);
                        hw_ids[i] = val;
                    } else
                        mpp_err("check valid client %-10s for hw_id failed\n",
                                mpp_service_hw_name[i]);
                }
            }
        }
    }

    cap->support_cmd = access("/proc/mpp_service/support_cmd", F_OK) ? 0 : 1;
    if (!cap->support_cmd)
        return ;

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
            mpp_src_dbg_probe("query %-11s support %04x\n", cfg->name, val);
        }
    }

    close(fd);
}

#define MAX_REG_OFFSET          16

typedef struct FdTransInfo_t {
    RK_U32          reg_idx;
    RK_U32          offset;
} RegOffsetInfo;

typedef struct MppDevMppService_t {
    RK_S32          client_type;
    RK_S32          fd;

    RK_S32          req_cnt;
    RK_S32          reg_offset_count;
    MppReqV1        reqs[MAX_REQ_NUM];
    RegOffsetInfo   reg_offset_info[MAX_REG_OFFSET];

    /* support max cmd buttom  */
    const MppServiceCmdCap *cap;
} MppDevMppService;

MPP_RET mpp_service_init(void *ctx, MppClientType type)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MPP_RET ret = MPP_NOK;

    p->cap = mpp_get_mpp_service_cmd_cap();
    p->fd = open(mpp_service_name, O_RDWR);
    if (p->fd < 0) {
        mpp_err("open mpp_service failed\n");
        return ret;
    }

    /* set client type first */
    ret = mpp_service_ioctl(p->fd, MPP_CMD_INIT_CLIENT_TYPE, sizeof(type), &type);
    if (ret)
        mpp_err("set client type %d failed\n", type);

    return ret;
}

MPP_RET mpp_service_deinit(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    if (p->fd)
        close(p->fd);

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

    if (!cfg->offset)
        return MPP_OK;

    RegOffsetInfo *info = &p->reg_offset_info[p->reg_offset_count++];

    info->reg_idx = cfg->reg_idx;
    info->offset = cfg->offset;

    return MPP_OK;
}

MPP_RET mpp_service_set_info(void *ctx, MppDevSetInfoCfg *cfg)
{
    (void)ctx;
    (void)cfg;

    return MPP_OK;
}

MPP_RET mpp_service_cmd_send(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;

    if (p->req_cnt <= 0 || p->req_cnt > MAX_REQ_NUM) {
        mpp_err_f("ctx %p invalid request count %d\n", ctx, p->req_cnt);
        return MPP_ERR_VALUE;
    }

    /* set fd trans info if needed */
    if (p->reg_offset_count) {
        MppReqV1 *mpp_req = &p->reqs[p->req_cnt];

        mpp_req->cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
        mpp_req->flag = 0;
        mpp_req->size = p->reg_offset_count * sizeof(p->reg_offset_info[0]);
        mpp_req->offset = 0;
        mpp_req->data_ptr = REQ_DATA_PTR(&p->reg_offset_info[0]);
        p->req_cnt++;
    }

    /* setup flag for multi message request */
    if (p->req_cnt > 1) {
        RK_S32 i;

        for (i = 0; i < p->req_cnt; i++)
            p->reqs[i].flag |= MPP_FLAGS_MULTI_MSG;
    }
    p->reqs[p->req_cnt - 1].flag |=  MPP_FLAGS_LAST_MSG;

    MPP_RET ret = mpp_service_ioctl_request(p->fd, &p->reqs[0]);
    if (ret) {
        mpp_err_f("ioctl MPP_IOC_CFG_V1 failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    p->req_cnt = 0;
    p->reg_offset_count = 0;
    return ret;
}

MPP_RET mpp_service_cmd_poll(void *ctx)
{
    MppDevMppService *p = (MppDevMppService *)ctx;
    MppReqV1 dev_req;

    memset(&dev_req, 0, sizeof(dev_req));
    dev_req.cmd = MPP_CMD_POLL_HW_FINISH;
    dev_req.flag |= MPP_FLAGS_LAST_MSG;

    MPP_RET ret = mpp_service_ioctl_request(p->fd, &dev_req);
    if (ret) {
        mpp_err_f("ioctl MPP_IOC_CFG_V1 failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    return ret;
}

const MppDevApi mpp_service_api = {
    "mpp_service",
    sizeof(MppDevMppService),
    mpp_service_init,
    mpp_service_deinit,
    mpp_service_reg_wr,
    mpp_service_reg_rd,
    mpp_service_reg_offset,
    mpp_service_set_info,
    mpp_service_cmd_send,
    mpp_service_cmd_poll,
};
