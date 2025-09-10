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

#define MODULE_TAG "hal_jpege_vepu2"

#include <string.h>

#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "mpp_platform.h"
#include "mpp_dmabuf.h"

#include "mpp_enc_hal.h"
#include "vcodec_service.h"

#include "hal_jpege_debug.h"
#include "hal_jpege_api_v2.h"
#include "hal_jpege_base.h"

#define VEPU_JPEGE_VEPU2_NUM_REGS   184
#define VEPU2_REG_INPUT_Y           48
#define VEPU2_REG_INPUT_U           49
#define VEPU2_REG_INPUT_V           50

typedef struct jpege_vepu2_reg_set_t {
    RK_U32  val[VEPU_JPEGE_VEPU2_NUM_REGS];
} jpege_vepu2_reg_set;

#define MAX_CORE_NUM                4

typedef struct JpegeMultiCoreCtx_t {
    RK_U32              multi_core_enabled;
    RK_U32              partion_num;
    MppDevRegOffCfgs    *reg_cfg;

    MppBufferGroup      partions_group;
    MppBuffer           partions_buf[MAX_CORE_NUM - 1];
    RK_U32              buf_size;

    RK_U32              part_rows[MAX_CORE_NUM];
    RK_U32              ecs_cnt[MAX_CORE_NUM];

    void                *regs_base;
    void                *regs[MAX_CORE_NUM];
    void                *regs_out[MAX_CORE_NUM];
} JpegeMultiCoreCtx;

