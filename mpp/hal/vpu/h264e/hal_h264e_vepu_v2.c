/*
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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
#define MODULE_TAG "hal_h264e_vepu_v2"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_buffer.h"

#include "hal_h264e_debug.h"
#include "hal_h264e_com.h"
#include "hal_h264e_vepu_v2.h"
#include "hal_h264e_vpu_tbl_v2.h"

typedef struct HalH264eVepuMbRcImpl_t {
    RK_S32          width;
    RK_S32          height;
    RK_S32          mb_w;
    RK_S32          mb_h;

    RK_S32          pels;
    RK_S32          mbs;

    RK_S32          bits_per_pic;

    RK_S32          mb_bit_rc_enable;

    /* frame rate control */
    RK_S32          fps_in_num;
    RK_S32          fps_in_denorm;
    RK_S32          fps_out_num;
    RK_S32          fps_out_denorm;

    RK_S32          fps_count;
    RK_S32          fps_step;
    RK_S32          fps_threshold;

    /*
     * MAD based QP adjustment
     * mad_qp_change    [-8..7]
     * mad_threshold    MAD threshold div256
     */
    RK_S32          mad_qp_change;
    RK_S32          mad_threshold;

    /*
     * check point parameter
     */
    RK_S32          check_point_count;
    RK_S32          check_point_distance;

    /* estimated first I frame qp */
    RK_S32          qp_init_est;
} HalH264eVepuMbRcImpl;

static void vepu_swap_endian(RK_U32 *buf, RK_S32 size_bytes)
{
    RK_U32 i = 0;
    RK_S32 words = size_bytes / 4;
    RK_U32 val, val2, tmp, tmp2;

    mpp_assert((size_bytes % 8) == 0);

    while (words > 0) {
        val = buf[i];
        tmp = 0;

        tmp |= (val & 0xFF) << 24;
        tmp |= (val & 0xFF00) << 8;
        tmp |= (val & 0xFF0000) >> 8;
        tmp |= (val & 0xFF000000) >> 24;
        {
            val2 = buf[i + 1];
            tmp2 = 0;

            tmp2 |= (val2 & 0xFF) << 24;
            tmp2 |= (val2 & 0xFF00) << 8;
            tmp2 |= (val2 & 0xFF0000) >> 8;
            tmp2 |= (val2 & 0xFF000000) >> 24;

            buf[i] = tmp2;
            words--;
            i++;
        }
        buf[i] = tmp;
        words--;
        i++;
    }
}

static void vepu_write_cabac_table(MppBuffer buf, RK_S32 cabac_init_idc)
{
    const RK_S32(*context)[460][2];
    RK_S32 i, j, qp;
    RK_U8 table[H264E_CABAC_TABLE_BUF_SIZE] = {0};

    for (qp = 0; qp < 52; qp++) { /* All QP values */
        for (j = 0; j < 2; j++) { /* Intra/Inter */
            if (j == 0)
                /*lint -e(545) */
                context = &h264_context_init_intra;
            else
                /*lint -e(545) */
                context = &h264_context_init[cabac_init_idc];

            for (i = 0; i < 460; i++) {
                RK_S32 m = (RK_S32)(*context)[i][0];
                RK_S32 n = (RK_S32)(*context)[i][1];

                RK_S32 pre_ctx_state = H264E_HAL_CLIP3(((m * (RK_S32)qp) >> 4)
                                                       + n, 1, 126);

                if (pre_ctx_state <= 63)
                    table[qp * 464 * 2 + j * 464 + i] =
                        (RK_U8)((63 - pre_ctx_state) << 1);
                else
                    table[qp * 464 * 2 + j * 464 + i] =
                        (RK_U8)(((pre_ctx_state - 64) << 1) | 1);
            }
        }
    }

    vepu_swap_endian((RK_U32 *)table, H264E_CABAC_TABLE_BUF_SIZE);
    mpp_buffer_write(buf, 0, table, H264E_CABAC_TABLE_BUF_SIZE);
}

