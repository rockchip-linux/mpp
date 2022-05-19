/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpi_rc_test"

#include <string.h>
#include <math.h>

#include "rk_mpi.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpi_dec_utils.h"
#include "mpi_enc_utils.h"

#define MPI_BIT_DEPTH               8
#define MPI_PIXEL_MAX               ((1 << MPI_BIT_DEPTH) - 1)

typedef RK_U8 pixel;

typedef struct {
    double          psnr_y;
    double          ssim_y;
    RK_U32          avg_bitrate; // Every sequence, byte per second
    RK_U32          ins_bitrate; // Every second, byte per second
    RK_U32          frame_size; // Every frame, byte
} MpiRcStat;

typedef struct {
    FILE *fp_input;
    FILE *fp_enc_out;
    FILE *fp_stat;
} MpiRcFile;

typedef struct {
    MpiEncTestArgs* enc_cmd;
    MpiRcStat       stat;
    MpiRcFile       file;

    RK_U8           *com_buf;
    MppCtx          enc_ctx;
    MppCtx          dec_ctx_post;
    MppCtx          dec_ctx_pre;
    MppApi          *enc_mpi;
    MppApi          *dec_mpi_post;
    MppApi          *dec_mpi_pre;

    /* 1. pre-decoder data */
    FileReader      reader;
    MppCodingType   dec_type;
    RK_S32          pre_pkt_idx;
    RK_S32          pre_frm_idx;
    RK_S32          pre_frm_num;
    MppPacket       dec_pkt_pre;
    RK_U8           *dec_in_buf_pre;
    RK_U32          dec_in_buf_pre_size;

    /* 2. encoder data */
    MppBufferGroup  pkt_grp;
    MppBuffer       enc_pkt_buf;
    MppPacket       enc_pkt;

    MppEncRcCfg     rc_cfg;
    MppEncPrepCfg   prep_cfg;

    MppTask         enc_in_task;
    MppTask         enc_out_task;

    /* 3. post-decoder data */
    MppPacket       dec_pkt_post;
    RK_U8           *dec_in_buf_post;
    RK_U32          dec_in_buf_post_size;

    RK_U32          loop_end;
    RK_U32          pkt_eos;
    RK_S32          frm_eos;
    RK_U32          enc_pkt_eos;
    MppEncCfg       cfg;

    RK_S32          frm_idx;
    RK_U64          total_bits;
    double          total_psnrs;
    double          total_ssims;
    RK_U32          calc_base_idx;
    RK_U32          stream_size_1s;
    pthread_t       dec_thr;
    RK_S64          start_enc;
} MpiRc2TestCtx;

static void mpi_rc_deinit(MpiRc2TestCtx *ctx)
{
    MpiRcFile *file = &ctx->file;

    if (file->fp_enc_out) {
        fclose(file->fp_enc_out);
        file->fp_enc_out = NULL;
    }

    if (file->fp_stat) {
        fclose(file->fp_stat);
        file->fp_stat = NULL;
    }

    if (file->fp_input) {
        fclose(file->fp_input);
        file->fp_input = NULL;
    }

    if (ctx->reader) {
        reader_deinit(ctx->reader);
        ctx->reader = NULL;
    }

    MPP_FREE(ctx->com_buf);
}

static MPP_RET mpi_rc_init(MpiRc2TestCtx *ctx)
{
    MpiEncTestArgs* enc_cmd = ctx->enc_cmd;

    if (enc_cmd->file_input)
        reader_init(&ctx->reader, enc_cmd->file_input);

    if (NULL == ctx->reader) {
        mpp_err("failed to open dec input file %s\n", enc_cmd->file_input);
        return MPP_NOK;
    }

    mpp_log("input file %s size %ld\n", enc_cmd->file_input, reader_size(ctx->reader));
    ctx->dec_type = enc_cmd->type_src;

    if (enc_cmd->file_output) {
        MpiRcFile *file = &ctx->file;

        file->fp_enc_out = fopen(enc_cmd->file_output, "w+b");
        if (NULL == file->fp_enc_out) {
            mpp_err("failed to open enc output file %s\n", enc_cmd->file_output);
            return MPP_ERR_OPEN_FILE;
        }
    }

    return MPP_OK;
}

static MPP_RET mpi_rc_cmp_frame(MppFrame frame_in, MppFrame frame_out)
{
    void *enc_buf = mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_in));
    void *dec_buf = mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_out));
    RK_U32 enc_width  = mpp_frame_get_width(frame_in);
    RK_U32 enc_height = mpp_frame_get_height(frame_in);
    RK_U32 dec_width  = mpp_frame_get_width(frame_out);
    RK_U32 dec_height = mpp_frame_get_height(frame_out);

    if (!enc_buf) {
        mpp_err_f("enc buf is NULL");
        return MPP_NOK;
    }

    if (!dec_buf) {
        mpp_err_f("dec buf is NULL");
        return MPP_NOK;
    }

    if (enc_width != dec_width) {
        mpp_err_f("enc_width %d != dec_width %d", enc_width, dec_width);
        return MPP_NOK;
    }

    if (enc_height != dec_height) {
        mpp_err_f("enc_height %d != dec_height %d", enc_height, dec_height);
        return MPP_NOK;
    }

    return MPP_OK;
}