MPP_RET hal_jpege_vepu2_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    MppClientType type;
    RK_U32 vcodec_type = mpp_get_vcodec_type();

    mpp_env_get_u32("hal_jpege_debug", &hal_jpege_debug, 0);
    hal_jpege_dbg_func("enter hal %p cfg %p\n", hal, cfg);

    /* update output to MppEnc */
    type = (vcodec_type & HAVE_VEPU2_JPEG) ?
           VPU_CLIENT_VEPU2_JPEG : VPU_CLIENT_VEPU2;

    cfg->type = type;
    ret = mpp_dev_init(&cfg->dev, type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }
    ctx->dev = cfg->dev;
    ctx->type = cfg->type;
    ctx->task_cnt = cfg->task_cnt;

    jpege_bits_init(&ctx->bits);
    mpp_assert(ctx->bits);

    ctx->cfg = cfg->cfg;
    ctx->reg_size = sizeof(RK_U32) * VEPU_JPEGE_VEPU2_NUM_REGS;
    ctx->regs = mpp_calloc_size(void, (ctx->reg_size + EXTRA_INFO_SIZE) * ctx->task_cnt);
    if (NULL == ctx->regs) {
        mpp_err_f("failed to malloc vepu2 regs\n");
        return MPP_NOK;
    }

    ctx->regs_out = mpp_calloc_size(void, (ctx->reg_size + EXTRA_INFO_SIZE) *  ctx->task_cnt);
    if (NULL == ctx->regs_out) {
        mpp_err_f("failed to malloc vepu2 regs\n");
        return MPP_NOK;
    }

    hal_jpege_rc_init(&ctx->hal_rc);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_deinit(void *hal)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx->bits) {
        jpege_bits_deinit(ctx->bits);
        ctx->bits = NULL;
    }

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    if (ctx->ctx_ext) {
        JpegeMultiCoreCtx *ctx_ext = ctx->ctx_ext;
        RK_U32 i;

        if (ctx_ext->reg_cfg) {
            mpp_dev_multi_offset_deinit(ctx_ext->reg_cfg);
            ctx_ext->reg_cfg = NULL;
        }

        for (i = 0; i < MAX_CORE_NUM - 1; i++)
            if (ctx_ext->partions_buf[i])
                mpp_buffer_put(ctx_ext->partions_buf[i]);

        if (ctx_ext->partions_group) {
            mpp_buffer_group_put(ctx_ext->partions_group);
            ctx_ext->partions_group = NULL;
        }

        MPP_FREE(ctx_ext->regs_base);
        MPP_FREE(ctx->ctx_ext);
    }

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->regs_out);

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_get_task(void *hal, HalEncTask *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeSyntax *syntax = (JpegeSyntax *)task->syntax.data;
    JpegeMultiCoreCtx *ctx_ext = (JpegeMultiCoreCtx *)ctx->ctx_ext;
    RK_U32 i = 0;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    memcpy(&ctx->syntax, syntax, sizeof(ctx->syntax));

    ctx->hal_start_pos = mpp_packet_get_length(task->packet);

    /* prepare for part encoding */
    ctx->mcu_y = 0;
    ctx->mcu_h = syntax->mcu_ver_cnt;
    ctx->sw_bit = 0;
    ctx->part_bytepos = 0;
    ctx->part_x_fill = 0;
    ctx->part_y_fill = 0;
    ctx->rst_marker_idx = 0;
    task->part_first = 1;
    task->part_last = 0;
    task->flags.reg_idx = 0;

    /* rk3588 4 core frame parallel */
    if (ctx->task_cnt > 1) {
        task->flags.reg_idx = ctx->task_idx++;
        if (ctx->task_idx >= ctx->task_cnt)
            ctx->task_idx = 0;
        goto MULTI_CORE_SPLIT_DONE;
    }

    /* Split single task to multi cores on rk3588 */
    if (ctx_ext)
        ctx_ext->multi_core_enabled = 0;

    if (ctx->type == VPU_CLIENT_VEPU2_JPEG) {
        RK_U32 width = ctx->cfg->prep.width;
        RK_U32 height = ctx->cfg->prep.height;
        RK_U32 buf_size = width * height / 2;

        /* small image do not need to split into four segments */
        if (width * height <= 1280 * 720 && (height <= 720 || width <= 720))
            goto MULTI_CORE_SPLIT_DONE;

        if (!ctx_ext) {
            ctx_ext = mpp_calloc(JpegeMultiCoreCtx, 1);
            ctx->ctx_ext = ctx_ext;
        }

        mpp_assert(ctx_ext);

        if (!ctx_ext->partions_group) {
            mpp_buffer_group_get_internal(&ctx_ext->partions_group, MPP_BUFFER_TYPE_DMA_HEAP | MPP_BUFFER_FLAGS_CACHABLE);
            if (!ctx_ext->partions_group)
                mpp_buffer_group_get_internal(&ctx_ext->partions_group, MPP_BUFFER_TYPE_ION);
        }

        mpp_assert(ctx_ext->partions_group);

        if (ctx_ext->buf_size != buf_size) {
            MppBuffer buf = NULL;

            for (i = 0; i < MAX_CORE_NUM - 1; i++) {
                buf = ctx_ext->partions_buf[i];
                if (buf)
                    mpp_buffer_put(buf);
            }

            mpp_buffer_group_clear(ctx_ext->partions_group);

            for (i = 0; i < MAX_CORE_NUM - 1; i++) {
                mpp_buffer_get(ctx_ext->partions_group, &buf, buf_size);
                mpp_assert(buf);
                ctx_ext->partions_buf[i] = buf;
            }

            ctx_ext->buf_size = buf_size;
        }

        if (!ctx_ext->regs_base) {
            void *regs_base = mpp_calloc_size(void, ctx->reg_size * MAX_CORE_NUM * 2);
            size_t reg_size = ctx->reg_size;

            ctx_ext->regs_base = regs_base;
            for (i = 0; i < MAX_CORE_NUM; i++) {
                ctx_ext->regs[i] = regs_base;
                regs_base += reg_size;

                ctx_ext->regs_out[i] = regs_base;
                regs_base += reg_size;
            }
        }

        {
            RK_U32 mb_w = MPP_ALIGN(width, 16) / 16;
            RK_U32 mb_h = MPP_ALIGN(height, 16) / 16;
            RK_U32 part_rows = MPP_ALIGN(mb_h, 4) / 4;

            ctx_ext->partion_num = 0;

            if (ctx->cfg->split.split_mode == MPP_ENC_SPLIT_BY_CTU) {
                RK_U32 ecs_num = (mb_h + syntax->part_rows - 1) / syntax->part_rows;
                RK_U32 *core_ecs = ctx_ext->ecs_cnt;

                if (ecs_num > 24 || ecs_num <= 8) {
                    RK_U32 divider = ecs_num > 24 ? 8 : 1;
                    RK_U32 quotient = ecs_num / divider;
                    RK_U32 remainder = ecs_num % divider;
                    RK_U32 runs = quotient  / MAX_CORE_NUM;
                    RK_U32 runs_left = quotient % MAX_CORE_NUM;

                    if (runs > 0) {
                        for (i = 0; i < MAX_CORE_NUM; i++)
                            core_ecs[i] = runs * divider;
                    }

                    for (i = 0; i < runs_left; i++)
                        core_ecs[i] += divider;

                    core_ecs[MAX_CORE_NUM - 1] += remainder;
                } else if (ecs_num > 20) {
                    core_ecs[0] = core_ecs[1] = 8;
                    core_ecs[2] = (ecs_num - 8 * 2) / 2;
                    core_ecs[3] = ecs_num - 8 * 2 - core_ecs[2];
                } else if (ecs_num > 16) {
                    core_ecs[0] = 8;
                    core_ecs[1] = core_ecs[2] = 4;
                    core_ecs[3] = ecs_num - 8 - 4 * 2;
                } else if (ecs_num > 8) {
                    core_ecs[0] = core_ecs[1] = 4;
                    core_ecs[2] = (ecs_num - 4 * 2) / 2;
                    core_ecs[3] = ecs_num - 4 * 2 - core_ecs[2];
                }

                for (i = 0; i < MAX_CORE_NUM; i++) {
                    ctx_ext->part_rows[i] = core_ecs[i] * syntax->part_rows;
                    hal_jpege_dbg_detail("part %d, ecs %d, rows %d", i, core_ecs[i],
                                         ctx_ext->part_rows[i]);
                    if (core_ecs[i])
                        ctx_ext->partion_num++;
                }
            } else {
                for (i = 0; i < MAX_CORE_NUM; i++) {
                    part_rows = (mb_h >= part_rows) ? part_rows : mb_h;

                    ctx_ext->part_rows[i] = part_rows;
                    ctx_ext->ecs_cnt[i] = 1;

                    hal_jpege_dbg_detail("part %d row %d restart %d\n",
                                         i, part_rows, mb_w * part_rows);

                    if (part_rows)
                        ctx_ext->partion_num++;

                    if (i == 0 && !ctx->syntax.restart_ri)
                        ctx->syntax.restart_ri = mb_w * part_rows;

                    mb_h -= part_rows;
                }
            }
        }

        if (!ctx_ext->reg_cfg)
            mpp_dev_multi_offset_init(&ctx_ext->reg_cfg, 24);

        syntax->low_delay = 1;
        ctx_ext->multi_core_enabled = 1;
    }

    if (ctx->cfg->jpeg.update) {
        hal_jpege_rc_update(&ctx->hal_rc, syntax);
        ctx->cfg->jpeg.update = 0;
    }

    task->rc_task->frm.is_intra = 1;

MULTI_CORE_SPLIT_DONE:

    hal_jpege_dbg_func("leave hal %p\n", hal);

    return MPP_OK;
}