MPP_RET h264e_vepu_buf_init(HalH264eVepuBufs *bufs)
{
    MPP_RET ret = MPP_OK;

    hal_h264e_dbg_buffer("enter %p\n", bufs);

    memset(bufs, 0, sizeof(*bufs));

    // do not create buffer on cavlc case
    bufs->cabac_init_idc = -1;
    ret = mpp_buffer_group_get_internal(&bufs->group, MPP_BUFFER_TYPE_ION);
    if (ret)
        mpp_err_f("get buffer group failed ret %d\n", ret);

    hal_h264e_dbg_buffer("leave %p\n", bufs);

    return ret;
}

MPP_RET h264e_vepu_buf_deinit(HalH264eVepuBufs *bufs)
{
    RK_S32 i;

    hal_h264e_dbg_buffer("enter %p\n", bufs);

    if (bufs->cabac_table)
        mpp_buffer_put(bufs->cabac_table);

    if (bufs->nal_size_table)
        mpp_buffer_put(bufs->nal_size_table);

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(bufs->frm_buf); i++) {
        if (bufs->frm_buf[i])
            mpp_buffer_put(bufs->frm_buf[i]);
    }

    if (bufs->group)
        mpp_buffer_group_put(bufs->group);

    memset(bufs, 0, sizeof(*bufs));
    bufs->cabac_init_idc = -1;

    hal_h264e_dbg_buffer("leave %p\n", bufs);

    return MPP_OK;
}

MPP_RET h264e_vepu_buf_set_cabac_idc(HalH264eVepuBufs *bufs, RK_S32 idc)
{
    hal_h264e_dbg_buffer("enter %p\n", bufs);

    if (idc >= 0 && !bufs->cabac_table)
        mpp_buffer_get(bufs->group, &bufs->cabac_table, H264E_CABAC_TABLE_BUF_SIZE);

    if (bufs->cabac_table && idc != bufs->cabac_init_idc && idc >= 0)
        vepu_write_cabac_table(bufs->cabac_table, idc);

    bufs->cabac_init_idc = idc;

    hal_h264e_dbg_buffer("leave %p\n", bufs);

    return MPP_OK;
}

MPP_RET h264e_vepu_buf_set_frame_size(HalH264eVepuBufs *bufs, RK_S32 w, RK_S32 h)
{
    RK_S32 aligned_w = MPP_ALIGN(w, 16);
    RK_S32 aligned_h = MPP_ALIGN(h, 16);
    size_t yuv_size = aligned_w * aligned_h;
    size_t frm_size = yuv_size * 3 / 2;
    RK_S32 i;
    RK_S32 cnt = (RK_S32)MPP_ARRAY_ELEMS(bufs->frm_buf);

    hal_h264e_dbg_buffer("enter %p\n", bufs);

    mpp_assert(frm_size);

    if (frm_size != bufs->frm_size) {
        if (bufs->frm_size) {
            /* reallocate only on larger frame size to save time */
            mpp_log("new frame size [%d:%d] require buffer %d not equal to %d\n",
                    w, h, frm_size, bufs->frm_size);
        }

        for (i = 0; i < cnt; i++) {
            if (bufs->frm_buf[i]) {
                mpp_buffer_put(bufs->frm_buf[i]);
                bufs->frm_buf[i] = NULL;
                bufs->frm_cnt--;
            }
        }
    }

    bufs->mb_h = aligned_h >> 4;
    if (bufs->mb_h)
        bufs->nal_tab_size = MPP_ALIGN((bufs->mb_h + 1) * sizeof(RK_U32), 8);
    else
        bufs->nal_tab_size = 0;

    bufs->yuv_size = yuv_size;
    bufs->frm_size = frm_size;

    hal_h264e_dbg_buffer("leave %p\n", bufs);

    return MPP_OK;
}

MppBuffer h264e_vepu_buf_get_nal_size_table(HalH264eVepuBufs *bufs)
{
    MppBuffer buf = bufs->nal_size_table;

    hal_h264e_dbg_buffer("enter %p\n", bufs);

    if (NULL == buf) {
        mpp_buffer_get(bufs->group, &buf, bufs->nal_tab_size);
        mpp_assert(buf);
        bufs->nal_size_table = buf;
    }

    hal_h264e_dbg_buffer("leave %p\n", bufs);

    return buf;
}

