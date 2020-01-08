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

#define MODULE_TAG  "hal_h265e_rkv"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_device.h"
#include "mpp_device_msg.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "h265e_syntax_new.h"
#include "hal_h265e_api.h"
#include "hal_h265e_rkv.h"
#include "hal_h265e_rkv_ctx.h"
#include "rc_api.h"

#define H265E_DBG_SIMPLE            0x00000010
#define H265E_DBG_REG               0x00000020
#define H265E_DBG_DETAIL            0x00000040
#define H265E_DBG_FLOW              0x00000100
extern RK_U32 hal_h265e_debug;

#define H265EHW_EN

#define h265e_hal_err(fmt, ...) \
    do {\
        mpp_err_f(fmt, ## __VA_ARGS__);\
    } while (0)

#define h265e_hal_dbg(type, fmt, ...) \
    do {\
        if (hal_h265e_debug & type)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define h265e_hal_dbg_f(type, fmt, ...) \
    do {\
        if (hal_h265e_debug & type)\
            mpp_log_f(fmt, ## __VA_ARGS__);\
    } while (0)

#define h265e_hal_enter() \
    do {\
        if (hal_h265e_debug & H265E_DBG_FLOW)\
            mpp_log("line(%d), func(%s), enter", __LINE__, __FUNCTION__);\
    } while (0)

#define h265e_hal_leave() \
    do {\
        if (hal_h265e_debug & H265E_DBG_FLOW)\
            mpp_log("line(%d), func(%s), leave", __LINE__, __FUNCTION__);\
    } while (0)

RK_U32 klut_weight[24] = {
    0x50800080, 0x00330000, 0xA1000100, 0x00660000, 0x42000200, 0x00CC0001,
    0x84000400, 0x01980002, 0x08000800, 0x03300005, 0x10001000, 0x0660000A,
    0x20002000, 0x0CC00014, 0x40004000 , 0x19800028, 0x80008000, 0x33000050,
    0x00010000, 0x660000A1, 0x00020000, 0xCC000142, 0xFF83FFFF, 0x000001FF
};
RK_U8 aq_thd_default[16] = {
    0, 0, 0, 0,  3,  3,  5,  5,
    8, 8, 8, 15, 15, 20, 25, 35
};

RK_S8 aq_qp_dealt_default[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    0,  1,  2,  3,  4,  5,  6,  8,
};

RK_U16 lvl32_intra_cst_thd[4] = {2, 6, 16, 36};

RK_U16 lvl16_intra_cst_thd[4] = {2, 6, 16, 36};

RK_U8 lvl32_intra_cst_wgt[8] = {23, 22, 21, 20, 22, 24, 26};

RK_U8 lvl16_intra_cst_wgt[8] = {17, 17, 17, 18, 17, 18, 18};

RK_U8 lvl16_4_atr_wgt[12] = {0};

RK_U16 atf_thd[14] = {0, 0, 0, 0, 0, 4, 0, 4, 0, 4, 24, 24, 24, 24}; /*thd 4, sad 4, wgt 6*/

RK_U16 atf_sad_ofst[4] = {0, 0, 0, 0};

RK_U32 lamd_satd_qp[52] = {
    0x00000183, 0x000001b2, 0x000001e7, 0x00000223, 0x00000266, 0x000002b1, 0x00000305, 0x00000364,
    0x000003ce, 0x00000445, 0x000004cb, 0x00000562, 0x0000060a, 0x000006c8, 0x0000079c, 0x0000088b,
    0x00000996, 0x00000ac3, 0x00000c14, 0x00000d8f, 0x00000f38, 0x00001115, 0x0000132d, 0x00001586,
    0x00001829, 0x00001b1e, 0x00001e70, 0x0000222b, 0x0000265a, 0x00002b0c, 0x00003052, 0x0000363c,
    0x00003ce1, 0x00004455, 0x00004cb4, 0x00005618, 0x000060a3, 0x00006c79, 0x000079c2, 0x000088ab,
    0x00009967, 0x0000ac30, 0x0000c147, 0x0000d8f2, 0x0000f383, 0x00011155, 0x000132ce, 0x00015861,
    0x0001828d, 0x0001b1e4, 0x0001e706, 0x000222ab
};

RK_U32 lamd_moda_qp[52] = {
    0x00000049 , 0x0000005c , 0x00000074 , 0x00000092 , 0x000000b8 , 0x000000e8 , 0x00000124 , 0x00000170,
    0x000001cf , 0x00000248 , 0x000002df , 0x0000039f , 0x0000048f , 0x000005bf , 0x0000073d , 0x0000091f,
    0x00000b7e , 0x00000e7a , 0x0000123d , 0x000016fb , 0x00001cf4 , 0x0000247b , 0x00002df6 , 0x000039e9,
    0x000048f6 , 0x00005bed , 0x000073d1 , 0x000091ec , 0x0000b7d9 , 0x0000e7a2 , 0x000123d7 , 0x00016fb2,
    0x0001cf44 , 0x000247ae , 0x0002df64 , 0x00039e89 , 0x00048f5c , 0x0005bec8 , 0x00073d12 , 0x00091eb8,
    0x000b7d90 , 0x000e7a23 , 0x00123d71 , 0x0016fb20 , 0x001cf446 , 0x00247ae1 , 0x002df640 , 0x0039e88c,
    0x0048f5c3, 0x005bec81, 0x0073d119, 0x0091eb85
};

RK_U32 lamd_modb_qp[52] = {
    0x00000070, 0x00000089, 0x000000b0, 0x000000e0, 0x00000112, 0x00000160, 0x000001c0, 0x00000224,
    0x000002c0, 0x00000380, 0x00000448, 0x00000580, 0x00000700, 0x00000890, 0x00000b00, 0x00000e00,
    0x00001120, 0x00001600, 0x00001c00, 0x00002240, 0x00002c00, 0x00003800, 0x00004480, 0x00005800,
    0x00007000, 0x00008900, 0x0000b000, 0x0000e000, 0x00011200, 0x00016000, 0x0001c000, 0x00022400,
    0x0002c000, 0x00038000, 0x00044800, 0x00058000, 0x00070000, 0x00089000, 0x000b0000, 0x000e0000,
    0x00112000, 0x00160000, 0x001c0000, 0x00224000, 0x002c0000, 0x00380000, 0x00448000, 0x00580000,
    0x00700000, 0x00890000, 0x00b00000, 0x00e00000
};

static RK_U32 mb_num[12] = {
    0x0,    0xc8,   0x2bc,  0x4b0,
    0x7d0,  0xfa0,  0x1f40, 0x3e80,
    0x4e20, 0x4e20, 0x4e20, 0x4e20,
};

static RK_U32 tab_bit[12] = {
    0xEC4, 0xDF2, 0xC4E, 0xB7C, 0xAAA, 0xEC4, 0x834, 0x690, 0x834, 0x834, 0x834, 0x834,
};

static RK_U8 qp_table[96] = {
    0xF,  0xF,  0xF, 0xF,   0xF,  0x10, 0x12, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x19, 0x1A, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E,
    0x1E, 0x1F, 0x1F, 0x20, 0x20, 0x21, 0x21, 0x21, 0x22, 0x22, 0x22,
    0x22, 0x23, 0x23, 0x23, 0x24, 0x24, 0x24, 0x24, 0x24, 0x25, 0x25,
    0x25, 0x25, 0x26, 0x26, 0x26, 0x26, 0x26, 0x27, 0x27, 0x27, 0x27,
    0x27, 0x27, 0x28, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x29, 0x29,
    0x29, 0x29, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2B,
    0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2C,
    0x2C, 0x2C, 0x2C, 0x2C, 0x2D, 0x2D, 0x2D, 0x2D,
};

static RK_S32 cal_first_i_start_qp(RK_S32 target_bit, RK_U32 total_mb)
{
    RK_S32 cnt = 0, index, i;

    for (i = 0; i < 11; i++) {
        if (mb_num[i] > total_mb)
            break;
        cnt++;
    }
    index = (total_mb * tab_bit[cnt] - 300) / target_bit; //
    index = mpp_clip(index, 4, 95);

    return qp_table[index];
}


static MPP_RET h265e_rkv_free_buffers(H265eRkvHalContext *ctx)
{
    RK_S32 k = 0;
    h265e_hal_rkv_buffers *buffers = (h265e_hal_rkv_buffers *)ctx->buffers;
    h265e_hal_enter();

    for (k = 0; k < RKVE_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_mei_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_mei_buf[k])) {
                h265e_hal_err("hw_mei_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    for (k = 0; k < RKVE_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_roi_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_roi_buf[k])) {
                h265e_hal_err("hw_roi_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    h265e_hal_leave();
    return MPP_OK;
}

RK_U32 convert_format(MppFrameFormat format)
{
    RK_U32 input_format;
    switch (format) {
    case MPP_FMT_YUV420P: {
        input_format = (RK_U32)RKVE_CSP_YUV420P;
        break;
    }
    case MPP_FMT_YUV420SP: {
        input_format = (RK_U32)RKVE_CSP_YUV420SP;
        break;
    }
    case MPP_FMT_YUV420SP_10BIT: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV420SP_VU: { //TODO: to be confirmed
        input_format = (RK_U32)RKVE_CSP_YUV420SP;
        break;
    }
    case MPP_FMT_YUV422P: {
        input_format = (RK_U32)RKVE_CSP_YUV422P;
        break;
    }
    case MPP_FMT_YUV422SP: {
        input_format = (RK_U32)RKVE_CSP_YUV422SP;
        break;
    }
    case MPP_FMT_YUV422SP_10BIT: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV422SP_VU: {
        input_format = (RK_U32)RKVE_CSP_YUV422SP;
        break;
    }
    case MPP_FMT_YUV422_YUYV: {
        input_format = (RK_U32)RKVE_CSP_YUYV422;
        break;
    }
    case MPP_FMT_YUV422_UYVY: {
        input_format = (RK_U32)RKVE_CSP_UYVY422;
        break;
    }
    case MPP_FMT_RGB565: {
        input_format = (RK_U32)RKVE_CSP_BGR565;
        break;
    }
    case MPP_FMT_BGR565: {
        input_format = (RK_U32)RKVE_CSP_BGR565;
        break;
    }
    case MPP_FMT_RGB555: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_BGR555: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_RGB444: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_BGR444: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_RGB888: {
        input_format = (RK_U32)RKVE_CSP_BGR888;
        break;
    }
    case MPP_FMT_BGR888: {
        input_format = (RK_U32)RKVE_CSP_BGR888;
        break;
    }
    case MPP_FMT_RGB101010: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_BGR101010: {
        input_format = (RK_U32)RKVE_CSP_NONE;
        break;
    }
    case MPP_FMT_ARGB8888: {
        input_format = (RK_U32)RKVE_CSP_BGRA8888;
        break;
    }
    case MPP_FMT_ABGR8888: {
        input_format = (RK_U32)RKVE_CSP_BGRA8888;
        break;
    }
    default: {
        h265e_hal_err("unvalid src color space: %d", format);
        input_format = (RK_U32)RKVE_CSP_NONE;
    }
    }
    return input_format;
}

static MPP_RET h265e_rkv_allocate_buffers(H265eRkvHalContext *ctx, H265eSyntax_new *syn)
{
    RK_S32 k = 0;
    MPP_RET ret = MPP_OK;
    h265e_hal_rkv_buffers *buffers = (h265e_hal_rkv_buffers *)ctx->buffers;
    RK_U32 num_mbs_oneframe;
    RK_U32 frame_size;
    RkveCsp input_fmt = RKVE_CSP_YUV420P;
    h265e_hal_enter();

    num_mbs_oneframe = (syn->pp.pic_width + 15) / 16 * ((syn->pp.pic_height + 15) / 16);
    frame_size = MPP_ALIGN(syn->pp.pic_width, 16) * MPP_ALIGN(syn->pp.pic_height, 16);

    input_fmt = convert_format(syn->pp.mpp_format);
    ctx->input_fmt = input_fmt;
    switch (input_fmt) {
    case RKVE_CSP_YUV420P:
    case RKVE_CSP_YUV420SP: {
        frame_size = frame_size * 3 / 2;
    } break;
    case RKVE_CSP_YUV422P:
    case RKVE_CSP_YUV422SP:
    case RKVE_CSP_YUYV422:
    case RKVE_CSP_UYVY422:
    case RKVE_CSP_BGR565: {
        frame_size *= 2;
    } break;
    case RKVE_CSP_BGR888: {
        frame_size *= 3;
    } break;
    case RKVE_CSP_BGRA8888: {
        frame_size *= 4;
    } break;
    default: {
        h265e_hal_err("invalid src color space: %d\n", input_fmt);
        return MPP_NOK;
    }
    }

    if (frame_size > ctx->frame_size) {
        h265e_rkv_free_buffers(ctx);
        if (1) {
            for (k = 0; k < RKVE_LINKTABLE_FRAME_NUM; k++) {
                ret = mpp_buffer_get(buffers->hw_buf_grp[ENC_HAL_RKV_BUF_GRP_ROI],
                                     &buffers->hw_roi_buf[k], num_mbs_oneframe * 1);
                if (ret) {
                    h265e_hal_err("hw_roi_buf[%d] get failed", k);
                    return ret;
                }
            }
        }
        ctx->frame_size = frame_size;
    }
    h265e_hal_leave();
    return ret;
}

static void h265e_rkv_set_l2_regs(H265eRkvHalContext *ctx, H265eRkvL2RegSet *regs)
{
    MppDevReqV1 req;
    memcpy(&regs->lvl32_intra_CST_THD0, lvl32_intra_cst_thd, sizeof(lvl32_intra_cst_thd));
    memcpy(&regs->lvl16_intra_CST_THD0, lvl16_intra_cst_thd, sizeof(lvl16_intra_cst_thd));
    memcpy(&regs->lvl32_intra_CST_WGT0, lvl32_intra_cst_wgt, sizeof(lvl16_intra_cst_wgt));
    memcpy(&regs->lvl16_intra_CST_WGT0, lvl16_intra_cst_wgt, sizeof(lvl16_intra_cst_wgt));
    regs->rdo_quant.quant_f_bias_I = 171;
    regs->rdo_quant.quant_f_bias_P = 85;
    memcpy(&regs->atr_thd0, atf_thd, sizeof(atf_thd));
    memcpy(&regs->lvl16_atr_wgt, lvl16_4_atr_wgt, sizeof(lvl16_4_atr_wgt));
    memcpy(&regs->atf_thd0, atf_thd, sizeof(atf_thd));
    regs->atf_sad_ofst0.atf_wgt_p16 = 16;

    memcpy(&regs->atf_sad_ofst1, atf_sad_ofst, sizeof(atf_sad_ofst));
    memcpy(&regs->lamd_satd_qp[0], lamd_satd_qp, sizeof(lamd_satd_qp));
    memcpy(&regs->lamd_moda_qp[0], lamd_moda_qp, sizeof(lamd_moda_qp));
    memcpy(&regs->lamd_modb_qp[0], lamd_modb_qp, sizeof(lamd_modb_qp));

    memcpy(&regs->aq_thd0, aq_thd_default, sizeof(aq_thd_default));
    memcpy(&regs->aq_qp_dlt0, aq_qp_dealt_default, sizeof(aq_qp_dealt_default));

#ifdef H265EHW_EN
    req.cmd = MPP_CMD_SET_REG_WRITE;
    req.flag = 0;
    req.offset = 0x10004;
    req.size = sizeof(H265eRkvL2RegSet);
    req.data = (void*)regs;
    mpp_device_add_request(ctx->dev_ctx, &req);
#endif
    return;
}


MPP_RET hal_h265e_rkv_init(void *hal, MppEncHalCfg *cfg)
{
    RK_U32 k = 0;
    MPP_RET ret = MPP_OK;
    H265eRkvHalContext *ctx = (H265eRkvHalContext *)hal;
    h265e_hal_rkv_buffers *buffers = NULL;
    h265e_hal_enter();
    ctx->ioctl_input    = mpp_calloc(H265eRkvIoctlInput, 1);
    ctx->ioctl_output   = mpp_calloc(H265eRkvIoctlOutput, 1);
    ctx->regs           = mpp_calloc(H265eRkvRegSet, RKVE_LINKTABLE_FRAME_NUM);
    ctx->l2_regs        = mpp_calloc(H265eRkvL2RegSet, RKVE_LINKTABLE_FRAME_NUM);
    ctx->rc_hal_cfg     = mpp_calloc(RcHalCfg, RKVE_LINKTABLE_FRAME_NUM);
    ctx->buffers        = mpp_calloc(h265e_hal_rkv_buffers, RKVE_LINKTABLE_FRAME_NUM);
    ctx->set            = cfg->set;
    ctx->cfg            = cfg->cfg;

    ctx->frame_cnt = 0;
    ctx->frame_cnt_gen_ready = 0;
    ctx->frame_cnt_send_ready = 0;
    ctx->num_frames_to_send = 1;
    ctx->osd_plt_type = RKVE_OSD_PLT_TYPE_NONE;
    ctx->enc_mode = RKV_ENC_MODE;

    MppDevCfg dev_cfg = {
        .type = MPP_CTX_ENC,            /* type */
        .coding = MPP_VIDEO_CodingHEVC,  /* coding */
        .platform = HAVE_RKVENC,         /* platform */
        .pp_enable = 0,                  /* pp_enable */
    };

#ifdef H265EHW_EN
    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);

    if (ret != MPP_OK) {
        h265e_hal_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }
#endif

    buffers = (h265e_hal_rkv_buffers *)ctx->buffers;
    for (k = 0; k < ENC_HAL_RKV_BUF_GRP_BUTT; k++) {
        if (MPP_OK != mpp_buffer_group_get_internal(&buffers->hw_buf_grp[k], MPP_BUFFER_TYPE_ION)) {
            h265e_hal_err("buf group[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }

    h265e_rkv_set_l2_regs(ctx, (H265eRkvL2RegSet*)ctx->l2_regs);
    h265e_hal_leave();
    return ret;
}

MPP_RET hal_h265e_rkv_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    H265eRkvHalContext *ctx = (H265eRkvHalContext *)hal;
    h265e_hal_enter();
    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->l2_regs);
    MPP_FREE(ctx->ioctl_input);
    MPP_FREE(ctx->ioctl_output);
    MPP_FREE(ctx->rc_hal_cfg);

    if (ctx->buffers) {
        RK_U32 k = 0;
        h265e_hal_rkv_buffers *buffers = (h265e_hal_rkv_buffers *)ctx->buffers;

        h265e_rkv_free_buffers(ctx);
        for (k = 0; k < ENC_HAL_RKV_BUF_GRP_BUTT; k++) {
            if (buffers->hw_buf_grp[k]) {
                if (MPP_OK != mpp_buffer_group_put(buffers->hw_buf_grp[k])) {
                    h265e_hal_err("buf group[%d] put failed", k);
                    return MPP_NOK;
                }
            }
        }
        MPP_FREE(ctx->buffers);
    }
    if (ctx->dev_ctx) {
        ret = mpp_device_deinit(ctx->dev_ctx);
        if (ret)
            h265e_hal_err("mpp_device_deinit failed. ret: %d\n", ret);
    }
    h265e_hal_leave();
    return MPP_OK;
}

static MPP_RET
h265e_rkv_set_ioctl_extra_info(H265eRkvIoctlExtraInfo *extra_info,
                               H265eSyntax_new *syn, RkveCsp input_fmt)
{
    H265eRkvIoctlExtraInfoElem *info = NULL;
    RK_U32 hor_stride = syn->pp.hor_stride;
    RK_U32 ver_stride = syn->pp.ver_stride ? syn->pp.ver_stride : syn->pp.pic_height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;

    switch (input_fmt) {
    case RKVE_CSP_YUV420P: {
        u_offset = frame_size;
        v_offset = frame_size * 5 / 4;
    } break;
    case RKVE_CSP_YUV420SP:
    case RKVE_CSP_YUV422SP: {
        u_offset = frame_size;
        v_offset = frame_size;
    } break;
    case RKVE_CSP_YUV422P: {
        u_offset = frame_size;
        v_offset = frame_size * 3 / 2;
    } break;
    case RKVE_CSP_YUYV422:
    case RKVE_CSP_UYVY422: {
        u_offset = 0;
        v_offset = 0;
    } break;
    default: {
        h265e_hal_err("unknown color space: %d\n", input_fmt);
        u_offset = frame_size;
        v_offset = frame_size * 5 / 4;
    }
    }

    extra_info->magic = 0;
    extra_info->cnt = 2;

    /* input cb addr */
    info = &extra_info->elem[0];
    info->reg_idx = 71;
    info->offset  = u_offset;

    /* input cr addr */
    info = &extra_info->elem[1];
    info->reg_idx = 72;
    info->offset  = v_offset;

    return MPP_OK;
}

static MPP_RET h265e_rkv_set_osd_regs(H265eRkvHalContext *ctx, H265eRkvRegSet *regs)
{

#define ENC_DEFAULT_OSD_INV_THR 15 //TODO: open interface later
    MppEncOSDData *osd_data = ctx->osd_data;
    if (!osd_data) {
        return MPP_OK;
    }
    mpp_log("set_osd_regs xxxxxx ");
    if ( osd_data->num_region && osd_data->buf) { // enable OSD

        RK_U32 num = osd_data->num_region;
        RK_S32 buf_fd = mpp_buffer_get_fd(osd_data->buf);

        if (buf_fd >= 0) {
            RK_U32 k = 0;
            MppEncOSDRegion *region = osd_data->region;

            regs->osd_cfg.osd_clk_sel    = 1;
            regs->osd_cfg.osd_plt_type   = ctx->osd_plt_type == RKVE_OSD_PLT_TYPE_NONE ?
                                           RKVE_OSD_PLT_TYPE_DEFAULT :
                                           ctx->osd_plt_type;

            for (k = 0; k < num; k++) {
                regs->osd_cfg.osd_en |= region[k].enable << k;
                regs->osd_cfg.osd_inv |= region[k].inverse << k;

                if (region[k].enable) {
                    regs->osd_pos[k].lt_pos_x = region[k].start_mb_x;
                    regs->osd_pos[k].lt_pos_y = region[k].start_mb_y;
                    regs->osd_pos[k].rd_pos_x =
                        region[k].start_mb_x + region[k].num_mb_x - 1;
                    regs->osd_pos[k].rd_pos_y =
                        region[k].start_mb_y + region[k].num_mb_y - 1;

                    regs->osd_addr[k] =
                        buf_fd | (region[k].buf_offset << 10);
                }
            }

            if (region[0].inverse)
                regs->osd_inv.osd_inv_r0 = ENC_DEFAULT_OSD_INV_THR;
            if (region[1].inverse)
                regs->osd_inv.osd_inv_r1 = ENC_DEFAULT_OSD_INV_THR;
            if (region[2].inverse)
                regs->osd_inv.osd_inv_r2 = ENC_DEFAULT_OSD_INV_THR;
            if (region[3].inverse)
                regs->osd_inv.osd_inv_r3 = ENC_DEFAULT_OSD_INV_THR;
            if (region[4].inverse)
                regs->osd_inv.osd_inv_r4 = ENC_DEFAULT_OSD_INV_THR;
            if (region[5].inverse)
                regs->osd_inv.osd_inv_r5 = ENC_DEFAULT_OSD_INV_THR;
            if (region[6].inverse)
                regs->osd_inv.osd_inv_r6 = ENC_DEFAULT_OSD_INV_THR;
            if (region[7].inverse)
                regs->osd_inv.osd_inv_r7 = ENC_DEFAULT_OSD_INV_THR;
        }
    }

    return MPP_OK;
}

MPP_RET h265e_config_roi_area(H265eRkvHalContext *ctx, RK_U8 *roi_base)
{
    h265e_hal_enter();
    RK_U32 ret = MPP_OK, idx = 0, num = 0;
    RK_U32 init_pos_x, init_pos_y, roi_width, roi_height, mb_width, pos_y;
    RkvRoiCfg_v2 cfg;
    MppEncROIRegion *region;
    RK_U8 *ptr = roi_base;

    if (ctx == NULL || roi_base == NULL) {
        mpp_err("NULL pointer ctx %p roi_base %p\n", ctx, roi_base);
        return MPP_NOK;
    }
    region = ctx->roi_data->regions;
    for (num = 0; num < ctx->roi_data->number; num++) {
        init_pos_x = (region->x + 15) / 16;
        init_pos_y = (region->y + 15) / 16;
        roi_width = (region->w + 15) / 16;
        roi_height = (region->h + 15) / 16;
        mb_width = (ctx->cfg->prep.width + 15) / 16;
        pos_y = init_pos_y;

        for (idx = 0; idx < roi_width * roi_height; idx++) {
            if (idx % roi_width == 0)
                pos_y = init_pos_y + idx / roi_width;
            ptr = roi_base + (pos_y * mb_width + init_pos_x) + (idx % roi_width);
            cfg.qp_area_idx = region->qp_area_idx;
            cfg.area_map = region->area_map_en;
            cfg.qp_y = region->quality;
            cfg.set_qp_y = region->abs_qp_en;
            cfg.forbid_inter = region->intra;
            memcpy(ptr, &cfg, sizeof(RkvRoiCfg_v2));
        }
        region++;
    }
    h265e_hal_leave();
    return ret;
}

static MPP_RET
h265e_rkv_set_roi_regs(H265eRkvHalContext *ctx, H265eRkvRegSet *regs)
{
    MppEncROICfg *cfg = (MppEncROICfg*)ctx->roi_data;
    h265e_hal_rkv_buffers *bufs = (h265e_hal_rkv_buffers *)ctx->buffers;
    RK_U8 *roi_base;
    if (!cfg) {
        return MPP_OK;
    }

    if (cfg->number && cfg->regions) {
        regs->enc_pic.roi_en = 1;
        regs->roi_addr_hevc = mpp_buffer_get_fd(bufs->hw_roi_buf[0]);
        roi_base = (RK_U8 *)mpp_buffer_get_ptr(bufs->hw_roi_buf[0]);
        mpp_log("config roi");
        h265e_config_roi_area(ctx, roi_base);
    }
    return MPP_OK;
}

static MPP_RET h265e_rkv_set_rc_regs(H265eRkvHalContext *ctx, H265eRkvRegSet *regs, H265eSyntax_new *syn)
{
    H265eFrmInfo *frms = &syn->frms;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncRcCfg *rc = &cfg->rc;
    RcHalCfg    *rc_cfg = &frms->rc_cfg;
    MppEncCodecCfg *codec = &cfg->codec;
    MppEncH265Cfg *h265 = &codec->h265;
    RK_U32 ctu_target_bits_mul_16 = (rc_cfg->bit_target << 4) / frms->mb_per_frame;
    RK_U32 ctu_target_bits;
    RK_S32 negative_bits_thd, positive_bits_thd;

    if (frms->seq_idx == 0 && frms->status.is_intra) {
        if (h265->qp_init == -1) {
            ctx->start_qp = cal_first_i_start_qp(frms->rc_cfg.bit_target, frms->mb_per_frame << 4);
            ctx->cur_scale_qp = (ctx->start_qp  + h265->ip_qp_delta) << 6;
        } else {

            ctx->start_qp = h265->qp_init;
            ctx->cur_scale_qp = (ctx->start_qp + h265->ip_qp_delta) << 6;
        }

        ctx->pre_i_qp = ctx->cur_scale_qp >> 6;
        ctx->pre_p_qp = ctx->cur_scale_qp >> 6;
    } else {
        RK_S32 qp_scale = ctx->cur_scale_qp + frms->rc_cfg.next_ratio;
        RK_S32 start_qp = 0;

        if (frms->status.is_intra) {
            qp_scale = mpp_clip(qp_scale, (h265->min_i_qp << 6), (h265->max_i_qp << 6));
            start_qp = ((ctx->pre_i_qp + ((qp_scale + frms->rc_cfg.next_i_ratio) >> 6)) >> 1);

            start_qp = mpp_clip(start_qp, h265->min_i_qp, h265->max_i_qp);
            ctx->pre_i_qp = start_qp;
            ctx->start_qp = start_qp;
            ctx->cur_scale_qp = qp_scale;
        } else {
            qp_scale = mpp_clip(qp_scale, (h265->min_qp << 6), (h265->max_qp << 6));
            ctx->cur_scale_qp = qp_scale;
            ctx->start_qp = qp_scale >> 6;
        }
    }
    if (rc->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        regs->enc_pic.pic_qp      = h265->qp_init;
        regs->synt_sli1.sli_qp    = h265->qp_init;

        regs->rc_qp.rc_max_qp     = h265->qp_init;
        regs->rc_qp.rc_min_qp     = h265->qp_init;
    } else {
        RK_S32 rc_max_qp, rc_min_qp;
        if (ctu_target_bits_mul_16 >= 0x100000) {
            ctu_target_bits_mul_16 = 0x50000;
        }
        ctu_target_bits = (ctu_target_bits_mul_16 * frms->mb_wid) >> 4;
        negative_bits_thd = 0 - ctu_target_bits / 4;
        positive_bits_thd = ctu_target_bits / 4;

        if (frms->status.is_intra) {
            rc_max_qp = h265->max_i_qp;
            rc_min_qp = h265->min_i_qp;
        } else {
            rc_max_qp = h265->max_qp;
            rc_min_qp = h265->min_qp;
        }
        mpp_log("ctx->start_qp = %d", ctx->start_qp);
        regs->enc_pic.pic_qp      = ctx->start_qp;
        regs->rc_cfg.rc_en        = 1;
        regs->synt_sli1.sli_qp    = ctx->start_qp;

        regs->rc_cfg.rc_ctu_num   = frms->mb_wid;
        regs->rc_qp.rc_qp_range   =  h265->raw_dealt_qp;
        regs->rc_qp.rc_max_qp     = rc_max_qp;
        regs->rc_qp.rc_min_qp     = rc_min_qp;
        regs->rc_tgt.ctu_ebits    = ctu_target_bits_mul_16;

        regs->rc_erp0.bits_thd0    = negative_bits_thd;
        regs->rc_erp1.bits_thd1    = positive_bits_thd;
        regs->rc_erp2.bits_thd2    = positive_bits_thd;
        regs->rc_erp3.bits_thd3    = positive_bits_thd;
        regs->rc_erp4.bits_thd4    = positive_bits_thd;
        regs->rc_erp5.bits_thd5    = positive_bits_thd;
        regs->rc_erp6.bits_thd6    = positive_bits_thd;
        regs->rc_erp7.bits_thd7    = positive_bits_thd;
        regs->rc_erp8.bits_thd8    = positive_bits_thd;

        regs->rc_adj0.qp_adjust0    = -1;
        regs->rc_adj0.qp_adjust1    = 0;
        regs->rc_adj0.qp_adjust2    = 0;
        regs->rc_adj0.qp_adjust3    = 0;
        regs->rc_adj0.qp_adjust4    = 0;
        regs->rc_adj1.qp_adjust5    = 0;
        regs->rc_adj1.qp_adjust6    = 0;
        regs->rc_adj1.qp_adjust7    = 0;
        regs->rc_adj1.qp_adjust8    = 1;

        regs->qpmap0.qpmin_area0 = h265->qpmin_map[0];
        regs->qpmap0.qpmax_area0 = h265->qpmax_map[0];
        regs->qpmap0.qpmin_area1 = h265->qpmin_map[1];
        regs->qpmap0.qpmax_area1 = h265->qpmax_map[1];
        regs->qpmap0.qpmin_area2 = h265->qpmin_map[2];
        regs->qpmap1.qpmax_area2 = h265->qpmax_map[2];
        regs->qpmap1.qpmin_area3 = h265->qpmin_map[3];
        regs->qpmap1.qpmax_area3 = h265->qpmax_map[3];
        regs->qpmap1.qpmin_area4 = h265->qpmin_map[4];
        regs->qpmap1.qpmax_area4 = h265->qpmax_map[4];
        regs->qpmap2.qpmin_area5 = h265->qpmin_map[5];
        regs->qpmap2.qpmax_area5 = h265->qpmax_map[5];
        regs->qpmap2.qpmin_area6 = h265->qpmin_map[6];
        regs->qpmap2.qpmax_area6 = h265->qpmax_map[6];
        regs->qpmap2.qpmin_area7 = h265->qpmin_map[7];
        regs->qpmap3.qpmax_area7 = h265->qpmax_map[7];
        regs->qpmap3.qpmap_mode  = h265->qpmap_mode;
    }
    if (ctx->frame_type == INTRA_FRAME) {
        regs->enc_pic.rdo_wgt_sel = 0;
    } else {
        regs->enc_pic.rdo_wgt_sel = 1;
    }
    regs->dtrns_cfg.cime_dspw_orsd = (ctx->frame_type == INTER_P_FRAME);
    return MPP_OK;
}

static MPP_RET h265e_rkv_set_pp_regs(H265eRkvRegSet *regs, MppEncPrepCfg *prep_cfg)
{
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    regs->src_proc.src_rot = prep_cfg->rotation;

    if (prep_cfg->hor_stride) {
        stridey = prep_cfg->hor_stride;
    } else {
        stridey = prep_cfg->width ;
        if (regs->src_fmt.src_cfmt == RKVE_CSP_BGRA8888 )
            stridey = (stridey + 1) * 4;
        else if (regs->src_fmt.src_cfmt == RKVE_CSP_BGR888 )
            stridey = (stridey + 1) * 3;
        else if ( regs->src_fmt.src_cfmt == RKVE_CSP_BGR565
                  || regs->src_fmt.src_cfmt == RKVE_CSP_YUYV422
                  || regs->src_fmt.src_cfmt == RKVE_CSP_UYVY422 )
            stridey = (stridey + 1) * 2;
    }
    stridec = (regs->src_fmt.src_cfmt == RKVE_CSP_YUV422SP
               || regs->src_fmt.src_cfmt == RKVE_CSP_YUV420SP)
              ? stridey : stridey / 2;

    regs->src_strid.src_ystrid    = stridey;
    regs->src_strid.src_cstrid    = stridec;
    return MPP_OK;
}

static void h265e_rkv_set_slice_regs(H265eSyntax_new *syn, H265eRkvRegSet *regs)
{
    regs->synt_sps.smpl_adpt_ofst_en    = syn->pp.sample_adaptive_offset_enabled_flag;//slice->m_sps->m_bUseSAO;
    regs->synt_sps.num_st_ref_pic       = syn->pp.num_short_term_ref_pic_sets;
    regs->synt_sps.num_lt_ref_pic       = syn->pp.num_long_term_ref_pics_sps;
    regs->synt_sps.lt_ref_pic_prsnt     = syn->pp.long_term_ref_pics_present_flag;
    regs->synt_sps.tmpl_mvp_en          = syn->pp.sps_temporal_mvp_enabled_flag;
    regs->synt_sps.log2_max_poc_lsb     = syn->pp.log2_max_pic_order_cnt_lsb_minus4;
    regs->synt_sps.strg_intra_smth      = syn->pp.strong_intra_smoothing_enabled_flag;

    regs->synt_pps.dpdnt_sli_seg_en     = syn->pp.dependent_slice_segments_enabled_flag;
    regs->synt_pps.out_flg_prsnt_flg    = syn->pp.output_flag_present_flag;
    regs->synt_pps.num_extr_sli_hdr     = syn->pp.num_extra_slice_header_bits;
    regs->synt_pps.sgn_dat_hid_en       = syn->pp.sign_data_hiding_enabled_flag;
    regs->synt_pps.cbc_init_prsnt_flg   = syn->pp.cabac_init_present_flag;
    regs->synt_pps.pic_init_qp          = syn->pp.init_qp_minus26 + 26;
    regs->synt_pps.cu_qp_dlt_en         = syn->pp.cu_qp_delta_enabled_flag;
    regs->synt_pps.chrm_qp_ofst_prsn    = syn->pp.pps_slice_chroma_qp_offsets_present_flag;
    regs->synt_pps.lp_fltr_acrs_sli     = syn->pp.pps_loop_filter_across_slices_enabled_flag;
    regs->synt_pps.dblk_fltr_ovrd_en    = syn->pp.deblocking_filter_override_enabled_flag;
    regs->synt_pps.lst_mdfy_prsnt_flg   = syn->pp.lists_modification_present_flag;
    regs->synt_pps.sli_seg_hdr_extn     = syn->pp.slice_segment_header_extension_present_flag;
    regs->synt_pps.cu_qp_dlt_depth      = syn->pp.diff_cu_qp_delta_depth;


    regs->synt_sli0.cbc_init_flg        = syn->sp.cbc_init_flg;
    regs->synt_sli0.mvd_l1_zero_flg     = syn->sp.mvd_l1_zero_flg;
    regs->synt_sli0.merge_up_flag       = syn->sp.merge_up_flag;
    regs->synt_sli0.merge_left_flag     = syn->sp.merge_left_flag;
    regs->synt_sli0.ref_pic_lst_mdf_l0  = syn->sp.ref_pic_lst_mdf_l0;

    regs->synt_sli0.num_refidx_l1_act   = syn->sp.num_refidx_l1_act;
    regs->synt_sli0.num_refidx_l0_act   = syn->sp.num_refidx_l0_act;

    regs->synt_sli0.num_refidx_act_ovrd = syn->sp.num_refidx_act_ovrd;

    regs->synt_sli0.sli_sao_chrm_flg    = syn->sp.sli_sao_chrm_flg;
    regs->synt_sli0.sli_sao_luma_flg    = syn->sp.sli_sao_luma_flg;
    regs->synt_sli0.sli_tmprl_mvp_en    = syn->sp.sli_tmprl_mvp_en;
    regs->enc_pic.tot_poc_num           = syn->sp.tot_poc_num;

    regs->synt_sli0.pic_out_flg         = syn->sp.pic_out_flg;
    regs->synt_sli0.sli_type            = syn->sp.slice_type;
    regs->synt_sli0.sli_rsrv_flg        = syn->sp.slice_rsrv_flg;
    regs->synt_sli0.dpdnt_sli_seg_flg   = syn->sp.dpdnt_sli_seg_flg;
    regs->synt_sli0.sli_pps_id          = syn->sp.sli_pps_id;
    regs->synt_sli0.no_out_pri_pic      = syn->sp.no_out_pri_pic;


    regs->synt_sli1.sli_tc_ofst_div2      = syn->sp.sli_tc_ofst_div2;;
    regs->synt_sli1.sli_beta_ofst_div2    = syn->sp.sli_beta_ofst_div2;
    regs->synt_sli1.sli_lp_fltr_acrs_sli  = syn->sp.sli_lp_fltr_acrs_sli;
    regs->synt_sli1.sli_dblk_fltr_dis     = syn->sp.sli_dblk_fltr_dis;
    regs->synt_sli1.dblk_fltr_ovrd_flg    = syn->sp.dblk_fltr_ovrd_flg;
    regs->synt_sli1.sli_cb_qp_ofst        = syn->sp.sli_cb_qp_ofst;
    regs->synt_sli1.max_mrg_cnd           = syn->sp.max_mrg_cnd;

    regs->synt_sli1.col_ref_idx           = syn->sp.col_ref_idx;
    regs->synt_sli1.col_frm_l0_flg        = syn->sp.col_frm_l0_flg;
    regs->synt_sli2_rodr.sli_poc_lsb      = syn->sp.sli_poc_lsb;
    regs->synt_sli2_rodr.sli_hdr_ext_len  = syn->sp.sli_hdr_ext_len;

}

static void h265e_rkv_set_ref_regs(H265eSyntax_new *syn, H265eRkvRegSet *regs)
{
    regs->synt_ref_mark0.st_ref_pic_flg = syn->sp.st_ref_pic_flg;
    regs->synt_ref_mark0.poc_lsb_lt0 = syn->sp.poc_lsb_lt0;

    regs->synt_ref_mark1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->synt_ref_mark1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->synt_ref_mark1.used_by_lt_flg0 = syn->sp.used_by_lt_flg0;
    regs->synt_ref_mark1.used_by_lt_flg1 = syn->sp.used_by_lt_flg1;
    regs->synt_ref_mark1.used_by_lt_flg2 = syn->sp.used_by_lt_flg2;
    regs->synt_ref_mark1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->synt_ref_mark1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->synt_ref_mark1.dlt_poc_msb_prsnt1 = syn->sp.dlt_poc_msb_prsnt1;
    regs->synt_ref_mark1.num_neg_pic = syn->sp.num_neg_pic;
    regs->synt_ref_mark1.num_pos_pic = syn->sp.num_pos_pic;

    regs->synt_ref_mark1.used_by_s0_flg = syn->sp.used_by_s0_flg;
    regs->synt_ref_mark2.dlt_poc_s0_m10 = syn->sp.dlt_poc_s0_m10;
    regs->synt_ref_mark2.dlt_poc_s0_m11 = syn->sp.dlt_poc_s0_m11;
    regs->synt_ref_mark3.dlt_poc_s0_m12 = syn->sp.dlt_poc_s0_m12;
    regs->synt_ref_mark3.dlt_poc_s0_m13 = syn->sp.dlt_poc_s0_m13;

    regs->synt_ref_mark4.poc_lsb_lt1 = syn->sp.poc_lsb_lt1;
    regs->synt_ref_mark5.dlt_poc_msb_cycl1 = syn->sp.dlt_poc_msb_cycl1;
    regs->synt_ref_mark4.poc_lsb_lt2 = syn->sp.poc_lsb_lt2;
    regs->synt_ref_mark1.dlt_poc_msb_prsnt2 = syn->sp.dlt_poc_msb_prsnt2;
    regs->synt_ref_mark5.dlt_poc_msb_cycl2 = syn->sp.dlt_poc_msb_cycl2;
    regs->synt_sli1.lst_entry_l0 = syn->sp.lst_entry_l0;
    regs->synt_sli0.ref_pic_lst_mdf_l0 = syn->sp.ref_pic_lst_mdf_l0;

    return;
}
MPP_RET hal_h265e_rkv_gen_regs(void *hal, HalEncTask *task)
{
    H265eRkvHalContext *ctx = (H265eRkvHalContext *)hal;
    HalEncTask *enc_task = task;
    H265eSyntax_new *syn = (H265eSyntax_new *)enc_task->syntax.data;
    H265eRkvRegSet *regs = NULL;
    H265eRkvIoctlRegInfo *ioctl_reg_info = NULL;
    RcHalCfg *hal_cfg = (RcHalCfg *)ctx->rc_hal_cfg;
    H265eRkvIoctlInput *ioctl_info = (H265eRkvIoctlInput *)ctx->ioctl_input;
    H265eRkvRegSet *reg_list = (H265eRkvRegSet *)ctx->regs;
    MppBuffer mv_info_buf = enc_task->mv_info;
    RK_U32 pic_width_align8, pic_height_align8;
    RK_S32 pic_wd64, pic_h64, fbc_header_len;

    h265e_hal_enter();
    pic_width_align8 = (syn->pp.pic_width + 7) & (~7);
    pic_height_align8 = (syn->pp.pic_height + 7) & (~7);
    pic_wd64 = (syn->pp.pic_width + 63) / 64;
    pic_h64 = (syn->pp.pic_height + 63) / 64;

    fbc_header_len = (pic_wd64 * pic_h64) << 6;
    h265e_hal_dbg(H265E_DBG_SIMPLE,
                  "frame %d | type %d | start gen regs",
                  ctx->frame_cnt, ctx->frame_type);

    if (ctx->enc_mode == 2 || ctx->enc_mode == 3) { //link table mode
        RK_U32 idx = ctx->frame_cnt_gen_ready;
        ctx->num_frames_to_send = RKVE_LINKTABLE_EACH_NUM;
        if (idx == 0) {
            ioctl_info->enc_mode = ctx->enc_mode;
            ioctl_info->frame_num = ctx->num_frames_to_send;
        }
        regs = &reg_list[idx];
        ioctl_info->reg_info[idx].reg_num = sizeof(H265eRkvRegSet) / 4;
        ioctl_reg_info = &ioctl_info->reg_info[idx];
    } else {
        ctx->num_frames_to_send = 1;
        ioctl_info->frame_num = ctx->num_frames_to_send;
        ioctl_info->enc_mode = ctx->enc_mode;
        regs = &reg_list[0];
        ioctl_info->reg_info[0].reg_num = sizeof(H265eRkvRegSet) / 4;
        ioctl_reg_info = &ioctl_info->reg_info[0];
    }

    memset(regs, 0, sizeof(H265eRkvRegSet));
    regs->enc_strt.lkt_num = 0;
    regs->enc_strt.rkvenc_cmd = ctx->enc_mode;
    regs->enc_strt.enc_cke = 1;
    regs->enc_clr.safe_clr = 0x0;

    regs->lkt_addr.lkt_addr     = 0x0;
    regs->int_en.enc_done_en    = 1;
    regs->int_en.lkt_done_en    = 1;
    regs->int_en.sclr_done_en    = 1;
    regs->int_en.slc_done_en    = 1;
    regs->int_en.bsf_ovflw_en    = 1;
    regs->int_en.brsp_ostd_en    = 1;
    regs->int_en.wbus_err_en    = 1;
    regs->int_en.rbus_err_en    = 1;
    regs->int_en.wdg_en         = 1;

    regs->enc_rsl.pic_wd8_m1  = pic_width_align8 / 8 - 1;
    regs->enc_rsl.pic_wfill   = (syn->pp.pic_width & 0x7)
                                ? (8 - (syn->pp.pic_width & 0x7)) : 0;
    regs->enc_rsl.pic_hd8_m1  = pic_height_align8 / 8 - 1;
    regs->enc_rsl.pic_hfill   = (syn->pp.pic_height & 0x7)
                                ? (8 - (syn->pp.pic_height & 0x7)) : 0;

    regs->enc_pic.enc_stnd       = 1; //H265
    regs->enc_pic.cur_frm_ref    = 1; //current frame will be refered
    regs->enc_pic.bs_scp         = 1;
    regs->enc_pic.node_int       = 0;
    regs->enc_pic.log2_ctu_num = ceil(log2((double)pic_wd64 * pic_h64));

    if (ctx->frame_type == INTRA_FRAME) {
        regs->enc_pic.rdo_wgt_sel = 0;
    } else {
        regs->enc_pic.rdo_wgt_sel = 1;
    }

    regs->enc_wdg.vs_load_thd     = 0;
    regs->enc_wdg.rfp_load_thd    = 0;

    regs->dtrns_cfg.cime_dspw_orsd = (ctx->frame_type == INTER_P_FRAME);

    regs->dtrns_map.src_bus_ordr     = 0x0;
    regs->dtrns_map.cmvw_bus_ordr    = 0x0;
    regs->dtrns_map.dspw_bus_ordr    = 0x0;
    regs->dtrns_map.rfpw_bus_ordr    = 0x0;
    regs->dtrns_map.src_bus_edin     = 0x0;
    regs->dtrns_map.meiw_bus_edin    = 0x0;
    regs->dtrns_map.bsw_bus_edin     = 0x7;
    regs->dtrns_map.lktr_bus_edin    = 0x0;
    regs->dtrns_map.roir_bus_edin    = 0x0;
    regs->dtrns_map.lktw_bus_edin    = 0x0;
    regs->dtrns_map.afbc_bsize    = 0x1;

    regs->dtrns_cfg.axi_brsp_cke      = 0x0;
    regs->dtrns_cfg.cime_dspw_orsd    = 0x0;

    regs->src_fmt.src_cfmt = ctx->input_fmt;
    regs->src_proc.src_mirr = 0;
    regs->src_proc.src_rot = 0;
    regs->src_proc.txa_en  = 1;
    regs->src_proc.afbcd_en  = 0;

    h265e_rkv_set_ioctl_extra_info(&ioctl_reg_info->extra_info, syn, (RkveCsp)ctx->input_fmt);

    regs->klut_ofst.chrm_kult_ofst = 0;
    memcpy(&regs->klut_wgt0, &klut_weight[0], sizeof(klut_weight));

    regs->adr_srcy_hevc     =  mpp_buffer_get_fd(enc_task->input);
    regs->adr_srcu_hevc     = regs->adr_srcy_hevc;
    regs->adr_srcv_hevc     = regs->adr_srcy_hevc;

    regs->rfpw_h_addr_hevc  = syn->sp.recon_pic.bPicEntry[0];
    regs->rfpw_b_addr_hevc  = ((fbc_header_len << 10) | regs->rfpw_h_addr_hevc);
    regs->dspw_addr_hevc = syn->sp.recon_pic.bPicEntry[1];
    regs->cmvw_addr_hevc  = syn->sp.recon_pic.bPicEntry[2];
    regs->rfpr_h_addr_hevc = syn->sp.ref_pic.bPicEntry[0];
    regs->rfpr_b_addr_hevc = (fbc_header_len << 10 | regs->rfpr_h_addr_hevc);
    regs->dspr_addr_hevc = syn->sp.ref_pic.bPicEntry[1];
    regs->cmvr_addr_hevc = syn->sp.ref_pic.bPicEntry[2];

    if (mv_info_buf) {
        regs->enc_pic.mei_stor    = 1;
        regs->meiw_addr_hevc = mpp_buffer_get_fd(mv_info_buf);
    } else {
        regs->enc_pic.mei_stor    = 0;
        regs->meiw_addr_hevc = 0;
    }

    regs->bsbb_addr_hevc    = mpp_buffer_get_fd(enc_task->output);
    /* TODO: stream size relative with syntax */
    regs->bsbt_addr_hevc    = regs->bsbb_addr_hevc | (mpp_buffer_get_size(enc_task->output) << 10);
    regs->bsbr_addr_hevc    = regs->bsbb_addr_hevc;
    regs->bsbw_addr_hevc    = regs->bsbb_addr_hevc;

    regs->sli_spl.sli_splt_mode     = syn->sp.sli_splt_mode;
    regs->sli_spl.sli_splt_cpst     = syn->sp.sli_splt_cpst;
    regs->sli_spl.sli_splt          = syn->sp.sli_splt;
    regs->sli_spl.sli_flsh         = syn->sp.sli_flsh;
    regs->sli_spl.sli_max_num_m1   = syn->sp.sli_max_num_m1;
    regs->sli_spl.sli_splt_cnum_m1  = syn->sp.sli_splt_cnum_m1;
    regs->sli_spl_byte.sli_splt_byte = syn->sp.sli_splt_byte;

    {
        RK_U32 cime_w = 11, cime_h = 7;
        RK_S32 merangx = (cime_w + 1) * 32;
        RK_S32 merangy = (cime_h + 1) * 32;
        if (merangx > 384) {
            merangx = 384;
        }
        if (merangy > 320) {
            merangy = 320;
        }

        if (syn->pp.pic_width  < merangx + 60 || syn->pp.pic_width  <= 352) {
            if (merangx > syn->pp.pic_width ) {
                merangx =  syn->pp.pic_width;
            }
            merangx = merangx / 4 * 2;
        }

        if (syn->pp.pic_height < merangy + 60 || syn->pp.pic_height <= 288) {
            if (merangy > syn->pp.pic_height) {
                merangy = syn->pp.pic_height;
            }
            merangy = merangy / 4 * 2;
        }

        {
            RK_S32 merange_x = merangx / 2;
            RK_S32 merange_y = merangy / 2;
            RK_S32 mxneg = ((-(merange_x << 2)) >> 2) / 4;
            RK_S32 myneg = ((-(merange_y << 2)) >> 2) / 4;
            RK_S32 mxpos = (((merange_x << 2) - 4) >> 2) / 4;
            RK_S32 mypos = (((merange_y << 2) - 4) >> 2) / 4;

            mxneg = MPP_MIN(abs(mxneg), mxpos) * 4;
            myneg = MPP_MIN(abs(myneg), mypos) * 4;

            merangx = mxneg * 2;
            merangy = myneg * 2;
        }
        regs->me_rnge.cime_srch_h    = merangx / 32;
        regs->me_rnge.cime_srch_v    = merangy / 32;
    }

    regs->me_rnge.rime_srch_h    = 7;
    regs->me_rnge.rime_srch_v    = 5;
    regs->me_rnge.dlt_frm_num    = 0x1;

    regs->me_cnst.pmv_mdst_h    = 5;
    regs->me_cnst.pmv_mdst_v    = 5;
    regs->me_cnst.mv_limit      = 0;
    regs->me_cnst.mv_num        = 2;

    if (syn->pp.pic_width <= 1920) {
        if (syn->pp.pic_width > 1152) {
            regs->me_ram.cime_rama_h =  12;
        } else if (syn->pp.pic_width > 960) {
            regs->me_ram.cime_rama_h =  16;
        } else {
            regs->me_ram.cime_rama_h =  20;
        }
    } else {
        if (syn->pp.pic_width > 2688) {
            regs->me_ram.cime_rama_h = 12;
        } else if (syn->pp.pic_width > 2048) {
            regs->me_ram.cime_rama_h = 16;
        } else {
            regs->me_ram.cime_rama_h = 20;
        }
    }

    {
        RK_S32 swin_scope_wd16 = (regs->me_rnge.cime_srch_h  + 3 + 1) / 4 * 2 + 1;
        RK_S32 tmpMin = (regs->me_rnge.cime_srch_v + 3) / 4 * 2 + 1;
        if (regs->me_ram.cime_rama_h / 4 < tmpMin) {
            tmpMin = regs->me_ram.cime_rama_h / 4;
        }
        regs->me_ram.cime_rama_max = (pic_wd64 * (tmpMin - 1)) + ((pic_wd64 >= swin_scope_wd16) ? swin_scope_wd16 : pic_wd64 * 2);
    }
    regs->me_ram.cach_l2_tag      = 0x0;

    pic_wd64 = pic_wd64 << 6;

    if (pic_wd64 <= 512)
        regs->me_ram.cach_l2_tag  = 0x0;
    else if (pic_wd64 <= 1024)
        regs->me_ram.cach_l2_tag  = 0x1;
    else if (pic_wd64 <= 2048)
        regs->me_ram.cach_l2_tag  = 0x2;
    else if (pic_wd64 <= 4096)
        regs->me_ram.cach_l2_tag  = 0x3;



    regs->rdo_cfg.chrm_special   = 1;
    regs->rdo_cfg.cu_inter_en    = 0xf;
    regs->rdo_cfg.cu_intra_en    = 0xf;

    if (syn->pp.num_long_term_ref_pics_sps) {
        regs->rdo_cfg.ltm_col   = 1;
        regs->rdo_cfg.ltm_idx0l0 = 1;
    } else {
        regs->rdo_cfg.ltm_col   = 0;
        regs->rdo_cfg.ltm_idx0l0 = 0;
    }

    regs->rdo_cfg.chrm_klut_en = 0;
    regs->rdo_cfg.seq_scaling_matrix_present_flg = syn->pp.scaling_list_data_present_flag;
    regs->rdo_cfg.atf_i_en = 0;
    regs->rdo_cfg.atf_p_en = 0;
    {
        RK_U32 i_nal_type = 0;

        /* TODO: extend syn->frame_coding_type definition */
        if (ctx->frame_type == INTRA_FRAME) {
            /* reset ref pictures */
            i_nal_type    = NAL_IDR_W_RADL;
        } else if (ctx->frame_type == INTER_P_FRAME ) {
            i_nal_type    = NAL_TRAIL_R;
        } else {
            i_nal_type    = NAL_TRAIL_R;
        }
        regs->synt_nal.nal_unit_type    = i_nal_type;
    }
    h265e_rkv_set_pp_regs(regs, &ctx->cfg->prep);

    h265e_rkv_set_rc_regs(ctx, regs, syn);

    h265e_rkv_set_slice_regs(syn, regs);

    h265e_rkv_set_ref_regs(syn, regs);

    h265e_rkv_set_osd_regs(ctx, regs);
    /* ROI configure */
    h265e_rkv_set_roi_regs(ctx, regs);

    memcpy(&hal_cfg[ctx->frame_cnt_gen_ready], task->hal_ret.data, sizeof(RcHalCfg));

    ctx->frame_cnt_gen_ready++;

    ctx->frame_num++;

    h265e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h265e_rkv_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    H265eRkvHalContext *ctx = (H265eRkvHalContext *)hal;
    H265eRkvRegSet *reg_list = (H265eRkvRegSet *)ctx->regs;
    RK_U32 length = 0, k = 0;
    H265eRkvIoctlInput *ioctl_info = (H265eRkvIoctlInput *)ctx->ioctl_input;
    H265eRkvIoctlOutput *reg_out = (H265eRkvIoctlOutput *)ctx->ioctl_output;
    HalEncTask *enc_task = task;

    h265e_hal_enter();
    if (enc_task->flags.err) {
        h265e_hal_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        h265e_hal_dbg(H265E_DBG_DETAIL,
                      "frame_cnt_gen_ready(%d) != num_frames_to_send(%d), start hardware later",
                      ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    }

    h265e_hal_dbg(H265E_DBG_DETAIL,
                  "memcpy %d frames' regs from reg list to reg info",
                  ioctl_info->frame_num);

    for (k = 0; k < ioctl_info->frame_num; k++) {
        RK_U32 i;
        MppDevReqV1 req;
        RK_U32 *regs = (RK_U32*)&reg_list[k];
        H265eRkvIoctlExtraInfo *extra_info = NULL;

        memcpy(&ioctl_info->reg_info[k].regs, &reg_list[k],
               sizeof(H265eRkvRegSet));
#ifdef H265EHW_EN
        memset(&req, 0, sizeof(req));
        req.flag = 0;
        if (ctx->enc_mode == 2) {
            req.flag |= MPP_FLAGS_LINK_MODE_FIX;
        } else if (ctx->enc_mode == 3) {
            req.flag |= MPP_FLAGS_LINK_MODE_UPDATE;
        }
        /* set input info */
        req.cmd = MPP_CMD_SET_REG_WRITE;
        req.size = sizeof(H265eRkvRegSet);
        req.offset = 0;
        req.data = &ioctl_info->reg_info[k].regs;
        mpp_device_add_request(ctx->dev_ctx, &req);

        extra_info = &ioctl_info->reg_info[k].extra_info;
        req.flag = 0;
        req.cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
        req.size = extra_info->cnt * sizeof(extra_info->elem[0]);
        req.offset = 0;
        req.data = extra_info->elem;
        mpp_device_add_request(ctx->dev_ctx, &req);

        /* set output request */
        memset(&req, 0, sizeof(req));
        req.flag = 0;
        req.cmd = MPP_CMD_SET_REG_READ;
        req.size = sizeof(RK_U32);
        req.data = &reg_out->elem[k].hw_status;
        req.offset = 0x1c;
        mpp_device_add_request(ctx->dev_ctx, &req);
        memset(&req, 0, sizeof(req));
        req.flag = 0;
        req.cmd = MPP_CMD_SET_REG_READ;
        req.size = sizeof(H265eRkvIoctlOutputElem) - 4;
        req.data = &reg_out->elem[k].st_bsl;
        req.offset = 0x210;
        mpp_device_add_request(ctx->dev_ctx, &req);
#endif
        for (i = 0; i < sizeof(H265eRkvRegSet) / 4; i++) {
            h265e_hal_dbg(H265E_DBG_REG, "set reg[%04d]: 0%08x\n", i, regs[i]);
        }
    }

    // length = (sizeof(ioctl_info->enc_mode) + sizeof(ioctl_info->frame_num) +
    //           sizeof(ioctl_info->reg_info[0]) * ioctl_info->frame_num) >> 2;

    ctx->frame_cnt_send_ready++;

    h265e_hal_dbg(H265E_DBG_DETAIL, "vpu client is sending %d regs", length);

#ifdef H265EHW_EN
    mpp_device_send_request(ctx->dev_ctx);
#endif

    h265e_hal_leave();
    return ret;
}

static MPP_RET h265e_rkv_set_feedback(H265eRkvHalContext *ctx,
                                      H265eRkvIoctlOutput *out, HalEncTask *enc_task)
{
    RK_U32 k = 0;
    H265eRkvIoctlOutputElem *elem = NULL;
    RcHalCfg *hal_cfg = (RcHalCfg *)ctx->rc_hal_cfg;
    h265e_feedback *fb = &ctx->feedback;
    h265e_hal_enter();
    MppEncCfgSet    *cfg = ctx->cfg;
    RK_S32 mb64_num = ((cfg->prep.width + 63) / 64) * ((cfg->prep.height + 63) / 64);
    RK_S32 mb8_num = (mb64_num << 6);
    RK_S32 mb4_num = (mb8_num << 2);
    (void)enc_task;

    for (k = 0; k < ctx->num_frames_to_send; k++) {
        elem = &out->elem[k];
        fb->qp_sum = elem->st_sse_qp.qp_sum;
        fb->out_hw_strm_size =
            fb->out_strm_size = elem->st_bsl.bs_lgth;
        fb->sse_sum = elem->st_sse_l32.sse_l32 +
                      ((RK_S64)(elem->st_sse_qp.sse_h8 & 0xff) << 32);

        fb->hw_status = elem->hw_status;
        h265e_hal_dbg(H265E_DBG_DETAIL, "hw_status: 0x%08x", elem->hw_status);
        if (elem->hw_status & RKV_ENC_INT_LINKTABLE_FINISH)
            h265e_hal_err("RKV_ENC_INT_LINKTABLE_FINISH");

        if (elem->hw_status & RKV_ENC_INT_ONE_FRAME_FINISH)
            h265e_hal_dbg(H265E_DBG_DETAIL, "RKV_ENC_INT_ONE_FRAME_FINISH");

        if (elem->hw_status & RKV_ENC_INT_ONE_SLICE_FINISH)
            h265e_hal_err("RKV_ENC_INT_ONE_SLICE_FINISH");

        if (elem->hw_status & RKV_ENC_INT_SAFE_CLEAR_FINISH)
            h265e_hal_err("RKV_ENC_INT_SAFE_CLEAR_FINISH");

        if (elem->hw_status & RKV_ENC_INT_BIT_STREAM_OVERFLOW)
            h265e_hal_err("RKV_ENC_INT_BIT_STREAM_OVERFLOW");

        if (elem->hw_status & RKV_ENC_INT_BUS_WRITE_FULL)
            h265e_hal_err("RKV_ENC_INT_BUS_WRITE_FULL");

        if (elem->hw_status & RKV_ENC_INT_BUS_WRITE_ERROR)
            h265e_hal_err("RKV_ENC_INT_BUS_WRITE_ERROR");

        if (elem->hw_status & RKV_ENC_INT_BUS_READ_ERROR)
            h265e_hal_err("RKV_ENC_INT_BUS_READ_ERROR");

        if (elem->hw_status & RKV_ENC_INT_TIMEOUT_ERROR)
            h265e_hal_err("RKV_ENC_INT_TIMEOUT_ERROR");
        if (elem->st_mb_num) {
            fb->st_madi = elem->st_madi / elem->st_mb_num;
        } else {
            fb->st_madi = 0;
        }
        if (elem->st_ctu_num) {
            fb->st_madp = elem->st_madi / elem->st_ctu_num;
        } else {
            fb->st_madp = 0;
        }
        fb->st_lvl64_inter_num = elem->st_lvl64_inter_num;
        fb->st_lvl32_inter_num = elem->st_lvl32_inter_num;
        fb->st_lvl32_intra_num = elem->st_lvl32_intra_num;
        fb->st_lvl16_inter_num = elem->st_lvl16_inter_num;
        fb->st_lvl16_intra_num = elem->st_lvl16_intra_num;
        fb->st_lvl8_inter_num  = elem->st_lvl8_inter_num;
        fb->st_lvl8_intra_num  = elem->st_lvl8_intra_num;
        fb->st_lvl4_intra_num  = elem->st_lvl4_intra_num;
        memcpy(&fb->st_cu_num_qp[0], &elem->st_cu_num_qp[0], sizeof(elem->st_cu_num_qp));

        hal_cfg[k].madi = fb->st_madi;
        hal_cfg[k].madp = fb->st_madp;
        if (mb64_num > 0) {
            hal_cfg[k].intra_lv4_prop  = ((fb->st_lvl4_intra_num + (fb->st_lvl8_intra_num << 2) +
                                           (fb->st_lvl16_intra_num << 4) +
                                           (fb->st_lvl32_intra_num << 6)) << 8) / mb4_num;

            hal_cfg[k].inter_lv8_prop = ((fb->st_lvl8_inter_num + (fb->st_lvl16_inter_num << 2) +
                                          (fb->st_lvl32_inter_num << 4) +
                                          (fb->st_lvl64_inter_num << 6)) << 8) / mb8_num;

            hal_cfg[k].quality_real = (fb->qp_sum << 6) / mb64_num;

            hal_cfg[k].sse          = fb->sse_sum / mb64_num;
        }
    }

    h265e_hal_leave();

    return MPP_OK;
}


MPP_RET hal_h265e_rkv_wait(void *hal, HalEncTask *task)
{
    RK_S32 hw_ret = 0;
    H265eRkvHalContext *ctx = (H265eRkvHalContext *)hal;
    H265eRkvIoctlOutput *reg_out = (H265eRkvIoctlOutput *)ctx->ioctl_output;
    RK_S32 length = (sizeof(reg_out->frame_num)
                     + sizeof(reg_out->elem[0])
                     * ctx->num_frames_to_send) >> 2;

    HalEncTask *enc_task = task;

    h265e_hal_enter();

    if (enc_task->flags.err) {
        h265e_hal_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        h265e_hal_dbg(H265E_DBG_DETAIL,
                      "frame_cnt_gen_ready(%d) != num_frames_to_send(%d), wait hardware later",
                      ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    } else {
        ctx->frame_cnt_gen_ready = 0;
        if (ctx->enc_mode == RKVENC_LINKTABLE_UPDATE) {
            /* TODO: remove later */
            h265e_hal_dbg(H265E_DBG_DETAIL, "only for test enc_mode 3 ...");
            if (ctx->frame_cnt_send_ready != RKVE_LINKTABLE_FRAME_NUM) {
                h265e_hal_dbg(H265E_DBG_DETAIL,
                              "frame_cnt_send_ready(%d) != RKV5_LINKTABLE_FRAME_NUM(%d), wait hardware later",
                              ctx->frame_cnt_send_ready, RKVE_LINKTABLE_FRAME_NUM);
                return MPP_OK;
            } else {
                ctx->frame_cnt_send_ready = 0;
            }
        }
    }

    h265e_hal_dbg(H265E_DBG_DETAIL, "mpp_device_wait_reg expect length %d\n",
                  length);

#ifdef H265EHW_EN
    {
        MppDevReqV1 req;

        memset(&req, 0, sizeof(req));
        req.cmd = MPP_CMD_POLL_HW_FINISH;
        mpp_device_add_request(ctx->dev_ctx, &req);
    }
    mpp_device_send_request(ctx->dev_ctx);
#endif

    h265e_hal_dbg(H265E_DBG_DETAIL, "mpp_device_wait_reg: ret %d\n", hw_ret);

    if (hw_ret != MPP_OK) {
        h265e_hal_err("hardware returns error:%d", hw_ret);
        return MPP_ERR_VPUHW;
    }
    h265e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_rkv_get_task(void *hal, HalEncTask *task)
{
    H265eRkvHalContext *ctx = (H265eRkvHalContext *)hal;
    H265eSyntax_new *syn = (H265eSyntax_new *)task->syntax.data;
    MppFrame frame = task->frame;
    MppMeta meta = mpp_frame_get_meta(frame);
    H265eFrmInfo *frms = &syn->frms;

    h265e_hal_enter();
    if ((!ctx->alloc_flg)) {
        if (MPP_OK != h265e_rkv_allocate_buffers(ctx, syn)) {
            h265e_hal_err("h265e_rkv_allocate_buffers failed, free buffers and return\n");
            task->flags.err |= HAL_ENC_TASK_ERR_ALLOC;
            h265e_rkv_free_buffers(ctx);
            return MPP_ERR_MALLOC;
        }
        ctx->alloc_flg = 1;
    }

    if (frms->status.is_intra) {
        ctx->frame_type = INTRA_FRAME;
    } else {
        ctx->frame_type = INTER_P_FRAME;
    }

    ctx->roi_data = NULL;
    ctx->osd_data = NULL;
    mpp_meta_get_ptr(meta, KEY_ROI_DATA, (void**)&ctx->roi_data);
    mpp_log("meta = %p, roi_data %p", meta, ctx->roi_data);
    mpp_meta_get_ptr(meta, KEY_OSD_DATA, (void**)&ctx->osd_data);
    mpp_log("meta = %p, osd_data %p", meta, ctx->osd_data);
    if (syn->ud.plt_data) {
        mpp_log("config osd plt");
        ctx->osd_plt_type = RKVE_OSD_PLT_TYPE_USERDEF;
#ifdef H265EHW_EN
        MppDevReqV1 req;
        req.cmd = MPP_CMD_SET_REG_WRITE;
        req.flag = 0;
        req.offset = 0;
        req.size = sizeof(MppEncOSDPlt);
        req.data = (void*)syn->ud.plt_data;
        mpp_device_add_request(ctx->dev_ctx, &req);
#endif
    }
    h265e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_rkv_ret_task(void *hal, HalEncTask *task)
{
    H265eRkvHalContext *ctx = (H265eRkvHalContext *)hal;
    H265eRkvIoctlOutput *reg_out = (H265eRkvIoctlOutput *)ctx->ioctl_output;
    HalEncTask *enc_task = task;
    h265e_feedback *fb = &ctx->feedback;
    h265e_hal_enter();

    h265e_rkv_set_feedback(ctx, reg_out, enc_task);

    enc_task->length = fb->out_strm_size;
    enc_task->hal_ret.data = ctx->rc_hal_cfg;
    enc_task->hal_ret.number = ctx->num_frames_to_send;

    h265e_hal_dbg(H265E_DBG_DETAIL, "output stream size %d\n",
                  fb->out_strm_size);

    h265e_hal_leave();
    return MPP_OK;
}