static MPP_RET hal_jpege_vepu2_set_extra_info(MppDev dev, JpegeSyntax *syntax,
                                              RK_U32 start_mbrow)
{
    VepuOffsetCfg cfg;

    cfg.fmt = syntax->format;
    cfg.width = syntax->width;
    cfg.height = syntax->height;
    cfg.hor_stride = syntax->hor_stride;
    cfg.ver_stride = syntax->ver_stride;
    cfg.offset_x = syntax->offset_x;
    cfg.offset_y = syntax->offset_y + start_mbrow * 16;

    get_vepu_offset_cfg(&cfg);

    if (cfg.offset_byte[0])
        mpp_dev_set_reg_offset(dev, VEPU2_REG_INPUT_Y, cfg.offset_byte[0]);

    if (cfg.offset_byte[1])
        mpp_dev_set_reg_offset(dev, VEPU2_REG_INPUT_U, cfg.offset_byte[1]);

    if (cfg.offset_byte[2])
        mpp_dev_set_reg_offset(dev, VEPU2_REG_INPUT_V, cfg.offset_byte[2]);

    return MPP_OK;
}

MPP_RET hal_jpege_vepu2_gen_regs(void *hal, HalEncTask *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    MppBuffer input  = task->input;
    MppBuffer output = task->output;
    JpegeSyntax *syntax = &ctx->syntax;
    RK_U32 width        = syntax->width;
    RK_U32 width_align  = MPP_ALIGN(width, 16);
    RK_U32 height       = syntax->height;
    MppFrameFormat fmt  = syntax->format;
    RK_U32 hor_stride   = 0;
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    JpegeBits bits      = ctx->bits;
    RK_S32 reg_idx      = task->flags.reg_idx;
    RK_U32 *regs = (RK_U32 *)((RK_U8 *)ctx->regs + ctx->reg_size * reg_idx);
    size_t length = mpp_packet_get_length(task->packet);
    RK_U8  *buf = mpp_buffer_get_ptr(output);
    size_t size = mpp_buffer_get_size(output);
    RK_S32 bitpos;
    RK_S32 bytepos;
    RK_U32 x_fill = 0;
    RK_U32 y_fill = 0;
    VepuFormatCfg fmt_cfg;
    RK_U32 rotation = 0;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    // do not support mirroring
    if (syntax->mirroring)
        mpp_err_f("Warning: do not support mirroring\n");

    if (syntax->rotation == MPP_ENC_ROT_90)
        rotation = 1;
    else if (syntax->rotation == MPP_ENC_ROT_270)
        rotation = 2;
    else if (syntax->rotation != MPP_ENC_ROT_0)
        mpp_err_f("Warning: only support 90 or 270 degree rotate, request rotate %d", syntax->rotation);
    if (rotation) {
        MPP_SWAP(RK_U32, width, height);
        MPP_SWAP(RK_U32, width_align, ver_stride);
    }
    hor_stride = get_vepu_pixel_stride(&ctx->stride_cfg, width,
                                       syntax->hor_stride, fmt);

    //hor_stride must be align with 8, and ver_stride mus align with 2
    if ((hor_stride & 0x7) || (ver_stride & 0x1) || (hor_stride >= (1 << 15))) {
        mpp_err_f("illegal resolution, hor_stride %d, ver_stride %d, width %d, height %d\n",
                  syntax->hor_stride, syntax->ver_stride,
                  syntax->width, syntax->height);
    }

    x_fill = (width_align - width) / 4;
    y_fill = (ver_stride - height);
    mpp_assert(x_fill <= 3);
    mpp_assert(y_fill <= 15);
    ctx->part_x_fill = x_fill;
    ctx->part_y_fill = y_fill;

    mpp_buffer_sync_begin(output);

    if (syntax->q_mode == JPEG_QFACTOR) {
        syntax->q_factor = 100 - task->rc_task->info.quality_target;
        hal_jpege_rc_update(&ctx->hal_rc, syntax);
    }

    /* write header to output buffer */
    jpege_bits_setup(bits, buf, (RK_U32)size);
    /* seek length bytes data */
    jpege_seek_bits(bits, length << 3);
    /* NOTE: write header will update qtable */
    write_jpeg_header(bits, syntax, &ctx->hal_rc);

    memset(regs, 0, sizeof(RK_U32) * VEPU_JPEGE_VEPU2_NUM_REGS);
    // input address setup
    regs[VEPU2_REG_INPUT_Y] = mpp_buffer_get_fd(input);
    regs[VEPU2_REG_INPUT_U] = regs[VEPU2_REG_INPUT_Y];
    regs[VEPU2_REG_INPUT_V] = regs[VEPU2_REG_INPUT_Y];

    // output address setup
    bitpos = jpege_bits_get_bitpos(bits);
    bytepos = (bitpos + 7) >> 3;
    ctx->base = buf;
    ctx->size = size;
    ctx->sw_bit = bitpos;
    ctx->part_bytepos = bytepos;

    get_msb_lsb_at_pos(&regs[51], &regs[52], buf, bytepos);

    mpp_buffer_sync_end(output);

    regs[53] = size - bytepos;

    // bus config
    regs[54] = 16 << 8;

    regs[60] = (((bytepos & 7) * 8) << 16) |
               (x_fill << 4) |
               (y_fill);
    regs[61] = hor_stride;

    regs[77] = mpp_buffer_get_fd(output);
    if (bytepos)
        mpp_dev_set_reg_offset(ctx->dev, 77, bytepos);
    /* 95 - 97 color conversion parameter */
    {
        RK_U32 coeffA;
        RK_U32 coeffB;
        RK_U32 coeffC;
        RK_U32 coeffE;
        RK_U32 coeffF;

        switch (syntax->color_conversion_type) {
        case 0 : {  /* BT.601 */
            /*
             * Y  = 0.2989 R + 0.5866 G + 0.1145 B
             * Cb = 0.5647 (B - Y) + 128
             * Cr = 0.7132 (R - Y) + 128
             */
            coeffA = 19589;
            coeffB = 38443;
            coeffC = 7504;
            coeffE = 37008;
            coeffF = 46740;
        } break;
        case 1 : {  /* BT.709 */
            /*
             * Y  = 0.2126 R + 0.7152 G + 0.0722 B
             * Cb = 0.5389 (B - Y) + 128
             * Cr = 0.6350 (R - Y) + 128
             */
            coeffA = 13933;
            coeffB = 46871;
            coeffC = 4732;
            coeffE = 35317;
            coeffF = 41615;
        } break;
        case 2 : {
            coeffA = syntax->coeffA;
            coeffB = syntax->coeffB;
            coeffC = syntax->coeffC;
            coeffE = syntax->coeffE;
            coeffF = syntax->coeffF;
        } break;
        default : {
            mpp_err("invalid color conversion type %d\n",
                    syntax->color_conversion_type);
            coeffA = 19589;
            coeffB = 38443;
            coeffC = 7504;
            coeffE = 37008;
            coeffF = 46740;
        } break;
        }

        regs[95] = coeffA | (coeffB << 16);
        regs[96] = coeffC | (coeffE << 16);
        regs[97] = coeffF;
    }

    regs[103] = (width_align >> 4) << 8  |
                (ver_stride >> 4) << 20 |
                (1 << 6) |  /* intra coding  */
                (2 << 4) |  /* format jpeg   */
                1;          /* encoder start */

    if (!get_vepu_fmt(&fmt_cfg, fmt)) {
        regs[74] = (fmt_cfg.format << 4) |
                   (rotation << 2);
        regs[98] = (fmt_cfg.b_mask & 0x1f) << 16 |
                   (fmt_cfg.g_mask & 0x1f) << 8  |
                   (fmt_cfg.r_mask & 0x1f);
        regs[105] = 7 << 26 | (fmt_cfg.swap_32_in & 1) << 29 |
                    (fmt_cfg.swap_16_in & 1) << 30 |
                    (fmt_cfg.swap_8_in & 1) << 31;
    }

    regs[107] = ((syntax->part_rows & 0xff) << 16) |
                jpege_restart_marker[ctx->rst_marker_idx & 7];

    /* encoder interrupt */
    regs[109] = 1 << 12 |   /* clock gating */
                1 << 10;    /* enable timeout interrupt */

    if (syntax->low_delay) {
        /* slice encode end by RST */
        regs[107] |= (1 << 24);
        /* slice interrupt enable */
        regs[109] |= (1 << 16);
    }

    /* 0 ~ 31 quantization tables */
    {
        RK_S32 i;

        for (i = 0; i < 16; i++) {
            /* qtable need to reorder in particular order */
            regs[i] = ctx->hal_rc.qtables[0][qp_reorder_table[i * 4 + 0]] << 24 |
                      ctx->hal_rc.qtables[0][qp_reorder_table[i * 4 + 1]] << 16 |
                      ctx->hal_rc.qtables[0][qp_reorder_table[i * 4 + 2]] << 8 |
                      ctx->hal_rc.qtables[0][qp_reorder_table[i * 4 + 3]];
        }
        for (i = 0; i < 16; i++) {
            /* qtable need to reorder in particular order */
            regs[i + 16] = ctx->hal_rc.qtables[1][qp_reorder_table[i * 4 + 0]] << 24 |
                           ctx->hal_rc.qtables[1][qp_reorder_table[i * 4 + 1]] << 16 |
                           ctx->hal_rc.qtables[1][qp_reorder_table[i * 4 + 2]] << 8 |
                           ctx->hal_rc.qtables[1][qp_reorder_table[i * 4 + 3]];
        }
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

static MPP_RET multi_core_start(HalJpegeCtx *ctx, HalEncTask *task)
{
    JpegeMultiCoreCtx *ctx_ext = ctx->ctx_ext;
    JpegeSyntax *syntax = &ctx->syntax;
    MppDevRegOffCfgs *reg_cfg = ctx_ext->reg_cfg;
    MppDev dev = ctx->dev;
    RK_S32 reg_idx = task->flags.reg_idx;
    RK_U32 *src = (RK_U32 *)((RK_U8 *)ctx->regs + ctx->reg_size * reg_idx);
    RK_U32 reg_size = ctx->reg_size;
    MPP_RET ret = MPP_OK;
    RK_U32 partion_num = ctx_ext->partion_num;
    RK_U32 mcu_y = 0;
    RK_U32 i;

    hal_jpege_dbg_detail("start %d partions\n", partion_num);

    for (i = 0; i < partion_num; i++) {
        RK_U32 part_not_end = i < partion_num - 1;
        RK_U32 part_not_start = i > 0;
        RK_U32 *regs = (RK_U32 *)ctx_ext->regs[i];
        RK_U32 part_enc_mcu_h = ctx_ext->part_rows[i];
        RK_U32 part_x_fill = ctx->part_x_fill;
        RK_U32 part_y_fill = ctx->part_y_fill;
        RK_U32 part_bytepos = ctx->part_bytepos;

        // it only needs to fill the partition on the right and below.
        if (syntax->rotation == MPP_ENC_ROT_90) {
            if (part_not_end)
                part_x_fill = 0;
        } else if (syntax->rotation == MPP_ENC_ROT_0 || syntax->rotation == MPP_ENC_ROT_180) {
            if (part_not_end)
                part_y_fill = 0;
        } else if (syntax->rotation == MPP_ENC_ROT_270) {
            if (part_not_start)
                part_x_fill = 0;
        } else
            mpp_err_f("input rotation %d not supported", syntax->rotation);

        memcpy(regs, src, reg_size);

        mpp_dev_multi_offset_reset(reg_cfg);

        if (i == 0) {
            get_msb_lsb_at_pos(&regs[51], &regs[52], ctx->base, part_bytepos);
            regs[77] = mpp_buffer_get_fd(task->output);
            regs[53] = mpp_buffer_get_size(task->output) - part_bytepos;
            regs[60] = (((part_bytepos & 7) * 8) << 16) |
                       (part_x_fill << 4) |
                       (part_y_fill);
            /* the stream offset had been setup */
        } else {
            MppBuffer buf = ctx_ext->partions_buf[i - 1];

            regs[77] = mpp_buffer_get_fd(buf);
            regs[53] = mpp_buffer_get_size(buf);
            regs[60] = (((0 & 7) * 8) << 16) |
                       (part_x_fill << 4) |
                       (part_y_fill);
        }

        regs[103] = syntax->mcu_hor_cnt << 8  |
                    (part_enc_mcu_h) << 20 |
                    (1 << 6) |  /* intra coding  */
                    (2 << 4) |  /* format jpeg   */
                    1;          /* encoder start */

        hal_jpege_dbg_detail("part %d, part_not_end 0x%x, rst_marker_idx %d",
                             i, part_not_end, ctx->rst_marker_idx);
        regs[107] = part_not_end << 24 | ((syntax->part_rows & 0xff) << 16) |
                    jpege_restart_marker[ctx->rst_marker_idx & 7];
        ctx->rst_marker_idx += ctx_ext->ecs_cnt[i];

        VepuOffsetCfg cfg;

        memset(&cfg, 0, sizeof(cfg));

        cfg.fmt = syntax->format;
        cfg.width = syntax->width;
        cfg.height = syntax->height;
        cfg.hor_stride = syntax->hor_stride;
        cfg.ver_stride = syntax->ver_stride;
        cfg.offset_x = syntax->offset_x;
        cfg.offset_y = syntax->offset_y + mcu_y * 16;

        if (syntax->rotation == MPP_ENC_ROT_90 || syntax->rotation == MPP_ENC_ROT_270) {
            regs[103] = part_enc_mcu_h << 8  |
                        (syntax->mcu_hor_cnt) << 20 |
                        (1 << 6) |  /* intra coding  */
                        (2 << 4) |  /* format jpeg   */
                        1;          /* encoder start */

            /*
             * It is opposite that position of partitions
             * of rotation 90 degree and rotation 270 degree.
             */
            if (syntax->rotation == MPP_ENC_ROT_270)
                cfg.offset_x = syntax->offset_x +
                               (syntax->mcu_ver_cnt - ctx_ext->part_rows[0] - mcu_y) * 16;
            else
                cfg.offset_x = syntax->offset_x + mcu_y * 16;

            cfg.offset_y = syntax->offset_y;
        }

        get_vepu_offset_cfg(&cfg);
        mpp_dev_multi_offset_update(reg_cfg, VEPU2_REG_INPUT_Y, cfg.offset_byte[0]);
        mpp_dev_multi_offset_update(reg_cfg, VEPU2_REG_INPUT_U, cfg.offset_byte[1]);
        mpp_dev_multi_offset_update(reg_cfg, VEPU2_REG_INPUT_V, cfg.offset_byte[2]);

        mcu_y += part_enc_mcu_h;

        do {
            MppDevRegWrCfg wr_cfg;
            MppDevRegRdCfg rd_cfg;

            wr_cfg.reg = regs;
            wr_cfg.size = reg_size;
            wr_cfg.offset = 0;

            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
            if (ret) {
                mpp_err_f("set register write failed %d\n", ret);
                break;
            }

            rd_cfg.reg = ctx_ext->regs_out[i];
            rd_cfg.size = reg_size;
            rd_cfg.offset = 0;

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
            if (ret) {
                mpp_err_f("set register read failed %d\n", ret);
                break;
            }

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFS, reg_cfg);
            if (ret) {
                mpp_err_f("set register offsets failed %d\n", ret);
                break;
            }

            if (i < partion_num - 1) {
                ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_DELIMIT, NULL);
                if (ret) {
                    mpp_err_f("send delimit failed %d\n", ret);
                    break;
                }
            }
        } while (0);
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
    if (ret)
        mpp_err_f("send cmd failed %d\n", ret);

    return ret;
}

static MPP_RET multi_core_wait(HalJpegeCtx *ctx, HalEncTask *task)
{
    JpegeMultiCoreCtx *ctx_ext = (JpegeMultiCoreCtx *)ctx->ctx_ext;
    JpegeFeedback *feedback = &ctx->feedback;
    RK_U32 sw_bit = 0;
    RK_U32 hw_bit = 0;
    MPP_RET ret = MPP_OK;
    RK_U32 val;
    RK_U32 i;

    hal_jpege_dbg_detail("poll partion_num %d\n", ctx_ext->partion_num);

    for (i = 0; i < ctx_ext->partion_num; i++) {
        RK_U32 *regs = ctx_ext->regs_out[i];

        hal_jpege_dbg_detail("poll reg %d %p", i, regs);

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret)
            mpp_err_f("poll cmd failed %d\n", ret);

        if (i == 0) {
            RK_S32 fd = mpp_buffer_get_fd(task->output);

            val = regs[109];
            hal_jpege_dbg_output("hw_status %08x\n", val);
            feedback->hw_status = val & 0x70;
            val = regs[53];
            sw_bit = jpege_bits_get_bitpos(ctx->bits);
            hw_bit = val;
            feedback->stream_length = ((sw_bit / 8) & (~0x7)) + hw_bit / 8;
            hal_jpege_dbg_detail("partion len = %d", hw_bit / 8);
            task->length = feedback->stream_length;
            task->hw_length = task->length - ctx->hal_start_pos;

            mpp_dmabuf_sync_partial_begin(fd, 1, 0, task->length, __FUNCTION__);
        } else {
            void *stream_ptr = mpp_buffer_get_ptr(task->output);
            void *partion_ptr = mpp_buffer_get_ptr(ctx_ext->partions_buf[i - 1]);
            RK_S32 partion_fd = mpp_buffer_get_fd(ctx_ext->partions_buf[i - 1]);
            RK_U32 partion_len = 0;

            val = regs[109];
            hal_jpege_dbg_output("hw_status %08x\n", val);
            feedback->hw_status = val & 0x70;
            partion_len = regs[53] / 8;
            hal_jpege_dbg_detail("partion_len = %d", partion_len);

            mpp_dmabuf_sync_partial_begin(partion_fd, 1, 0, partion_len, __FUNCTION__);

            memcpy(stream_ptr + feedback->stream_length, partion_ptr, partion_len);
            feedback->stream_length += partion_len;
            task->length = feedback->stream_length;
            task->hw_length += partion_len;
        }
    }

    hal_jpege_dbg_output("stream bit: sw %d hw %d total %d hw_length %d\n",
                         sw_bit, hw_bit, feedback->stream_length, task->hw_length);

    return ret;
}

