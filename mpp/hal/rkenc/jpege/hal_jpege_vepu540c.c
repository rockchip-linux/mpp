/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define MODULE_TAG  "hal_jpege_v540c"

#include <linux/string.h>

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_soc.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"

#include "hal_jpege_debug.h"
#include "jpege_syntax.h"
#include "hal_bufs.h"
#include "rkv_enc_def.h"
#include "vepu5xx_common.h"
#include "vepu540c_common.h"
#include "hal_jpege_vepu540c.h"
#include "hal_jpege_vepu540c_reg.h"
#include "hal_jpege_hdr.h"

typedef struct jpegeV540cHalContext_t {
    MppEncHalApi        api;
    MppDev              dev;
    void                *regs;
    void                *reg_out;

    void                *dump_files;

    RK_S32              frame_type;
    RK_S32              last_frame_type;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_cnt;
    void                *roi_data;
    MppEncCfgSet        *cfg;

    RK_U32              enc_mode;
    RK_U32              frame_size;
    RK_S32              max_buf_cnt;
    RK_S32              hdr_status;
    void                *input_fmt;
    RK_U8               *src_buf;
    RK_U8               *dst_buf;
    RK_S32              buf_size;
    RK_U32              frame_num;
    RK_S32              fbc_header_len;
    RK_U32              title_num;

    JpegeBits           bits;
    JpegeSyntax         syntax;
} jpegeV540cHalContext;

MPP_RET hal_jpege_v540c_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;

    mpp_env_get_u32("hal_jpege_debug", &hal_jpege_debug, 0);

    hal_jpege_enter();

    ctx->reg_out  = mpp_calloc(JpegV540cStatus, 1);
    ctx->regs           = mpp_calloc(JpegV540cRegSet, 1);
    ctx->input_fmt      = mpp_calloc(VepuFmtCfg, 1);
    ctx->cfg            = cfg->cfg;

    ctx->frame_cnt = 0;
    ctx->enc_mode = 1;
    cfg->type = VPU_CLIENT_RKVENC;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }

    ctx->dev = cfg->dev;
    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);

    hal_jpege_leave();
    return ret;
}

MPP_RET hal_jpege_v540c_deinit(void *hal)
{
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;

    hal_jpege_enter();
    jpege_bits_deinit(ctx->bits);
    MPP_FREE(ctx->regs);

    MPP_FREE(ctx->reg_out);

    MPP_FREE(ctx->input_fmt);

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }
    hal_jpege_leave();
    return MPP_OK;
}

