#include <string.h>
#include "vp9d_codec.h"
#include "vp9d_parser.h"
#include "vp9d_syntax.h"

static int vp9d_fill_segmentation(VP9Context *s, DXVA_segmentation_VP9 *seg)
{
    int i;

    seg->enabled = s->segmentation.enabled;
    seg->update_map = s->segmentation.update_map;
    seg->temporal_update = s->segmentation.temporal;
    seg->abs_delta = s->segmentation.absolute_vals;
    seg->ReservedSegmentFlags4Bits = 0;

    for (i = 0; i < 7; i++) {
        seg->tree_probs[i] = s->prob.seg[i];
    }

    seg->pred_probs[0] = s->prob.segpred[0];
    seg->pred_probs[1] = s->prob.segpred[1];
    seg->pred_probs[2] = s->prob.segpred[2];

    for (i = 0; i < 8; i++) {
        seg->feature_data[i][0] = s->segmentation.feat[i].q_val;
        seg->feature_data[i][1] = s->segmentation.feat[i].lf_val;
        seg->feature_data[i][2] = s->segmentation.feat[i].ref_val;
        seg->feature_data[i][3] = s->segmentation.feat[i].skip_enabled;
        seg->feature_mask[i] = s->segmentation.feat[i].q_enabled
                               | (s->segmentation.feat[i].lf_enabled << 1)
                               | (s->segmentation.feat[i].ref_enabled << 2)
                               | (s->segmentation.feat[i].skip_enabled << 3);
#if 0
        mpp_log("seg->feature_data[%d][0] = 0x%x", i, seg->feature_data[i][0]);

        mpp_log("seg->feature_data[%d][1] = 0x%x", i, seg->feature_data[i][0]);

        mpp_log("seg->feature_data[%d][2] = 0x%x", i, seg->feature_data[i][0]);

        mpp_log("seg->feature_data[%d][3] = 0x%x", i, seg->feature_data[i][0]);
        mpp_log("seg->feature_mask[%d] = 0x%x", i, seg->feature_mask[i]);
#endif
    }

    return 0;
}

