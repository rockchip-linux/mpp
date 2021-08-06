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

#include "iep2_api.h"

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_env.h"

#include "iep2_ff.h"
#include "iep2_pd.h"
#include "iep2_gmv.h"
#include "iep2_osd.h"
#include "iep2_roi.h"

#include "iep2.h"
#include "mpp_service.h"
#include "mpp_platform.h"

#define IEP2_TILE_W_MAX     120
#define IEP2_TILE_H_MAX     480

RK_U32 iep_debug = 0;

static MPP_RET iep2_init(IepCtx *ictx)
{
    MPP_RET ret;
    struct iep2_api_ctx *ctx = *ictx;
    MppReqV1 mpp_req;
    RK_U32 client_data = IEP_CLIENT_TYPE;

    mpp_env_get_u32("iep_debug", &iep_debug, 0);

    ctx->fd = open("/dev/mpp_service", O_RDWR | O_CLOEXEC);
    if (ctx->fd < 0) {
        mpp_err("can NOT find device /dev/iep2\n");
        return MPP_NOK;
    }

    mpp_req.cmd = MPP_CMD_INIT_CLIENT_TYPE;
    mpp_req.flag = 0;
    mpp_req.size = sizeof(client_data);
    mpp_req.data_ptr = REQ_DATA_PTR(&client_data);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        mpp_err("ioctl set_client failed\n");
        return MPP_NOK;
    }

    // set default parameters.
    ctx->params.src_fmt = IEP2_FMT_YUV420;
    ctx->params.src_yuv_swap = IEP2_YUV_SWAP_SP_UV;
    ctx->params.dst_fmt = IEP2_FMT_YUV420;
    ctx->params.dst_yuv_swap = IEP2_YUV_SWAP_SP_UV;

    ctx->params.src_y_stride = 720 / 4;
    ctx->params.src_uv_stride = 720 / 4;
    ctx->params.dst_y_stride = 720 / 4;

    ctx->params.tile_cols = 720 / 16;
    ctx->params.tile_rows = 480 / 4;

    ctx->params.dil_mode = IEP2_DIL_MODE_I1O1T;

    ctx->params.dil_out_mode = IEP2_OUT_MODE_LINE;
    ctx->params.dil_field_order = IEP2_FIELD_ORDER_TFF;

    ctx->params.md_theta = 1;
    ctx->params.md_r = 4;
    ctx->params.md_lambda = 4;

    ctx->params.dect_resi_thr = 30;
    ctx->params.osd_area_num = 0;
    ctx->params.osd_gradh_thr = 60;
    ctx->params.osd_gradv_thr = 60;

    ctx->params.osd_pos_limit_en = 0;
    ctx->params.osd_pos_limit_num = 0;

    ctx->params.osd_pec_thr = 20;
    ctx->params.osd_line_num = 2;

    ctx->params.me_pena = 4;
    ctx->params.mv_similar_thr = 4;
    ctx->params.mv_similar_num_thr0 = 4;
    ctx->params.mv_bonus = 10;
    ctx->params.me_thr_offset = 20;

    ctx->params.mv_left_limit = 28;
    ctx->params.mv_right_limit = 27;

    ctx->params.eedi_thr0 = 12;

    memset(ctx->params.comb_osd_vld, 1, sizeof(ctx->params.comb_osd_vld));
    ctx->params.comb_t_thr = 4;
    ctx->params.comb_feature_thr = 16;
    ctx->params.comb_cnt_thr = 0;

    ctx->params.ble_backtoma_num = 1;

    ctx->params.mtn_en = 0;

    ctx->params.roi_en = 0;
    ctx->params.roi_layer_num = 0;

    ctx->params.dil_mode = IEP2_DIL_MODE_I1O1T;
    ctx->params.src_fmt = IEP2_FMT_YUV420;
    ctx->params.src_yuv_swap = IEP2_YUV_SWAP_SP_UV;
    ctx->params.dst_fmt = IEP2_FMT_YUV420;
    ctx->params.dst_yuv_swap = IEP2_YUV_SWAP_SP_UV;
    ctx->params.src_y_stride = 720 / 4;
    ctx->params.src_uv_stride = 720 / 4;
    ctx->params.dst_y_stride = 720 / 4;
    ctx->params.tile_cols = 720 / 16;
    ctx->params.tile_rows = 480 / 4;

    memset(&ctx->ff_inf, 0, sizeof(ctx->ff_inf));

    memset(&ctx->pd_inf, 0, sizeof(ctx->pd_inf));
    ctx->pd_inf.pdtype = PD_TYPES_UNKNOWN;
    ctx->pd_inf.step = -1;

    ret = mpp_buffer_group_get_internal(&ctx->memGroup, MPP_BUFFER_TYPE_DRM);
    if (MPP_OK != ret) {
        close(ctx->fd);
        mpp_err("memGroup mpp_buffer_group_get failed\n");
        return ret;
    }

    ret = mpp_buffer_get(ctx->memGroup, &ctx->mv_buf,
                         IEP2_TILE_W_MAX * IEP2_TILE_H_MAX);
    if (ret) {
        close(ctx->fd);
        mpp_buffer_group_put(ctx->memGroup);
        mpp_err_f("allocate mv buffer failed\n");
        return MPP_NOK;
    }

    ret = mpp_buffer_get(ctx->memGroup, &ctx->md_buf, 1920 * 1088);
    if (ret) {
        close(ctx->fd);
        mpp_buffer_group_put(ctx->memGroup);
        mpp_buffer_put(ctx->mv_buf);
        mpp_err_f("allocate md buffer failed\n");
        return MPP_NOK;
    }

    ctx->params.mv_addr = mpp_buffer_get_fd(ctx->mv_buf);
    ctx->params.md_addr = mpp_buffer_get_fd(ctx->md_buf);

    return MPP_OK;
}

