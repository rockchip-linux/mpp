/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_PARSER_H
#define H265D_PARSER_H

#include <limits.h>
#include <string.h>

#include "mpp_bitread.h"
#include "mpp_buf_slot.h"
#include "mpp_mem_pool.h"
#include "mpp_ref_pool.h"

#include "parser_api.h"

#include "hal_dec_task.h"
#include "h265d_slice.h"
#include "h2645d_sei.h"
#include "h265d_spliter.h"

/* Frame flags - values match bit field positions in H265dFrame */
#define H265D_FRM_FLAG_OUTPUT       (1 << 0)  /* ref_status->output  */
#define H265D_FRM_FLAG_SHORT_REF    (1 << 1)  /* ref_status->st_ref  */
#define H265D_FRM_FLAG_LONG_REF     (1 << 2)  /* ref_status->lt_ref  */
#define H265D_FRM_FLAG_ERROR        (1 << 3)  /* err_status->has_err */

typedef void* H265dParser;

typedef struct H265dCtx_t {
    ParserCfg *parser_cfg;
    MppDecCfgSet *cfg;

    RK_S32 width;
    RK_S32 height;
    RK_S32 coded_width;
    RK_S32 coded_height;
    RK_U32 pix_fmt;
    RK_U32 bit_depth;

    MppFrameColorSpace colorspace;
    MppFrameColorRange color_range;

    RK_U8 *extradata;
    RK_U32 extradata_size;

    H265dSpliter spliter;
    H265dParser parser;
} H265dCtx;

/* Frame reference/output status - used by h265d_frame_unref */
typedef union {
    RK_U8 flags;
    struct {
        RK_U8 output : 1;
        RK_U8 st_ref : 1;
        RK_U8 lt_ref : 1;
    };
} H265dFrmRefStatus;

/* Frame error status - managed independently */
typedef union {
    RK_U8 flags;
    struct {
        RK_U8 has_err : 1;
    };
} H265dFrmErrStatus;

typedef struct H265dFrame_t {
    MppFrame frame;

    RK_S32 poc;
    RK_S32 slot_index;
    RK_S32 hdr_meta_index;          /* HDR metadata index in pool (-1 = none) */

    RK_U16 sequence;
    H265dFrmRefStatus ref_status;   /* Reference/output status */
    H265dFrmErrStatus err_status;   /* Error status */
} H265dFrame;

typedef struct H265dRefList_t {
    H265dFrame *ref[MAX_REFS];
    RK_S32 list[MAX_REFS];
    RK_S32 isLongTerm[MAX_REFS];
    RK_S32 nb_refs;
} H265dRefList;

typedef struct H265dNal_t {
    RK_S32 size;              /* NAL data size (without start code) */
    RK_S32 bitstream_offset;  /* offset in the prepared bitstream */
} H265dNal;

typedef struct HEVCSEIAlternativeTransfer {
    RK_S32 present;
    RK_S32 preferred_transfer_characteristics;
} HEVCSEIAlternativeTransfer;

typedef struct H265dPrs_t {
    H265dCtx *ctx;
    MppBufSlots slots;
    MppBufSlots packet_slots;
    const MppDecHwCap *hw_info;

    HalDecTask *task;
    MppPacket input_packet;
    void *hal_pic_private;

    BitReadCtx_t bit;

    MppFrame frame;

    const H265dVps *vps;
    const H265dSps *sps;
    const H265dPps *pps;
    H265dVps *vps_list[MAX_VPS_COUNT];
    H265dSps *sps_list[MAX_SPS_COUNT];
    H265dPps *pps_list[MAX_PPS_COUNT];

    MppMemPool vps_pool;
    MppMemPool sps_pool;

    H265dSlice slice;

    H265dRefList rps[5];

    enum NALUnitType nal_unit_type;
    RK_S32 temporal_id;
    H265dFrame *ref;
    H265dFrame dpb[MAX_DPB_SIZE];
    RK_S32 poc;
    RK_S32 pocTid0;
    RK_S32 slice_idx;
    RK_S32 slice_cnt;
    RK_S32 eos;
    RK_S32 max_ra;

    RK_S32 is_decoded;

    RK_U16 seq_decode;
    RK_U16 seq_output;

    H265dNal *nals;
    RK_S32 nb_nals;
    RK_S32 nals_allocated;
    enum NALUnitType first_nal_type;

    RK_U8 *bitstream;         /* prepared bitstream with start codes */
    RK_S32 bitstream_size;     /* size of allocated bitstream buffer */
    RK_S32 bitstream_pos;      /* current position in bitstream */

    RK_U8 is_nalff;
    RK_S32 temporal_layer_id;

    RK_S32 active_seq_parameter_set_id;

    RK_S32 nal_length_size;
    RK_S32 nuh_layer_id;

    RK_U8 slice_initialized;

    RK_U32 nb_frame;
    RK_U32 extra_has_frame;

    MppFrameMasteringDisplayMetadata mastering_display;
    MppFrameContentLightMetadata content_light;
    MppRefPool hdr_meta_pool;       /* HDR dynamic metadata pool */
    RK_S32 hdr_meta_current;            /* current active meta index in pool */
    HEVCSEIAlternativeTransfer alternative_transfer;

    RK_S64 pts;
    RK_S64 dts;
    RK_U8  miss_ref_flag;
    RK_U8  pre_pps_id;
    RK_U8  ps_need_upate;
    RK_U8  rps_need_upate;

    /* Bitmap to track which SPS/PPS IDs have been (re)parsed since last check */
    RK_U32  sps_update_mask;   /* Bit i set means SPS i was (re)parsed */
    RK_U64  pps_update_mask;   /* Bit i set means PPS i was (re)parsed */

    RK_S32  nb_refs;             /* number of reference frames for current frame */

    RK_U32  start_bit;
    RK_U32  end_bit;
    RK_S32  first_i_fast_play;
    RK_U32  is_hdr;
    RK_U32  hdr_dynamic;

    RK_U32  deny_flag;
    RecoveryPoint recovery;
    RK_U32  cap_hw_h265_rps;
    RK_U32  consumed_bytes;
} H265dPrs;

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET h265d_flush(void *ctx);

RK_S32 h265d_parser_init(H265dParser *s, H265dCtx *ctx);
RK_S32 h265d_parser_deinit(H265dParser s);

RK_S32 h265d_extradata(H265dPrs *p);
RK_S32 H265d_nal_head(H265dPrs *p);
RK_S32 h265d_nal_units(H265dPrs *p);
RK_S32 h265d_nal_unit(H265dPrs *p, const RK_U8 *nal, RK_S32 length);

RK_S32 h265d_split_nal(H265dPrs *p, RK_U8 *buf, RK_S32 length);

void h265d_fill_dynamic_meta(H265dPrs *p, const RK_U8 *data, RK_U32 size, RK_U32 hdr_fmt);

RK_S32 h265d_parser2_syntax(H265dCtx *ctx);
RK_S32 h265d_syntax_fill_slice(H265dCtx *ctx, RK_S32 input_index);

#ifdef  __cplusplus
}
#endif

#endif /* H265D_PARSER_H */
