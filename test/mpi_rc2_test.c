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

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_rc_test"

#include <string.h>
#include <math.h>
#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "utils.h"

#include "vpu_api.h"

#include "mpp_parse_cfg.h"
#include "mpp_event_trigger.h"

#define MPI_RC_FILE_NAME_LEN        256
#define MPI_DEC_STREAM_SIZE         (SZ_1K)

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
    char            file_input[MPI_RC_FILE_NAME_LEN];
    char            file_enc_out[MPI_RC_FILE_NAME_LEN];
    char            file_stat[MPI_RC_FILE_NAME_LEN];
    char            file_config[MPI_RC_FILE_NAME_LEN];
    MppCodingType   type;
    RK_U32          debug;
    RK_U32          num_frames;
    RK_U32          psnr_en;
    RK_U32          ssim_en;

    RK_U32          have_input;
    RK_U32          have_enc_out;
    RK_U32          have_stat_out;
    RK_U32          have_config_file;

    struct rc_test_config cfg;
    struct event_ctx *ectx;
    struct event_packet e;
} MpiRcTestCmd;

typedef struct {
    MpiRcTestCmd    cmd;
    MpiRcStat       stat;
    MpiRcFile       file;

    RK_U8           *com_buf;
    MppCtx          enc_ctx;
    MppCtx          dec_ctx_post;
    MppCtx          dec_ctx_pre;
    MppApi          *enc_mpi;
    MppApi          *dec_mpi_post;
    MppApi          *dec_mpi_pre;

    MppEncRcCfg     rc_cfg;
    MppEncPrepCfg   prep_cfg;
    MppEncCodecCfg  codec_cfg;

    MppBufferGroup  pkt_grp;
    MppBuffer       enc_pkt_buf;

    MppPacket       enc_pkt;
    MppPacket       dec_pkt_post;
    MppPacket       dec_pkt_pre;

    MppTask         enc_in_task;
    MppTask         enc_out_task;

    RK_U8           *dec_in_buf_post;
    RK_U8           *dec_in_buf_pre;
    RK_U32          dec_in_buf_pre_size;

    RK_U32          pkt_eos;
    RK_S32          frm_eos;

    RK_U32          frm_idx;
    RK_U32          calc_base_idx;
    RK_U32          stream_size_1s;
} MpiRc2TestCtx;

static OptionInfo mpi_rc_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"t",               "type",                 "output stream coding type"},
    {"n",               "max frame number",     "max encoding frame number"},
    {"d",               "debug",                "debug flag"},
    {"s",               "stat_file",            "stat output file name"},
    {"c",               "rc test item",         "rc test item flags, one bit each item: roi|force_intra|gop|fps|bps" },
    {"g",               "config file",          "read config from a file"},
    {"p",               "enable psnr",          "enable psnr calculate"},
    {"m",               "enable ssim",          "enable ssim calculate"},
};

/* a callback to notify test that a event occur */
static void event_occur(void *parent, void *event)
{
    struct rc_event *e = (struct rc_event *)event;
    int ret;
    MpiRc2TestCtx *ctx = (MpiRc2TestCtx *)parent;
    MppCtx *enc_ctx = &ctx->enc_ctx;
    MppApi *enc_mpi = ctx->enc_mpi;
    MppEncRcCfg rc_cfg;

    mpp_log("event %d occur, set fps %f, bps %d\n", e->idx, e->fps, e->bps);

    ret = enc_mpi->control(*enc_ctx, MPP_ENC_GET_RC_CFG, &rc_cfg);
    if (ret < 0) {
        mpp_err("get encoder rc config failed\n");
        return;
    }

    rc_cfg.bps_target = e->bps;
    rc_cfg.bps_max = e->bps * 2;
    rc_cfg.bps_min = e->bps / 2;
    rc_cfg.fps_in_num = rc_cfg.fps_out_num = e->fps;
    rc_cfg.fps_in_flex = rc_cfg.fps_out_flex = 1;
    rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_BPS;

    ret = enc_mpi->control(*enc_ctx, MPP_ENC_SET_RC_CFG, &rc_cfg);
    if (ret) {
        mpp_err("set encoder rc config failed\n");
    }

    ctx->calc_base_idx = ctx->frm_idx;
}

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

    MPP_FREE(ctx->com_buf);

    if (ctx->cmd.ectx)
        event_ctx_release(ctx->cmd.ectx);
}

