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

#include "jpege_debug.h"
#include "jpege_api_v2.h"
#include "jpege_syntax.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_bitwrite.h"

typedef struct {
    MppEncCfgSet    *cfg;
    JpegeSyntax     syntax;
} JpegeCtx;

#define QUANTIZE_TABLE_SIZE 64

/*
 *  from RFC435 spec.
 */
static const RK_U8 jpege_luma_quantizer[QUANTIZE_TABLE_SIZE] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

static const RK_U8 jpege_chroma_quantizer[QUANTIZE_TABLE_SIZE] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

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
        MppEncJpegCfg *jpeg_cfg = &p->cfg->codec.jpeg;

        rc->fps_in_flex = 0;
        rc->fps_in_num = 30;
        rc->fps_in_denorm = 1;
        rc->fps_out_flex = 0;
        rc->fps_out_num = 30;
        rc->fps_out_denorm = 1;
        rc->rc_mode = MPP_ENC_RC_MODE_VBR;
        /* init default quant */
        jpeg_cfg->quant = 10;
    }

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_deinit_v2(void *ctx)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    MppEncJpegCfg *jpeg_cfg = &p->cfg->codec.jpeg;

    jpege_dbg_func("enter ctx %p\n", ctx);

    MPP_FREE(jpeg_cfg->qtable_y);
    MPP_FREE(jpeg_cfg->qtable_u);
    MPP_FREE(jpeg_cfg->qtable_v);

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_proc_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;
    MppFrameFormat fmt = dst->format & MPP_FRAME_FMT_MASK;

    mpp_assert(change);
    if (change) {
        MppEncPrepCfg bak = *dst;

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT)
            dst->format = src->format;

        if (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)
            dst->rotation = src->rotation;

        /* jpeg encoder do not have mirring / denoise feature */

        if ((change & MPP_ENC_PREP_CFG_CHANGE_INPUT) ||
            (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)) {
            if (dst->rotation == MPP_ENC_ROT_90 || dst->rotation == MPP_ENC_ROT_270) {
                dst->width = src->height;
                dst->height = src->width;
            } else {
                dst->width = src->width;
                dst->height = src->height;
            }
            dst->hor_stride = src->hor_stride;
            dst->ver_stride = src->ver_stride;
        }

        if (dst->width < 16 && dst->width > 8192) {
            mpp_err_f("invalid width %d is not in range [16..8192]\n", dst->width);
            ret = MPP_NOK;
        }

        if (dst->height < 16 && dst->height > 8192) {
            mpp_err_f("invalid height %d is not in range [16..8192]\n", dst->height);
            ret = MPP_NOK;
        }

        if ((fmt != MPP_FMT_YUV420SP    &&
             fmt != MPP_FMT_YUV420P     &&
             fmt != MPP_FMT_YUV422SP_VU &&
             fmt != MPP_FMT_YUV422_YUYV &&
             fmt != MPP_FMT_YUV422_UYVY &&
             fmt < MPP_FRAME_FMT_RGB)   ||
            fmt == MPP_FMT_RGB888       ||
            fmt == MPP_FMT_BGR888) {
            mpp_err_f("invalid format %d is not supportted\n", dst->format);
            ret = MPP_NOK;
        }

        dst->change |= change;

        // parameter checking
        if (dst->rotation == MPP_ENC_ROT_90 || dst->rotation == MPP_ENC_ROT_270) {
            if (dst->height > dst->hor_stride || dst->width > dst->ver_stride) {
                mpp_err("invalid size w:h [%d:%d] stride [%d:%d]\n",
                        dst->width, dst->height, dst->hor_stride, dst->ver_stride);
                ret = MPP_ERR_VALUE;
            }
        } else {
            if (dst->width > dst->hor_stride || dst->height > dst->ver_stride) {
                mpp_err("invalid size w:h [%d:%d] stride [%d:%d]\n",
                        dst->width, dst->height, dst->hor_stride, dst->ver_stride);
                ret = MPP_ERR_VALUE;
            }
        }

        if (ret) {
            mpp_err_f("failed to accept new prep config\n");
            *dst = bak;
        } else {
            jpege_dbg_ctrl("MPP_ENC_SET_PREP_CFG w:h [%d:%d] stride [%d:%d]\n",
                           dst->width, dst->height,
                           dst->hor_stride, dst->ver_stride);
        }
    }

    return ret;
}

