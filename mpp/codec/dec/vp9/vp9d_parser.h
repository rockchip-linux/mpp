/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef VP9D_PARSER_H
#define VP9D_PARSER_H

#include <stdlib.h>

#include "mpp_debug.h"
#include "mpp_bitread.h"

#include "parser_api.h"
#include "vp9d_codec.h"
#include "vp9d_syntax.h"

extern RK_U32 vp9d_debug;

#define VP9D_DBG_FUNCTION           (0x00000001)
#define VP9D_DBG_HEADER             (0x00000002)
#define VP9D_DBG_REF                (0x00000004)
#define VP9D_DBG_PORBE              (0x00000008)
#define VP9D_DBG_STRMIN             (0x00000010)

#define vp9d_dbg(flag, fmt, ...)    mpp_dbg(vp9d_debug, flag, fmt, ##__VA_ARGS__)

typedef struct VP9RefInfo_t {
    RK_S32 ref_count;
    RK_U32 show_frame_flag;
    RK_U32 segMapIndex;
    RK_U32 is_output;
} VP9RefInfo;

typedef struct VP9Frame_t {
    MppFrame f;
    RK_S32 slot_index;
    VP9RefInfo *ref;
} VP9Frame;

#define VP9_CUR_FRAME 0
#define VP9_REF_FRAME_MVPAIR 1

#define MAX_SEGMENT 8
typedef struct VP9Context_t {
    BitReadCtx_t gb;
    Vp9dReader c;
    Vp9dReader *c_b;
    RK_U32 c_b_size;
    RK_S32 pass;
    RK_S32 row, row7, col, col7;
    RK_U8 *dst[3];
    RK_S32 y_stride, uv_stride;

    // bitstream header
    RK_U8 show_existing_frame;
    RK_U8 keyframe, last_keyframe;
    RK_U8 last_bpp, bpp, bpp_index, bytesperpixel;
    RK_U8 show_frame_flag;
    RK_U8 use_last_frame_mvs;
    RK_U8 error_res_mode;
    RK_U8 subsampling_x;
    RK_U8 subsampling_y;
    RK_U8 extra_plane;
    RK_U8 intraonly;
    RK_U8 resetctx;
    RK_U8 refresh_frame_flags;
    RK_U8 allow_high_precision_mv;
    VP9FilterMode interp_filter;
    RK_U8 allowcompinter;
    RK_U8 fixcompref;
    RK_U8 refresh_frame_context;
    RK_U8 frame_parallel_mode;
    RK_U8 frame_context_idx;
    RK_U8 refidx[3];
    RK_U8 signbias[3];
    RK_U8 varcompref[2];
    VP9Frame frames[3];
    VP9Frame refs[8];
    RK_U32 got_keyframes;

    struct {
        RK_U8 level;
        RK_S8 sharpness;
        RK_U8 lim_lut[64];
        RK_U8 mblim_lut[64];
    } filter;
    struct {
        RK_U8 enabled;
        RK_U8 update;
        RK_S8 mode[2];
        RK_S8 ref[4];
    } lf_delta;
    RK_U8 base_qindex;
    RK_U8 y_dc_delta_q;
    RK_U8 uv_dc_delta_q;
    RK_U8 uv_ac_delta_q;
    RK_U8 lossless;

    struct {
        RK_U8 enabled;
        RK_U8 temporal;
        RK_U8 absolute_vals;
        RK_U8 update_map;
        RK_U8 ignore_refmap;
        struct {
            RK_U8 q_enabled;
            RK_U8 lf_enabled;
            RK_U8 ref_enabled;
            RK_U8 skip_enabled;
            RK_U8 ref_val;
            RK_S16 q_val;
            RK_S8 lf_val;
            RK_S16 qmul[2][2];
            RK_U8 lflvl[4][2];
        } feat[MAX_SEGMENT];
    } segmentation;
    struct {
        RK_U32 log2_tile_cols;
        RK_U32 log2_tile_rows;
        RK_U32 tile_cols;
        RK_U32 tile_rows;
        RK_U32 tile_row_start;
        RK_U32 tile_row_end;
        RK_U32 tile_col_start;
        RK_U32 tile_col_end;
    } tiling;
    RK_U32 sb_cols, sb_rows, rows, cols;
    struct {
        Vp9ProbCtx p;
        RK_U8 coef[4][2][2][6][6][3];
    } prob_ctx[4];
    struct {
        Vp9ProbCtx p;
        RK_U8 coef[4][2][2][6][6][3];
        RK_U8 seg[7];
        RK_U8 segpred[3];
    } prob;
    struct {
        Vp9ProbCtx p_flag;
        Vp9ProbCtx p_delta;
        RK_U8 coef_flag[4][2][2][6][6][3];
        RK_U8 coef_delta[4][2][2][6][6][3];
    } prob_flag_delta;
    struct {
        RK_U32 partition[4][4][4];
        RK_U32 skip[3][2];
        RK_U32 intra[4][2];
        RK_U32 tx32p[2][4];
        RK_U32 tx16p[2][4]; //orign tx16p
        RK_U32 tx8p[2][2];
        RK_U32 y_mode[4][10];
        RK_U32 uv_mode[10][10];
        RK_U32 comp[5][2];
        RK_U32 comp_ref[5][2];
        RK_U32 single_ref[5][2][2];
        RK_U32 mv_mode[7][4];
        RK_U32 filter[4][3];
        RK_U32 mv_joint[4];
        RK_U32 sign[2][2];
        RK_U32 classes[2][12]; // orign classes[12]
        RK_U32 class0[2][2];
        RK_U32 bits[2][10][2];
        RK_U32 class0_fp[2][2][4];
        RK_U32 fp[2][4];
        RK_U32 class0_hp[2][2];
        RK_U32 hp[2][2];
        RK_U32 coef[4][2][2][6][6][3];
        RK_U32 eob[4][2][2][6][6][2];
    } counts;
    VP9TxfmMode txfmmode;
    VP9CompPredMode comppredmode;

    // block reconstruction intermediates
    RK_U16 mvscale[3][2];
    RK_U8 mvstep[3][2];

    RK_U32 uncompress_head_size_in_byte;
    RK_S32 first_partition_size;
    MppBufSlots slots;
    MppBufSlots packet_slots;
    MppDecCfgSet *cfg;
    const MppDecHwCap *hw_info;
    HalDecTask *task;
    RK_S32 eos;
    RK_S64 pts;
    RK_S64 dts;
    RK_S32 upprobe_num;
    RK_S32 outframe_num;
    RK_U32 cur_poc;
} VP9Context;