MPP_RET hal_jpege_vepu2_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeMultiCoreCtx *ctx_ext = (JpegeMultiCoreCtx *)ctx->ctx_ext;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx_ext && ctx_ext->multi_core_enabled) {
        multi_core_start(ctx, task);
    } else {
        hal_jpege_vepu2_set_extra_info(ctx->dev, &ctx->syntax, 0);
        do {
            MppDevRegWrCfg wr_cfg;
            MppDevRegRdCfg rd_cfg;
            RK_U32 reg_size = ctx->reg_size;
            RK_S32 reg_idx = task->flags.reg_idx;
            RK_U32 *regs = (RK_U32 *)((RK_U8 *)ctx->regs + reg_size * reg_idx);

            wr_cfg.reg = regs;
            wr_cfg.size = reg_size;
            wr_cfg.offset = 0;

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
            if (ret) {
                mpp_err_f("set register write failed %d\n", ret);
                break;
            }

            rd_cfg.reg = regs;
            rd_cfg.size = reg_size;
            rd_cfg.offset = 0;

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
            if (ret) {
                mpp_err_f("set register read failed %d\n", ret);
                break;
            }

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
            if (ret) {
                mpp_err_f("send cmd failed %d\n", ret);
                break;
            }
        } while (0);
    }
    hal_jpege_dbg_func("leave hal %p\n", hal);
    (void)task;
    return ret;
}