static void mpi_rc_calc_psnr(MpiRcStat *stat, MppFrame frame_in,
                             MppFrame frame_out)
{
    RK_U32 x, y;
    RK_S32 tmp;
    double ssd_y = 0.0;
    double mse_y = 0.0;
    double psnr_y = 0.0;

    RK_U32 width  = mpp_frame_get_width(frame_in);
    RK_U32 height  = mpp_frame_get_height(frame_in);
    RK_U32 luma_size = width * height;
    RK_U32 enc_hor_stride = mpp_frame_get_hor_stride(frame_in);
    RK_U32 dec_hor_stride = mpp_frame_get_hor_stride(frame_out);
    RK_U8 *enc_buf = (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_in));
    RK_U8 *dec_buf = (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_out));
    RK_U8 *enc_buf_y = enc_buf;
    RK_U8 *dec_buf_y = dec_buf;

    for (y = 0 ; y < height; y++) {
        for (x = 0; x < width; x++) {
            tmp = enc_buf_y[x + y * enc_hor_stride] -
                  dec_buf_y[x + y * dec_hor_stride];
            tmp *= tmp;
            ssd_y += tmp;
        }
    }
    // NOTE: should be 65025.0 rather than 65025,
    // because 65025*luma_size may exceed
    // 1 << 32.
    mse_y = ssd_y / (65025.0 * luma_size); // 65025=255*255
    if (mse_y <= 0.0000000001)
        psnr_y = 100;
    else
        psnr_y = -10.0 * log10f(mse_y);

    stat->psnr_y = psnr_y;
}

static float ssim_end1( int s1, int s2, int ss, int s12  )
{
    /*
     * Maximum value for 10-bit is: ss*64 = (2^10-1)^2*16*4*64 = 4286582784,
     * which will overflow in some cases.
     * s1*s1, s2*s2, and s1*s2 also obtain this value for edge cases:
     * ((2^10-1)*16*4)^2 = 4286582784.
     * Maximum value for 9-bit is: ss*64 = (2^9-1)^2*16*4*64 = 1069551616,
     * which will not overflow.
     */
    static const int ssim_c1 =
        (int)(.01 * .01 * MPI_PIXEL_MAX * MPI_PIXEL_MAX * 64 + .5);
    static const int ssim_c2 =
        (int)(.03 * .03 * MPI_PIXEL_MAX * MPI_PIXEL_MAX * 64 * 63 + .5);
    int fs1 = s1;
    int fs2 = s2;
    int fss = ss;
    int fs12 = s12;
    int vars = fss * 64 - fs1 * fs1 - fs2 * fs2;
    int covar = fs12 * 64 - fs1 * fs2;

    return (float)(2 * fs1 * fs2 + ssim_c1) *
           (float)(2 * covar + ssim_c2) /
           ((float)(fs1 * fs1 + fs2 * fs2 + ssim_c1) *
            (float)(vars + ssim_c2));
}

static float ssim_end4( int sum0[5][4], int sum1[5][4], int width  )
{
    double ssim = 0.0;
    int i  = 0;

    for (i = 0; i < width; i++  )
        ssim += ssim_end1(sum0[i][0] + sum0[i + 1][0] +
                          sum1[i][0] + sum1[i + 1][0],
                          sum0[i][1] + sum0[i + 1][1] +
                          sum1[i][1] + sum1[i + 1][1],
                          sum0[i][2] + sum0[i + 1][2] +
                          sum1[i][2] + sum1[i + 1][2],
                          sum0[i][3] + sum0[i + 1][3] +
                          sum1[i][3] + sum1[i + 1][3] );
    return ssim;
}

static void ssim_4x4x2_core( const pixel *pix1, intptr_t stride1,
                             const pixel *pix2, intptr_t stride2,
                             int sums[2][4]  )
{
    int a = 0;
    int b = 0;
    int x = 0;
    int y = 0;
    int z = 0;

    for (z = 0; z < 2; z++  ) {
        uint32_t s1 = 0, s2 = 0, ss = 0, s12 = 0;
        for (y = 0; y < 4; y++  )
            for ( x = 0; x < 4; x++  ) {
                a = pix1[x + y * stride1];
                b = pix2[x + y * stride2];
                s1  += a;
                s2  += b;
                ss  += a * a;
                ss  += b * b;
                s12 += a * b;

            }
        sums[z][0] = s1;
        sums[z][1] = s2;
        sums[z][2] = ss;
        sums[z][3] = s12;
        pix1 += 4;
        pix2 += 4;
    }
}