MppBuffer h264e_vepu_buf_get_frame_buffer(HalH264eVepuBufs *bufs, RK_S32 index)
{
    MppBuffer buf = bufs->frm_buf[index];

    hal_h264e_dbg_buffer("enter\n", bufs);

    if (NULL == buf) {
        mpp_buffer_get(bufs->group, &buf, bufs->frm_size);
        mpp_assert(buf);
        bufs->frm_buf[index] = buf;
        bufs->frm_cnt++;
    }

    hal_h264e_dbg_buffer("leave %p\n", bufs);

    return buf;
}

static H264eVpuCsp fmt_to_vepu_csp_yuv[MPP_FMT_YUV_BUTT] = {
    H264E_VPU_CSP_YUV420SP,     // MPP_FMT_YUV420SP         /* YYYY... UV... (NV12)     */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV420SP_10BIT   ///< Not part of ABI
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV422SP         /* YYYY... UVUV... (NV16)   */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV422SP_10BIT   ///< Not part of ABI
    H264E_VPU_CSP_YUV420P,      // MPP_FMT_YUV420P          /* YYYY... U...V...  (I420) */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV420SP_VU      /* YYYY... VUVUVU... (NV21) */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV422P          /* YYYY... UU...VV...(422P) */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV422SP_VU      /* YYYY... VUVUVU... (NV61) */
    H264E_VPU_CSP_YUYV422,      // MPP_FMT_YUV422_YUYV      /* YUYVYUYV... (YUY2)       */
    H264E_VPU_CSP_UYVY422,      // MPP_FMT_YUV422_UYVY      /* UYVYUYVY... (UYVY)       */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV400           /* YYYY...                  */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV440SP         /* YYYY... UVUV...          */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV411SP         /* YYYY... UV...            */
    H264E_VPU_CSP_NONE,         // MPP_FMT_YUV444SP         /* YYYY... UVUVUVUV...      */
};

static H264eVpuCsp fmt_to_vepu_csp_rgb[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB] = {
    H264E_VPU_CSP_RGB565,       // MPP_FMT_RGB565           /* 16-bit RGB               */
    H264E_VPU_CSP_RGB565,       // MPP_FMT_BGR565           /* 16-bit RGB               */
    H264E_VPU_CSP_RGB555,       // MPP_FMT_RGB555           /* 15-bit RGB               */
    H264E_VPU_CSP_RGB555,       // MPP_FMT_BGR555           /* 15-bit RGB               */
    H264E_VPU_CSP_RGB444,       // MPP_FMT_RGB444           /* 12-bit RGB               */
    H264E_VPU_CSP_RGB444,       // MPP_FMT_BGR444           /* 12-bit RGB               */
    H264E_VPU_CSP_RGB888,       // MPP_FMT_RGB888           /* 24-bit RGB               */
    H264E_VPU_CSP_RGB888,       // MPP_FMT_BGR888           /* 24-bit RGB               */
    H264E_VPU_CSP_RGB101010,    // MPP_FMT_RGB101010        /* 30-bit RGB               */
    H264E_VPU_CSP_RGB101010,    // MPP_FMT_BGR101010        /* 30-bit RGB               */
    H264E_VPU_CSP_ARGB8888,     // MPP_FMT_ARGB8888         /* 32-bit RGB               */
    H264E_VPU_CSP_ARGB8888,     // MPP_FMT_ABGR8888         /* 32-bit RGB               */
};

static RK_S32 fmt_to_vepu_mask_msb[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB][3] = {
    //   R   G   B              // mask msb position
    {   15, 10,  4, },          // MPP_FMT_RGB565           /* 16-bit RGB               */
    {    4, 10, 15, },          // MPP_FMT_BGR565           /* 16-bit RGB               */
    {   14,  9,  4, },          // MPP_FMT_RGB555           /* 15-bit RGB               */
    {    4,  9, 14, },          // MPP_FMT_BGR555           /* 15-bit RGB               */
    {   11,  7,  3, },          // MPP_FMT_RGB444           /* 12-bit RGB               */
    {    3,  7, 11, },          // MPP_FMT_BGR444           /* 12-bit RGB               */
    {   23, 15,  7, },          // MPP_FMT_RGB888           /* 24-bit RGB               */
    {    7, 15, 23, },          // MPP_FMT_BGR888           /* 24-bit RGB               */
    {   29, 19,  9, },          // MPP_FMT_RGB101010        /* 30-bit RGB               */
    {    9, 19, 29, },          // MPP_FMT_BGR101010        /* 30-bit RGB               */
    {   23, 15,  7, },          // MPP_FMT_ARGB8888         /* 32-bit RGB               */
    {    7, 15, 23, },          // MPP_FMT_ABGR8888         /* 32-bit RGB               */
};

