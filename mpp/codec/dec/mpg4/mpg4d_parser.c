/*
 *
 * Copyright 2010 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpg4d_parser"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"

#include "mpg4d_parser.h"
#include "mpg4d_syntax.h"


RK_U32 mpg4d_debug = 0;
#define mpg4d_dbg(flag, fmt, ...)   _mpp_dbg(mpg4d_debug, flag, fmt, ## __VA_ARGS__)
#define mpg4d_dbg_f(flag, fmt, ...) _mpp_dbg_f(mpg4d_debug, flag, fmt, ## __VA_ARGS__)

#define mpg4d_dbg_func(fmt, ...)    mpg4d_dbg_f(MPG4D_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define mpg4d_dbg_bit(fmt, ...)     mpg4d_dbg(MPG4D_DBG_BITS, fmt, ## __VA_ARGS__)
#define mpg4d_dbg_result(fmt, ...)  mpg4d_dbg(MPG4D_DBG_RESULT, fmt, ## __VA_ARGS__)

#define MPEG4_VIDOBJ_START_CODE             0x00000100  /* ..0x0000011f */
#define MPEG4_VIDOBJLAY_START_CODE          0x00000120  /* ..0x0000012f */
#define MPEG4_VISOBJSEQ_START_CODE          0x000001b0
#define MPEG4_VISOBJSEQ_STOP_CODE           0x000001b1
#define MPEG4_USERDATA_START_CODE           0x000001b2
#define MPEG4_GRPOFVOP_START_CODE           0x000001b3
#define MPEG4_VISOBJ_START_CODE             0x000001b5
#define MPEG4_VOP_START_CODE                0x000001b6

#define MPG4_VOL_STARTCODE                  0x120
#define MPG4_VOL_STOPCODE                   0x12F
#define MPG4_VOS_STARTCODE                  0x1B0
#define MPG4_VOS_STOPCODE                   0x1B1
#define MPG4_USER_DATA_STARTCODE            0x1B2
#define MPG4_GOP_STARTCODE                  0x1B3
#define MPG4_VISUAL_OBJ_STARTCODE           0x1B5
#define MPG4_VOP_STARTCODE                  0x1B6

typedef struct {
    RK_S32 method;

    RK_S32 opaque;
    RK_S32 transparent;
    RK_S32 intra_cae;
    RK_S32 inter_cae;
    RK_S32 no_update;
    RK_S32 upsampling;

    RK_S32 intra_blocks;
    RK_S32 inter_blocks;
    RK_S32 inter4v_blocks;
    RK_S32 gmc_blocks;
    RK_S32 not_coded_blocks;

    RK_S32 dct_coefs;
    RK_S32 dct_lines;
    RK_S32 vlc_symbols;
    RK_S32 vlc_bits;

    RK_S32 apm;
    RK_S32 npm;
    RK_S32 interpolate_mc_q;
    RK_S32 forw_back_mc_q;
    RK_S32 halfpel2;
    RK_S32 halfpel4;

    RK_S32 sadct;
    RK_S32 quarterpel;
} Mpeg4Estimation;

typedef struct Mp4HdrVol_t {
    RK_S32  vo_type;
    RK_U32  low_delay;
    RK_U32  shape;
    RK_S32  time_inc_resolution;
    RK_U32  time_inc_bits;
    RK_S32  width;
    RK_S32  height;
    RK_U32  mb_width;
    RK_U32  mb_height;
    RK_S32  hor_stride;
    RK_S32  ver_stride;
    RK_U32  totalMbInVop;
    RK_U32  interlacing;
    RK_S32  sprite_enable;
    RK_U32  quant_bits;
    RK_U32  quant_type;
    RK_S32  quarter_sample;
    RK_S32  complexity_estimation_disable;
    RK_U32  resync_marker_disable;
    RK_S32  newpred_enable;
    RK_S32  reduced_resolution_enable;
    RK_S32  scalability;
    RK_S32  ver_id;
} Mp4HdrVol;

typedef struct Mp4HdrUserData_t {
    RK_S32  packed_mode;                /* bframes packed bits? (1 = yes) */
} Mp4HdrUserData;

typedef struct Mp4HdrVop_t {
    RK_S32  coding_type;
    RK_U32  frameNumber;
    RK_U32  rounding;
    RK_U32  intra_dc_vlc_threshold;
    RK_U32  top_field_first;
    RK_U32  alternate_vertical_scan;
    RK_U32  fcode_forward;
    RK_U32  fcode_backward;
    RK_U32  quant;                      // OFFSET_OF_QUANT_IN_DEC
    RK_U32  hdr_bits;
} Mp4HdrVop;

typedef struct Mpg4Hdr_t {
    // vol parameter
    Mp4HdrVol       vol;

    // user data parameter
    Mp4HdrUserData  usr;

    // vop header parameter
    Mp4HdrVop       vop;

    // frame related parameter
    RK_S64  pts;
    RK_S32  slot_idx;
    RK_U32  enqueued;

    RK_U32  last_time_base;
    RK_U32  time_base;
    RK_U32  time;
    RK_U32  time_pp;
    RK_U32  time_bp;
    RK_U32  last_non_b_time;
} Mpg4Hdr;


typedef struct {
    // global paramter
    MppBufSlots     frame_slots;
    RK_U32          found_vol;
    RK_U32          found_vop;
    RK_U32          found_i_vop;

    // frame size parameter
    RK_S32          width;
    RK_S32          height;
    RK_S32          hor_stride;
    RK_S32          ver_stride;
    RK_U32          info_change;
    RK_U32          eos;

    // spliter parameter
    RK_U32          state;
    RK_U32          vop_header_found;   // flag: visual object plane header found

    // bit read context
    BitReadCtx_t    *bit_ctx;
    // vos infomation
    RK_U32          profile;
    RK_U32          level;
    RK_U32          custorm_version;
    // commom buffer for header information
    /*
     * NOTE: We assume that quant matrix only used for current frame decoding
     *       So we use only one quant matrix buffer and only support
     */
    RK_U32          new_qm[2];              // [0] - intra [1] - inter
    RK_U8           quant_matrices[128];    // 0-63: intra 64-127: inter
    Mpeg4Estimation estimation;
    Mpg4Hdr         hdr_curr;               /* header for current decoding frame */
    Mpg4Hdr         hdr_ref0;               /* header for reference frame 0 */
    Mpg4Hdr         hdr_ref1;               /* header for reference frame 1 */

    // dpb/output information
    RK_S32          output;
    RK_S64          last_pts;
    RK_S64          pts_inc;
    RK_S64          pts;
    RK_U32          frame_num;

    // syntax for hal
    mpeg4d_dxva2_picture_context_t *syntax;
} Mpg4dParserImpl;

static RK_S32 log2bin(RK_U32 value)
{
    RK_S32 n = 0;

    while (value) {
        value >>= 1;
        n++;
    }

    return n;
}