static MPP_RET iep2_deinit(IepCtx ictx)
{
    struct iep2_api_ctx *ctx = ictx;

    close(ctx->fd);

    mpp_buffer_put(ctx->mv_buf);
    mpp_buffer_put(ctx->md_buf);

    if (ctx->memGroup) {
        mpp_buffer_group_put(ctx->memGroup);
        ctx->memGroup = NULL;
    }

    return MPP_OK;
}

static MPP_RET iep2_done(struct iep2_api_ctx *ctx)
{

    if (ctx->params.dil_mode == IEP2_DIL_MODE_I5O2 ||
        ctx->params.dil_mode == IEP2_DIL_MODE_I5O1T ||
        ctx->params.dil_mode == IEP2_DIL_MODE_I5O1B) {
        struct mv_list ls;

        iep2_set_osd(ctx, &ls);
        iep2_update_gmv(ctx, &ls);
        iep2_check_ffo(ctx);
        iep2_check_pd(ctx);
#if 0
        if (ctx->params.roi_en && ctx->params.osd_area_num > 0) {
            struct iep2_rect r;

            ctx->params.roi_layer_num = 0;

            r.x = 0;
            r.y = 0;
            r.w = ctx->params.tile_cols;
            r.h = ctx->params.tile_rows;
            iep2_set_roi(ctx, &r, ROI_MODE_MA);

            r.x = ctx->params.osd_x_sta[0];
            r.y = ctx->params.osd_y_sta[0];
            r.w = ctx->params.osd_x_end[0] - ctx->params.osd_x_sta[0];
            r.h = ctx->params.osd_y_end[0] - ctx->params.osd_y_sta[0];
            iep2_set_roi(ctx, &r, ROI_MODE_MA_MC);
        }
#endif
    }

    if (ctx->params.dil_mode == IEP2_DIL_MODE_DECT ||
        ctx->params.dil_mode == IEP2_DIL_MODE_PD) {
        iep2_check_ffo(ctx);
        iep2_check_pd(ctx);
    }

    if (ctx->pd_inf.pdtype != PD_TYPES_UNKNOWN) {
        ctx->params.dil_mode = IEP2_DIL_MODE_PD;
        ctx->params.pd_mode = iep2_pd_get_output(&ctx->pd_inf);
    } else {
        // TODO, revise others mode replace by I5O2
        //ctx->params.dil_mode = IEP2_DIL_MODE_I5O2;
    }

    return 0;
}

static void iep2_set_param(struct iep2_api_ctx *ctx,
                           union iep2_api_content *param,
                           enum IEP2_PARAM_TYPE type)
{
    switch (type) {
    case IEP2_PARAM_TYPE_COM:
            ctx->params.src_fmt = param->com.sfmt;
        ctx->params.src_yuv_swap = param->com.sswap;
        ctx->params.dst_fmt = param->com.dfmt;
        ctx->params.dst_yuv_swap = param->com.dswap;
        ctx->params.src_y_stride = param->com.width;
        ctx->params.src_y_stride /= 4;
        ctx->params.src_uv_stride =
            param->com.sswap == IEP2_YUV_SWAP_P ?
            (param->com.width / 2 + 15) / 16 * 16 : param->com.width;
        ctx->params.src_uv_stride /= 4;
        ctx->params.dst_y_stride = param->com.width;
        ctx->params.dst_y_stride /= 4;
        ctx->params.tile_cols = (param->com.width + 15) / 16;
        ctx->params.tile_rows = (param->com.height + 3) / 4;
        iep_dbg_trace("set tile size (%d, %d)\n", param->com.width, param->com.height);
        ctx->params.osd_pec_thr = (param->com.width * 26) >> 7;
        break;
    case IEP2_PARAM_TYPE_MODE:
        ctx->params.dil_mode = param->mode.dil_mode;
        ctx->params.dil_out_mode = param->mode.out_mode;
        if (!ctx->ff_inf.fo_detected) {
            ctx->params.dil_field_order = param->mode.dil_order;
        }

        if (param->mode.dil_order == 0) {
            ctx->ff_inf.tff_offset = 5;
            ctx->ff_inf.bff_offset = 0;
        } else {
            ctx->ff_inf.tff_offset = 0;
            ctx->ff_inf.bff_offset = 5;
        }
        break;
    case IEP2_PARAM_TYPE_MD:
        ctx->params.md_theta = param->md.md_theta;
        ctx->params.md_r = param->md.md_r;
        ctx->params.md_lambda = param->md.md_lambda;
        break;
    case IEP2_PARAM_TYPE_DECT:
    case IEP2_PARAM_TYPE_OSD:
    case IEP2_PARAM_TYPE_ME:
    case IEP2_PARAM_TYPE_EEDI:
    case IEP2_PARAM_TYPE_BLE:
    case IEP2_PARAM_TYPE_COMB:
        break;
    case IEP2_PARAM_TYPE_ROI:
        ctx->params.roi_en = param->roi.roi_en;
        break;
    default:
        ;
    }
}