static MPP_RET hal_jpege_vepu540c_prepare(void *hal)
{
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;

    hal_jpege_dbg_func("enter %p\n", hal);
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    vepu5xx_set_fmt(fmt, ctx->cfg->prep.format);

    hal_jpege_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

MPP_RET hal_jpege_v540c_gen_regs(void *hal, HalEncTask *task)
{
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;
    JpegV540cRegSet *regs = ctx->regs;
    jpeg_vepu540c_control_cfg *reg_ctl = &regs->reg_ctl;
    jpeg_vepu540c_base *reg_base = &regs->reg_base;
    JpegeBits bits = ctx->bits;
    const RK_U8 *qtable[2] = {NULL};
    size_t length = mpp_packet_get_length(task->packet);
    RK_U8  *buf = mpp_buffer_get_ptr(task->output);
    size_t size = mpp_buffer_get_size(task->output);
    JpegeSyntax *syntax = &ctx->syntax;
    Vepu540cJpegCfg cfg;
    RK_S32 bitpos;

    hal_jpege_enter();
    cfg.enc_task = task;
    cfg.jpeg_reg_base = &reg_base->jpegReg;
    cfg.dev = ctx->dev;
    cfg.input_fmt = ctx->input_fmt;

    memset(regs, 0, sizeof(JpegV540cRegSet));

    /* write header to output buffer */
    jpege_bits_setup(bits, buf, (RK_U32)size);
    /* seek length bytes data */
    jpege_seek_bits(bits, length << 3);
    /* NOTE: write header will update qtable */
    write_jpeg_header(bits, syntax, qtable);

    bitpos = jpege_bits_get_bitpos(bits);
    task->length = (bitpos + 7) >> 3;
    mpp_buffer_sync_partial_end(task->output, 0, task->length);
    mpp_packet_set_length(task->packet, task->length);
    reg_ctl->reg0004_enc_strt.lkt_num      = 0;
    reg_ctl->reg0004_enc_strt.vepu_cmd     = ctx->enc_mode;
    reg_ctl->reg0005_enc_clr.safe_clr      = 0x0;
    reg_ctl->reg0005_enc_clr.force_clr     = 0x0;

    reg_ctl->reg0008_int_en.enc_done_en         = 1;
    reg_ctl->reg0008_int_en.lkt_node_done_en    = 1;
    reg_ctl->reg0008_int_en.sclr_done_en        = 1;
    reg_ctl->reg0008_int_en.slc_done_en         = 1;
    reg_ctl->reg0008_int_en.bsf_oflw_en         = 1;
    reg_ctl->reg0008_int_en.brsp_otsd_en        = 1;
    reg_ctl->reg0008_int_en.wbus_err_en         = 1;
    reg_ctl->reg0008_int_en.rbus_err_en         = 1;
    reg_ctl->reg0008_int_en.wdg_en              = 1;
    reg_ctl->reg0008_int_en.lkt_err_int_en      = 0;

    reg_ctl->reg0012_dtrns_map.jpeg_bus_edin    = 0x7;
    reg_ctl->reg0012_dtrns_map.src_bus_edin     = 0x0;
    reg_ctl->reg0012_dtrns_map.meiw_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.bsw_bus_edin     = 0x0;
    reg_ctl->reg0012_dtrns_map.lktr_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.roir_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.lktw_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.rec_nfbc_bus_edin   = 0x0;
    reg_base->reg0192_enc_pic.enc_stnd = 2; // disable h264 or hevc

    reg_ctl->reg0013_dtrns_cfg.axi_brsp_cke     = 0x0;
    reg_ctl->reg0014_enc_wdg.vs_load_thd        = 0x1fffff;
    reg_ctl->reg0014_enc_wdg.rfp_load_thd       = 0;

    vepu540c_set_jpeg_reg(&cfg);
    {
        RK_U16 *tbl = &regs->jpeg_table.qua_tab0[0];
        RK_U32 i, j;

        for ( i = 0; i < 8; i++) {
            for ( j = 0; j < 8; j++) {
                tbl[i * 8 + j] = 0x8000 / qtable[0][j * 8 + i];
            }
        }
        tbl += 64;
        for ( i = 0; i < 8; i++) {
            for ( j = 0; j < 8; j++) {
                tbl[i * 8 + j] = 0x8000 / qtable[1][j * 8 + i];
            }
        }
        tbl += 64;
        for ( i = 0; i < 8; i++) {
            for ( j = 0; j < 8; j++) {
                tbl[i * 8 + j] = 0x8000 / qtable[1][j * 8 + i];
            }
        }
    }
    ctx->frame_num++;

    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_v540c_start(void *hal, HalEncTask *enc_task)
{
    MPP_RET ret = MPP_OK;
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;
    JpegV540cRegSet *hw_regs = ctx->regs;
    JpegV540cStatus *reg_out = ctx->reg_out;
    MppDevRegWrCfg cfg;
    MppDevRegRdCfg cfg1;
    hal_jpege_enter();

    if (enc_task->flags.err) {
        mpp_err_f("enc_task->flags.err %08x, return e arly",
                  enc_task->flags.err);
        return MPP_NOK;
    }

    cfg.reg = (RK_U32*)&hw_regs->reg_ctl;
    cfg.size = sizeof(jpeg_vepu540c_control_cfg);
    cfg.offset = VEPU540C_CTL_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg.reg = &hw_regs->jpeg_table;
    cfg.size = sizeof(vepu540c_jpeg_tab);
    cfg.offset = VEPU540C_JPEGTAB_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg.reg = &hw_regs->reg_base;
    cfg.size = sizeof(jpeg_vepu540c_base);
    cfg.offset = VEPU540C_BASE_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->hw_status;
    cfg1.size = sizeof(RK_U32);
    cfg1.offset = VEPU540C_REG_BASE_HW_STATUS;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->st;
    cfg1.size = sizeof(JpegV540cStatus) - 4;
    cfg1.offset = VEPU540C_STATUS_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        return ret;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
    if (ret) {
        mpp_err_f("send cmd failed %d\n", ret);
    }
    hal_jpege_leave();
    return ret;
}

//#define DUMP_DATA
static MPP_RET hal_jpege_vepu540c_status_check(void *hal)
{
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;
    JpegV540cStatus *elem = (JpegV540cStatus *)ctx->reg_out;

    vepu540c_hw_status hw_status = elem->hw_status;

    hal_jpege_dbg_detail("hw_status: 0x%08x", hw_status.val);
    if (hw_status.int_sta.enc_done_sta)
        hal_jpege_dbg_detail("RKV_ENC_INT_ENC_DONE");

    if (hw_status.int_sta.wdg_sta)
        mpp_err_f("RKV_ENC_INT_WDG_TIMEOUT");

    if (hw_status.int_sta.jslc_done_sta)
        hal_jpege_dbg_detail("RKV_ENC_INT_JSL_DONE");

    if (hw_status.int_sta.jbsf_oflw_sta)
        mpp_err_f("RKV_ENC_INT_JBSF_OFLOW");


    return MPP_OK;
}



MPP_RET hal_jpege_v540c_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;
    HalEncTask *enc_task = task;
    JpegV540cStatus *elem = (JpegV540cStatus *)ctx->reg_out;
    hal_jpege_enter();

    if (enc_task->flags.err) {
        mpp_err_f("enc_task->flags.err %08x, return early",
                  enc_task->flags.err);
        return MPP_NOK;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret) {
        mpp_err_f("poll cmd failed %d\n", ret);
        ret = MPP_ERR_VPUHW;
    } else {
        hal_jpege_vepu540c_status_check(hal);
        task->hw_length += elem->st.jpeg_head_bits_l32;
    }

    hal_jpege_leave();
    return ret;
}

MPP_RET hal_jpege_v540c_get_task(void *hal, HalEncTask *task)
{
    jpegeV540cHalContext *ctx = (jpegeV540cHalContext *)hal;
    MppFrame frame = task->frame;
    EncFrmStatus  *frm_status = &task->rc_task->frm;
    JpegeSyntax *syntax = (JpegeSyntax *)task->syntax.data;

    hal_jpege_enter();

    memcpy(&ctx->syntax, syntax, sizeof(ctx->syntax));

    ctx->last_frame_type = ctx->frame_type;

    if (!frm_status->reencode && mpp_frame_has_meta(task->frame)) {
        MppMeta meta = mpp_frame_get_meta(frame);

        mpp_meta_get_ptr(meta, KEY_ROI_DATA, (void **)&ctx->roi_data);
    }

    hal_jpege_leave();
    return MPP_OK;
}

MPP_RET hal_jpege_v540c_ret_task(void *hal, HalEncTask *task)
{
    (void)hal;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    hal_jpege_enter();

    task->length += task->hw_length;

    // setup bit length for rate control
    rc_info->bit_real = task->hw_length * 8;
    rc_info->quality_real = rc_info->quality_target;

    hal_jpege_leave();
    return MPP_OK;
}

const MppEncHalApi hal_jpege_vepu540c = {
    "hal_jpege_v540c",
    MPP_VIDEO_CodingMJPEG,
    sizeof(jpegeV540cHalContext),
    0,
    hal_jpege_v540c_init,
    hal_jpege_v540c_deinit,
    hal_jpege_vepu540c_prepare,
    hal_jpege_v540c_get_task,
    hal_jpege_v540c_gen_regs,
    hal_jpege_v540c_start,
    hal_jpege_v540c_wait,
    NULL,
    NULL,
    hal_jpege_v540c_ret_task,
};
