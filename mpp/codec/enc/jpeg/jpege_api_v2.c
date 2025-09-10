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

#define MODULE_TAG "jpege_api_v2"

#include <string.h>

#include "mpp_err.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_2str.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_bitwrite.h"
#include "mpp_soc.h"

#include "jpege_debug.h"
#include "jpege_api_v2.h"
#include "jpege_syntax.h"

typedef struct {
    MppEncCfgSet    *cfg;
    JpegeSyntax     syntax;
} JpegeCtx;

/*
 *  from RFC435 spec.
 */

RK_U32 jpege_debug = 0;

static MPP_RET jpege_init_v2(void *ctx, EncImplCfg *cfg)
{
    JpegeCtx *p = (JpegeCtx *)ctx;

    mpp_env_get_u32("jpege_debug", &jpege_debug, 0);
    jpege_dbg_func("enter ctx %p\n", ctx);

    p->cfg = cfg->cfg;

    mpp_assert(cfg->coding = MPP_VIDEO_CodingMJPEG);

    {
        /* init default rc config */
        MppEncRcCfg *rc = &p->cfg->rc;
        MppEncJpegCfg *jpeg_cfg = &p->cfg->jpeg;

        rc->fps_in_flex = 0;
        rc->fps_in_num = 30;
        rc->fps_in_denom = 1;
        rc->fps_out_flex = 0;
        rc->fps_out_num = 30;
        rc->fps_out_denom = 1;
        rc->rc_mode = MPP_ENC_RC_MODE_FIXQP;

        /* set quant to invalid value */
        jpeg_cfg->quant = -1;
        jpeg_cfg->quant_ext = -1;
    }

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_deinit_v2(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;

    jpege_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case MPP_ENC_SET_CFG :
    case MPP_ENC_SET_IDR_FRAME :
    case MPP_ENC_SET_OSD_PLT_CFG :
    case MPP_ENC_SET_OSD_DATA_CFG :
    case MPP_ENC_GET_SEI_DATA :
    case MPP_ENC_SET_SEI_CFG : {
    } break;
    default:
        mpp_err_f("No correspond cmd(%08x) found, and can not config!", cmd);
        ret = MPP_NOK;
        break;
    }

    jpege_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET jpege_start(void *ctx, HalEncTask *task)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    JpegeSyntax syntax = p->syntax;
    MppPacket pkt = task->packet;
    RK_U8 *ptr = mpp_packet_get_pos(pkt);
    size_t buf_size = mpp_packet_get_size(pkt);
    RK_S32 size = 0;
    MppWriteCtx bit_ctx;
    MppWriteCtx *bits = &bit_ctx;

    mpp_writer_init(bits, ptr, buf_size);

    /* add SOI and APP0 data */
    /* SOI */
    mpp_writer_put_raw_bits(bits, 0xFFD8, 16);
    /* APP0 */
    mpp_writer_put_raw_bits(bits, 0xFFE0, 16);
    /* length */
    mpp_writer_put_raw_bits(bits, 0x0010, 16);
    /* "JFIF" ID */
    /* Ident1 */
    mpp_writer_put_raw_bits(bits, 0x4A46, 16);
    /* Ident2 */
    mpp_writer_put_raw_bits(bits, 0x4946, 16);
    /* Ident3 */
    mpp_writer_put_raw_bits(bits, 0x00, 8);
    /* Version */
    mpp_writer_put_raw_bits(bits, 0x0102, 16);

    if (syntax.density_x && syntax.density_y) {
        /* Units */
        mpp_writer_put_raw_bits(bits, syntax.units_type, 8);
        /* Xdensity */
        mpp_writer_put_raw_bits(bits, syntax.density_x, 16);
        /* Ydensity */
        mpp_writer_put_raw_bits(bits, syntax.density_y, 16);
    } else {
        /* Units */
        mpp_writer_put_raw_bits(bits, 0, 8);
        /* Xdensity */
        mpp_writer_put_raw_bits(bits, 0, 8);
        mpp_writer_put_raw_bits(bits, 1, 8);
        /* Ydensity */
        mpp_writer_put_raw_bits(bits, 1, 16);
    }
    /* XThumbnail */
    mpp_writer_put_raw_bits(bits, 0x00, 8);
    /* YThumbnail */
    mpp_writer_put_raw_bits(bits, 0x00, 8);
    /* Do NOT write thumbnail */
    size = mpp_writer_bytes(bits);
    mpp_packet_set_length(pkt, size);
    task->length += size;

    return MPP_OK;
}