static MPP_RET mpi_rc_init(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    MpiRcTestCmd *cmd = &ctx->cmd;
    MpiRcFile *file = &ctx->file;

    if (cmd->have_input) {
        file->fp_input = fopen(cmd->file_input, "rb");
        if (NULL == file->fp_input) {
            mpp_err("failed to open input file %s\n", cmd->file_input);
            mpp_err("create default yuv image for test\n");
        }
    }

    if (cmd->have_enc_out) {
        file->fp_enc_out = fopen(cmd->file_enc_out, "w+b");
        if (NULL == file->fp_enc_out) {
            mpp_err("failed to open enc output file %s\n", cmd->file_enc_out);
            ret = MPP_ERR_OPEN_FILE;
            goto err;
        }
    }

    if (cmd->have_stat_out) {
        file->fp_stat = fopen(cmd->file_stat, "w+b");
        if (NULL == file->fp_stat) {
            mpp_err("failed to open stat file %s\n", cmd->file_stat);
            ret = MPP_ERR_OPEN_FILE;
            goto err;
        }
        fprintf(file->fp_stat,
                "frame,size(byte),psnr,ssim,ins_bitrate(byte/s),"
                "avg_bitrate(byte/s)\n");
    }

    ctx->com_buf = mpp_malloc(RK_U8, 1920 * 1080 * 2);
    if (ctx->com_buf == NULL) {
        mpp_err_f("ctx->com_buf malloc failed");
        ret = MPP_NOK;
        goto err;
    }

    if (cmd->have_config_file) {
        if (0 > mpp_parse_config(cmd->file_config, &cmd->cfg)) {
            mpp_err("parse config failed\n");
            cmd->have_config_file = 0;
        } else {
            int i;

            cmd->e.cnt = cmd->cfg.event_cnt;
            cmd->e.loop = cmd->cfg.loop;
            for (i = 0; i < cmd->cfg.event_cnt; ++i) {
                cmd->e.e[i].idx = cmd->cfg.event[i].idx;
                cmd->e.e[i].event = &cmd->cfg.event[i];
            }
        }
    }

err:

    return ret;
}