MPP_RET hal_jpege_vepu2_wait(void *hal, HalEncTask *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeMultiCoreCtx *ctx_ext = (JpegeMultiCoreCtx *)ctx->ctx_ext;
    MPP_RET ret = MPP_OK;

    hal_jpege_dbg_func("enter hal %p\n", hal);

    if (ctx_ext && ctx_ext->multi_core_enabled) {
        multi_core_wait(ctx, task);
    } else {
        JpegeFeedback *feedback = &ctx->feedback;
        JpegeBits bits = ctx->bits;
        RK_S32 reg_idx = task->flags.reg_idx;
        RK_U32 *regs = (RK_U32 *)((RK_U8 *)ctx->regs + ctx->reg_size * reg_idx);
        RK_U32 sw_bit = 0;
        RK_U32 hw_bit = 0;
        RK_U32 val;

        if (ctx->dev) {
            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
            if (ret)
                mpp_err_f("poll cmd failed %d\n", ret);
        }

        val = regs[109];
        hal_jpege_dbg_output("hw_status %08x\n", val);
        feedback->hw_status = val & 0x70;
        val = regs[53];

        sw_bit = jpege_bits_get_bitpos(bits);
        hw_bit = val;

        // NOTE: hardware will return 64 bit access byte count
        feedback->stream_length = ((sw_bit / 8) & (~0x7)) + hw_bit / 8;
        task->length = feedback->stream_length;
        task->hw_length = task->length - ctx->hal_start_pos;

        hal_jpege_dbg_output("stream bit: sw %d hw %d total %d hw_length %d\n",
                             sw_bit, hw_bit, feedback->stream_length, task->hw_length);
    }

    hal_jpege_dbg_func("leave hal %p\n", hal);
    return ret;
}

