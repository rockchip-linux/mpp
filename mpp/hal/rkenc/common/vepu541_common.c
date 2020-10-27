/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#define MODULE_TAG  "vepu541_common"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "vepu541_common.h"
#include "mpp_device_msg.h"

static const RK_S32 zeros[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

static VepuFmtCfg vepu541_yuv_cfg[MPP_FMT_YUV_BUTT] = {
    {   /* MPP_FMT_YUV420SP */
        .format     = VEPU541_FMT_YUV420SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV420SP_10BIT */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422SP */
        .format     = VEPU541_FMT_YUV422SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422SP_10BIT */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV420P */
        .format     = VEPU541_FMT_YUV420P,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV420SP_VU   */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422P */
        .format     = VEPU541_FMT_YUV422P,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422SP_VU */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422_YUYV */
        .format     = VEPU541_FMT_YUYV422,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422_YVYU */
        .format     = VEPU541_FMT_YUYV422,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422_UYVY */
        .format     = VEPU541_FMT_UYVY422,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV422_VYUY */
        .format     = VEPU541_FMT_UYVY422,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV400 */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV440SP */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV411SP */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_YUV444SP */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
};

static VepuFmtCfg vepu541_rgb_cfg[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB] = {
    {   /* MPP_FMT_RGB565 */
        .format     = VEPU541_FMT_BGR565,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_BGR565 */
        .format     = VEPU541_FMT_BGR565,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_RGB555 */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_BGR555 */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_RGB444 */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_BGR444 */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_RGB888 */
        .format     = VEPU541_FMT_BGR888,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_BGR888 */
        .format     = VEPU541_FMT_BGR888,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_RGB101010 */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_BGR101010 */
        .format     = VEPU541_FMT_NONE,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_ARGB8888 */
        .format     = VEPU541_FMT_BGRA8888,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_ABGR8888 */
        .format     = VEPU541_FMT_BGRA8888,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_BGRA8888 */
        .format     = VEPU541_FMT_BGRA8888,
        .alpha_swap = 1,
        .rbuv_swap  = 1,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
    {   /* MPP_FMT_RGBA8888 */
        .format     = VEPU541_FMT_BGRA8888,
        .alpha_swap = 1,
        .rbuv_swap  = 0,
        .src_range  = 0,
        .weight     = zeros,
        .offset     = zeros,
    },
};

MPP_RET vepu541_set_fmt(VepuFmtCfg *cfg, MppFrameFormat format)
{
    VepuFmtCfg *fmt = NULL;
    MPP_RET ret = MPP_OK;

    format &= MPP_FRAME_FMT_MASK;

    if (MPP_FRAME_FMT_IS_YUV(format)) {
        fmt = &vepu541_yuv_cfg[format - MPP_FRAME_FMT_YUV];
    } else if (MPP_FRAME_FMT_IS_RGB(format)) {
        fmt = &vepu541_rgb_cfg[format - MPP_FRAME_FMT_RGB];
    } else {
        memset(cfg, 0, sizeof(*cfg));
        cfg->format = VEPU541_FMT_NONE;
    }

    if (fmt && fmt->format != VEPU541_FMT_NONE) {
        memcpy(cfg, fmt, sizeof(*cfg));
    } else {
        mpp_err_f("unsupport frame format %x\n", format);
        cfg->format = VEPU541_FMT_NONE;
        ret = MPP_NOK;
    }

    return ret;
}

RK_S32 vepu541_get_roi_buf_size(RK_S32 w, RK_S32 h)
{
    RK_S32 stride_h = MPP_ALIGN(w, 64) / 16;
    RK_S32 stride_v = MPP_ALIGN(h, 64) / 16;
    RK_S32 buf_size = stride_h * stride_v * sizeof(Vepu541RoiCfg);

    /* extra 32 byte for hardware access padding */
    return buf_size + 32;
}

MPP_RET vepu541_set_roi(void *buf, MppEncROICfg *roi, RK_S32 w, RK_S32 h)
{
    MppEncROIRegion *region = roi->regions;
    Vepu541RoiCfg *ptr = (Vepu541RoiCfg *)buf;
    RK_S32 mb_w = MPP_ALIGN(w, 16) / 16;
    RK_S32 mb_h = MPP_ALIGN(h, 16) / 16;
    RK_S32 stride_h = MPP_ALIGN(mb_w, 4);
    RK_S32 stride_v = MPP_ALIGN(mb_h, 4);
    Vepu541RoiCfg cfg;
    MPP_RET ret = MPP_NOK;
    RK_S32 i;

    if (NULL == buf || NULL == roi) {
        mpp_err_f("invalid buf %p roi %p\n", buf, roi);
        goto DONE;
    }

    cfg.force_intra = 0;
    cfg.reserved    = 0;
    cfg.qp_area_idx = 0;
    cfg.qp_area_en  = 1;
    cfg.qp_adj      = 0;
    cfg.qp_adj_mode = 0;

    /* step 1. reset all the config */
    for (i = 0; i < stride_h * stride_v; i++, ptr++)
        memcpy(ptr, &cfg, sizeof(cfg));

    if (w <= 0 || h <= 0) {
        mpp_err_f("invalid size [%d:%d]\n", w, h);
        goto DONE;
    }

    if (roi->number > VEPU541_MAX_ROI_NUM) {
        mpp_err_f("invalid region number %d\n", buf, roi->number);
        goto DONE;
    }

    /* check region config */
    ret = MPP_OK;
    for (i = 0; i < (RK_S32)roi->number; i++, region++) {
        if (region->x + region->w > w || region->y + region->h > h)
            ret = MPP_NOK;

        if (region->intra > 1 || region->qp_area_idx >= VEPU541_MAX_ROI_NUM ||
            region->area_map_en > 1 || region->abs_qp_en > 1)
            ret = MPP_NOK;

        if ((region->abs_qp_en && region->quality > 51) ||
            (!region->abs_qp_en && (region->quality > 51 || region->quality < -51)))
            ret = MPP_NOK;

        if (ret) {
            mpp_err_f("region %d invalid param:\n", i);
            mpp_err_f("position [%d:%d:%d:%d] vs [%d:%d]\n",
                      region->x, region->y, region->w, region->h, w, h);
            mpp_err_f("force intra %d qp area index %d\n",
                      region->intra, region->qp_area_idx);
            mpp_err_f("abs qp mode %d value %d\n",
                      region->abs_qp_en, region->quality);
            goto DONE;
        }
    }

    region = roi->regions;
    /* step 2. setup region for top to bottom */
    for (i = 0; i < (RK_S32)roi->number; i++, region++) {
        RK_S32 roi_width  = (region->w + 15) / 16;
        RK_S32 roi_height = (region->h + 15) / 16;
        RK_S32 pos_x_init = (region->x + 15) / 16;
        RK_S32 pos_y_init = (region->y + 15) / 16;
        RK_S32 pos_x_end  = pos_x_init + roi_width;
        RK_S32 pos_y_end  = pos_y_init + roi_height;
        RK_S32 x, y;

        mpp_assert(pos_x_init >= 0 && pos_x_init < mb_w);
        mpp_assert(pos_x_end  >= 0 && pos_x_end <= mb_w);
        mpp_assert(pos_y_init >= 0 && pos_y_init < mb_h);
        mpp_assert(pos_y_end  >= 0 && pos_y_end <= mb_h);

        cfg.force_intra = region->intra;
        cfg.reserved    = 0;
        cfg.qp_area_idx = region->qp_area_idx;
        // NOTE: When roi is enabled the qp_area_en should be one.
        cfg.qp_area_en  = 1; // region->area_map_en;
        cfg.qp_adj      = region->quality;
        cfg.qp_adj_mode = region->abs_qp_en;

        ptr = (Vepu541RoiCfg *)buf;
        ptr += pos_y_init * stride_h + pos_x_init;
        for (y = 0; y < roi_height; y++) {
            Vepu541RoiCfg *dst = ptr;

            for (x = 0; x < roi_width; x++, dst++)
                memcpy(dst, &cfg, sizeof(cfg));

            ptr += stride_h;
        }
    }

DONE:
    return ret;
}

//TODO: open interface later
#define ENC_DEFAULT_OSD_INV_THR         15
#define VEPU541_OSD_ADDR_IDX_BASE       124
#define VEPU541_OSD_CFG_OFFSET          0x01C0
#define VEPU541_OSD_PLT_OFFSET          0x0400

typedef enum Vepu541OsdPltType_e {
    VEPU541_OSD_PLT_TYPE_USERDEF    = 0,
    VEPU541_OSD_PLT_TYPE_DEFAULT    = 1,
} Vepu541OsdPltType;

typedef struct Vepu541OsdReg_t {
    /*
     * OSD_CFG
     * Address offset: 0x01C0 Access type: read and write
     * OSD configuration
     */
    struct {
        /* OSD region enable, each bit controls corresponding OSD region. */
        RK_U32  osd_e                   : 8;
        /* OSD inverse color enable, each bit controls corresponding region. */
        RK_U32  osd_inv_e               : 8;
        /*
         * OSD palette clock selection.
         * 1'h0: Configure bus clock domain.
         * 1'h1: Core clock domain.
         */
        RK_U32  osd_plt_cks             : 1;
        /*
         * OSD palette type.
         * 1'h1: Default type.
         * 1'h0: User defined type.
         */
        RK_U32  osd_plt_typ             : 1;
        RK_U32  reserved                : 14;
    } reg112;

    /*
     * OSD_INV
     * Address offset: 0x01C4 Access type: read and write
     * OSD color inverse configuration
     */
    struct {
        /* Color inverse theshold for OSD region0. */
        RK_U32  osd_ithd_r0             : 4;
        /* Color inverse theshold for OSD region1. */
        RK_U32  osd_ithd_r1             : 4;
        /* Color inverse theshold for OSD region2. */
        RK_U32  osd_ithd_r2             : 4;
        /* Color inverse theshold for OSD region3. */
        RK_U32  osd_ithd_r3             : 4;
        /* Color inverse theshold for OSD region4. */
        RK_U32  osd_ithd_r4             : 4;
        /* Color inverse theshold for OSD region5. */
        RK_U32  osd_ithd_r5             : 4;
        /* Color inverse theshold for OSD region6. */
        RK_U32  osd_ithd_r6             : 4;
        /* Color inverse theshold for OSD region7. */
        RK_U32  osd_ithd_r7             : 4;
    } reg113;

    RK_U32 reg114;
    RK_U32 reg115;

    /*
     * OSD_POS reg116_123
     * Address offset: 0x01D0~0x01EC Access type: read and write
     * OSD region position
     */
    Vepu541OsdPos  osd_pos[8];

    /*
     * ADR_OSD reg124_131
     * Address offset: 0x01F0~0x20C Access type: read and write
     * Base address for OSD region, 16B aligned
     */
    RK_U32  osd_addr[8];
} Vepu541OsdReg;

MPP_RET vepu541_set_osd(Vepu541OsdCfg *cfg)
{
    Vepu541OsdReg *regs = (Vepu541OsdReg *)(cfg->reg_base + (size_t)VEPU541_OSD_CFG_OFFSET);
    MppDevCtx dev = cfg->dev;
    MppEncOSDPltCfg *plt_cfg = cfg->plt_cfg;
    MppEncOSDData *osd = cfg->osd_data;

    if (plt_cfg->type == MPP_ENC_OSD_PLT_TYPE_USERDEF) {
        MppDevReqV1 req;

        req.cmd = MPP_CMD_SET_REG_WRITE;
        req.flag = 0;
        req.offset = VEPU541_REG_BASE_OSD_PLT;
        req.size = sizeof(MppEncOSDPlt);
        req.data = REQ_DATA_PTR(plt_cfg->plt);
        mpp_device_add_request(dev, &req);

        regs->reg112.osd_plt_cks = 1;
        regs->reg112.osd_plt_typ = VEPU541_OSD_PLT_TYPE_USERDEF;
    } else {
        regs->reg112.osd_plt_cks = 0;
        regs->reg112.osd_plt_typ = VEPU541_OSD_PLT_TYPE_DEFAULT;
    }

    regs->reg112.osd_e = 0;
    regs->reg112.osd_inv_e = 0;

    if (NULL == osd || osd->num_region == 0 || NULL == osd->buf)
        return MPP_OK;

    if (osd->num_region > 8) {
        mpp_err_f("do NOT support more than 8 regions invalid num %d\n",
                  osd->num_region);
        mpp_assert(osd->num_region <= 8);
        return MPP_NOK;
    }

    MppBuffer buf = osd->buf;
    RK_S32 fd = mpp_buffer_get_fd(buf);
    if (fd < 0) {
        mpp_err_f("invalid osd buffer fd %d\n", fd);
        return MPP_NOK;
    }

    size_t buf_size = mpp_buffer_get_size(buf);
    RK_U32 num = osd->num_region;
    RK_U32 k = 0;
    MppEncOSDRegion *region = osd->region;
    MppEncOSDRegion *tmp = region;

    mpp_device_patch_init(&cfg->extra);

    for (k = 0; k < num; k++, tmp++) {
        regs->reg112.osd_e      |= tmp->enable << k;
        regs->reg112.osd_inv_e  |= tmp->inverse << k;

        if (tmp->enable && tmp->num_mb_x && tmp->num_mb_y) {
            Vepu541OsdPos *pos = &regs->osd_pos[k];
            size_t blk_len = tmp->num_mb_x * tmp->num_mb_y * 256;

            pos->osd_lt_x = tmp->start_mb_x;
            pos->osd_lt_y = tmp->start_mb_y;
            pos->osd_rb_x = tmp->start_mb_x + tmp->num_mb_x - 1;
            pos->osd_rb_y = tmp->start_mb_y + tmp->num_mb_y - 1;

            regs->osd_addr[k] = fd;
            mpp_device_patch_add(cfg->reg_base, &cfg->extra,
                                 VEPU541_OSD_ADDR_IDX_BASE + k,
                                 tmp->buf_offset);

            /* There should be enough buffer and offset should be 16B aligned */
            if (buf_size < tmp->buf_offset + blk_len ||
                (tmp->buf_offset & 0xf)) {
                mpp_err_f("invalid osd cfg: %d x:y:w:h:off %d:%d:%d:%x\n",
                          k, tmp->start_mb_x, tmp->start_mb_y,
                          tmp->num_mb_x, tmp->num_mb_y, tmp->buf_offset);
            }
        }
    }

    mpp_device_send_extra_info(dev, &cfg->extra);

    if (region[0].inverse)
        regs->reg113.osd_ithd_r0 = ENC_DEFAULT_OSD_INV_THR;
    if (region[1].inverse)
        regs->reg113.osd_ithd_r1 = ENC_DEFAULT_OSD_INV_THR;
    if (region[2].inverse)
        regs->reg113.osd_ithd_r2 = ENC_DEFAULT_OSD_INV_THR;
    if (region[3].inverse)
        regs->reg113.osd_ithd_r3 = ENC_DEFAULT_OSD_INV_THR;
    if (region[4].inverse)
        regs->reg113.osd_ithd_r4 = ENC_DEFAULT_OSD_INV_THR;
    if (region[5].inverse)
        regs->reg113.osd_ithd_r5 = ENC_DEFAULT_OSD_INV_THR;
    if (region[6].inverse)
        regs->reg113.osd_ithd_r6 = ENC_DEFAULT_OSD_INV_THR;
    if (region[7].inverse)
        regs->reg113.osd_ithd_r7 = ENC_DEFAULT_OSD_INV_THR;

    return MPP_OK;
}