static MPP_RET init_jpeg_component_info(JpegeSyntax *syntax)
{
    MPP_RET ret = MPP_OK;
    JPEGCompInfo *comp_info = (JPEGCompInfo *)syntax->comp_info;

    jpege_dbg_input("Chroma format %d\n", syntax->format_out);

    if (syntax->format_out == MPP_CHROMA_UNSPECIFIED)
        syntax->format_out = MPP_CHROMA_420;

    memset(comp_info, 0, sizeof(JPEGCompInfo) * MAX_NUMBER_OF_COMPONENTS);

    switch (syntax->format_out) {
    case MPP_CHROMA_400:
        syntax->nb_components = 1;
        comp_info[0].val = 1 | 1 << 8 | 1 << 16;
        break;
    case MPP_CHROMA_420:
        syntax->nb_components = 3;
        comp_info[0].val = 1 | 2 << 8 | 2 << 16;
        comp_info[1].val = 2 | 1 << 8 | 1 << 16 | 1 << 24;
        comp_info[2].val = 3 | 1 << 8 | 1 << 16 | 1 << 24;
        break;
    case MPP_CHROMA_422:
        syntax->nb_components = 3;
        comp_info[0].val = 1 | 2 << 8 | 1 << 16;
        comp_info[1].val = 2 | 1 << 8 | 1 << 16 | 1 << 24;
        comp_info[2].val = 3 | 1 << 8 | 1 << 16 | 1 << 24;
        break;
    case MPP_CHROMA_444:
        syntax->nb_components = 3;
        comp_info[0].val = 1 | 1 << 8 | 1 << 16;
        comp_info[1].val = 2 | 1 << 8 | 1 << 16 | 1 << 24;
        comp_info[2].val = 3 | 1 << 8 | 1 << 16 | 1 << 24;
        break;
    default:
        syntax->nb_components = 1;
        comp_info[0].val = 1 | 1 << 8 | 1 << 16;
        mpp_err("Unsupported chroma format %d\n", syntax->format_out);
        ret = MPP_ERR_VALUE;
        break;
    }

    syntax->mcu_width = comp_info[0].h_sample_factor * DCT_SIZE;
    syntax->mcu_height = comp_info[0].v_sample_factor * DCT_SIZE;
    syntax->mcu_hor_cnt = (syntax->width + syntax->mcu_width - 1) / syntax->mcu_width;
    syntax->mcu_ver_cnt = (syntax->height + syntax->mcu_height - 1) / syntax->mcu_height;
    syntax->mcu_cnt = syntax->mcu_hor_cnt * syntax->mcu_ver_cnt;

    return ret;
}