MPP_RET h264e_vepu_prep_setup(HalH264eVepuPrep *prep, MppEncPrepCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    MppFrameFormat format = cfg->format;

    hal_h264e_dbg_buffer("enter\n");

    prep->src_fmt = format;
    prep->src_w = cfg->width;
    prep->src_h = cfg->height;

    if (format < MPP_FRAME_FMT_RGB) {
        // YUV case
        prep->src_fmt = fmt_to_vepu_csp_yuv[format];
        if (prep->src_fmt == H264E_VPU_CSP_NONE) {
            mpp_err("vepu do not support input frame format %d\n", format);
            ret = MPP_NOK;
        }
        prep->r_mask_msb = 0;
        prep->g_mask_msb = 0;
        prep->b_mask_msb = 0;

        prep->color_conversion_coeff_a = 0;
        prep->color_conversion_coeff_b = 0;
        prep->color_conversion_coeff_c = 0;
        prep->color_conversion_coeff_e = 0;
        prep->color_conversion_coeff_f = 0;
    } else {
        // RGB case
        RK_S32 rgb_idx = format - MPP_FRAME_FMT_RGB;

        mpp_assert(rgb_idx < MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB);

        prep->src_fmt = fmt_to_vepu_csp_rgb[rgb_idx];
        if (prep->src_fmt == H264E_VPU_CSP_NONE) {
            mpp_err("vepu do not support input frame format %d\n", format);
            ret = MPP_NOK;
        }
        prep->r_mask_msb = fmt_to_vepu_mask_msb[rgb_idx][0];
        prep->g_mask_msb = fmt_to_vepu_mask_msb[rgb_idx][1];
        prep->b_mask_msb = fmt_to_vepu_mask_msb[rgb_idx][2];

        switch (cfg->color) {
        case MPP_FRAME_SPC_RGB : {
            /* BT.601 */
            /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
             * Cb = 0.5647 (B - Y) + 128
             * Cr = 0.7132 (R - Y) + 128
             */
            prep->color_conversion_coeff_a = 19589;
            prep->color_conversion_coeff_b = 38443;
            prep->color_conversion_coeff_c = 7504;
            prep->color_conversion_coeff_e = 37008;
            prep->color_conversion_coeff_f = 46740;
        } break;
        case MPP_FRAME_SPC_BT709 : {
            /* BT.709 */
            /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
             * Cb = 0.5389 (B - Y) + 128
             * Cr = 0.6350 (R - Y) + 128
             */
            prep->color_conversion_coeff_a = 13933;
            prep->color_conversion_coeff_b = 46871;
            prep->color_conversion_coeff_c = 4732;
            prep->color_conversion_coeff_e = 35317;
            prep->color_conversion_coeff_f = 41615;
        } break;
        default : {
            prep->color_conversion_coeff_a = 19589;
            prep->color_conversion_coeff_b = 38443;
            prep->color_conversion_coeff_c = 7504;
            prep->color_conversion_coeff_e = 37008;
            prep->color_conversion_coeff_f = 46740;
        } break;
        }
    }

    RK_S32 hor_stride = cfg->hor_stride;
    RK_S32 ver_stride = cfg->ver_stride;
    prep->offset_cb = 0;
    prep->offset_cr = 0;

    switch (format) {
    case MPP_FMT_YUV420SP : {
        prep->offset_cb = hor_stride * ver_stride;
        prep->size_y = hor_stride * MPP_ALIGN(prep->src_h, 16);
        prep->size_c = hor_stride / 2 * MPP_ALIGN(prep->src_h / 2, 8);
    } break;
    case MPP_FMT_YUV420P : {
        prep->offset_cb = hor_stride * ver_stride;
        prep->offset_cr = prep->offset_cb + ((hor_stride * ver_stride) / 4);
        prep->size_y = hor_stride * MPP_ALIGN(prep->src_h, 16);
        prep->size_c = hor_stride / 2 * MPP_ALIGN(prep->src_h / 2, 8);
    } break;
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_UYVY : {
        prep->size_y = hor_stride * 2 * MPP_ALIGN(prep->src_h, 16);
        prep->size_c = 0;
    } break;
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR444 : {
        prep->size_y = hor_stride * 2 * MPP_ALIGN(prep->src_h, 16);
        prep->size_c = 0;
    } break;
    case MPP_FMT_BGR888 :
    case MPP_FMT_RGB888 :
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGR101010 : {
        prep->size_y = hor_stride * 4 * MPP_ALIGN(prep->src_h, 16);
        prep->size_c = 0;
    } break;
    default: {
        mpp_err_f("invalid format %d", format);
        ret = MPP_NOK;
    }
    }

    hal_h264e_dbg_buffer("leave\n");

    return ret;
}