static MPP_RET mpg4d_parse_matrix(BitReadCtx_t *gb, RK_U8 * matrix)
{
    RK_S32 i = 0;
    RK_S32 last, value = 0;

    do {
        last = value;
        READ_BITS(gb, 8, &value);
        matrix[i++] = value;
    } while (value != 0 && i < 64);

    if (value != 0)
        return MPP_ERR_STREAM;

    i--;

    while (i < 64) {
        matrix[i++ ] = last;
    }

    return MPP_OK;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static void mpg4d_set_intra_matrix(RK_U8 * quant_matrices, RK_U8 * matrix)
{
    RK_S32 i;
    RK_U8 *intra_matrix = quant_matrices + 0 * 64;

    for (i = 0; i < 64; i++) {
        intra_matrix[i] = (!i) ? (RK_U8)8 : (RK_U8)matrix[i];
    }
}

static void mpg4d_set_inter_matrix(RK_U8 * quant_matrices, RK_U8 * matrix)
{
    RK_S32 i;
    RK_U8 *inter_matrix = quant_matrices + 1 * 64;

    for (i = 0; i < 64; i++) {
        inter_matrix[i] = (RK_U8) (matrix[i]);
    }
}

static MPP_RET read_vol_complexity_estimation_header(Mpeg4Estimation *e, BitReadCtx_t *gb)
{
    RK_U32 val;

    READ_BITS(gb, 2, &(e->method));                     /* estimation_method */

    if (e->method == 0 || e->method == 1) {
        READ_BITS(gb, 1, &val);
        if (!val) {                                     /* shape_complexity_estimation_disable */
            READ_BITS(gb, 1, &(e->opaque));             /* opaque */
            READ_BITS(gb, 1, &(e->transparent));        /* transparent */
            READ_BITS(gb, 1, &(e->intra_cae));          /* intra_cae */
            READ_BITS(gb, 1, &(e->inter_cae));          /* inter_cae */
            READ_BITS(gb, 1, &(e->no_update));          /* no_update */
            READ_BITS(gb, 1, &(e->upsampling));         /* upsampling */
        }

        READ_BITS(gb, 1, &val);
        if (!val) {                                     /* texture_complexity_estimation_set_1_disable */
            READ_BITS(gb, 1, &(e->intra_blocks));       /* intra_blocks */
            READ_BITS(gb, 1, &(e->inter_blocks));       /* inter_blocks */
            READ_BITS(gb, 1, &(e->inter4v_blocks));     /* inter4v_blocks */
            READ_BITS(gb, 1, &(e->not_coded_blocks));   /* not_coded_blocks */
        }
    }

    SKIP_BITS(gb, 1);

    READ_BITS(gb, 1, &val);
    if (!val) {                                         /* texture_complexity_estimation_set_2_disable */
        READ_BITS(gb, 1, &(e->dct_coefs));              /* dct_coefs */
        READ_BITS(gb, 1, &(e->dct_lines));              /* dct_lines */
        READ_BITS(gb, 1, &(e->vlc_symbols));            /* vlc_symbols */
        READ_BITS(gb, 1, &(e->vlc_bits));               /* vlc_bits */
    }

    READ_BITS(gb, 1, &val);
    if (!val) {                                         /* motion_compensation_complexity_disable */
        READ_BITS(gb, 1, &(e->apm));                    /* apm */
        READ_BITS(gb, 1, &(e->npm));                    /* npm */
        READ_BITS(gb, 1, &(e->interpolate_mc_q));       /* interpolate_mc_q */
        READ_BITS(gb, 1, &(e->forw_back_mc_q));         /* forw_back_mc_q */
        READ_BITS(gb, 1, &(e->halfpel2));               /* halfpel2 */
        READ_BITS(gb, 1, &(e->halfpel4));               /* halfpel4 */
    }

    SKIP_BITS(gb, 1);

    if (e->method == 1) {
        READ_BITS(gb, 1, &val);
        if (!val) {                                     /* version2_complexity_estimation_disable */
            READ_BITS(gb, 1, &(e->sadct));              /* sadct */
            READ_BITS(gb, 1, &(e->quarterpel));         /* quarterpel */
        }
    }

    return MPP_OK;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

/* vop estimation header */
static MPP_RET read_vop_complexity_estimation_header(Mpeg4Estimation *e, BitReadCtx_t *gb, Mpg4Hdr *mp4Hdr, int coding_type)
{
    if (e->method == 0 || e->method == 1) {
        if (coding_type == MPEG4_I_VOP) {
            if (e->opaque)  SKIP_BITS(gb, 8);           /* dcecs_opaque */
            if (e->transparent) SKIP_BITS(gb, 8);       /* */
            if (e->intra_cae) SKIP_BITS(gb, 8);         /* */
            if (e->inter_cae) SKIP_BITS(gb, 8);         /* */
            if (e->no_update) SKIP_BITS(gb, 8);         /* */
            if (e->upsampling) SKIP_BITS(gb, 8);        /* */
            if (e->intra_blocks) SKIP_BITS(gb, 8);      /* */
            if (e->not_coded_blocks) SKIP_BITS(gb, 8);  /* */
            if (e->dct_coefs) SKIP_BITS(gb, 8);         /* */
            if (e->dct_lines) SKIP_BITS(gb, 8);         /* */
            if (e->vlc_symbols) SKIP_BITS(gb, 8);       /* */
            if (e->vlc_bits) SKIP_BITS(gb, 8);          /* */
            if (e->sadct)  SKIP_BITS(gb, 8);            /* */
        }

        if (coding_type == MPEG4_P_VOP) {
            if (e->opaque) SKIP_BITS(gb, 8);            /* */
            if (e->transparent) SKIP_BITS(gb, 8);       /* */
            if (e->intra_cae) SKIP_BITS(gb, 8);         /* */
            if (e->inter_cae) SKIP_BITS(gb, 8);         /* */
            if (e->no_update) SKIP_BITS(gb, 8);         /* */
            if (e->upsampling) SKIP_BITS(gb, 8);        /* */
            if (e->intra_blocks) SKIP_BITS(gb, 8);      /* */
            if (e->not_coded_blocks) SKIP_BITS(gb, 8);  /* */
            if (e->dct_coefs) SKIP_BITS(gb, 8);         /* */
            if (e->dct_lines) SKIP_BITS(gb, 8);         /* */
            if (e->vlc_symbols) SKIP_BITS(gb, 8);       /* */
            if (e->vlc_bits) SKIP_BITS(gb, 8);          /* */
            if (e->inter_blocks) SKIP_BITS(gb, 8);      /* */
            if (e->inter4v_blocks) SKIP_BITS(gb, 8);    /* */
            if (e->apm) SKIP_BITS(gb, 8);               /* */
            if (e->npm) SKIP_BITS(gb, 8);               /* */
            if (e->forw_back_mc_q) SKIP_BITS(gb, 8);    /* */
            if (e->halfpel2) SKIP_BITS(gb, 8);          /* */
            if (e->halfpel4) SKIP_BITS(gb, 8);          /* */
            if (e->sadct) SKIP_BITS(gb, 8);             /* */
            if (e->quarterpel) SKIP_BITS(gb, 8);        /* */
        }

        if (coding_type == MPEG4_B_VOP) {
            if (e->opaque)  SKIP_BITS(gb, 8);           /* */
            if (e->transparent) SKIP_BITS(gb, 8);       /* */
            if (e->intra_cae) SKIP_BITS(gb, 8);         /* */
            if (e->inter_cae) SKIP_BITS(gb, 8);         /* */
            if (e->no_update) SKIP_BITS(gb, 8);         /* */
            if (e->upsampling) SKIP_BITS(gb, 8);        /* */
            if (e->intra_blocks) SKIP_BITS(gb, 8);      /* */
            if (e->not_coded_blocks) SKIP_BITS(gb, 8);  /* */
            if (e->dct_coefs) SKIP_BITS(gb, 8);         /* */
            if (e->dct_lines) SKIP_BITS(gb, 8);         /* */
            if (e->vlc_symbols) SKIP_BITS(gb, 8);       /* */
            if (e->vlc_bits) SKIP_BITS(gb, 8);          /* */
            if (e->inter_blocks) SKIP_BITS(gb, 8);      /* */
            if (e->inter4v_blocks) SKIP_BITS(gb, 8);    /* */
            if (e->apm) SKIP_BITS(gb, 8);               /* */
            if (e->npm) SKIP_BITS(gb, 8);               /* */
            if (e->forw_back_mc_q) SKIP_BITS(gb, 8);    /* */
            if (e->halfpel2) SKIP_BITS(gb, 8);          /* */
            if (e->halfpel4) SKIP_BITS(gb, 8);          /* */
            if (e->interpolate_mc_q) SKIP_BITS(gb, 8);  /* */
            if (e->sadct)  SKIP_BITS(gb, 8);            /* */
            if (e->quarterpel) SKIP_BITS(gb, 8);        /* */
        }

#ifdef GMC_SUPPORT
        if (coding_type == MPEG4_S_VOP && mp4Hdr->vol.sprite_enable == MPEG4_SPRITE_STATIC) {
            if (e->intra_blocks) SKIP_BITS(gb, 8);      /* */
            if (e->not_coded_blocks) SKIP_BITS(gb, 8);  /* */
            if (e->dct_coefs) SKIP_BITS(gb, 8);         /* */
            if (e->dct_lines) SKIP_BITS(gb, 8);         /* */
            if (e->vlc_symbols) SKIP_BITS(gb, 8);       /* */
            if (e->vlc_bits) SKIP_BITS(gb, 8);          /* */
            if (e->inter_blocks) SKIP_BITS(gb, 8);      /* */
            if (e->inter4v_blocks) SKIP_BITS(gb, 8);    /* */
            if (e->apm) SKIP_BITS(gb, 8);               /* */
            if (e->npm) SKIP_BITS(gb, 8);               /* */
            if (e->forw_back_mc_q) SKIP_BITS(gb, 8);    /* */
            if (e->halfpel2) SKIP_BITS(gb, 8);          /* */
            if (e->halfpel4) SKIP_BITS(gb, 8);          /* */
            if (e->interpolate_mc_q) SKIP_BITS(gb, 8);  /* */
        }
#else
        (void)mp4Hdr;
#endif
    }

    return MPP_OK;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static void init_mpg4_hdr_vol(Mpg4Hdr *header)
{
    memset(&header->vol, 0, sizeof(header->vol));
    header->vol.ver_id = 1;
}

static void init_mpg4_hdr_vop(Mpg4Hdr *header)
{
    memset(&header->vop, 0, sizeof(header->vop));
    header->vop.coding_type = MPEG4_INVALID_VOP;
}

static void init_mpg4_header(Mpg4Hdr *header)
{
    init_mpg4_hdr_vol(header);
    header->usr.packed_mode = 0;
    init_mpg4_hdr_vop(header);
    header->pts         = 0;
    header->slot_idx    = -1;
    header->enqueued    = 0;
}

static MPP_RET mpg4d_parse_vol_header(Mpg4dParserImpl *p, BitReadCtx_t *cb)
{
    RK_U32 val = 0;
    Mpg4Hdr *mp4Hdr = &p->hdr_curr;
    RK_S32 vol_ver_id;
    RK_S32 aspect_ratio;
    RK_S32 vol_control_parameters;

    SKIP_BITS(cb, 1);                                   /* random_accessible_vol */
    READ_BITS(cb, 8, &(mp4Hdr->vol.vo_type));

    READ_BITS(cb, 1, &val); /* is_object_layer_identifier */
    if (val) {
        READ_BITS(cb, 4, &vol_ver_id);  /* video_object_layer_verid */
        SKIP_BITS(cb, 3);                               /* video_object_layer_priority */
    } else {
        vol_ver_id = mp4Hdr->vol.ver_id;
    }

    READ_BITS(cb, 4, &aspect_ratio);

    if (aspect_ratio == MPEG4_VIDOBJLAY_AR_EXTPAR) {    /* aspect_ratio_info */
        RK_S32 par_width, par_height;

        READ_BITS(cb, 8, &par_width);                   /* par_width */
        READ_BITS(cb, 8, &par_height);                  /* par_height */
    }

    READ_BITS(cb, 1, &vol_control_parameters);

    if (vol_control_parameters) {                       /* vol_control_parameters */
        SKIP_BITS(cb, 2);                               /* chroma_format */
        READ_BITS(cb, 1, &(mp4Hdr->vol.low_delay));     /* low_delay flage (1 means no B_VOP) */

        READ_BITS(cb, 1, &val); /* vbv_parameters */
        if (val) {
            RK_U32 bitrate;
            RK_U32 buffer_size;
            RK_U32 occupancy;

            READ_BITS(cb, 15, &val); /* first_half_bit_rate */
            bitrate = val << 15;
            SKIP_BITS(cb, 1);
            READ_BITS(cb, 15, &val); /* latter_half_bit_rate */
            bitrate |= val;
            SKIP_BITS(cb, 1);

            READ_BITS(cb, 15, &val); /* first_half_vbv_buffer_size */
            buffer_size = val << 3;
            SKIP_BITS(cb, 1);
            READ_BITS(cb, 3, &val);
            buffer_size |= val;                             /* latter_half_vbv_buffer_size */

            READ_BITS(cb, 11, &val);
            occupancy = val << 15;                          /* first_half_vbv_occupancy */
            SKIP_BITS(cb, 1);
            READ_BITS(cb, 15, &val);
            occupancy |= val;                               /* latter_half_vbv_occupancy */
            SKIP_BITS(cb, 1);
        }
    } else {
        mp4Hdr->vol.low_delay = 0;
    }

    if (mp4Hdr->vol.vo_type == 0 && vol_control_parameters == 0 && mp4Hdr->vop.frameNumber == 0) {
        mpp_log("looks like this file was encoded with (divx4/(old)xvid/opendivx)\n");
        return MPP_NOK;
    }

    READ_BITS(cb, 2, &(mp4Hdr->vol.shape));                 /* video_object_layer_shape */

    if (mp4Hdr->vol.shape != MPEG4_VIDOBJLAY_SHAPE_RECTANGULAR) {
        mpp_log("unsupported shape %d\n", mp4Hdr->vol.shape);
        return MPP_NOK;
    }

    if (mp4Hdr->vol.shape == MPEG4_VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1) {
        SKIP_BITS(cb, 4);                                   /* video_object_layer_shape_extension */
    }

    SKIP_BITS(cb, 1);

    READ_BITS(cb, 16, &(mp4Hdr->vol.time_inc_resolution));  /* vop_time_increment_resolution */

    if (mp4Hdr->vol.time_inc_resolution > 0) {
        mp4Hdr->vol.time_inc_bits = MPP_MAX(log2bin(mp4Hdr->vol.time_inc_resolution - 1), 1);
    } else {
        /* for "old" xvid compatibility, set time_inc_bits = 1 */
        mp4Hdr->vol.time_inc_bits = 1;
    }

    SKIP_BITS(cb, 1);

    READ_BITS(cb, 1, &val);
    if (val) {                                          /* fixed_vop_rate */
        SKIP_BITS(cb, mp4Hdr->vol.time_inc_bits);       /* fixed_vop_time_increment */
    }

    if (mp4Hdr->vol.shape != MPEG4_VIDOBJLAY_SHAPE_BINARY_ONLY) {
        if (mp4Hdr->vol.shape == MPEG4_VIDOBJLAY_SHAPE_RECTANGULAR) {
            RK_S32 width, height;

            SKIP_BITS(cb, 1);
            READ_BITS(cb, 13, &width);                  /* video_object_layer_width */
            SKIP_BITS(cb, 1);
            READ_BITS(cb, 13, &height);                 /* video_object_layer_height */
            SKIP_BITS(cb, 1);

            mpg4d_dbg_bit("width %4d height %4d\n", width, height);

            mp4Hdr->vol.width  = width;
            mp4Hdr->vol.height = height;
            mp4Hdr->vol.mb_width  = (mp4Hdr->vol.width  + 15) >> 4;
            mp4Hdr->vol.mb_height = (mp4Hdr->vol.height + 15) >> 4;
            mp4Hdr->vol.totalMbInVop = mp4Hdr->vol.mb_width * mp4Hdr->vol.mb_height;
            mp4Hdr->vol.hor_stride = 16 * mp4Hdr->vol.mb_width;
            mp4Hdr->vol.ver_stride = 16 * mp4Hdr->vol.mb_height;
        }

        READ_BITS(cb, 1, &(mp4Hdr->vol.interlacing));

        READ_BITS(cb, 1, &val);
        if (!val) {                                     /* obmc_disable */
            /* TODO */
            /* fucking divx4.02 has this enabled */
        }

        /* sprite_enable */
        READ_BITS(cb, (vol_ver_id == 1 ? 1 : 2), &(mp4Hdr->vol.sprite_enable));

        if (mp4Hdr->vol.sprite_enable != MPEG4_SPRITE_NONE) {
            mpp_err("GMC is not supported\n");
            return MPP_ERR_PROTOL;
        }

        if (vol_ver_id != 1 &&
            mp4Hdr->vol.shape != MPEG4_VIDOBJLAY_SHAPE_RECTANGULAR) {
            SKIP_BITS(cb, 1);                           /* sadct_disable */
        }

        READ_BITS(cb, 1, &val);
        if (val) {                                      /* not_8_bit */
            READ_BITS(cb, 4, &(mp4Hdr->vol.quant_bits));/* quant_precision */
            SKIP_BITS(cb, 4);                           /* bits_per_pixel */
        } else {
            mp4Hdr->vol.quant_bits = 5;
        }

        if (mp4Hdr->vol.shape == MPEG4_VIDOBJLAY_SHAPE_GRAYSCALE) {
            SKIP_BITS(cb, 1);                           /* no_gray_quant_update */
            SKIP_BITS(cb, 1);                           /* composition_method */
            SKIP_BITS(cb, 1);                           /* linear_composition */
        }

        READ_BITS(cb, 1, &(mp4Hdr->vol.quant_type));    /* quant_type */

        if (mp4Hdr->vol.quant_type) {
            RK_U8 matrix[64];

            READ_BITS(cb, 1, &val);
            p->new_qm[0] = val;
            if (val) {                                  /* load_intra_quant_mat */
                mpg4d_parse_matrix(cb, matrix);
                mpg4d_set_intra_matrix(p->quant_matrices, matrix);
            }

            READ_BITS(cb, 1, &val);
            p->new_qm[1] = val;
            if (val) {                                  /* load_inter_quant_mat */
                mpg4d_parse_matrix(cb, matrix);
                mpg4d_set_inter_matrix(p->quant_matrices, matrix);
            }

            if (mp4Hdr->vol.shape == MPEG4_VIDOBJLAY_SHAPE_GRAYSCALE) {
                mpp_err("SHAPE_GRAYSCALE is not supported\n");
                return MPP_NOK;
            }
        } else {
            p->new_qm[0] = 0;
            p->new_qm[1] = 0;
        }

        if (vol_ver_id != 1) {
            READ_BITS(cb, 1, &(mp4Hdr->vol.quarter_sample));
        } else
            mp4Hdr->vol.quarter_sample = 0;

        /* complexity estimation disable */
        READ_BITS(cb, 1, &(mp4Hdr->vol.complexity_estimation_disable));

        if (!mp4Hdr->vol.complexity_estimation_disable) {
            mpg4d_dbg_bit("read_vol_complexity_estimation_header\n");
            if (read_vol_complexity_estimation_header(&p->estimation, cb))
                return MPP_ERR_STREAM;
        }

        /* resync_marker_disable */
        READ_BITS(cb, 1, &(mp4Hdr->vol.resync_marker_disable));
        if (!mp4Hdr->vol.resync_marker_disable) {
            mpp_log("resync marker enabled\n");
            // return MPEG4_RESYNC_VOP;
        }

        READ_BITS(cb, 1, &val);
        if (val) {                                      /* data_partitioned */
            SKIP_BITS(cb, 1);                           /* reversible_vlc */
        }

        if (vol_ver_id != 1) {
            READ_BITS(cb, 1, &(mp4Hdr->vol.newpred_enable));

            if (mp4Hdr->vol.newpred_enable) {               /* newpred_enable */
                SKIP_BITS(cb, 2);                       /* requested_upstream_message_type */
                SKIP_BITS(cb, 1);                       /* newpred_segment_type */
            }

            /* reduced_resolution_vop_enable */
            READ_BITS(cb, 1, &(mp4Hdr->vol.reduced_resolution_enable));
        } else {
            mp4Hdr->vol.newpred_enable = 0;
            mp4Hdr->vol.reduced_resolution_enable = 0;
        }

        READ_BITS(cb, 1, &mp4Hdr->vol.scalability);         /* scalability */

        if (mp4Hdr->vol.scalability) {
            SKIP_BITS(cb, 1);                           /* hierarchy_type */
            SKIP_BITS(cb, 4);                           /* ref_layer_id */
            SKIP_BITS(cb, 1);                           /* ref_layer_sampling_direc */
            SKIP_BITS(cb, 5);                           /* hor_sampling_factor_n */
            SKIP_BITS(cb, 5);                           /* hor_sampling_factor_m */
            SKIP_BITS(cb, 5);                           /* vert_sampling_factor_n */
            SKIP_BITS(cb, 5);                           /* vert_sampling_factor_m */
            SKIP_BITS(cb, 1);                           /* enhancement_type */

            if (mp4Hdr->vol.shape == MPEG4_VIDOBJLAY_SHAPE_BINARY /* && hierarchy_type==0 */) {
                /* use_ref_shape and so on*/
                SKIP_BITS(cb, ( 1 + 1 + 1 + 5 + 5 + 5 + 5));
            }

            mpp_err("scalability is not supported\n");
            return MPP_NOK;
        }
    } else { /* mp4Hdr->shape == BINARY_ONLY */
        mpp_err("shape %d is not supported\n");
        return MPP_NOK;
    }

    return MPP_OK;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET mpg4d_parse_user_data(Mpg4dParserImpl *p, BitReadCtx_t *gb)
{
    RK_U8 tmp[256];
    Mpg4Hdr *mp4Hdr = &p->hdr_curr;
    RK_U32 remain_bit = gb->bytes_left_ * 8 + gb->num_remaining_bits_in_curr_byte_;
    RK_S32 i;

    memset(tmp, 0, 256);

    for (i = 0; i < 256 && gb->bytes_left_; i++) {
        RK_U32 show_bit = MPP_MIN(remain_bit, 23);
        RK_U32 val;

        SHOW_BITS(gb, show_bit, &val);
        if (val == 0)
            break;

        READ_BITS(gb, 8, &val);
        tmp[i] = val;
        remain_bit -= 8;
    }

    tmp[i] = 0;

    if (!mp4Hdr->usr.packed_mode) {
        RK_U32 packed = 0;

        if ((tmp[0] == 'D') &&
            (tmp[1] == 'i') &&
            (tmp[2] == 'v') &&
            (tmp[3] == 'X')) {
            RK_U32 j = 4;

            if (tmp[j] <= '4') {
                p->custorm_version = 4;
            } else {
                p->custorm_version = 5;
            }

            while ((tmp[j] >= '0') &&
                   (tmp[j] <= '9'))
                j++;

            if (tmp[j] == 'b') {
                j++;

                while ((tmp[j] >= '0') &&
                       (tmp[j] <= '9'))
                    j++;

                packed = tmp[j];
            } else if ((tmp[j + 0] == 'B') &&
                       (tmp[j + 1] == 'u') &&
                       (tmp[j + 2] == 'i') &&
                       (tmp[j + 3] == 'l') &&
                       (tmp[j + 4] == 'd')) {
                j += 5;

                while ((tmp[j] >= '0') && (tmp[j] <= '9'))
                    j++;

                packed = tmp[j];
            }

            mp4Hdr->usr.packed_mode = ((packed == 'p') ? (1) : (0));
        } else {
            mp4Hdr->usr.packed_mode = 0;
        }
    }

    return MPP_OK;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET mpeg4_parse_gop_header(Mpg4dParserImpl *p, BitReadCtx_t *gb)
{
    (void) p;
    RK_S32 hours, minutes, seconds;

    READ_BITS(gb, 5, &hours);
    READ_BITS(gb, 6, &minutes);
    SKIP_BITS(gb, 1);
    READ_BITS(gb, 6, &seconds);

    SKIP_BITS(gb, 1); /* closed_gov */
    SKIP_BITS(gb, 1); /* broken_link */

    return MPP_OK;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET mpeg4_parse_profile_level(Mpg4dParserImpl *p, BitReadCtx_t *bc)
{
    READ_BITS(bc, 4, &p->profile);
    READ_BITS(bc, 4, &p->level);
    return MPP_OK;

__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET mpeg4_parse_vop_header(Mpg4dParserImpl *p, BitReadCtx_t *gb)
{
    RK_U32 val;
    RK_U32 time_incr = 0;
    RK_S32 time_increment = 0;
    Mpg4Hdr *mp4Hdr = &p->hdr_curr;

    READ_BITS(gb, 2, &(mp4Hdr->vop.coding_type));       /* vop_coding_type */

    READ_BITS(gb, 1, &val);
    while (val != 0) {
        time_incr++;                                    /* time_base */
        READ_BITS(gb, 1, &val);
    }

    SKIP_BITS(gb, 1);

    if (mp4Hdr->vol.time_inc_bits) {
        /* vop_time_increment */
        READ_BITS(gb, mp4Hdr->vol.time_inc_bits, &time_increment);
    }

    if (mp4Hdr->vop.coding_type != MPEG4_B_VOP) {
        mp4Hdr->last_time_base = mp4Hdr->time_base;
        mp4Hdr->time_base += time_incr;
        mp4Hdr->time = mp4Hdr->time_base * mp4Hdr->vol.time_inc_resolution + time_increment;
        mp4Hdr->time_pp = (RK_S32)(mp4Hdr->time - mp4Hdr->last_non_b_time);
        mp4Hdr->last_non_b_time = mp4Hdr->time;
    } else {
        mp4Hdr->time = (mp4Hdr->last_time_base + time_incr) * mp4Hdr->vol.time_inc_resolution + time_increment;
        mp4Hdr->time_bp = mp4Hdr->time_pp - (RK_S32)(mp4Hdr->last_non_b_time - mp4Hdr->time);
    }

    SKIP_BITS(gb, 1);

    READ_BITS(gb, 1, &val);
    if (!val) {                                         /* vop_coded */
        mp4Hdr->vop.coding_type = MPEG4_N_VOP;
        mpg4d_dbg_result("found N frame\n");
        return MPP_OK;
    }
    /* do coding_type detection here in order to save time_bp / time_pp */
    if (mp4Hdr->vop.coding_type == MPEG4_B_VOP &&
        (p->hdr_ref0.slot_idx == -1 || p->hdr_ref1.slot_idx == -1)) {
        mpg4d_dbg_result("MPEG4 DIVX 5 PBBI case found!\n");
        return MPP_NOK;
    }
    if (mp4Hdr->vop.coding_type == MPEG4_I_VOP)
        p->found_i_vop = 1;

    if (mp4Hdr->vol.newpred_enable) {
        RK_S32 vop_id;
        RK_S32 vop_id_for_prediction;

        READ_BITS(gb, (MPP_MIN(mp4Hdr->vol.time_inc_bits + 3, 15)), &vop_id);

        READ_BITS(gb, 1, &val);
        if (val) {
            /* vop_id_for_prediction_indication */
            READ_BITS(gb, MPP_MIN(mp4Hdr->vol.time_inc_bits + 3, 15), &vop_id_for_prediction);
        }

        SKIP_BITS(gb, 1);
    }

    if ((mp4Hdr->vol.shape != MPEG4_VIDOBJLAY_SHAPE_BINARY_ONLY) &&
        ((mp4Hdr->vop.coding_type == MPEG4_P_VOP) ||
         (mp4Hdr->vop.coding_type == MPEG4_S_VOP &&
          mp4Hdr->vol.sprite_enable == MPEG4_SPRITE_GMC))) {
        READ_BITS(gb, 1, &(mp4Hdr->vop.rounding));          /* rounding_type */
    }

    if (mp4Hdr->vol.reduced_resolution_enable &&
        mp4Hdr->vol.shape == MPEG4_VIDOBJLAY_SHAPE_RECTANGULAR &&
        (mp4Hdr->vop.coding_type == MPEG4_P_VOP || mp4Hdr->vop.coding_type == MPEG4_I_VOP)) {

        READ_BITS(gb, 1, &val);
    }

    mpp_assert(mp4Hdr->vol.shape == MPEG4_VIDOBJLAY_SHAPE_RECTANGULAR);

    if (mp4Hdr->vol.shape != MPEG4_VIDOBJLAY_SHAPE_BINARY_ONLY) {
        if (!mp4Hdr->vol.complexity_estimation_disable) {
            read_vop_complexity_estimation_header(&p->estimation, gb, mp4Hdr, mp4Hdr->vop.coding_type);
        }

        READ_BITS(gb, 3, &val);
        /* intra_dc_vlc_threshold */
        mp4Hdr->vop.intra_dc_vlc_threshold = val;
        mp4Hdr->vop.top_field_first = 0;
        mp4Hdr->vop.alternate_vertical_scan = 0;

        if (mp4Hdr->vol.interlacing) {
            READ_BITS(gb, 1, &(mp4Hdr->vop.top_field_first));
            READ_BITS(gb, 1, &(mp4Hdr->vop.alternate_vertical_scan));
        }
    }

    if ((mp4Hdr->vol.sprite_enable == MPEG4_SPRITE_STATIC ||
         mp4Hdr->vol.sprite_enable == MPEG4_SPRITE_GMC) &&
        mp4Hdr->vop.coding_type == MPEG4_S_VOP) {
        mpp_err("unsupport split mode %d coding type %d\n",
                mp4Hdr->vol.sprite_enable, mp4Hdr->vop.coding_type);
        return MPP_ERR_STREAM;
    }

    READ_BITS(gb, mp4Hdr->vol.quant_bits, &(mp4Hdr->vop.quant));
    if (mp4Hdr->vop.quant < 1)                              /* vop_quant */
        mp4Hdr->vop.quant = 1;

    if (mp4Hdr->vop.coding_type != MPEG4_I_VOP) {
        READ_BITS(gb, 3, &(mp4Hdr->vop.fcode_forward));     /* fcode_forward */
    }

    if (mp4Hdr->vop.coding_type == MPEG4_B_VOP) {
        READ_BITS(gb, 3, &(mp4Hdr->vop.fcode_backward));    /* fcode_backward */
    }

    if (!mp4Hdr->vol.scalability) {
        if ((mp4Hdr->vol.shape != MPEG4_VIDOBJLAY_SHAPE_RECTANGULAR) &&
            (mp4Hdr->vop.coding_type != MPEG4_I_VOP)) {
            SKIP_BITS(gb, 1);                           /* vop_shape_coding_type */
        }
    }

    // NOTE: record header used bits here
    mp4Hdr->vop.hdr_bits = gb->used_bits;

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static void mpg4d_fill_picture_parameters(const Mpg4dParserImpl *p,
                                          DXVA_PicParams_MPEG4_PART2 *pp)
{
    const Mpg4Hdr *hdr_curr = &p->hdr_curr;
    const Mpg4Hdr *hdr_ref0 = &p->hdr_ref0;
    const Mpg4Hdr *hdr_ref1 = &p->hdr_ref1;

    pp->short_video_header = 0;
    pp->vop_coding_type = hdr_curr->vop.coding_type;
    pp->vop_quant = hdr_curr->vop.quant;
    pp->wDecodedPictureIndex = hdr_curr->slot_idx;
    pp->wForwardRefPictureIndex = hdr_ref0->slot_idx;
    pp->wBackwardRefPictureIndex = hdr_ref1->slot_idx;
    pp->vop_time_increment_resolution = hdr_curr->vol.time_inc_resolution;
    pp->TRB[0] = 0; /* FIXME: */
    pp->TRD[0] = 0; /* FIXME: */
    pp->unPicPostProc = 0; /* FIXME: */
    pp->interlaced = hdr_curr->vol.interlacing;
    pp->quant_type = hdr_curr->vol.quant_type;
    pp->quarter_sample = hdr_curr->vol.quarter_sample;
    pp->resync_marker_disable = 0; /* FIXME: */
    pp->data_partitioned = 0; /* FIXME: */
    pp->reversible_vlc = 0; /* FIXME: */
    pp->reduced_resolution_vop_enable = hdr_curr->vol.reduced_resolution_enable;
    pp->vop_coded = (hdr_curr->vop.coding_type != MPEG4_N_VOP);
    pp->vop_rounding_type = hdr_curr->vop.rounding;
    pp->intra_dc_vlc_thr = hdr_curr->vop.intra_dc_vlc_threshold;
    pp->top_field_first = hdr_curr->vop.top_field_first;
    pp->alternate_vertical_scan_flag = hdr_curr->vop.alternate_vertical_scan;
    pp->profile_and_level_indication = (p->profile << 4) | p->level;
    pp->video_object_layer_verid = hdr_curr->vol.ver_id;
    pp->vop_width = hdr_curr->vol.width;
    pp->vop_height = hdr_curr->vol.height;
    pp->sprite_enable = hdr_curr->vol.sprite_enable;
    pp->no_of_sprite_warping_points = 0; /* FIXME: */
    pp->sprite_warping_accuracy = 0; /* FIXME: */
    memset(pp->warping_mv, 0, sizeof(pp->warping_mv));
    pp->vop_fcode_forward = hdr_curr->vop.fcode_forward;
    pp->vop_fcode_backward = hdr_curr->vop.fcode_backward;
    pp->StatusReportFeedbackNumber = 0; /* FIXME: */
    pp->Reserved16BitsA = 0; /* FIXME: */
    pp->Reserved16BitsB = 0; /* FIXME: */

    // Rockchip special data
    pp->custorm_version = p->custorm_version;
    pp->prev_coding_type = (hdr_ref0->vop.coding_type == MPEG4_INVALID_VOP) ? (0) : (hdr_ref0->vop.coding_type);
    pp->time_bp = hdr_curr->time_bp;
    pp->time_pp = hdr_curr->time_pp;
    pp->header_bits = hdr_curr->vop.hdr_bits;
}

static void mpg4d_fill_quantization_matrices(const Mpg4dParserImpl *p,
                                             DXVA_QmatrixData *qm)
{
    RK_S32 i = 0;

    qm->bNewQmatrix[0] = p->new_qm[0];
    qm->bNewQmatrix[1] = p->new_qm[1];
    qm->bNewQmatrix[2] = 0;
    qm->bNewQmatrix[3] = 0;

    // intra Y
    if (p->new_qm[0]) {
        for (i = 0; i < 64; i++) {
            qm->Qmatrix[0][i] = p->quant_matrices[i];
        }
    } else {
        memset(qm->Qmatrix[0], 0, sizeof(qm->Qmatrix[0]));
    }

    // inter Y
    if (p->new_qm[1]) {
        for (i = 0; i < 64; i++) {
            qm->Qmatrix[1][i] = p->quant_matrices[64 + i];
        }
    } else {
        memset(qm->Qmatrix[1], 0, sizeof(qm->Qmatrix[1]));
    }

    memset(qm->Qmatrix[2], 0, sizeof(qm->Qmatrix[2]));
    memset(qm->Qmatrix[3], 0, sizeof(qm->Qmatrix[3]));
}

static void mpg4_syntax_init(mpeg4d_dxva2_picture_context_t *syntax)
{
    DXVA2_DecodeBufferDesc *data = &syntax->desc[0];

    //!< commit picture paramters
    memset(data, 0, sizeof(*data));
    data->CompressedBufferType = DXVA2_PictureParametersBufferType;
    data->pvPVPState = (void *)&syntax->pp;
    data->DataSize = sizeof(syntax->pp);
    syntax->data[0] = data;

    //!< commit Qmatrix
    data = &syntax->desc[1];
    memset(data, 0, sizeof(*data));
    data->CompressedBufferType = DXVA2_InverseQuantizationMatrixBufferType;
    data->pvPVPState = (void *)&syntax->qm;
    data->DataSize = sizeof(syntax->qm);
    data->NumMBsInBuffer = 0;
    syntax->data[1] = data;

    //!< commit bitstream
    data = &syntax->desc[2];
    memset(data, 0, sizeof(*data));
    data->CompressedBufferType = DXVA2_BitStreamDateBufferType;
    syntax->data[2] = data;
}

MPP_RET mpp_mpg4_parser_init(Mpg4dParser *ctx, MppBufSlots frame_slots)
{
    BitReadCtx_t *bit_ctx = mpp_calloc(BitReadCtx_t, 1);
    Mpg4dParserImpl *p = mpp_calloc(Mpg4dParserImpl, 1);
    mpeg4d_dxva2_picture_context_t *syntax = mpp_calloc(mpeg4d_dxva2_picture_context_t, 1);

    if (NULL == p || NULL == bit_ctx || NULL == syntax) {
        mpp_err_f("malloc context failed\n");
        if (p)
            mpp_free(p);
        if (bit_ctx)
            mpp_free(bit_ctx);
        if (syntax)
            mpp_free(syntax);
        return MPP_NOK;
    }

    mpg4d_dbg_func("in\n");

    mpp_buf_slot_setup(frame_slots, 8);
    p->frame_slots      = frame_slots;
    p->state            = -1;
    p->vop_header_found = 0;
    p->bit_ctx          = bit_ctx;
    init_mpg4_header(&p->hdr_curr);
    init_mpg4_header(&p->hdr_ref0);
    init_mpg4_header(&p->hdr_ref1);
    mpg4_syntax_init(syntax);
    p->syntax = syntax;

    mpp_env_get_u32("mpg4d_debug", &mpg4d_debug, 0);

    mpg4d_dbg_func("out\n");

    *ctx = p;
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_deinit(Mpg4dParser ctx)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;

    mpg4d_dbg_func("in\n");

    if (p) {
        if (p->bit_ctx) {
            mpp_free(p->bit_ctx);
            p->bit_ctx = NULL;
        }
        if (p->syntax) {
            mpp_free(p->syntax);
            p->syntax = NULL;
        }
        mpp_free(p);
    }

    mpg4d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_flush(Mpg4dParser ctx)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    Mpg4Hdr *hdr_ref0 = &p->hdr_ref0;
    RK_S32 index = hdr_ref0->slot_idx;

    mpg4d_dbg_func("in\n");

    if (!hdr_ref0->enqueued && index >= 0) {
        mpp_buf_slot_set_flag(slots, index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(slots, index, QUEUE_DISPLAY);
        hdr_ref0->enqueued = 1;
    }

    mpg4d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_reset(Mpg4dParser ctx)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    Mpg4Hdr *hdr_ref0 = &p->hdr_ref0;
    Mpg4Hdr *hdr_ref1 = &p->hdr_ref1;
    RK_S32 index = hdr_ref0->slot_idx;

    mpg4d_dbg_func("in\n");

    if (index >= 0) {
        if (!hdr_ref0->enqueued) {
            mpp_buf_slot_set_flag(slots, index, SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(slots, index, QUEUE_DISPLAY);
            hdr_ref0->enqueued = 1;
        }
        mpp_buf_slot_clr_flag(slots, index, SLOT_CODEC_USE);
        hdr_ref0->slot_idx = -1;
    }

    index = hdr_ref1->slot_idx;
    if (index >= 0) {
        mpp_buf_slot_clr_flag(slots, index, SLOT_CODEC_USE);
        hdr_ref1->slot_idx = -1;
    }

    p->found_i_vop      = 0;
    p->found_vop        = 0;
    p->state            = -1;
    p->vop_header_found = 0;

    mpg4d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_split(Mpg4dParser ctx, MppPacket dst, MppPacket src)
{
    MPP_RET ret = MPP_NOK;
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    RK_U8 *src_buf = (RK_U8 *)mpp_packet_get_pos(src);
    RK_U32 src_len = (RK_U32)mpp_packet_get_length(src);
    RK_U32 src_eos = mpp_packet_get_eos(src);
    RK_S64 src_pts = mpp_packet_get_pts(src);
    RK_U8 *dst_buf = (RK_U8 *)mpp_packet_get_data(dst);
    RK_U32 dst_len = (RK_U32)mpp_packet_get_length(dst);
    RK_U32 src_pos = 0;

    mpg4d_dbg_func("in\n");

    // find the began of the vop
    if (!p->vop_header_found) {
        // add last startcode to the new frame data
        if ((dst_len < sizeof(p->state))
            && ((p->state & 0x00FFFFFF) == 0x000001)) {
            dst_buf[0] = 0;
            dst_buf[1] = 0;
            dst_buf[2] = 1;
            dst_len = 3;
        }
        while (src_pos < src_len) {
            p->state = (p->state << 8) | src_buf[src_pos];
            dst_buf[dst_len++] = src_buf[src_pos++];
            if (p->state == MPG4_VOP_STARTCODE) {
                p->vop_header_found = 1;
                mpp_packet_set_pts(dst, src_pts);
                break;
            }
        }
    }
    // find the end of the vop
    if (p->vop_header_found) {
        while (src_pos < src_len) {
            p->state = (p->state << 8) | src_buf[src_pos];
            dst_buf[dst_len++] = src_buf[src_pos++];
            if ((p->state & 0x00FFFFFF) == 0x000001) {
                dst_len -= 3;
                p->vop_header_found = 0;
                ret = MPP_OK; // split complete
                break;
            }
        }
    }
    // the last packet
    if (src_eos && src_pos >= src_len) {
        mpp_packet_set_eos(dst);
        ret = MPP_OK;
    }
    // reset the src and dst
    mpp_packet_set_length(dst, dst_len);
    mpp_packet_set_pos(src, src_buf + src_pos);

    mpg4d_dbg_func("out\n");

    return ret;
}

MPP_RET mpp_mpg4_parser_decode(Mpg4dParser ctx, MppPacket pkt)
{
    MPP_RET ret = MPP_OK;
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    BitReadCtx_t *gb = p->bit_ctx;
    RK_U8 *buf = mpp_packet_get_data(pkt);
    RK_S32 len = (RK_S32)mpp_packet_get_length(pkt);
    RK_U32 startcode = 0xff;

    mpg4d_dbg_func("in\n");

    // setup bit read context
    mpp_set_bitread_ctx(gb, buf, len);
    p->found_vop = 0;

    while (gb->bytes_left_) {
        RK_U32 val = 0;

        READ_BITS(gb, 8, &val);
        startcode = (startcode << 8) | val;

        if ((startcode & 0xFFFFFF00) != 0x100)
            continue;

        mpg4d_dbg_bit("found startcode at byte %d\n", gb->used_bits >> 3);

        if (mpg4d_debug & MPG4D_DBG_STARTCODE) {
            mpp_log_f("start code %03x\n", startcode);
            if (startcode <= 0x11F)
                mpp_log_f("Video Object Start");
            else if (startcode <= 0x12F)
                mpp_log_f("Video Object Layer Start");
            else if (startcode <= 0x13F)
                mpp_log_f("Reserved");
            else if (startcode <= 0x15F)
                mpp_log_f("FGS bp start");
            else if (startcode <= 0x1AF)
                mpp_log_f("Reserved");
            else if (startcode == 0x1B0)
                mpp_log_f("Visual Object Seq Start");
            else if (startcode == 0x1B1)
                mpp_log_f("Visual Object Seq End");
            else if (startcode == 0x1B2)
                mpp_log_f("User Data");
            else if (startcode == 0x1B3)
                mpp_log_f("Group of VOP start");
            else if (startcode == 0x1B4)
                mpp_log_f("Video Session Error");
            else if (startcode == 0x1B5)
                mpp_log_f("Visual Object Start");
            else if (startcode == 0x1B6)
                mpp_log_f("Video Object Plane start");
            else if (startcode == 0x1B7)
                mpp_log_f("slice start");
            else if (startcode == 0x1B8)
                mpp_log_f("extension start");
            else if (startcode == 0x1B9)
                mpp_log_f("fgs start");
            else if (startcode == 0x1BA)
                mpp_log_f("FBA Object start");
            else if (startcode == 0x1BB)
                mpp_log_f("FBA Object Plane start");
            else if (startcode == 0x1BC)
                mpp_log_f("Mesh Object start");
            else if (startcode == 0x1BD)
                mpp_log_f("Mesh Object Plane start");
            else if (startcode == 0x1BE)
                mpp_log_f("Still Texture Object start");
            else if (startcode == 0x1BF)
                mpp_log_f("Texture Spatial Layer start");
            else if (startcode == 0x1C0)
                mpp_log_f("Texture SNR Layer start");
            else if (startcode == 0x1C1)
                mpp_log_f("Texture Tile start");
            else if (startcode == 0x1C2)
                mpp_log_f("Texture Shape Layer start");
            else if (startcode == 0x1C3)
                mpp_log_f("stuffing start");
            else if (startcode <= 0x1C5)
                mpp_log_f("reserved");
            else if (startcode <= 0x1FF)
                mpp_log_f("System start");
        }

        if (startcode >= MPG4_VOL_STARTCODE && startcode <= MPG4_VOL_STOPCODE) {
            ret = mpg4d_parse_vol_header(p, gb);
            if (!p->found_vol)
                p->found_vol = (ret == MPP_OK);
        } else if (startcode == MPG4_USER_DATA_STARTCODE) {
            ret = mpg4d_parse_user_data(p, gb);
        } else if (startcode == MPG4_GOP_STARTCODE) {
            ret = mpeg4_parse_gop_header(p, gb);
        } else if (startcode == MPG4_VOS_STARTCODE) {
            ret = mpeg4_parse_profile_level(p, gb);
        } else if (startcode == MPG4_VOP_STARTCODE) {
            ret = mpeg4_parse_vop_header(p, gb);
            if (MPP_OK == ret) {
                RK_S32 coding_type = p->hdr_curr.vop.coding_type;
                mpg4d_dbg_bit("frame %d coding_type %d\n", p->frame_num, coding_type);
                p->frame_num++;

                if (coding_type == MPEG4_N_VOP) {
                    ret = MPP_OK;
                    mpp_align_get_bits(gb);
                    continue;
                }

                p->found_vop = p->found_i_vop;
            }
        }

        if (ret)
            goto __BITREAD_ERR;

        mpp_align_get_bits(gb);

        if (p->found_vol && p->found_vop)
            break;
    }

    if (p->found_vol) {
        mpg4d_dbg_result("found vol w %d h %d\n", p->hdr_curr.vol.width, p->hdr_curr.vol.height);
        p->width    = p->hdr_curr.vol.width;
        p->height   = p->hdr_curr.vol.height;
    }

    p->pts  = mpp_packet_get_pts(pkt);

    ret = (p->found_vol && p->found_vop) ? (MPP_OK) : (MPP_NOK);

__BITREAD_ERR:
    mpg4d_dbg_result("found vol %d vop %d ret %d\n", p->found_vol, p->found_vop, ret);

    if (ret) {
        mpp_packet_set_pos(pkt, buf);
        mpp_packet_set_length(pkt, 0);
    } else {
        RK_U32 used_bytes = gb->used_bits >> 3;
        mpp_packet_set_pos(pkt, buf + used_bytes);
    }

    p->eos = mpp_packet_get_eos(pkt);

    mpg4d_dbg_func("out\n");

    return ret;
}

MPP_RET mpp_mpg4_parser_setup_syntax(Mpg4dParser ctx, MppSyntax *syntax)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    mpeg4d_dxva2_picture_context_t *syn = p->syntax;

    mpg4d_dbg_func("in\n");

    mpg4d_fill_picture_parameters(p, &syn->pp);
    mpg4d_fill_quantization_matrices(p, &syn->qm);

    // fill bit stream parameter
    syn->data[2]->DataSize   = p->bit_ctx->buf_len;
    syn->data[2]->DataOffset = p->hdr_curr.vop.hdr_bits;
    syn->data[2]->pvPVPState = p->bit_ctx->buf;

    syntax->number = 3;
    syntax->data = syn->data;

    mpg4d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_setup_hal_output(Mpg4dParser ctx, RK_S32 *output)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    Mpg4Hdr *hdr_curr = &p->hdr_curr;
    RK_S32 index = -1;

    mpg4d_dbg_func("in\n");

    if (p->found_i_vop && hdr_curr->vop.coding_type != MPEG4_N_VOP) {
        MppBufSlots slots = p->frame_slots;
        RK_U32 frame_mode = MPP_FRAME_FLAG_FRAME;
        MppFrame frame = NULL;

        mpp_frame_init(&frame);
        mpp_frame_set_width(frame, p->width);
        mpp_frame_set_height(frame, p->height);
        mpp_frame_set_hor_stride(frame, MPP_ALIGN(p->width, 16));
        mpp_frame_set_ver_stride(frame, MPP_ALIGN(p->height, 16));

        /*
         * set slots information
         * 1. output index MUST be set
         * 2. get unused index for output if needed
         * 3. set output index as hal_input
         * 4. set frame information to output index
         * 5. if one frame can be display, it SHOULD be enqueued to display queue
         */
        mpp_buf_slot_get_unused(slots, &index);
        mpp_buf_slot_set_flag(slots, index, SLOT_HAL_OUTPUT);
        mpp_frame_set_pts(frame, p->pts);

        if (hdr_curr->vol.interlacing) {
            frame_mode = MPP_FRAME_FLAG_PAIRED_FIELD;
            if (hdr_curr->vop.top_field_first)
                frame_mode |= MPP_FRAME_FLAG_TOP_FIRST;
            else
                frame_mode |= MPP_FRAME_FLAG_BOT_FIRST;
        }
        mpp_frame_set_mode(frame, frame_mode);

        mpp_buf_slot_set_prop(slots, index, SLOT_FRAME, frame);
        mpp_frame_deinit(&frame);
        mpp_assert(NULL == frame);

        hdr_curr->slot_idx = index;
    }

    p->output = index;
    *output = index;

    mpg4d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_setup_refer(Mpg4dParser ctx, RK_S32 *refer, RK_S32 max_ref)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    Mpg4Hdr *hdr_curr = &p->hdr_curr;
    MppBufSlots slots = p->frame_slots;
    RK_S32 index;

    mpg4d_dbg_func("in\n");

    memset(refer, -1, sizeof(max_ref * sizeof(*refer)));
    if (hdr_curr->vop.coding_type != MPEG4_N_VOP) {
        index = p->hdr_ref0.slot_idx;
        if (index >= 0) {
            mpp_buf_slot_set_flag(slots, index, SLOT_HAL_INPUT);
            refer[0] = index;
        }
        index = p->hdr_ref1.slot_idx;
        if (index >= 0) {
            mpp_buf_slot_set_flag(slots, index, SLOT_HAL_INPUT);
            refer[1] = index;
        }
    }

    mpg4d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_update_dpb(Mpg4dParser ctx)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    Mpg4Hdr *hdr_curr = &p->hdr_curr;
    Mpg4Hdr *hdr_ref0 = &p->hdr_ref0;
    Mpg4Hdr *hdr_ref1 = &p->hdr_ref1;
    RK_S32 coding_type = hdr_curr->vop.coding_type;
    RK_S32 index = p->output;

    mpg4d_dbg_func("in\n");

    // update pts increacement
    if (p->pts != p->last_pts)
        p->pts_inc = p->pts - p->last_pts;

    switch (coding_type) {
    case MPEG4_B_VOP : {
        mpp_assert((hdr_ref0->slot_idx >= 0) && (hdr_ref1->slot_idx >= 0));
        // B frame -> index current frame
        // output current frame and do not change ref0 and ref1
        index = hdr_curr->slot_idx;
        mpp_buf_slot_set_flag(slots, index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(slots, index, QUEUE_DISPLAY);
    }
    /*
     * NOTE: here fallback to N vop - do nothing
     */
    case MPEG4_INVALID_VOP :
    case MPEG4_N_VOP : {
    } break;
    case MPEG4_I_VOP :
    case MPEG4_P_VOP :
    case MPEG4_S_VOP :
        // the other case -> index reference 0
        index = hdr_ref0->slot_idx;
        if (!hdr_ref0->enqueued && index >= 0) {
            mpp_buf_slot_set_flag(slots, index, SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(slots, index, QUEUE_DISPLAY);
        }
        // non B frame send this frame to reference queue
        mpp_buf_slot_set_flag(slots, hdr_curr->slot_idx, SLOT_CODEC_USE);

        // release ref1
        index = hdr_ref1->slot_idx;
        if (index >= 0)
            mpp_buf_slot_clr_flag(slots, index, SLOT_CODEC_USE);

        // swap ref0 to ref1, current to ref0
        *hdr_ref1 = *hdr_ref0;
        *hdr_ref0 = *hdr_curr;
        hdr_curr->pts       = 0;
    }

    init_mpg4_hdr_vop(hdr_curr);
    hdr_curr->slot_idx = -1;
    p->last_pts = p->pts;

    mpg4d_dbg_func("out\n");

    return MPP_OK;
}