/* gen quantizer table by q_factor according to RFC2435 spec. */
static MPP_RET jpege_gen_qt_by_qfactor(MppEncJpegCfg *cfg, RK_S32 *factor)
{
    MPP_RET ret = MPP_OK;
    RK_U32 q, qfactor = *factor;
    RK_U32 i;
    RK_U8 *qtable_y = NULL;
    RK_U8 *qtable_c = NULL;

    if (!cfg->qtable_y)
        cfg->qtable_y = mpp_malloc(RK_U8, QUANTIZE_TABLE_SIZE);
    if (!cfg->qtable_u)
        cfg->qtable_u = mpp_malloc(RK_U8, QUANTIZE_TABLE_SIZE);

    if (NULL == cfg->qtable_y || NULL == cfg->qtable_u) {
        mpp_err_f("qtable is null, malloc err");
        return MPP_ERR_MALLOC;
    }
    qtable_y = cfg->qtable_y;
    qtable_c = cfg->qtable_u;

    if (qfactor < 50)
        q = 5000 / qfactor;
    else
        q = 200 - (qfactor << 1);

    for (i = 0; i < QUANTIZE_TABLE_SIZE; i++) {
        RK_S16 lq = (jpege_luma_quantizer[i] * q + 50) / 100;
        RK_S16 cq = (jpege_chroma_quantizer[i] * q + 50) / 100;

        /* Limit the quantizers to 1 <= q <= 255 */
        qtable_y[i] = MPP_CLIP3(1, 255, lq);
        qtable_c[i] = MPP_CLIP3(1, 255, cq);
    }
    return ret;
}

static MPP_RET jpege_proc_jpeg_cfg(MppEncJpegCfg *dst, MppEncJpegCfg *src, MppEncRcCfg *rc)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change) {
        MppEncJpegCfg bak = *dst;

        if (change & MPP_ENC_JPEG_CFG_CHANGE_QP) {
            dst->quant = src->quant;
            if (dst->quant < 0 || dst->quant > 10) {
                mpp_err_f("invalid quality level %d is not in range [0..10] set to default 8\n");
                dst->quant = 8;
            }

            if (rc->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
                mpp_log("setup quant %d change mode %s fixqp", dst->quant,
                        strof_rc_mode(rc->rc_mode));
                rc->rc_mode = MPP_ENC_RC_MODE_FIXQP;
            }
        } else if (change & MPP_ENC_JPEG_CFG_CHANGE_QFACTOR) {
            if (src->q_factor < 1 || src->q_factor > 99) {
                mpp_err_f("q_factor out of range, default set 80\n");
                src->q_factor = 80;
            }
            if (dst->q_factor != src->q_factor)
                ret = jpege_gen_qt_by_qfactor(dst, &src->q_factor);

            dst->q_factor = src->q_factor;
            if (src->qf_min < 1 || src->qf_min > 99) {
                mpp_err_f("qf_min out of range, default set 1\n");
                src->qf_min = 1;
            }
            dst->qf_min = src->qf_min;
            if (src->qf_max < 1 || src->qf_max > 99) {
                mpp_err_f("qf_max out of range, default set 99\n");
                src->qf_max = 99;
            }
            dst->qf_max = src->qf_max;
            jpege_dbg_input("q_factor %d, qf_min %d, qf_max %d\n",
                            dst->q_factor, dst->qf_min, dst->qf_max);
        } else if (change & MPP_ENC_JPEG_CFG_CHANGE_QTABLE) {
            if (src->qtable_y && src->qtable_u && src->qtable_v) {
                if (NULL == dst->qtable_y)
                    dst->qtable_y = mpp_malloc(RK_U8, QUANTIZE_TABLE_SIZE);
                if (NULL == dst->qtable_u)
                    dst->qtable_u = mpp_malloc(RK_U8, QUANTIZE_TABLE_SIZE);

                if (NULL == dst->qtable_y || NULL == dst->qtable_u) {
                    mpp_err_f("qtable is null, malloc err\n");
                    return MPP_ERR_MALLOC;
                }
                /* check table value */
                if (src->qtable_u != src->qtable_v) {
                    RK_U32 i;

                    for (i = 0; i < QUANTIZE_TABLE_SIZE; i++) {
                        if (src->qtable_u[i] != src->qtable_v[i]) {
                            RK_U32 j;

                            jpege_dbg_input("qtable_u and qtable_v are different, use qtable_u\n");
                            for (j = 0; j < QUANTIZE_TABLE_SIZE; j++)
                                jpege_dbg_input("qtable_u[%d] %d qtable_v[%d] %d\n",
                                                j, src->qtable_u[j], j, src->qtable_v[j]);
                            break;
                        }
                    }
                }
                /* default use one chroma qtable, select qtable_u */
                memcpy(dst->qtable_y, src->qtable_y, QUANTIZE_TABLE_SIZE);
                memcpy(dst->qtable_u, src->qtable_u, QUANTIZE_TABLE_SIZE);

                if (rc->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
                    mpp_log("setup qtable will change mode %s fixqp",
                            strof_rc_mode(rc->rc_mode));
                    rc->rc_mode = MPP_ENC_RC_MODE_FIXQP;
                }
            } else {
                mpp_err_f("invalid qtable y %p u %p v %p\n",
                          src->qtable_y, src->qtable_u, src->qtable_v);
                ret = MPP_ERR_NULL_PTR;
            }
        }

        if (ret) {
            mpp_err_f("failed to accept new rc config\n");
            *dst = bak;
        } else {
            jpege_dbg_ctrl("MPP_ENC_SET_CODEC_CFG change 0x%x jpeg quant %d q_factor %d\n",
                           change, dst->quant, dst->q_factor);
            dst->change = src->change;
        }

        dst->change = src->change;
    }

    return ret;
}