static MPP_RET mpi_rc_cmp_frame(MppFrame frame_in, MppFrame frame_out)
{
    RK_U8 *enc_buf =
        (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_in));
    RK_U8 *dec_buf =
        (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_out));
    RK_U32 enc_width  = mpp_frame_get_width(frame_in);
    RK_U32 enc_height  = mpp_frame_get_height(frame_in);
    RK_U32 dec_width  = mpp_frame_get_width(frame_out);
    RK_U32 dec_height  = mpp_frame_get_height(frame_out);

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
    RK_U8 *enc_buf =
        (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_in));
    RK_U8 *dec_buf =
        (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame_out));
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
    if (MPP_OK != ret) {
        mpp_err_f("mpi_rc_cmp_frame failed ret %d", ret);
        return MPP_NOK;
    }

    if (ctx->cmd.psnr_en)
        mpi_rc_calc_psnr(stat, frame_in, frame_out);
    if (ctx->cmd.ssim_en)
        mpi_rc_calc_ssim(ctx, frame_in, frame_out);

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
    MPP_RET ret = MPP_OK;

    MppCtx *enc_ctx = &ctx->enc_ctx;
    MppApi *enc_mpi;
    MpiRcTestCmd *cmd = &ctx->cmd;
    MppPollType block = MPP_POLL_NON_BLOCK;

    MppCodingType type = cmd->type;

    MppEncRcCfg *rc_cfg = &ctx->rc_cfg;
    MppEncPrepCfg *prep_cfg = &ctx->prep_cfg;
    MppEncCodecCfg *codec_cfg = &ctx->codec_cfg;

    MppEncSeiMode sei_mode = MPP_ENC_SEI_MODE_ONE_SEQ;

    // encoder init
    ret = mpp_create(enc_ctx, &ctx->enc_mpi);
    if (MPP_OK != ret) {
        mpp_err("mpp_create encoder failed\n");
        goto MPP_TEST_OUT;
    }

    enc_mpi = ctx->enc_mpi;

    ret = enc_mpi->control(*enc_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
    if (MPP_OK != ret) {
        mpp_err("enc_mpi->control MPP_SET_INPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }
    ret = enc_mpi->control(*enc_ctx, MPP_SET_OUTPUT_TIMEOUT, (MppParam)&block);
    if (MPP_OK != ret) {
        mpp_err("enc_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(*enc_ctx, MPP_CTX_ENC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init enc failed\n");
        goto MPP_TEST_OUT;
    }

    rc_cfg->change = MPP_ENC_RC_CFG_CHANGE_ALL;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_VBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;
    rc_cfg->bps_target = 2000000;
    rc_cfg->bps_max = rc_cfg->bps_target * 3 / 2;
    rc_cfg->bps_min = rc_cfg->bps_target / 2;
    rc_cfg->fps_in_denorm = 1;
    rc_cfg->fps_out_denorm = 1;
    rc_cfg->fps_in_num = 30;
    rc_cfg->fps_out_num = 30;
    rc_cfg->fps_in_flex = 0;
    rc_cfg->fps_out_flex = 0;
    rc_cfg->gop = 60;
    rc_cfg->skip_cnt = 0;

    ret = enc_mpi->control(*enc_ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
    if (ret) {
        mpp_err("mpi control enc set rc cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    codec_cfg->coding = type;
    codec_cfg->h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                             MPP_ENC_H264_CFG_CHANGE_ENTROPY;
    /*
     * H.264 profile_idc parameter
     * 66  - Baseline profile
     * 77  - Main profile
     * 100 - High profile
     */
    codec_cfg->h264.profile  = 100;
    /*
     * H.264 level_idc parameter
     * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
     * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
     * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
     * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
     * 50 / 51 / 52         - 4K@30fps
     */
    codec_cfg->h264.level    = 40;
    codec_cfg->h264.entropy_coding_mode  = 1;
    codec_cfg->h264.cabac_init_idc  = 0;

    if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_VBR &&
        rc_cfg->quality == MPP_ENC_RC_QUALITY_CQP) {
        /* constant QP mode qp is fixed */
        codec_cfg->h264.qp_max   = 26;
        codec_cfg->h264.qp_min   = 26;
        codec_cfg->h264.qp_max_step  = 0;
        codec_cfg->h264.qp_init      = 26;
        codec_cfg->h264.change |= MPP_ENC_H264_CFG_CHANGE_QP_LIMIT;
    }

    ret = enc_mpi->control(*enc_ctx, MPP_ENC_SET_CODEC_CFG, codec_cfg);
    if (ret) {
        mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    /* optional */
    ret = enc_mpi->control(*enc_ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    prep_cfg->change        = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                              MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg->width         = 1920;
    prep_cfg->height        = 1080;
    prep_cfg->hor_stride    = 1920;
    prep_cfg->ver_stride    = 1088;
    prep_cfg->format        = MPP_FMT_YUV420SP;

    ret = enc_mpi->control(*enc_ctx, MPP_ENC_SET_PREP_CFG, prep_cfg);
    if (ret) {
        mpp_err("mpi control enc set prep cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    return ret;
}

static MPP_RET mpi_rc_enc_encode(MpiRc2TestCtx *ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;
    MppBuffer pkt_buf_out = ctx->enc_pkt_buf;
    MppApi *enc_mpi = ctx->enc_mpi;

    mpp_packet_init_with_buffer(&ctx->enc_pkt, pkt_buf_out);

    ret = enc_mpi->poll(ctx->enc_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
    if (ret) {
        mpp_err("mpp input poll failed\n");
        goto MPP_TEST_OUT;
    }

    ret = enc_mpi->dequeue(ctx->enc_ctx, MPP_PORT_INPUT, &ctx->enc_in_task);
    if (ret) {
        mpp_err("mpp task input dequeue failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_task_meta_set_frame (ctx->enc_in_task, KEY_INPUT_FRAME, frm);
    mpp_task_meta_set_packet(ctx->enc_in_task, KEY_OUTPUT_PACKET, ctx->enc_pkt);

    ret = enc_mpi->enqueue(ctx->enc_ctx, MPP_PORT_INPUT, ctx->enc_in_task);
    if (ret) {
        mpp_err("mpp task input enqueue failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    return ret;
}

static MPP_RET mpi_rc_enc_result(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;

    ret = ctx->enc_mpi->poll(ctx->enc_ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
    if (ret) {
        mpp_err("mpp task output poll failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = ctx->enc_mpi->dequeue(ctx->enc_ctx, MPP_PORT_OUTPUT, &ctx->enc_out_task);
    if (ret) {
        mpp_err("mpp task output dequeue failed\n");
        goto MPP_TEST_OUT;
    }

    if (!ctx->enc_out_task) {
        mpp_err("no enc_out_task available\n");
        ret = MPP_NOK;
        goto MPP_TEST_OUT;
    }

    MppFrame packet_out = NULL;

    mpp_task_meta_get_packet(ctx->enc_out_task, KEY_OUTPUT_PACKET, &packet_out);

    mpp_assert(packet_out == ctx->enc_pkt);
    if (!ctx->enc_pkt) {
        ret = MPP_NOK;
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    return ret;
}

static MPP_RET mpi_rc_post_dec_init(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    MpiRcTestCmd *cmd = &ctx->cmd;
    MppCodingType type = cmd->type;
    RK_U32 need_split = 0;
    MppPollType block = MPP_POLL_NON_BLOCK;

    // decoder init
    ret = mpp_create(&ctx->dec_ctx_post, &ctx->dec_mpi_post);
    if (MPP_OK != ret) {
        mpp_err("mpp_create decoder failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_packet_init(&ctx->dec_pkt_post,
                          ctx->dec_in_buf_post, 1920 * 1080);
    if (ret) {
        mpp_err("mpp_packet_init failed\n");
        goto MPP_TEST_OUT;
    }

    ret = ctx->dec_mpi_post->control(ctx->dec_ctx_post,
                                     MPP_DEC_SET_PARSER_SPLIT_MODE,
                                     &need_split);
    if (MPP_OK != ret) {
        mpp_err("dec_mpi->control failed\n");
        goto MPP_TEST_OUT;
    }
    ret = ctx->dec_mpi_post->control(ctx->dec_ctx_post, MPP_SET_INPUT_TIMEOUT,
                                     (MppParam)&block);
    if (MPP_OK != ret) {
        mpp_err("dec_mpi->control dec MPP_SET_INPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    block = MPP_POLL_BLOCK;
    ret = ctx->dec_mpi_post->control(ctx->dec_ctx_post, MPP_SET_OUTPUT_TIMEOUT,
                                     (MppParam)&block);
    if (MPP_OK != ret) {
        mpp_err("dec_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(ctx->dec_ctx_post, MPP_CTX_DEC, type);
    if (MPP_OK != ret) {
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
            if (MPP_OK != ret) {
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
    MPP_RET ret = MPP_OK;
    MpiRcTestCmd *cmd   = &ctx->cmd;
    RK_U32 need_split   = 1;
    MppPollType block = MPP_POLL_NON_BLOCK;
    MppApi *mpi = NULL;

    // decoder init
    ret = mpp_create(&ctx->dec_ctx_pre, &ctx->dec_mpi_pre);
    if (MPP_OK != ret) {
        mpp_err("mpp_create decoder failed\n");
        goto MPP_TEST_OUT;
    }

    mpi = ctx->dec_mpi_pre;

    ret = mpp_packet_init(&ctx->dec_pkt_pre, ctx->dec_in_buf_pre, ctx->dec_in_buf_pre_size);
    if (ret) {
        mpp_err("mpp_packet_init failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx->dec_ctx_pre, MPP_DEC_SET_PARSER_SPLIT_MODE,
                       &need_split);
    if (MPP_OK != ret) {
        mpp_err("dec_mpi->control failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx->dec_ctx_pre, MPP_SET_INPUT_TIMEOUT,
                       (MppParam)&block);
    if (MPP_OK != ret) {
        mpp_err("dec_mpi->control dec MPP_SET_INPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    block = MPP_POLL_BLOCK;
    ret = mpi->control(ctx->dec_ctx_pre, MPP_SET_OUTPUT_TIMEOUT,
                       (MppParam)&block);
    if (MPP_OK != ret) {
        mpp_err("dec_mpi->control MPP_SET_OUTPUT_TIMEOUT failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(ctx->dec_ctx_pre, MPP_CTX_DEC, cmd->type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init dec failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    return ret;
}

static MPP_RET mpi_rc_info_change(MpiRc2TestCtx *ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;
    RK_U32 width;
    RK_U32 height;
    MppFrame  frame_tmp = NULL;

    ctx->prep_cfg.width = width = mpp_frame_get_width(frm);
    ctx->prep_cfg.height = height = mpp_frame_get_height(frm);
    ctx->prep_cfg.format = mpp_frame_get_fmt(frm);

    ret = ctx->enc_mpi->control(ctx->enc_ctx,
                                MPP_ENC_SET_PREP_CFG,
                                &ctx->prep_cfg);

    mpp_frame_init(&frame_tmp);
    mpp_frame_set_width(frame_tmp, width);
    mpp_frame_set_height(frame_tmp, height);
    // NOTE: only for current version, otherwise there
    // will be info change
    mpp_frame_set_hor_stride(frame_tmp, MPP_ALIGN(width, 16));
    mpp_frame_set_ver_stride(frame_tmp, MPP_ALIGN(height, 16));
    mpp_frame_set_fmt(frame_tmp, MPP_FMT_YUV420SP);
    ctx->dec_mpi_post->control(ctx->dec_ctx_post,
                               MPP_DEC_SET_FRAME_INFO,
                               (MppParam)frame_tmp);

    ctx->enc_mpi->control(ctx->enc_ctx,
                          MPP_ENC_GET_EXTRA_INFO, &ctx->enc_pkt);

    /* get and write sps/pps for H.264 */
    if (ctx->enc_pkt) {
        void *ptr   = mpp_packet_get_pos(ctx->enc_pkt);
        size_t len  = mpp_packet_get_length(ctx->enc_pkt);
        MppPacket tmp_pkt = NULL;

        // write extra data to dec packet and send
        mpp_packet_init(&tmp_pkt, ptr, len);
        mpp_packet_set_extra_data(tmp_pkt);
        ctx->dec_mpi_post->decode_put_packet(ctx->dec_ctx_post,
                                             tmp_pkt);
        mpp_packet_deinit(&tmp_pkt);

        if (ctx->file.fp_enc_out)
            fwrite(ptr, 1, len, ctx->file.fp_enc_out);

        ctx->enc_pkt = NULL;
    }
    if (frame_tmp)
        mpp_frame_deinit(&frame_tmp);

    return ret;
}

static MPP_RET mpi_rc_dec_pre_decode(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    RK_S32 dec_pkt_done = 0;
    MppApi *mpi = ctx->dec_mpi_pre;
    MppCtx dec_ctx = ctx->dec_ctx_pre;

    MppFrame frm = NULL;

    do {
        // send the packet first if packet is not done
        if (!dec_pkt_done) {
            ret = mpi->decode_put_packet(dec_ctx, ctx->dec_pkt_pre);
            if (MPP_OK == ret)
                dec_pkt_done = 1;
        }

        // then get all available frame and release
        do {
            ret = mpi->decode_get_frame(dec_ctx, &frm);
            if (MPP_OK != ret) {
                mpp_err("decode_get_frame failed ret %d\n", ret);
                break;
            }

            if (frm) {
                ctx->frm_eos = mpp_frame_get_eos(frm);
                ctx->pkt_eos = mpp_packet_get_eos(ctx->dec_pkt_pre);

                if (ctx->frm_eos && mpp_frame_get_buffer(frm) == NULL)
                    break;

                if (mpp_frame_get_info_change(frm)) {
                    mpp_log("decode_get_frame get info changed found\n");
                    mpi->control(dec_ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                    mpi_rc_info_change(ctx, frm);
                } else {
                    void *ptr;
                    size_t len;

                    mpi_rc_enc_encode(ctx, frm);
                    mpi_rc_enc_result(ctx);

                    len = mpp_packet_get_length(ctx->enc_pkt);
                    ctx->stat.frame_size = len;

                    ctx->stream_size_1s += len;
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

                    mpi_rc_dec_post_decode(ctx, frm);

                    mpp_packet_deinit(&ctx->enc_pkt);

                    ctx->frm_idx++;
                    if (ctx->cmd.have_config_file)
                        ctx->cmd.ectx->notify(ctx->cmd.ectx);

                    ret = ctx->enc_mpi->enqueue(ctx->enc_ctx, MPP_PORT_OUTPUT,
                                                ctx->enc_out_task);
                    if (ret) {
                        mpp_err("mpp task output enqueue failed\n");
                        continue;
                    }
                }
                mpp_frame_deinit(&frm);
                frm = NULL;
            } else {
                break;
            }

            if (ctx->frm_idx >= ctx->cmd.num_frames) {
                mpp_log("test frame number fulfill\n");
                ctx->frm_eos = 1;
                ctx->pkt_eos = 1;
                dec_pkt_done = 1;
            }

            // if last packet is send but last frame is not found continue
            if (ctx->pkt_eos && dec_pkt_done && !ctx->frm_eos)
                continue;

            if (ctx->frm_eos)
                break;
        } while (1);

        if (dec_pkt_done)
            break;

        msleep(5);
    } while (1);

    return ret;
}

static MPP_RET mpi_rc_buffer_init(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;
    /* NOTE: packet buffer may overflow */
    size_t packet_size  = 1920 * 1080;

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

    ctx->dec_in_buf_pre_size = MPI_DEC_STREAM_SIZE;
    ctx->dec_in_buf_pre = mpp_calloc(RK_U8, ctx->dec_in_buf_pre_size);
    if (NULL == ctx->dec_in_buf_pre) {
        mpp_err("mpi_dec_test malloc input stream buffer failed\n");
        goto RET;
    }

    ctx->frm_idx = 0;
    ctx->calc_base_idx = 0;
    ctx->stream_size_1s = 0;

    return ret;
RET:
    mpp_free(ctx->dec_in_buf_pre);
    ctx->dec_ctx_pre = NULL;
    mpp_free(ctx->dec_in_buf_post);
    ctx->dec_ctx_post = NULL;

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

static MPP_RET mpi_rc_codec(MpiRc2TestCtx *ctx)
{
    MPP_RET ret = MPP_OK;

    CHECK_RET(mpi_rc_buffer_init(ctx));
    CHECK_RET(mpi_rc_post_dec_init(ctx));
    CHECK_RET(mpi_rc_pre_dec_init(ctx));
    CHECK_RET(mpi_rc_enc_init(ctx));

    ctx->cmd.ectx = event_ctx_create(&ctx->cmd.e, event_occur, ctx);

    while (1) {
        size_t read_size =
            fread(ctx->dec_in_buf_pre, 1, ctx->dec_in_buf_pre_size, ctx->file.fp_input);

        if (read_size != ctx->dec_in_buf_pre_size || feof(ctx->file.fp_input)) {
            // setup eos flag
            mpp_packet_set_eos(ctx->dec_pkt_pre);
        }

        mpp_packet_set_pos(ctx->dec_pkt_pre, ctx->dec_in_buf_pre);
        mpp_packet_set_length(ctx->dec_pkt_pre, read_size);

        mpi_rc_dec_pre_decode(ctx);

        if (ctx->pkt_eos && ctx->frm_eos) {
            mpp_log("stream finish\n");
            break;
        }
    }

    CHECK_RET(ctx->enc_mpi->reset(ctx->enc_ctx));
    CHECK_RET(ctx->dec_mpi_pre->reset(ctx->dec_ctx_pre));
    CHECK_RET(ctx->dec_mpi_post->reset(ctx->dec_ctx_post));

MPP_TEST_OUT:
    // encoder deinit
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

static void mpi_rc_test_help()
{
    mpp_log("usage: mpi_rc_test [options]\n");
    show_options(mpi_rc_cmd);
    mpp_show_support_format();
}

static RK_S32 mpi_enc_test_parse_options(int argc, char **argv,
                                         MpiRcTestCmd* cmd)
{
    const char *opt;
    const char *next;
    RK_S32 optindex = 1;
    RK_S32 handleoptions = 1;
    RK_S32 err = MPP_NOK;

    if ((argc < 2) || (cmd == NULL)) {
        err = 1;
        return err;
    }

    /* parse options */
    while (optindex < argc) {
        opt  = (const char*)argv[optindex++];
        next = (const char*)argv[optindex];

        if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
            if (opt[1] == '-') {
                if (opt[2] != '\0') {
                    opt++;
                } else {
                    handleoptions = 0;
                    continue;
                }
            }

            opt++;

            switch (*opt) {
            case 'i':
                if (next) {
                    strncpy(cmd->file_input, next, MPI_RC_FILE_NAME_LEN);
                    cmd->file_input[strlen(next)] = '\0';
                    cmd->have_input = 1;
                } else {
                    mpp_err("input file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'o':
                if (next) {
                    strncpy(cmd->file_enc_out, next, MPI_RC_FILE_NAME_LEN);
                    cmd->file_enc_out[strlen(next)] = '\0';
                    cmd->have_enc_out = 1;
                } else {
                    mpp_log("output file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'd':
                if (next) {
                    cmd->debug = atoi(next);;
                } else {
                    mpp_err("invalid debug flag\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 't':
                if (next) {
                    cmd->type = (MppCodingType)atoi(next);
                    err = mpp_check_support_format(MPP_CTX_ENC, cmd->type);
                }

                if (!next || err) {
                    mpp_err("invalid input coding type\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'n':
                if (next) {
                    cmd->num_frames = atoi(next);
                } else {
                    mpp_err("invalid input max number of frames\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 's':
                if (next) {
                    strncpy(cmd->file_stat, next, MPI_RC_FILE_NAME_LEN);
                    cmd->file_stat[strlen(next)] = '\0';
                    cmd->have_stat_out = 1;
                } else {
                    mpp_log("stat file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'g':
                if (next) {
                    strncpy(cmd->file_config, next, MPI_RC_FILE_NAME_LEN);
                    cmd->file_config[strlen(next)] = '\0';
                    cmd->have_config_file = 1;
                }
                break;
            case 'p':
                if (next) {
                    cmd->psnr_en = atoi(next);
                } else {
                    mpp_err("invalid psnr enable/disable value\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'm':
                if (next) {
                    cmd->ssim_en = atoi(next);
                } else {
                    mpp_err("invalid ssim enable/disable value\n");
                    goto PARSE_OPINIONS_OUT;
                }
            default:
                goto PARSE_OPINIONS_OUT;
                break;
            }

            optindex++;
        }
    }

    err = 0;

PARSE_OPINIONS_OUT:
    return err;
}

static void mpi_rc_test_show_options(MpiRcTestCmd* cmd)
{
    mpp_log("cmd parse result:\n");
    mpp_log("input   file name: %s\n", cmd->file_input);
    mpp_log("enc out file name: %s\n", cmd->file_enc_out);
    mpp_log("stat    file name: %s\n", cmd->file_stat);
    mpp_log("type             : %d\n", cmd->type);
    mpp_log("debug flag       : %x\n", cmd->debug);
    mpp_log("num frames       : %d\n", cmd->num_frames);
    mpp_log("config file      : %s\n", cmd->file_config);
    mpp_log("psnr enable      : %d\n", cmd->psnr_en);
    mpp_log("ssim enable      : %d\n", cmd->ssim_en);
}

int main(int argc, char **argv)
{
    MPP_RET ret = MPP_OK;
    MpiRc2TestCtx ctx;
    MpiRcTestCmd* cmd = &ctx.cmd;

    mpp_log("=========== mpi rc test start ===========\n");

    memset(&ctx, 0, sizeof(ctx));

    // parse the cmd option
    ret = mpi_enc_test_parse_options(argc, argv, cmd);
    if (ret) {
        if (ret < 0) {
            mpp_err("mpi_rc_test_parse_options: input parameter invalid\n");
        }

        mpi_rc_test_help();
        return ret;
    }

    mpi_rc_test_show_options(cmd);

    mpp_env_set_u32("mpi_rc_debug", cmd->debug);

    ret = mpi_rc_init(&ctx);
    if (ret != MPP_OK) {
        mpp_err("mpi_rc_init failded ret %d", ret);
        goto err;
    }

    ret = mpi_rc_codec(&ctx);
    if (ret != MPP_OK) {
        mpp_err("mpi_rc_codec failded ret %d", ret);
        goto err;
    }

err:
    mpi_rc_deinit(&ctx);

    mpp_log("=========== mpi rc test end ===========\n");

    return (int)ret;
}