MPP_RET h264e_vepu_prep_get_addr(HalH264eVepuPrep *prep, MppBuffer buffer,
                                 RK_U32 (*addr)[3])
{
    RK_U32 fd = (RK_U32)mpp_buffer_get_fd(buffer);
    size_t size = mpp_buffer_get_size(buffer);

    hal_h264e_dbg_buffer("enter\n");

    (*addr)[0] = fd;
    (*addr)[1] = fd + (prep->offset_cb << 10);
    (*addr)[2] = fd + (prep->offset_cr << 10);

    if (size < prep->size_y)
        mpp_err("warnning: input buffer size 0x%x is smaller then required size 0x%x",
                size, prep->size_y);

    if (prep->size_c && (prep->offset_cb || prep->offset_cr)) {
        if (prep->offset_cb && (size < prep->offset_cb + prep->size_c))
            mpp_err("warnning: input buffer size 0x%x is smaller then cb requirement 0x%x + 0x%x",
                    size, prep->offset_cb, prep->size_c);

        if (prep->offset_cr && (size < prep->offset_cr + prep->size_c))
            mpp_err("warnning: input buffer size 0x%x is smaller then cb requirement 0x%x + 0x%x",
                    size, prep->offset_cr, prep->size_c);
    }

    hal_h264e_dbg_buffer("leave\n");

    return MPP_OK;
}

MPP_RET h264e_vepu_mbrc_init(HalH264eVepuMbRcCtx *ctx, HalH264eVepuMbRc *mbrc)
{
    MPP_RET ret = MPP_OK;
    HalH264eVepuMbRcImpl *p = mpp_calloc(HalH264eVepuMbRcImpl, 1);
    if (!p) {
        mpp_err_f("failed to alloc rate control context\n");
        ret = MPP_ERR_NOMEM;
    }

    memset(mbrc, 0, sizeof(*mbrc));
    mbrc->qp_init   = -1;
    mbrc->qp_max    = 48;
    mbrc->qp_min    = 16;

    *ctx = p;
    return ret;
}

MPP_RET h264e_vepu_mbrc_deinit(HalH264eVepuMbRcCtx ctx)
{
    MPP_FREE(ctx);
    return MPP_OK;
}

static RK_S32 qp_tbl[2][13] = {
    {
        26, 36, 48, 63, 85, 110, 152, 208, 313, 427, 936,
        1472, 0x7fffffff
    },
    {42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12, 9, 6}
};