static MPP_RET jpege_proc_hal(void *ctx, HalEncTask *task)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    MppFrame frame = task->frame;
    JpegeSyntax *syntax = &p->syntax;
    MppEncCfgSet *cfg = p->cfg;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncJpegCfg *jpeg = &cfg->jpeg;
    MppEncSliceSplit *split = &cfg->split;

    jpege_dbg_func("enter ctx %p\n", ctx);

    syntax->width       = prep->width;
    syntax->height      = prep->height;
    syntax->hor_stride  = prep->hor_stride;
    syntax->ver_stride  = prep->ver_stride;
    syntax->format      = prep->format;
    syntax->format_out  = prep->format_out;
    syntax->color       = prep->color;
    syntax->rotation    = prep->rotation;
    syntax->mirroring   = prep->mirroring;
    syntax->offset_x    = mpp_frame_get_offset_x(frame);
    syntax->offset_y    = mpp_frame_get_offset_y(frame);
    syntax->q_mode      = jpeg->q_mode;
    syntax->quant       = jpeg->quant;
    syntax->q_factor    = jpeg->q_factor;
    syntax->qf_min      = jpeg->qf_min;
    syntax->qf_max      = jpeg->qf_max;
    syntax->qtable_y    = jpeg->qtable_y;
    syntax->qtable_u    = jpeg->qtable_u;
    syntax->qtable_v    = jpeg->qtable_v;
    syntax->part_rows   = 0;
    syntax->restart_ri  = 0;
    syntax->low_delay   = 0;

    init_jpeg_component_info(syntax);

    if (split->split_mode) {
        RK_U32 mb_w = syntax->mcu_hor_cnt;
        RK_U32 mb_h = syntax->mcu_ver_cnt;
        RK_U32 part_rows = 0;

        if (split->split_mode == MPP_ENC_SPLIT_BY_CTU) {
            RK_U32 part_mbs = split->split_arg;

            if (part_mbs > 0 && part_mbs <= syntax->mcu_cnt) {
                part_rows = (part_mbs + mb_w - 1) / mb_w;
                if (part_rows >= mb_h)
                    part_rows = 0;
            } else {
                mpp_err_f("warning: invalid split arg %d > max %d\n",
                          part_mbs, syntax->mcu_cnt);
            }
        } else {
            mpp_err_f("warning: only mcu split is supported\n");
        }

        if (part_rows) {
            syntax->part_rows   = part_rows;
            if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3576 && split->split_arg <= syntax->mcu_cnt)
                syntax->restart_ri = split->split_arg;
            else
                syntax->restart_ri  = syntax->mcu_hor_cnt * part_rows;
            syntax->low_delay   = cfg->base.low_delay && part_rows;
            jpege_dbg_func("Split by CTU, part_rows %d, restart_ri %d",
                           syntax->part_rows, syntax->restart_ri);
        }
    }

    task->valid = 1;
    task->syntax.data = syntax;
    task->syntax.number = 1;

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_add_Prefix(MppPacket pkt, RK_S32 *len, RK_U8 uuid[16],
                                const void *data, RK_S32 size)
{
    RK_U8 *ptr = mpp_packet_get_pos(pkt);
    size_t length = mpp_packet_get_length(pkt);
    size_t buf_size = mpp_packet_get_size(pkt);
    MppWriteCtx bit_ctx;
    MppWriteCtx *bits = &bit_ctx;
    const RK_U8 *user_data = data;
    RK_S32 i = 0, app_size = 0;

    mpp_writer_init(bits, ptr + length, buf_size - length);

    if ((size > 8) && user_data[0] == 0xFF && user_data[1] == 0xE1 &&
        user_data[4] == 0x45 && user_data[5] == 0x78 &&
        user_data[6] == 0x69 && user_data[7] == 0x66) {
        jpege_dbg_func("write EXIF data, total length %d\n", size);
    } else if ((size > 8) && user_data[0] == 0xFF && user_data[1] == 0xE2 &&
               user_data[4] == 0x4D && user_data[5] == 0x50 &&
               user_data[6] == 0x46 && user_data[7] == 0x00) {
        jpege_dbg_func("write MPF data, total length %d\n", size);
    } else {
        /* add user data to APP7 */
        mpp_writer_put_raw_bits(bits, 0xFFE7, 16);
        /* length */
        mpp_writer_put_raw_bits(bits, size + 2, 16);
    }

    /* data */
    for (i = 0; i < size; i++)
        mpp_writer_put_raw_bits(bits, user_data[i], 8);

    app_size = mpp_writer_bytes(bits);
    *len = app_size;
    length += app_size;
    mpp_packet_set_length(pkt, length);
    (void)uuid;
    return MPP_OK;
}

const EncImplApi api_jpege = {
    .name       = "jpege_control",
    .coding     = MPP_VIDEO_CodingMJPEG,
    .ctx_size   = sizeof(JpegeCtx),
    .flag       = 0,
    .init       = jpege_init_v2,
    .deinit     = jpege_deinit_v2,
    .proc_cfg   = jpege_proc_cfg,
    .gen_hdr    = NULL,
    .start      = jpege_start,
    .proc_dpb   = NULL,
    .proc_hal   = jpege_proc_hal,
    .add_prefix = jpege_add_Prefix,
    .sw_enc     = NULL,
};