MPP_RET hal_jpege_vepu2_part_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    JpegeSyntax *syntax = (JpegeSyntax *)task->syntax.data;
    RK_U32 mcu_w = syntax->mcu_hor_cnt;
    RK_U32 mcu_h = syntax->mcu_ver_cnt;
    RK_U32 mcu_y = ctx->mcu_y;
    RK_U32 part_mcu_h = syntax->part_rows;
    RK_S32 reg_idx = task->flags.reg_idx;
    RK_U32 *regs = (RK_U32 *)((RK_U8 *)ctx->regs + ctx->reg_size * reg_idx);
    RK_U32 part_enc_h;
    RK_U32 part_enc_mcu_h;
    RK_U32 part_y_fill;
    RK_U32 part_not_end;

    hal_jpege_dbg_func("enter part start %p\n", hal);

    /* Fix register for each part encoding */
    task->part_first = !mcu_y;
    if (mcu_y + part_mcu_h < mcu_h) {
        part_enc_h = part_mcu_h * 16;
        part_enc_mcu_h = part_mcu_h;
        part_y_fill = 0;
        part_not_end = 1;
        task->part_last = 0;
    } else {
        part_enc_h = syntax->height - mcu_y * 16;
        part_enc_mcu_h = MPP_ALIGN(part_enc_h, 16) / 16;;
        part_y_fill = ctx->part_y_fill;
        part_not_end = 0;
        task->part_last = 1;
    }

    hal_jpege_dbg_detail("part first %d last %d\n", task->part_first, task->part_last);

    get_msb_lsb_at_pos(&regs[51], &regs[52], ctx->base, ctx->part_bytepos);

    regs[53] = ctx->size - ctx->part_bytepos;

    regs[60] = (((ctx->part_bytepos & 7) * 8) << 16) |
               (ctx->part_x_fill << 4) |
               (part_y_fill);

    regs[77] = mpp_buffer_get_fd(task->output);
    if (ctx->part_bytepos)
        mpp_dev_set_reg_offset(ctx->dev, 77, ctx->part_bytepos);

    regs[103] = mcu_w << 8  |
                (part_enc_mcu_h) << 20 |
                (1 << 6) |  /* intra coding  */
                (2 << 4) |  /* format jpeg   */
                1;          /* encoder start */

    hal_jpege_dbg_detail("part_not_end 0x%x, rst_marker_idx %d",
                         part_not_end, ctx->rst_marker_idx);
    regs[107] = part_not_end << 24 | jpege_restart_marker[ctx->rst_marker_idx & 7];
    ctx->rst_marker_idx++;

    hal_jpege_vepu2_set_extra_info(ctx->dev, syntax, mcu_y);
    ctx->mcu_y += part_enc_mcu_h;

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = ctx->reg_size;

        wr_cfg.reg = ctx->regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = ctx->regs_out;
        rd_cfg.size = reg_size;
        rd_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    hal_jpege_dbg_func("leave part start %p\n", hal);
    (void)task;
    return ret;
}

