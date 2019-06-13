/*
*
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
#ifndef __VP9D_PARSER_H__
#define __VP9D_PARSER_H__

#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_dec.h"

#include "vpx_rac.h"
#include "vp9.h"
#include "vp9data.h"
#include "vp9d_syntax.h"

extern RK_U32 vp9d_debug;

#define VP9D_DBG_FUNCTION (0x00000001)
#define VP9D_DBG_HEADER   (0x00000002)
#define VP9D_DBG_REF      (0x00000004)
#define VP9D_DBG_PORBE    (0x00000008)
#define VP9D_DBG_STRMIN   (0x00000010)



#define vp9d_dbg(flag, fmt, ...) _mpp_dbg(vp9d_debug, flag, fmt, ##__VA_ARGS__)

enum CompPredMode {
    PRED_SINGLEREF,
    PRED_COMPREF,
    PRED_SWITCHABLE,
};

enum BlockLevel {
    BL_64X64,
    BL_32X32,
    BL_16X16,
    BL_8X8,
};

enum BlockSize {
    BS_64x64,
    BS_64x32,
    BS_32x64,
    BS_32x32,
    BS_32x16,
    BS_16x32,
    BS_16x16,
    BS_16x8,
    BS_8x16,
    BS_8x8,
    BS_8x4,
    BS_4x8,
    BS_4x4,
    N_BS_SIZES,
};

typedef struct VP9Block {
    RK_U8 seg_id, intra, comp, ref[2], mode[4], uvmode, skip;
    enum FilterMode filter;
    Vpxmv mv[4 /* b_idx */][2 /* ref */];
    enum BlockSize bs;
    enum TxfmMode tx, uvtx;
    enum BlockLevel bl;
    enum BlockPartition bp;
} VP9Block;

struct VP9mvrefPair {
    Vpxmv mv[2];
    RK_S8 ref[2];
};

typedef struct RefInfo {
    RK_S32 ref_count;
    RK_U32 invisible;
    RK_U32 segMapIndex;
} RefInfo;

typedef struct VP9Frame {
    MppFrame f;
    RK_S32 slot_index;
    RefInfo *ref;
} VP9Frame;

struct VP9Filter {
    uint8_t level[8 * 8];
    uint8_t /* bit=col */ mask[2 /* 0=y, 1=uv */][2 /* 0=col, 1=row */]
    [8 /* rows */][4 /* 0=16, 1=8, 2=4, 3=inner4 */];
};
#define CUR_FRAME 0
#define REF_FRAME_MVPAIR 1
#define REF_FRAME_SEGMAP 2

typedef struct VP9Context {
    BitReadCtx_t gb;
    VpxRangeCoder c;
    VpxRangeCoder *c_b;
    RK_U32 c_b_size;
    VP9Block *b_base, *b;
    RK_S32 pass;
    RK_S32 row, row7, col, col7;
    RK_U8 *dst[3];
    RK_S32 y_stride, uv_stride;

    // bitstream header
    RK_U8 keyframe, last_keyframe;
    RK_U8 last_bpp, bpp, bpp_index, bytesperpixel;
    RK_U8 invisible;
    RK_U8 use_last_frame_mvs;
    RK_U8 errorres;
    RK_U8 ss_h, ss_v;
    RK_U8 extra_plane;
    RK_U8 intraonly;
    RK_U8 resetctx;
    RK_U8 refreshrefmask;
    RK_U8 highprecisionmvs;
    enum FilterMode filtermode;
    RK_U8 allowcompinter;
    RK_U8 fixcompref;
    RK_U8 refreshctx;
    RK_U8 parallelmode;
    RK_U8 framectxid;
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
    RK_U8 yac_qi;
    RK_S8 ydc_qdelta, uvdc_qdelta, uvac_qdelta;
    RK_U8 lossless;
#define MAX_SEGMENT 8
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
        RK_U32 log2_tile_cols, log2_tile_rows;
        RK_U32 tile_cols, tile_rows;
        RK_U32 tile_row_start, tile_row_end, tile_col_start, tile_col_end;
    } tiling;
    RK_U32 sb_cols, sb_rows, rows, cols;
    struct {
        prob_context p;
        RK_U8 coef[4][2][2][6][6][3];
    } prob_ctx[4];
    struct {
        prob_context p;
        RK_U8 coef[4][2][2][6][6][11];
        RK_U8 seg[7];
        RK_U8 segpred[3];
    } prob;
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
    enum TxfmMode txfmmode;
    enum CompPredMode comppredmode;

    // contextual (left/above) cache
    RK_U8 left_y_nnz_ctx[16];
    RK_U8 left_mode_ctx[16];
    Vpxmv left_mv_ctx[16][2];
    RK_U8 left_uv_nnz_ctx[2][16];
    RK_U8 left_partition_ctx[8];
    RK_U8 left_skip_ctx[8];
    RK_U8 left_txfm_ctx[8];
    RK_U8 left_segpred_ctx[8];
    RK_U8 left_intra_ctx[8];
    RK_U8 left_comp_ctx[8];
    RK_U8 left_ref_ctx[8];
    RK_U8 left_filter_ctx[8];

    // block reconstruction intermediates
    RK_S32 block_alloc_using_2pass;
    RK_S16  *block, *uvblock_base[2], *uvblock[2];
    RK_U8 *eob_base, *uveob_base[2], *eob, *uveob[2];
    struct { RK_S32 x, y; } min_mv, max_mv;
    RK_U16 mvscale[3][2];
    RK_U8 mvstep[3][2];

    RK_S32 first_partition_size;
    MppBufSlots slots;
    MppBufSlots packet_slots;
    HalDecTask *task;
    RK_S32 eos;       ///< current packet contains an EOS/EOB NAL
    RK_S64 pts;
    RK_S32 upprobe_num;
    RK_S32 outframe_num;
} VP9Context;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET vp9d_parser_init(Vp9CodecContext *vp9_ctx, ParserCfg *init);

MPP_RET vp9d_parser_deinit(Vp9CodecContext *vp9_ctx);

RK_S32 vp9_parser_frame(Vp9CodecContext *ctx, HalDecTask *in_task);

void vp9_parser_update(Vp9CodecContext *ctx, void *count_info);
MPP_RET vp9d_paser_reset(Vp9CodecContext *ctx);
RK_S32 vp9d_split_frame(SplitContext_t *ctx,
                        RK_U8 **out_data, RK_S32 *out_size,
                        RK_U8 *data, RK_S32 size);

MPP_RET vp9d_get_frame_stream(Vp9CodecContext *ctx, RK_U8 *buf, RK_S32 length);

MPP_RET vp9d_split_deinit(Vp9CodecContext *vp9_ctx);

MPP_RET vp9d_split_init(Vp9CodecContext *vp9_ctx);

RK_S32 vp9d_parser2_syntax(Vp9CodecContext *ctx);

#ifdef  __cplusplus
}
#endif


#endif /* __VP9D_PARSER_H__ */