static float calc_ssim(pixel *pix1, RK_U32 stride1,
                       pixel *pix2, RK_U32 stride2,
                       int width, int height, void *buf, int *cnt)
{
    int x = 0;
    int y = 0;
    int z = 0;
    float ssim = 0.0;
    int (*sum0)[4] = buf;
    int (*sum1)[4] = sum0 + (width >> 2) + 3;

    width >>= 2;
    height >>= 2;
    for ( y = 1; y < height; y++  ) {
        for ( ; z <= y; z++  ) {
            MPP_SWAP( void*, sum0, sum1  );
            for ( x = 0; x < width; x += 2  )
                ssim_4x4x2_core(&pix1[4 * (x + z * stride1)],
                                stride1, &pix2[4 * (x + z * stride2)],
                                stride2, &sum0[x]  );
        }
        for ( x = 0; x < width - 1; x += 4  )
            ssim += ssim_end4(sum0 + x, sum1 + x,
                              MPP_MIN(4, width - x - 1));

    }
    *cnt = (height - 1) * (width - 1);
    return ssim;
}

static void mpi_rc_calc_ssim(MpiRc2TestCtx *ctx, MppFrame frame_in, MppFrame frame_out)
{
    int cnt = 0;
    float ssim = 0;
    MpiRcStat *stat = &ctx->stat;
    RK_U32 width  = mpp_frame_get_width(frame_in);
    RK_U32 height  = mpp_frame_get_height(frame_in);
    RK_U32 enc_hor_stride = mpp_frame_get_hor_stride(frame_in);
    RK_U32 dec_hor_stride = mpp_frame_get_hor_stride(frame_out);
    pixel *enc_buf =
        (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_in));
    pixel *dec_buf =
        (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_out));
    pixel *enc_buf_y = enc_buf;
    pixel *dec_buf_y = dec_buf;

    if (NULL == ctx->com_buf)
        ctx->com_buf = mpp_malloc(RK_U8, enc_hor_stride * mpp_frame_get_ver_stride(frame_out) * 2);

    ssim = calc_ssim(enc_buf_y, enc_hor_stride, dec_buf_y, dec_hor_stride,
                     width - 2, height - 2, ctx->com_buf, &cnt);
    ssim /= (float)cnt;

    stat->ssim_y = (double)ssim;
}

static MPP_RET mpi_rc_calc_stat(MpiRc2TestCtx *ctx,
                                MppFrame frame_in, MppFrame frame_out)
{
    MPP_RET ret = MPP_OK;
    MpiRcStat *stat = &ctx->stat;

    ret = mpi_rc_cmp_frame(frame_in, frame_out);
    if (ret) {
        mpp_err_f("mpi_rc_cmp_frame failed ret %d", ret);
        return MPP_NOK;
    }

    if (ctx->enc_cmd->psnr_en) {
        mpi_rc_calc_psnr(stat, frame_in, frame_out);
        ctx->total_psnrs += stat->psnr_y;
    }
    if (ctx->enc_cmd->ssim_en) {
        mpi_rc_calc_ssim(ctx, frame_in, frame_out);
        ctx->total_ssims += stat->ssim_y;
    }

    return ret;
}

static void mpi_rc_log_stat(MpiRc2TestCtx *ctx, RK_U32 frame_count,
                            RK_U32 one_second, RK_U32 seq_end)
{
    //static char rc_log_str[1024] = {0};
    MpiRcStat *stat = &ctx->stat;
    MpiRcFile *file  = &ctx->file;
    FILE *fp  = file->fp_stat;

    mpp_log("frame %3d | frame_size %6d bytes | psnr %5.2f | ssim %5.5f",
            frame_count, stat->frame_size, stat->psnr_y, stat->ssim_y);
    if (one_second)
        mpp_log("ins_bitrate %f kbps", stat->ins_bitrate * 8.0 / 1000);

    if (fp) {
        fprintf(fp, "%3d,%6d,%5.2f,%5.5f,",
                frame_count, stat->frame_size, stat->psnr_y, stat->ssim_y);
        if (one_second)
            fprintf(fp, "bitsrate: %d,", stat->ins_bitrate);
        if (!seq_end)
            fprintf(fp, "\n");
    }
}