static MPP_RET iep2_param_check(struct iep2_api_ctx *ctx)
{
    if (ctx->params.tile_cols <= 0 || ctx->params.tile_cols > IEP2_TILE_W_MAX ||
        ctx->params.tile_rows <= 0 || ctx->params.tile_rows > IEP2_TILE_H_MAX) {
        mpp_err("invalidate size (%u, %u)\n",
                ctx->params.tile_cols, ctx->params.tile_rows);
        return MPP_NOK;
    }

    return MPP_OK;
}

static MPP_RET iep2_start(struct iep2_api_ctx *ctx)
{
    MPP_RET ret;
    MppReqV1 mpp_req[2];

    mpp_assert(ctx);

    mpp_req[0].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[0].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[0].size =  sizeof(ctx->params);
    mpp_req[0].offset = 0;
    mpp_req[0].data_ptr = REQ_DATA_PTR(&ctx->params);

    mpp_req[1].cmd = MPP_CMD_SET_REG_READ;
    mpp_req[1].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_LAST_MSG;
    mpp_req[1].size =  sizeof(ctx->output);
    mpp_req[1].offset = 0;
    mpp_req[1].data_ptr = REQ_DATA_PTR(&ctx->output);

    iep_dbg_func("in\n");

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req[0]);

    if (ret) {
        mpp_err_f("ioctl SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    return ret;
}

static MPP_RET iep2_wait(struct iep2_api_ctx *ctx)
{
    MppReqV1 mpp_req;
    MPP_RET ret;

    memset(&mpp_req, 0, sizeof(mpp_req));
    mpp_req.cmd = MPP_CMD_POLL_HW_FINISH;
    mpp_req.flag |= MPP_FLAGS_LAST_MSG;

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);

    return ret;
}

static inline void set_addr(struct iep2_addr *addr, IepImg *img)
{
    addr->y = img->mem_addr;
    addr->cbcr = img->uv_addr;
    addr->cr = img->v_addr;
}

static MPP_RET iep2_control(IepCtx ictx, IepCmd cmd, void *iparam)
{
    struct iep2_api_ctx *ctx = ictx;

    switch (cmd) {
    case IEP_CMD_SET_DEI_CFG: {
        struct iep2_api_params *param = (struct iep2_api_params *)iparam;

        iep2_set_param(ctx, &param->param, param->ptype);
    }
    break;
    case IEP_CMD_SET_SRC:
        set_addr(&ctx->params.src[0], (IepImg *)iparam);
        break;
    case IEP_CMD_SET_DEI_SRC1:
        set_addr(&ctx->params.src[1], (IepImg *)iparam);
        break;
    case IEP_CMD_SET_DEI_SRC2:
        set_addr(&ctx->params.src[2], (IepImg *)iparam);
        break;
    case IEP_CMD_SET_DST:
        set_addr(&ctx->params.dst[0], (IepImg *)iparam);
        break;
    case IEP_CMD_SET_DEI_DST1:
        set_addr(&ctx->params.dst[1], (IepImg *)iparam);
        break;
    case IEP_CMD_RUN_SYNC: {
        struct iep2_api_info *inf = (struct iep2_api_info*)iparam;

        if (0 > iep2_param_check(ctx))
            break;
        if (0 > iep2_start(ctx))
            return MPP_NOK;
        iep2_wait(ctx);
        if (inf)
            inf->pd_flag = ctx->params.pd_mode;
        iep2_done(ctx);
        if (inf) {
            inf->dil_order = ctx->params.dil_field_order;
            inf->frm_mode = ctx->ff_inf.is_frm;
            inf->pd_types = ctx->pd_inf.pdtype;
        }
    }
    break;
    default:
        ;
    }

    return MPP_OK;
}

static iep_com_ops iep2_ops = {
    .init = iep2_init,
    .deinit = iep2_deinit,
    .control = iep2_control,
    .release = NULL,
};

iep_com_ctx* rockchip_iep2_api_alloc_ctx(void)
{
    iep_com_ctx *com_ctx = calloc(sizeof(*com_ctx), 1);
    struct iep2_api_ctx *iep2_ctx = calloc(sizeof(*iep2_ctx), 1);

    mpp_assert(com_ctx && iep2_ctx);

    com_ctx->ops = &iep2_ops;
    com_ctx->priv = iep2_ctx;

    return com_ctx;
}

void rockchip_iep2_api_release_ctx(iep_com_ctx *com_ctx)
{
    if (com_ctx->priv) {
        free(com_ctx->priv);
        com_ctx->priv = NULL;
    }

    free(com_ctx);
}