static int vp9d_fill_picparams(Vp9CodecContext *ctx, DXVA_PicParams_VP9 *pic)
{
    VP9Context *s = ctx->priv_data;
    RK_U8 partition_probs[16][3];
    RK_U8 uv_mode_prob[10][9];
    int i;

    pic->profile = ctx->profile;
    pic->frame_type = !s->keyframe;
    pic->show_frame = !s->invisible;
    pic->error_resilient_mode =  s->errorres;
    pic->subsampling_x = s->ss_h;
    pic->subsampling_y = s->ss_v;
    pic->extra_plane = s->extra_plane;
    pic->refresh_frame_context = s->refreshctx;
    pic->intra_only = s->intraonly;
    pic->frame_context_idx = s->framectxid;
    pic->reset_frame_context = s->resetctx;
    pic->allow_high_precision_mv = s->highprecisionmvs;
    pic->parallelmode = s->parallelmode;
    pic->width = ctx->width;
    pic->height = ctx->height;
    pic->BitDepthMinus8Luma = s->bpp - 8;
    pic->BitDepthMinus8Chroma = s->bpp - 8;
    pic->interp_filter = s->filtermode;
    pic->CurrPic.Index7Bits = s->frames[CUR_FRAME].slot_index;

    for (i = 0; i < 8; i++) {
        pic->ref_frame_map[i].Index7Bits = s->refs[i].slot_index;
        pic->ref_frame_coded_width[i] = mpp_frame_get_width(s->refs[i].f);
        pic->ref_frame_coded_height[i] = mpp_frame_get_height(s->refs[i].f);
    }
    pic->frame_refs[0].Index7Bits =  s->refidx[0];
    pic->frame_refs[1].Index7Bits =  s->refidx[1];
    pic->frame_refs[2].Index7Bits =  s->refidx[2];
    pic->ref_frame_sign_bias[1] = s->signbias[0];
    pic->ref_frame_sign_bias[2] = s->signbias[1];
    pic->ref_frame_sign_bias[3] = s->signbias[2];
    pic->filter_level = s->filter.level;
    pic->sharpness_level = s->filter.sharpness;
    pic->mode_ref_delta_enabled = s->lf_delta.enabled;
    pic->mode_ref_delta_update = s->lf_delta.update;
    pic->use_prev_in_find_mv_refs = s->use_last_frame_mvs;
    pic->ref_deltas[0] = s->lf_delta.ref[0];
    pic->ref_deltas[1] = s->lf_delta.ref[1];
    pic->ref_deltas[2] = s->lf_delta.ref[2];
    pic->ref_deltas[3] = s->lf_delta.ref[3];
    pic->mode_deltas[0] = s->lf_delta.mode[0];
    pic->mode_deltas[1] = s->lf_delta.mode[1];
    pic->base_qindex = s->yac_qi;
    pic->y_dc_delta_q = s->ydc_qdelta;
    pic->uv_dc_delta_q = s->uvdc_qdelta;
    pic->uv_ac_delta_q = s->uvac_qdelta;
    pic->txmode = s->txfmmode;
    pic->refmode = s->comppredmode;
    vp9d_fill_segmentation(s, &pic->stVP9Segments);
    pic->log2_tile_cols = s->tiling.log2_tile_cols;
    pic->log2_tile_rows = s->tiling.log2_tile_rows;
    pic->first_partition_size = s->first_partition_size;
    memcpy(pic->mvscale, s->mvscale, sizeof(s->mvscale));
    memcpy(&ctx->pic_params.prob, &s->prob, sizeof(ctx->pic_params.prob));
    {
        RK_U8 *uv_ptr = NULL;
        RK_U32 m = 0;
        /*change partition to hardware need style*/
        /*
              hardware            syntax
          *+++++8x8+++++*     *++++64x64++++*
          *+++++16x16+++*     *++++32x32++++*
          *+++++32x32+++*     *++++16x16++++*
          *+++++64x64+++*     *++++8x8++++++*
        */
        m = 0;
        for (i = 3; i >= 0; i--) {
            memcpy(&partition_probs[m][0], &ctx->pic_params.prob.partition[i][0][0], 12);
            m += 4;
        }
        /*change uv_mode to hardware need style*/
        /*
            hardware              syntax
         *+++++ dc  ++++*     *++++ v   ++++*
         *+++++ v   ++++*     *++++ h   ++++*
         *+++++ h   ++++*     *++++ dc  ++++*
         *+++++ d45 ++++*     *++++ d45 ++++*
         *+++++ d135++++*     *++++ d135++++*
         *+++++ d117++++*     *++++ d117++++*
         *+++++ d153++++*     *++++ d153++++*
         *+++++ d207++++*     *++++ d63 ++++*
         *+++++ d63 ++++*     *++++ d207++++*
         *+++++ tm  ++++*     *++++ tm  ++++*
        */

        for (i = 0; i < 10; i++) {
            if (i == 0) {
                uv_ptr = ctx->pic_params.prob.uv_mode[2];//dc
            } else if ( i == 1) {
                uv_ptr =  ctx->pic_params.prob.uv_mode[0]; //h
            }  else if ( i == 2) {
                uv_ptr = ctx->pic_params.prob.uv_mode[1]; //h
            }  else if ( i == 7) {
                uv_ptr = ctx->pic_params.prob.uv_mode[8]; //d207
            } else if (i == 8) {
                uv_ptr = ctx->pic_params.prob.uv_mode[7]; //d63
            } else {
                uv_ptr = ctx->pic_params.prob.uv_mode[i];
            }
            memcpy(&uv_mode_prob[i], uv_ptr, 9);
        }
        memcpy(ctx->pic_params.prob.partition, partition_probs, sizeof(partition_probs));
        memcpy(ctx->pic_params.prob.uv_mode, uv_mode_prob, sizeof(uv_mode_prob));
    }
    return 0;
}

void vp9d_fill_counts(Vp9CodecContext *ctx)
{
    VP9Context *s = ctx->priv_data;
    memcpy(&ctx->pic_params.counts, &s->counts, sizeof(s->counts));
}

RK_S32 vp9d_parser2_syntax(Vp9CodecContext *ctx)
{
    vp9d_fill_picparams(ctx, &ctx->pic_params);
    return 0;
}
