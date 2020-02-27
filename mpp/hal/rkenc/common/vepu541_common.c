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

MPP_RET vepu541_set_fmt(VepuFmtCfg *cfg, MppEncPrepCfg *prep)
{
    MppFrameFormat format = prep->format;
    VepuFmtCfg *fmt = NULL;
    MPP_RET ret = MPP_OK;

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

    return buf_size;
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
    RK_S32 i;

    mpp_assert(buf);
    mpp_assert(roi);
    mpp_assert(w);
    mpp_assert(h);

    cfg.force_intra = 0;
    cfg.reserved    = 0;
    cfg.qp_area_idx = 0;
    cfg.qp_area_en  = 1;
    cfg.qp_adj      = 0;
    cfg.qp_adj_mode = 0;

    /* step 1. reset all the config */
    for (i = 0; i < stride_h * stride_v; i++, ptr++)
        memcpy(ptr, &cfg, sizeof(cfg));

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
        cfg.qp_area_en  = region->area_map_en;
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

    return MPP_OK;
}

//TODO: open interface later
#define ENC_DEFAULT_OSD_INV_THR         15
#define VEPU541_OSD_CFG_OFFSET          0x01C0
#define VEPU541_OSD_PLT_OFFSET          0x0400

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

typedef enum Vepu541OsdPltType_e {
    VEPU541_OSD_PLT_TYPE_USERDEF    = 0,
    VEPU541_OSD_PLT_TYPE_DEFAULT    = 1,
} Vepu541OsdPltType;

MPP_RET vepu541_set_osd_region(void *reg_base, MppDevCtx dev,
                               MppEncOSDData *osd, MppEncOSDPlt *plt)
{
    RK_U8 *base = (RK_U8 *)reg_base + VEPU541_OSD_CFG_OFFSET;
    Vepu541OsdPltType type = VEPU541_OSD_PLT_TYPE_DEFAULT;
    Vepu541OsdReg *regs = (Vepu541OsdReg *)base;

    if (plt) {
        MppDevReqV1 req;

        req.cmd = MPP_CMD_SET_REG_WRITE;
        req.flag = 0;
        req.offset = VEPU541_REG_BASE_OSD_PLT;
        req.size = sizeof(MppEncOSDPlt);
        req.data = plt;
        mpp_device_add_request(dev, &req);

        type = VEPU541_OSD_PLT_TYPE_USERDEF;
    }

    if (NULL == osd || osd->num_region == 0 || NULL == osd->buf) {
        regs->reg112.osd_e = 0;
        regs->reg112.osd_inv_e = 0;
        regs->reg112.osd_plt_cks = 0;
        regs->reg112.osd_plt_typ = type;

        return MPP_OK;
    }

    RK_S32 fd = mpp_buffer_get_fd(osd->buf);
    if (fd < 0) {
        mpp_err_f("invalid osd buffer fd %d\n", fd);
        return MPP_NOK;
    }

    RK_U32 num = osd->num_region;
    RK_U32 k = 0;
    MppEncOSDRegion *region = osd->region;
    MppEncOSDRegion *tmp = region;

    regs->reg112.osd_e = 0;
    regs->reg112.osd_inv_e = 0;
    regs->reg112.osd_plt_cks = 0;
    regs->reg112.osd_plt_typ = type;

    for (k = 0; k < num; k++, tmp++) {
        regs->reg112.osd_e      |= tmp->enable << k;
        regs->reg112.osd_inv_e  |= tmp->inverse << k;

        if (tmp->enable) {
            Vepu541OsdPos *pos = &regs->osd_pos[k];

            pos->osd_lt_x = tmp->start_mb_x;
            pos->osd_lt_y = tmp->start_mb_y;
            pos->osd_rb_x = tmp->start_mb_x + tmp->num_mb_x - 1;
            pos->osd_rb_y = tmp->start_mb_y + tmp->num_mb_y - 1;

            regs->osd_addr[k] = fd | (tmp->buf_offset << 10);
        }
    }

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