static MPP_RET mpi_rc_enc_init(MpiRc2TestCtx *ctx)
{
    MpiEncTestArgs* enc_cmd = ctx->enc_cmd;
    MppApi *enc_mpi = NULL;
    MppCtx enc_ctx = NULL;
    MppPollType block = MPP_POLL_NON_BLOCK;
    MppEncRcCfg *rc_cfg = &ctx->rc_cfg;
    MppEncSeiMode sei_mode = MPP_ENC_SEI_MODE_ONE_SEQ;
    MppEncCfg cfg = ctx->cfg;
    RK_U32 debreath_en = 0;
    RK_U32 debreath_s = 0;
    MPP_RET ret = MPP_OK;

    // encoder init
    ret = mpp_create(&ctx->enc_ctx, &ctx->enc_mpi);
    if (ret) {
        mpp_err("mpp_create encoder failed\n");
        goto MPP_TEST_OUT;
    }

    enc_mpi = ctx->enc_mpi;
    enc_ctx = ctx->enc_ctx;

    rc_cfg->fps_in_denorm = 1;
    rc_cfg->fps_out_denorm = 1;
    rc_cfg->fps_in_num = 30;
    rc_cfg->fps_out_num = 30;
    rc_cfg->fps_in_flex = 0;
    rc_cfg->fps_out_flex = 0;
    rc_cfg->max_reenc_times = 1;
    rc_cfg->gop = enc_cmd->gop_len;

    block = MPP_POLL_BLOCK;
    ret = enc_mpi->control(enc_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        mpp_err("enc_mpi->control MPP_SET_INPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }
    block = MPP_POLL_BLOCK;
    ret = enc_mpi->control(enc_ctx, MPP_SET_OUTPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        mpp_err("enc_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(enc_ctx, MPP_CTX_ENC, enc_cmd->type);
    if (ret) {
        mpp_err("mpp_init enc failed\n");
        goto MPP_TEST_OUT;
    }

    if (enc_cmd->width)
        mpp_enc_cfg_set_s32(cfg, "prep:width", enc_cmd->width);
    if (enc_cmd->height)
        mpp_enc_cfg_set_s32(cfg, "prep:height", enc_cmd->height);
    if (enc_cmd->hor_stride)
        mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", enc_cmd->hor_stride);
    if (enc_cmd->ver_stride)
        mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", enc_cmd->ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_YUV420SP);

    mpp_enc_cfg_set_s32(cfg, "rc:mode",  enc_cmd->rc_mode);

    switch (enc_cmd->rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        /* do not set bps on fix qp mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_target", enc_cmd->bps_target);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", enc_cmd->bps_max ? enc_cmd->bps_max : enc_cmd->bps_target * 3 / 2);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", enc_cmd->bps_min ? enc_cmd->bps_max : enc_cmd->bps_target / 2);
    } break;
    case MPP_ENC_RC_MODE_VBR : {
        /* CBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_target", enc_cmd->bps_target);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", enc_cmd->bps_max ? enc_cmd->bps_max : enc_cmd->bps_target * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", enc_cmd->bps_min ? enc_cmd->bps_max : enc_cmd->bps_target * 1 / 16);
    } break;
    default : {
        mpp_err_f("unsupport encoder rc mode %d\n", enc_cmd->rc_mode);
    } break;
    }

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", rc_cfg->fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", rc_cfg->fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm",  rc_cfg->fps_in_denorm);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", rc_cfg->fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num",  rc_cfg->fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", rc_cfg->fps_out_denorm);
    mpp_enc_cfg_set_s32(cfg, "rc:gop", enc_cmd->gop_len ? enc_cmd->gop_len : 30 * 2);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    mpp_env_get_u32("dbrh_en", &debreath_en, 0);
    mpp_env_get_u32("dbrh_s",  &debreath_s, 16);

    mpp_enc_cfg_set_u32(cfg, "rc:debreath_en", debreath_en);
    mpp_enc_cfg_set_u32(cfg, "rc:debreath_strength", debreath_s);

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type",  enc_cmd->type);
    switch (enc_cmd->type) {
    case MPP_VIDEO_CodingAVC : {
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

        if (enc_cmd->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
            mpp_enc_cfg_set_s32(cfg, "h264:qp_init", 20);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max", 16);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min", 16);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max_i", 20);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min_i", 20);
        } else {
            mpp_enc_cfg_set_s32(cfg, "h264:qp_init", 26);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max", 51);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min", 10);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_max_i", 46);
            mpp_enc_cfg_set_s32(cfg, "h264:qp_min_i", 18);
        }
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    } break;
    case MPP_VIDEO_CodingVP8 : {
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_init", 40);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_max",  127);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_min",  0);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_max_i", 127);
        mpp_enc_cfg_set_s32(cfg, "vp8:qp_min_i", 0);
    } break;
    case MPP_VIDEO_CodingHEVC : {
        mpp_enc_cfg_set_s32(cfg, "h265:qp_init", enc_cmd->rc_mode == MPP_ENC_RC_MODE_FIXQP ? -1 : 26);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_max", 51);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_min", 10);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_max_i", 46);
        mpp_enc_cfg_set_s32(cfg, "h265:qp_min_i", 18);
    } break;
    default : {
        mpp_err_f("unsupport encoder coding type %d\n",  enc_cmd->type);
    } break;
    }

    ret = enc_mpi->control(enc_ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    /* optional */
    ret = enc_mpi->control(enc_ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }
MPP_TEST_OUT:
    return ret;
}

static MPP_RET mpi_rc_post_dec_init(MpiRc2TestCtx *ctx)
{
    MpiEncTestArgs *enc_cmd = ctx->enc_cmd;
    MppApi *dec_mpi = NULL;
    MppCtx dec_ctx = NULL;
    MppPollType block = MPP_POLL_NON_BLOCK;
    RK_U32 need_split = 0;
    MPP_RET ret = MPP_OK;

    // decoder init
    ret = mpp_create(&ctx->dec_ctx_post, &ctx->dec_mpi_post);
    if (ret) {
        mpp_err("mpp_create decoder failed\n");
        goto MPP_TEST_OUT;
    }

    dec_mpi = ctx->dec_mpi_post;
    dec_ctx = ctx->dec_ctx_post;

    ret = mpp_packet_init(&ctx->dec_pkt_post,
                          ctx->dec_in_buf_post, ctx->dec_in_buf_post_size);
    if (ret) {
        mpp_err("mpp_packet_init failed\n");
        goto MPP_TEST_OUT;
    }

    ret = dec_mpi->control(dec_ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, &need_split);
    if (ret) {
        mpp_err("dec_mpi->control failed\n");
        goto MPP_TEST_OUT;
    }
    ret = dec_mpi->control(dec_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        mpp_err("dec_mpi->control dec MPP_SET_INPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    block = MPP_POLL_NON_BLOCK;
    ret = dec_mpi->control(dec_ctx, MPP_SET_OUTPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        mpp_err("dec_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(dec_ctx, MPP_CTX_DEC, enc_cmd->type);
    if (ret) {
        mpp_err("mpp_init dec failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:

    return ret;
}

static MPP_RET mpi_rc_dec_post_decode(MpiRc2TestCtx *ctx, MppFrame orig_frm)
{
    MPP_RET ret = MPP_OK;
    RK_S32 dec_pkt_done = 0;
    MppFrame out_frm = NULL;

    do {
        if (ctx->pkt_eos)
            mpp_packet_set_eos(ctx->dec_pkt_post);

        // send the packet first if packet is not done
        if (!dec_pkt_done) {
            ret = ctx->dec_mpi_post->decode_put_packet(ctx->dec_ctx_post,
                                                       ctx->dec_pkt_post);
            if (MPP_OK == ret)
                dec_pkt_done = 1;
        }

        // then get all available frame and release
        do {
            RK_S32 dec_get_frm = 0;
            RK_U32 dec_frm_eos = 0;

            ret = ctx->dec_mpi_post->decode_get_frame(ctx->dec_ctx_post,
                                                      &out_frm);
            if (ret) {
                mpp_err("decode_get_frame failed ret %d\n", ret);
                break;
            }

            if (out_frm) {
                if (mpp_frame_get_info_change(out_frm)) {
                    mpp_log("decode_get_frame get info changed found\n");
                    ctx->dec_mpi_post->control(ctx->dec_ctx_post,
                                               MPP_DEC_SET_INFO_CHANGE_READY,
                                               NULL);
                } else {
                    mpi_rc_calc_stat(ctx, orig_frm, out_frm);
                    mpi_rc_log_stat(ctx, ctx->frm_idx,
                                    !((ctx->frm_idx - ctx->calc_base_idx + 1) %
                                      ctx->rc_cfg.fps_in_num),
                                    0);
                    dec_get_frm = 1;
                }
                dec_frm_eos = mpp_frame_get_eos(out_frm);
                mpp_frame_deinit(&out_frm);
                out_frm = NULL;
            }

            // if last packet is send but last frame is not found continue
            if (ctx->pkt_eos && dec_pkt_done && !dec_frm_eos)
                continue;

            if (dec_get_frm)
                break;
        } while (1);

        if (dec_pkt_done)
            break;

        msleep(5);
    } while (1);

    return ret;
}

static MPP_RET mpi_rc_pre_dec_init(MpiRc2TestCtx *ctx)
{
    RK_U32 need_split = 1;
    MppPollType block = MPP_POLL_NON_BLOCK;
    RK_U32 fast_en = 0;
    MppApi *dec_mpi = NULL;
    MppCtx dec_ctx = NULL;
    MppFrameFormat format = MPP_FMT_YUV420SP;
    RK_U32 fbc_en = 0;
    MPP_RET ret = MPP_OK;

    mpp_env_get_u32("fbc_dec_en", &fbc_en, 0);
    mpp_env_get_u32("fast_en", &fast_en, 0);

    if (fbc_en)
        format = format | MPP_FRAME_FBC_AFBC_V2;

    // decoder init
    ret = mpp_create(&ctx->dec_ctx_pre, &ctx->dec_mpi_pre);
    if (ret) {
        mpp_err("mpp_create decoder failed\n");
        goto MPP_TEST_OUT;
    }

    dec_mpi = ctx->dec_mpi_pre;
    dec_ctx = ctx->dec_ctx_pre;

    ret = mpp_packet_init(&ctx->dec_pkt_pre, ctx->dec_in_buf_pre, ctx->dec_in_buf_pre_size);
    if (ret) {
        mpp_err("mpp_packet_init failed\n");
        goto MPP_TEST_OUT;
    }

    ret = dec_mpi->control(dec_ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, &need_split);
    if (ret) {
        mpp_err("dec_mpi->control failed\n");
        goto MPP_TEST_OUT;
    }

    ret = dec_mpi->control(dec_ctx, MPP_DEC_SET_PARSER_FAST_MODE, &fast_en);
    if (ret) {
        mpp_err("dec_mpi->control failed\n");
        goto MPP_TEST_OUT;
    }

    ret = dec_mpi->control(dec_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        mpp_err("dec_mpi->control dec MPP_SET_INPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    block = MPP_POLL_NON_BLOCK;
    ret = dec_mpi->control(dec_ctx, MPP_SET_OUTPUT_TIMEOUT, (MppParam)&block);
    if (ret) {
        mpp_err("dec_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(dec_ctx, MPP_CTX_DEC, ctx->dec_type);
    if (ret) {
        mpp_err("mpp_init dec failed\n");
        goto MPP_TEST_OUT;
    }

    ret = dec_mpi->control(dec_ctx, MPP_DEC_SET_OUTPUT_FORMAT, &format);
    if (ret) {
        mpp_err("dec_mpi->control failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    return ret;
}

static MPP_RET mpi_rc_info_change(MpiRc2TestCtx *ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;

    mpp_enc_cfg_set_s32(ctx->cfg, "prep:width", mpp_frame_get_width(frm));
    mpp_enc_cfg_set_s32(ctx->cfg, "prep:height", mpp_frame_get_height(frm));
    mpp_enc_cfg_set_s32(ctx->cfg, "prep:hor_stride",  mpp_frame_get_hor_stride(frm));
    mpp_enc_cfg_set_s32(ctx->cfg, "prep:ver_stride", mpp_frame_get_ver_stride(frm));
    mpp_enc_cfg_set_s32(ctx->cfg, "prep:format", mpp_frame_get_fmt(frm));

    ret = ctx->enc_mpi->control(ctx->enc_ctx, MPP_ENC_SET_CFG, ctx->cfg);

    ctx->dec_mpi_post->control(ctx->dec_ctx_post, MPP_DEC_SET_FRAME_INFO, (MppParam)frm);

    return ret;
}

static MPP_RET mpi_rc_enc(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi = ctx->dec_mpi_pre;
    MppCtx dec_ctx = ctx->dec_ctx_pre;
    MppFrame frm = NULL;

    do {
        ret = mpi->decode_get_frame(dec_ctx, &frm);
        if (ret) {
            mpp_err("decode_get_frame failed ret %d\n", ret);
            break;
        }

        if (frm) {
            ctx->frm_eos = mpp_frame_get_eos(frm);
            if (mpp_frame_get_info_change(frm)) {
                mpp_log("decode_get_frame get info changed found\n");
                mpi->control(dec_ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                mpi_rc_info_change(ctx, frm);
                ctx->start_enc = mpp_time();
            } else {
                void *ptr;
                size_t len;
                /*force eos*/
                if ((ctx->enc_cmd->frame_num > 0) && (ctx->frm_idx > ctx->enc_cmd->frame_num)) {
                    ctx->loop_end = 1;
                    mpp_frame_set_eos(frm, 1);
                    ctx->frm_eos = mpp_frame_get_eos(frm);
                }
#ifndef ASYN_ENC
                ctx->enc_mpi->encode_put_frame(ctx->enc_ctx, frm);
                ctx->enc_mpi->encode_get_packet(ctx->enc_ctx, &ctx->enc_pkt);
#else
            LAST_PACKET:
                ctx->enc_mpi->encode(ctx->enc_ctx, frm, &ctx->enc_pkt);
                frm = NULL; //ASYN_ENC will free after get packet
#endif
                if (ctx->enc_pkt) {
                    len = mpp_packet_get_length(ctx->enc_pkt);
                    ctx->stat.frame_size = len;
                    ctx->stream_size_1s += len;
                    ctx->total_bits += len * 8;
                    if ((ctx->frm_idx - ctx->calc_base_idx + 1) %
                        ctx->rc_cfg.fps_in_num == 0) {
                        ctx->stat.ins_bitrate = ctx->stream_size_1s;
                        ctx->stream_size_1s = 0;
                    }

                    ptr = mpp_packet_get_pos(ctx->enc_pkt);
                    if (ctx->file.fp_enc_out)
                        fwrite(ptr, 1, len, ctx->file.fp_enc_out);

                    /* decode one frame */
                    // write packet to dec input
                    mpp_packet_write(ctx->dec_pkt_post, 0, ptr, len);
                    // reset pos
                    mpp_packet_set_pos(ctx->dec_pkt_post, ctx->dec_in_buf_post);
                    mpp_packet_set_length(ctx->dec_pkt_post, len);
                    mpp_packet_set_size(ctx->dec_pkt_post, len);

                    if (mpp_packet_has_meta(ctx->enc_pkt)) {
                        MppFrame frame = NULL;
                        MppMeta meta = mpp_packet_get_meta(ctx->enc_pkt);
                        mpp_meta_get_frame(meta,  KEY_INPUT_FRAME, &frame);
                        if (ctx->enc_cmd->ssim_en || ctx->enc_cmd->psnr_en) {
                            mpp_packet_write(ctx->dec_pkt_post, 0, ptr, len);
                            mpi_rc_dec_post_decode(ctx, frame);
                        }
                        if (frame) {
                            frm = frame;  //ASYN_ENC delay free
                        }
                    } else {
                        if (ctx->enc_cmd->ssim_en || ctx->enc_cmd->psnr_en) {
                            mpp_packet_write(ctx->dec_pkt_post, 0, ptr, len);
                            mpi_rc_dec_post_decode(ctx, frm);
                        }
                    }
                    ctx->enc_pkt_eos = mpp_packet_get_eos(ctx->enc_pkt);
                    ctx->frm_idx++;
                    mpp_packet_deinit(&ctx->enc_pkt);
                }

            }
            mpp_frame_deinit(&frm);
            frm = NULL;
        } else {
            msleep(3);
            continue;
        }
#ifdef ANSY_ENC
        if (ctx->frm_eos && !ctx->enc_pkt_eos) {
            goto LAST_PACKET;
        }
#endif
        if (ctx->enc_pkt_eos) {
            break;
        }
    } while (1);
    return ret;
}

static MPP_RET mpi_rc_buffer_init(MpiRc2TestCtx *ctx)
{
    /* NOTE: packet buffer may overflow */
    size_t packet_size  = SZ_256K;
    MPP_RET ret = MPP_OK;

    ret = mpp_buffer_group_get_internal(&ctx->pkt_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("failed to get buffer group for output packet ret %d\n", ret);
        goto RET;
    }

    ret = mpp_buffer_get(ctx->pkt_grp, &ctx->enc_pkt_buf, packet_size);
    if (ret) {
        mpp_err("failed to get buffer for input frame ret %d\n", ret);
        goto RET;
    }

    ctx->dec_in_buf_post = mpp_calloc(RK_U8, packet_size);
    if (NULL == ctx->dec_in_buf_post) {
        mpp_err("mpi_dec_test malloc input stream buffer failed\n");
        goto RET;
    }

    ctx->dec_in_buf_post_size = packet_size;

    ctx->frm_idx = 0;
    ctx->calc_base_idx = 0;
    ctx->stream_size_1s = 0;

    return ret;
RET:
    ctx->dec_ctx_pre = NULL;
    ctx->dec_ctx_post = NULL;
    MPP_FREE(ctx->dec_in_buf_post);

    if (ctx->enc_pkt_buf) {
        mpp_buffer_put(ctx->enc_pkt_buf);
        ctx->enc_pkt_buf = NULL;
    }

    if (ctx->pkt_grp) {
        mpp_buffer_group_put(ctx->pkt_grp);
        ctx->pkt_grp = NULL;
    }

    return ret;
}

#define CHECK_RET(x)    do { \
                            if (x < 0) { \
                                mpp_err_f("%d failed\n"); \
                                goto MPP_TEST_OUT; \
                            } \
                        } while (0)

static void *rc2_pre_dec_thread(void *param)
{
    MpiRc2TestCtx *ctx = (MpiRc2TestCtx *)param;
    FileReader reader = ctx->reader;
    FileBufSlot *slot = NULL;
    MppPacket packet = ctx->dec_pkt_pre;
    MppApi *mpi = ctx->dec_mpi_pre;
    MppCtx dec_ctx = ctx->dec_ctx_pre;
    MPP_RET ret = MPP_OK;
    RK_S32 dec_pkt_done = 0;

    while (!ctx->loop_end) {
        RK_U32 pkt_eos  = 0;
        dec_pkt_done = 0;
        ret = reader_index_read(reader, ctx->pre_pkt_idx++, &slot);
        pkt_eos = slot->eos;
        if (pkt_eos) {
            if (ctx->enc_cmd->frame_num < 0) {
                ctx->pre_pkt_idx = 0;
                pkt_eos = 0;
            } else if (!ctx->enc_cmd->frame_num) {
                ctx->loop_end = 1;
            } else {
                ctx->pre_pkt_idx = 0;
                pkt_eos = 0;
            }
        }

        if ((ctx->enc_cmd->frame_num > 0) && (ctx->frm_idx > ctx->enc_cmd->frame_num)) {
            mpp_log("frm_idx %d, frame_num %d",
                    ctx->frm_idx, ctx->enc_cmd->frame_num);
            ctx->loop_end = 1;
        }
        mpp_packet_set_data(packet, slot->data);
        mpp_packet_set_size(packet, slot->size);
        mpp_packet_set_pos(packet, slot->data);
        mpp_packet_set_length(packet, slot->size);

        if (ctx->loop_end) {
            mpp_packet_set_eos(packet);
            ctx->pkt_eos = 1;
        }

        do {
            ret = mpi->decode_put_packet(dec_ctx, packet);
            if (MPP_OK == ret)
                dec_pkt_done = 1;
            else
                msleep(5);
            if (ctx->loop_end) {
                break;
            }
        } while (!dec_pkt_done);

        if (ctx->pkt_eos) {
            mpp_log("dec stream finish\n");
            break;
        }
    }
    mpp_log("rc2_pre_dec_thread exit");
    return NULL;
}

static MPP_RET mpi_rc_codec(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    RK_S64 t_e;

    CHECK_RET(mpi_rc_buffer_init(ctx));
    CHECK_RET(mpi_rc_post_dec_init(ctx));
    CHECK_RET(mpi_rc_pre_dec_init(ctx));
    CHECK_RET(mpp_enc_cfg_init(&ctx->cfg));
    CHECK_RET(mpi_rc_enc_init(ctx));

    pthread_create(&ctx->dec_thr, NULL, rc2_pre_dec_thread, ctx);

    while (1) {
        mpi_rc_enc(ctx);
        if (ctx->enc_pkt_eos) {
            mpp_log("stream finish\n");
            break;
        }
    }

    t_e = mpp_time();
    RK_U64 elapsed_time = t_e - ctx->start_enc;
    float  frame_rate = (float)ctx->frm_idx * 1000000 / elapsed_time;
    mpp_log("enc_dec %d frame use time %lld ms frm rate %.2f\n",
            ctx->frm_idx, elapsed_time / 1000, frame_rate);

    if (ctx->frm_idx) {
        MpiEncTestArgs* enc_cmd = ctx->enc_cmd;

        mpp_log("%s: %s: average: bps %d | psnr %5.2f | ssim %5.5f",
                enc_cmd->file_input, enc_cmd->gop_mode ? "smart_p" : "normal_p",
                30 * (RK_U32)(ctx->total_bits / ctx->frm_idx),
                ctx->total_psnrs / ctx->frm_idx, ctx->total_ssims / ctx->frm_idx);
        if (ctx->file.fp_stat)
            fprintf(ctx->file.fp_stat, "%s: %s: average: bps %dk | psnr %5.2f | ssim %5.5f \n",
                    enc_cmd->file_input, enc_cmd->gop_mode ? "smart_p" : "normal_p",
                    30 * (RK_U32)(ctx->total_bits / ctx->frm_idx) / 1000,
                    ctx->total_psnrs / ctx->frm_idx, ctx->total_ssims / ctx->frm_idx);
    }

    CHECK_RET(ctx->enc_mpi->reset(ctx->enc_ctx));
    CHECK_RET(ctx->dec_mpi_pre->reset(ctx->dec_ctx_pre));
    CHECK_RET(ctx->dec_mpi_post->reset(ctx->dec_ctx_post));

MPP_TEST_OUT:
    // encoder deinit

    pthread_join(ctx->dec_thr, NULL);

    if (ctx->enc_ctx) {
        mpp_destroy(ctx->enc_ctx);
        ctx->enc_ctx = NULL;
    }

    if (ctx->enc_pkt_buf) {
        mpp_buffer_put(ctx->enc_pkt_buf);
        ctx->enc_pkt_buf = NULL;
    }

    if (ctx->pkt_grp) {
        mpp_buffer_group_put(ctx->pkt_grp);
        ctx->pkt_grp = NULL;
    }

    // decoder deinit
    if (ctx->dec_pkt_post) {
        mpp_packet_deinit(&ctx->dec_pkt_post);
        ctx->dec_pkt_post = NULL;
    }

    if (ctx->dec_pkt_pre) {
        mpp_packet_deinit(&ctx->dec_pkt_pre);
        ctx->dec_pkt_pre = NULL;
    }

    if (ctx->cfg) {
        mpp_enc_cfg_deinit(ctx->cfg);
        ctx->cfg = NULL;
    }

    if (ctx->dec_ctx_post) {
        mpp_destroy(ctx->dec_ctx_post);
        ctx->dec_ctx_post = NULL;
    }

    if (ctx->dec_ctx_pre) {
        mpp_destroy(ctx->dec_ctx_pre);
        ctx->dec_ctx_pre = NULL;
    }

    MPP_FREE(ctx->dec_in_buf_post);
    MPP_FREE(ctx->dec_in_buf_pre);

    return ret;
}

int main(int argc, char **argv)
{
    MpiEncTestArgs* enc_cmd = mpi_enc_test_cmd_get();
    MpiRc2TestCtx *ctx = NULL;
    MPP_RET ret = MPP_OK;

    ret = mpi_enc_test_cmd_update_by_args(enc_cmd, argc, argv);
    if (ret)
        goto DONE;

    mpi_enc_test_cmd_show_opt(enc_cmd);

    ctx = mpp_calloc(MpiRc2TestCtx, 1);
    if (NULL == ctx) {
        ret = MPP_ERR_MALLOC;
        goto DONE;
    }

    ctx->enc_cmd = enc_cmd;

    ret = mpi_rc_init(ctx);
    if (ret) {
        mpp_err("mpi_rc_init failded ret %d", ret);
        goto DONE;
    }

    ret = mpi_rc_codec(ctx);
    if (ret)
        mpp_err("mpi_rc_codec failded ret %d", ret);

DONE:
    if (ctx) {
        mpi_rc_deinit(ctx);
        ctx = NULL;
    }

    mpi_enc_test_cmd_put(enc_cmd);

    return (int)ret;
}