MPP_RET hal_jpege_vepu2_part_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    RK_S32 reg_idx = task->flags.reg_idx;
    RK_U32 *regs = (RK_U32 *)((RK_U8 *)ctx->regs_out + ctx->reg_size * reg_idx);
    JpegeFeedback *feedback = &ctx->feedback;
    RK_U32 hw_bit = 0;

    hal_jpege_dbg_func("enter part wait %p\n", hal);

    if (ctx->dev) {
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret)
            mpp_err_f("poll cmd failed %d\n", ret);
    }

    hal_jpege_dbg_detail("hw_status %08x\n", regs[109]);

    hw_bit = regs[53];

    hal_jpege_dbg_detail("byte pos %d -> %d\n", ctx->part_bytepos,
                         (ctx->part_bytepos & (~7)) + (hw_bit / 8));
    ctx->part_bytepos = (ctx->part_bytepos & (~7)) + (hw_bit / 8);

    feedback->stream_length = ctx->part_bytepos;
    task->length = ctx->part_bytepos;
    task->hw_length = task->length - ctx->hal_start_pos;

    hal_jpege_dbg_detail("stream_length %d, hw_byte %d",
                         feedback->stream_length, hw_bit / 8);

    hal_jpege_dbg_output("stream bit: sw %d hw %d total %d hw_length %d\n",
                         ctx->sw_bit, hw_bit, feedback->stream_length, task->hw_length);

    hal_jpege_dbg_func("leave part wait %p\n", hal);
    return ret;
}