static MPP_RET jpege_proc_split_cfg(MppEncSliceSplit *dst, MppEncSliceSplit *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change & MPP_ENC_SPLIT_CFG_CHANGE_MODE) {
        dst->split_mode = src->split_mode;
        dst->split_arg = src->split_arg;
    }

    if (change & MPP_ENC_SPLIT_CFG_CHANGE_ARG)
        dst->split_arg = src->split_arg;

    dst->change |= change;

    return ret;
}

static MPP_RET jpege_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    MppEncCfgSet *cfg = p->cfg;
    MPP_RET ret = MPP_OK;

    jpege_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        MppEncCfgImpl *impl = (MppEncCfgImpl *)param;
        MppEncCfgSet *src = &impl->cfg;

        if (src->prep.change) {
            ret |= jpege_proc_prep_cfg(&cfg->prep, &src->prep);
            src->prep.change = 0;
        }
        if (src->codec.jpeg.change) {
            ret |= jpege_proc_jpeg_cfg(&cfg->codec.jpeg, &src->codec.jpeg, &cfg->rc);
            src->codec.jpeg.change = 0;
        }
        if (src->split.change) {
            ret |= jpege_proc_split_cfg(&cfg->split, &src->split);
            src->split.change = 0;
        }
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        ret = jpege_proc_prep_cfg(&cfg->prep, param);
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        MppEncCodecCfg *codec = (MppEncCodecCfg *)param;
        ret = jpege_proc_jpeg_cfg(&cfg->codec.jpeg, &codec->jpeg, &cfg->rc);
    } break;
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

static MPP_RET jpege_proc_hal(void *ctx, HalEncTask *task)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    MppFrame frame = task->frame;
    JpegeSyntax *syntax = &p->syntax;
    MppEncCfgSet *cfg = p->cfg;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncCodecCfg *codec = &cfg->codec;
    MppEncSliceSplit *split = &cfg->split;

    jpege_dbg_func("enter ctx %p\n", ctx);

    syntax->width       = prep->width;
    syntax->height      = prep->height;
    syntax->hor_stride  = prep->hor_stride;
    syntax->ver_stride  = prep->ver_stride;
    syntax->mcu_w       = MPP_ALIGN(prep->width, 16) / 16;
    syntax->mcu_h       = MPP_ALIGN(prep->height, 16) / 16;
    syntax->format      = prep->format;
    syntax->color       = prep->color;
    syntax->rotation    = prep->rotation;
    syntax->offset_x    = mpp_frame_get_offset_x(frame);
    syntax->offset_y    = mpp_frame_get_offset_y(frame);
    syntax->quality     = codec->jpeg.quant;
    syntax->q_factor    = codec->jpeg.q_factor;
    syntax->qf_min      = codec->jpeg.qf_min;
    syntax->qf_max      = codec->jpeg.qf_max;
    syntax->qtable_y    = codec->jpeg.qtable_y;
    syntax->qtable_c    = codec->jpeg.qtable_u;
    syntax->part_rows   = 0;
    syntax->restart_ri  = 0;
    syntax->low_delay   = 0;

    if (split->split_mode) {
        RK_U32 mb_h = MPP_ALIGN(prep->height, 16) / 16;
        RK_U32 part_rows = 0;

        if (split->split_mode == MPP_ENC_SPLIT_BY_CTU) {
            RK_U32 part_mbs = split->split_arg;
            RK_U32 mb_w = MPP_ALIGN(prep->width, 16) / 16;
            RK_U32 mb_all = mb_w * mb_h;

            if (part_mbs > 0 && part_mbs <= mb_all) {
                part_rows = (part_mbs + mb_w - 1) / mb_w;
                if (part_rows >= mb_h)
                    part_rows = 0;
            } else {
                mpp_err_f("warning: invalid split arg %d > max %d\n",
                          part_mbs, mb_all);
            }
        } else {
            mpp_err_f("warning: only mcu split is supported\n");
        }

        if (part_rows) {
            syntax->part_rows   = part_rows;
            syntax->restart_ri  = syntax->mcu_w * part_rows;
            syntax->low_delay   = cfg->base.low_delay && part_rows;
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