MPP_RET h264e_vepu_mbrc_setup(HalH264eVepuMbRcCtx ctx, MppEncCfgSet*cfg)
{
    HalH264eVepuMbRcImpl *p = (HalH264eVepuMbRcImpl *)ctx;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;

    hal_h264e_dbg_rc("enter\n");

    // get necessary parameter from config
    p->width    = prep->width;
    p->height   = prep->height;
    p->mb_w     = MPP_ALIGN(prep->width, 16) / 16;
    p->mb_h     = MPP_ALIGN(prep->height, 16) / 16;
    p->pels     = p->width * p->height;
    p->mbs      = p->mb_w * p->mb_h;
    p->bits_per_pic = axb_div_c(rc->bps_target, rc->fps_out_denorm,
                                rc->fps_out_num);

    mpp_assert(p->pels);

    // frame rate control
    mpp_assert(rc->fps_out_num / rc->fps_out_denorm <= rc->fps_in_num / rc->fps_in_denorm);

    p->fps_in_num       = rc->fps_in_num;
    p->fps_in_denorm    = rc->fps_in_denorm;
    p->fps_out_num      = rc->fps_out_num;
    p->fps_out_denorm   = rc->fps_out_denorm;

    p->fps_step         = rc->fps_in_denorm * rc->fps_out_num;
    p->fps_threshold    = rc->fps_in_num * rc->fps_out_denorm;
    p->fps_count        = p->fps_threshold;

    // if not constant
    p->mb_bit_rc_enable = rc->rc_mode != MPP_ENC_RC_MODE_FIXQP;

    // init first frame QP
    if (p->bits_per_pic > 1000000)
        p->qp_init_est = codec->qp_min;
    else {
        RK_S32 upscale = 8000;
        RK_S32 i = -1;
        RK_S32 bits_per_pic = p->bits_per_pic;
        RK_S32 mbs = p->mbs;

        bits_per_pic >>= 5;

        bits_per_pic *= mbs + 250;
        bits_per_pic /= 350 + (3 * mbs) / 4;
        bits_per_pic = axb_div_c(bits_per_pic, upscale, mbs << 6);

        while (qp_tbl[0][++i] < bits_per_pic);

        p->qp_init_est = qp_tbl[1][i];
    }
    hal_h264e_dbg_rc("estimated init qp %d\n", p->qp_init_est);

    // init first frame mad parameter
    p->mad_qp_change = 2;
    p->mad_threshold = 256 * 6;

    // init check point position
    if (p->mb_bit_rc_enable) {
        p->check_point_count = MPP_MIN(p->mb_h - 1, CHECK_POINTS_MAX);
        p->check_point_distance = p->mbs / (p->check_point_count + 1);
    } else {
        p->check_point_count = 0;
        p->check_point_distance = 0;
    }

    hal_h264e_dbg_rc("leave\n");
    return MPP_OK;
}

MPP_RET h264e_vepu_mbrc_prepare(HalH264eVepuMbRcCtx ctx, RcSyntax *rc,
                                EncFrmStatus *info, HalH264eVepuMbRc *mbrc)
{
    HalH264eVepuMbRcImpl *p = (HalH264eVepuMbRcImpl *)ctx;
    RK_S32 target_bits = rc->bit_target;
    RK_S32 target_step = target_bits / (p->check_point_count + 1);
    RK_S32 target_sum = 0;
    RK_S32 tmp;
    RK_S32 i;

    mpp_assert(target_step >= 0);
    (void) info;

    for (i = 0; i < VEPU_CHECK_POINTS_MAX; i++) {
        target_sum += target_step;
        mbrc->cp_target[i] = mpp_clip(target_sum / 32, -32768, 32767);
    }

    tmp = target_step / 8;

    mbrc->cp_error[0] = -tmp * 3;
    mbrc->cp_delta_qp[0] = -3;
    mbrc->cp_error[1] = -tmp * 2;
    mbrc->cp_delta_qp[1] = -2;
    mbrc->cp_error[2] = -tmp * 1;
    mbrc->cp_delta_qp[2] = -1;
    mbrc->cp_error[3] = tmp * 1;
    mbrc->cp_delta_qp[3] = 0;
    mbrc->cp_error[4] = tmp * 2;
    mbrc->cp_delta_qp[4] = 1;
    mbrc->cp_error[5] = tmp * 3;
    mbrc->cp_delta_qp[5] = 2;
    mbrc->cp_error[6] = tmp * 4;
    mbrc->cp_delta_qp[6] = 3;

    mbrc->mad_qp_change = 2;
    mbrc->mad_threshold = 2;

    mbrc->cp_distance_mbs = p->check_point_distance;
    return MPP_OK;
}

MPP_RET h264e_vepu_mbrc_update(HalH264eVepuMbRcCtx ctx, HalH264eVepuMbRc *mbrc)
{
    HalH264eVepuMbRcImpl *p = (HalH264eVepuMbRcImpl *)ctx;
    (void) p;
    (void) mbrc;

    hal_h264e_dbg_rc("enter\n");
    hal_h264e_dbg_rc("leave\n");
    return MPP_OK;
}