MPP_RET hal_jpege_vepu2_ret_task(void *hal, HalEncTask *task)
{
    HalJpegeCtx *ctx = (HalJpegeCtx *)hal;
    EncRcTaskInfo *rc_info = &task->rc_task->info;

    task->rc_task->info.bit_real = ctx->feedback.stream_length * 8;
    task->hal_ret.data = &ctx->feedback;
    task->hal_ret.number = 1;

    rc_info->quality_real = rc_info->quality_target;

    return MPP_OK;
}

const MppEncHalApi hal_jpege_vepu2 = {
    .name       = "hal_jpege_vepu2",
    .coding     = MPP_VIDEO_CodingMJPEG,
    .ctx_size   = sizeof(HalJpegeCtx),
    .flag       = 0,
    .init       = hal_jpege_vepu2_init,
    .deinit     = hal_jpege_vepu2_deinit,
    .prepare    = NULL,
    .get_task   = hal_jpege_vepu2_get_task,
    .gen_regs   = hal_jpege_vepu2_gen_regs,
    .start      = hal_jpege_vepu2_start,
    .wait       = hal_jpege_vepu2_wait,
    .part_start = hal_jpege_vepu2_part_start,
    .part_wait  = hal_jpege_vepu2_part_wait,
    .ret_task   = hal_jpege_vepu2_ret_task,
};