typedef struct VP9ParseCtx_t {
    RK_S32 n_frames; // 1-8
    RK_S32 size[8];
    RK_S64 pts;
} VP9ParseCtx;

#define MPP_PARSER_PTS_NB 4
typedef struct VP9SplitCtx_t {
    RK_U8 *buffer;
    RK_U32 buffer_size;
    RK_S32 index;
    RK_S32 last_index;
    RK_U32 state;
    RK_S32 frame_start_found;
    RK_S32 overread;
    RK_S32 overread_index;
    RK_U64 state64;
    RK_S64 pts;                 // pts of the current frame
    RK_S64 dts;                 // dts of the current frame
    RK_S64 frame_offset;
    RK_S64 cur_offset;
    RK_S64 next_frame_offset;

    /* private data */
    void  *priv_data;
    RK_S64 last_pts;
    RK_S64 last_dts;
    RK_S32 fetch_timestamp;

    RK_S32 cur_frame_start_index;
    RK_S64 cur_frame_offset[MPP_PARSER_PTS_NB];
    RK_S64 cur_frame_pts[MPP_PARSER_PTS_NB];
    RK_S64 cur_frame_dts[MPP_PARSER_PTS_NB];
    RK_S64 offset;              // byte offset from starting packet start
    RK_S64 cur_frame_end[MPP_PARSER_PTS_NB];

    RK_S32 key_frame;
    RK_S32 eos;
} VP9SplitCtx;

typedef struct Vp9DecCtx_t {
    void *priv_data;           // VP9ParseCtx
    void *priv_data2;          // VP9SplitCtx

    RK_S32 pix_fmt;
    RK_S32 profile;
    RK_S32 level;
    MppFrameColorSpace colorspace;
    MppFrameColorRange color_range;
    RK_S32 width, height;
    MppPacket pkt;

    DXVA_PicParams_VP9 pic_params;
    RK_S32 eos;
} Vp9DecCtx;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET vp9d_parser_init(Vp9DecCtx *vp9_ctx, ParserCfg *init);
MPP_RET vp9d_parser_deinit(Vp9DecCtx *vp9_ctx);
RK_S32 vp9_parser_frame(Vp9DecCtx *ctx, HalDecTask *in_task);
MPP_RET vp9_parser_update(Vp9DecCtx *ctx, void *count_info);
MPP_RET vp9d_paser_reset(Vp9DecCtx *ctx);
RK_S32 vp9d_split_frame(VP9SplitCtx *ctx, RK_U8 **out_data, RK_S32 *out_size,
                        RK_U8 *data, RK_S32 size);

MPP_RET vp9d_get_frame_stream(Vp9DecCtx *ctx, RK_U8 *buf, RK_S32 length);
MPP_RET vp9d_split_init(Vp9DecCtx *vp9_ctx);
MPP_RET vp9d_split_deinit(Vp9DecCtx *vp9_ctx);

#ifdef  __cplusplus
}
#endif

#endif /* VP9D_PARSER_H */
