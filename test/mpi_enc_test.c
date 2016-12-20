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

#define MODULE_TAG "mpi_enc_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "utils.h"

#include "vpu_api.h"

#define MPI_ENC_IO_COUNT            (4)
#define MAX_FILE_NAME_LENGTH        256

#define MPI_ENC_TEST_SET_IDR_FRAME  0
#define MPI_ENC_TEST_SET_OSD        0

typedef struct {
    char            file_input[MAX_FILE_NAME_LENGTH];
    char            file_output[MAX_FILE_NAME_LENGTH];
    MppCodingType   type;
    RK_U32          width;
    RK_U32          height;
    MppFrameFormat  format;
    RK_U32          debug;
    RK_U32          num_frames;

    RK_U32          have_input;
    RK_U32          have_output;
} MpiEncTestCmd;

static OptionInfo mpi_enc_cmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input picture"},
    {"h",               "height",               "the height of input picture"},
    {"f",               "format",               "the format of input picture"},
    {"t",               "type",                 "output stream coding type"},
    {"n",               "max frame number",     "max encoding frame number"},
    {"d",               "debug",                "debug flag"},
};

static MPP_RET read_yuv_image(RK_U8 *buf, MppEncConfig *mpp_cfg, FILE *fp)
{
    MPP_RET ret = MPP_OK;
    RK_U32 read_size;
    RK_U32 row = 0;
    RK_U32 width        = mpp_cfg->width;
    RK_U32 height       = mpp_cfg->height;
    RK_U32 hor_stride   = mpp_cfg->hor_stride;
    RK_U32 ver_stride   = mpp_cfg->ver_stride;
    MppFrameFormat fmt  = mpp_cfg->format;
    RK_U8 *buf_y = buf;
    RK_U8 *buf_u = buf_y + hor_stride * ver_stride; // NOTE: diff from gen_yuv_image
    RK_U8 *buf_v = buf_u + hor_stride * ver_stride / 4; // NOTE: diff from gen_yuv_image

    switch (fmt) {
    case MPP_FMT_YUV420SP : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                mpp_err_f("read ori yuv file luma failed");
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_u + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                mpp_err_f("read ori yuv file cb failed");
                ret  = MPP_NOK;
                goto err;
            }
        }
    } break;
    case MPP_FMT_YUV420P : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                mpp_err_f("read ori yuv file luma failed");
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_u + row * hor_stride / 2, 1, width / 2, fp);
            if (read_size != width / 2) {
                mpp_err_f("read ori yuv file cb failed");
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_v + row * hor_stride / 2, 1, width / 2, fp);
            if (read_size != width / 2) {
                mpp_err_f("read ori yuv file cr failed");
                ret  = MPP_NOK;
                goto err;
            }
        }
    } break;
    default : {
        mpp_err_f("read image do not support fmt %d\n", fmt);
        ret = MPP_NOK;
    } break;
    }

err:

    return ret;
}

static MPP_RET fill_yuv_image(RK_U8 *buf, MppEncConfig *mpp_cfg, RK_U32 frame_count)
{
    MPP_RET ret = MPP_OK;
    RK_U32 width        = mpp_cfg->width;
    RK_U32 height       = mpp_cfg->height;
    RK_U32 hor_stride   = mpp_cfg->hor_stride;
    RK_U32 ver_stride   = mpp_cfg->ver_stride;
    MppFrameFormat fmt  = mpp_cfg->format;
    RK_U8 *buf_y = buf;
    RK_U8 *buf_c = buf + hor_stride * ver_stride;
    RK_U32 x, y;

    switch (fmt) {
    case MPP_FMT_YUV420SP : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }

        p = buf_c;
        for (y = 0; y < height / 2; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 2 + 0] = 128 + y + frame_count * 2;
                p[x * 2 + 1] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV420P : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }

        p = buf_c;
        for (y = 0; y < height / 2; y++, p += hor_stride / 2) {
            for (x = 0; x < width / 2; x++) {
                p[x] = 128 + y + frame_count * 2;
            }
        }

        p = buf_c + hor_stride * ver_stride / 4;
        for (y = 0; y < height / 2; y++, p += hor_stride / 2) {
            for (x = 0; x < width / 2; x++) {
                p[x] = 64 + x + frame_count * 5;
            }
        }
    } break;
    default : {
        mpp_err_f("filling function do not support type %d\n", fmt);
        ret = MPP_NOK;
    } break;
    }
    return ret;
}

static MPP_RET mpi_enc_gen_osd_data(MppEncOSDData *osd_data, MppBuffer osd_buf, RK_U32 frame_cnt)
{
    RK_U32 k = 0, buf_size = 0;
    RK_U8 data = 0;

    osd_data->num_region = 8;
    osd_data->buf = osd_buf;
    for (k = 0; k < osd_data->num_region; k++) {
        osd_data->region[k].enable = 1;
        osd_data->region[k].inverse = frame_cnt & 1;
        osd_data->region[k].start_mb_x = k * 3;
        osd_data->region[k].start_mb_y = k * 2;
        osd_data->region[k].num_mb_x = 2;
        osd_data->region[k].num_mb_y = 2;

        buf_size = osd_data->region[k].num_mb_x * osd_data->region[k].num_mb_y * 256;
        osd_data->region[k].buf_offset = k * buf_size;

        data = k;
        memset((RK_U8 *)mpp_buffer_get_ptr(osd_data->buf) + osd_data->region[k].buf_offset, data, buf_size);
    }

    return MPP_OK;
}

static MPP_RET mpi_enc_gen_osd_plt(MppEncOSDPlt *osd_plt, RK_U32 *table)
{
    RK_U32 k = 0;
    if (osd_plt->buf) {
        for (k = 0; k < 256; k++)
            osd_plt->buf[k] = table[k % 8];
    }
    return MPP_OK;
}

int mpi_enc_test(MpiEncTestCmd *cmd)
{
    MPP_RET ret             = MPP_OK;
    RK_U32 frm_eos          = 0;
    RK_U32 pkt_eos          = 0;
    FILE *fp_input          = NULL;
    FILE *fp_output         = NULL;

    // base flow context
    MppCtx ctx              = NULL;
    MppApi *mpi             = NULL;
    MppEncConfig mpp_cfg;

    // input / output
    RK_S32 i;
    MppBufferGroup frm_grp  = NULL;
    MppBufferGroup pkt_grp  = NULL;
    MppFrame  frame         = NULL;
    MppPacket packet        = NULL;
    MppBuffer frm_buf[MPI_ENC_IO_COUNT] = { NULL };
    MppBuffer pkt_buf[MPI_ENC_IO_COUNT] = { NULL };
    MppBuffer md_buf[MPI_ENC_IO_COUNT] = { NULL };
    MppBuffer osd_idx_buf[MPI_ENC_IO_COUNT] = { NULL };
    MppEncOSDPlt osd_plt;
    MppEncSeiMode sei_mode = MPP_ENC_SEI_MODE_ONE_SEQ;

    // paramter for resource malloc
    RK_U32 width        = cmd->width;
    RK_U32 height       = cmd->height;
    RK_U32 hor_stride   = MPP_ALIGN(width, 16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    MppFrameFormat fmt  = cmd->format;
    MppCodingType type  = cmd->type;
    RK_U32 num_frames   = cmd->num_frames;

    // resources
    size_t frame_size   = hor_stride * ver_stride * 3 / 2;
    /* NOTE: packet buffer may overflow */
    size_t packet_size  = width * height;
    /* 32bits for each 16x16 block */
    size_t mdinfo_size  = (((hor_stride + 255) & (~255)) / 16) * (ver_stride / 16) * 4; //NOTE: hor_stride should be 16-MB aligned
    /* osd idx size range from 16x16 bytes(pixels) to hor_stride*ver_stride(bytes). for general use, 1/8 Y buffer is enough. */
    size_t osd_idx_size  = hor_stride * ver_stride / 8;
    RK_U32 frame_count  = 0;
    RK_U64 stream_size  = 0;
    RK_U32 plt_table[8] = {
        MPP_ENC_OSD_PLT_WHITE,
        MPP_ENC_OSD_PLT_YELLOW,
        MPP_ENC_OSD_PLT_CYAN,
        MPP_ENC_OSD_PLT_GREEN,
        MPP_ENC_OSD_PLT_TRANS,
        MPP_ENC_OSD_PLT_RED,
        MPP_ENC_OSD_PLT_BLUE,
        MPP_ENC_OSD_PLT_BLACK
    };

    mpp_log("mpi_enc_test start\n");

    if (cmd->have_input) {
        fp_input = fopen(cmd->file_input, "rb");
        if (NULL == fp_input) {
            mpp_err("failed to open input file %s\n", cmd->file_input);
            mpp_err("create default yuv image for test\n");
        }
    }

    if (cmd->have_output) {
        fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            ret = MPP_ERR_OPEN_FILE;
            goto MPP_TEST_OUT;
        }
    }

    ret = mpp_buffer_group_get_internal(&frm_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("failed to get buffer group for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_group_get_internal(&pkt_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("failed to get buffer group for output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    for (i = 0; i < MPI_ENC_IO_COUNT; i++) {
        ret = mpp_buffer_get(frm_grp, &frm_buf[i], frame_size);
        if (ret) {
            mpp_err("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpp_buffer_get(frm_grp, &osd_idx_buf[i], osd_idx_size);
        if (ret) {
            mpp_err("failed to get buffer for osd idx buf ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpp_buffer_get(pkt_grp, &pkt_buf[i], packet_size);
        if (ret) {
            mpp_err("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpp_buffer_get(pkt_grp, &md_buf[i], mdinfo_size);
        if (ret) {
            mpp_err("failed to get buffer for motion detection info ret %d\n", ret);
            goto MPP_TEST_OUT;
        }
    }

    mpp_log("mpi_enc_test encoder test start w %d h %d type %d\n", width, height, type);

    // encoder demo
    ret = mpp_create(&ctx, &mpi);

    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(ctx, MPP_CTX_ENC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_OUT;
    }

    memset(&mpp_cfg, 0, sizeof(mpp_cfg));
    mpp_cfg.size        = sizeof(mpp_cfg);
    mpp_cfg.width       = width;
    mpp_cfg.height      = height;
    mpp_cfg.hor_stride  = hor_stride;
    mpp_cfg.ver_stride  = ver_stride;
    mpp_cfg.format      = fmt;
    mpp_cfg.rc_mode     = 0;
    mpp_cfg.skip_cnt    = 0;
    mpp_cfg.fps_in      = 30;
    mpp_cfg.fps_out     = 30;
    mpp_cfg.bps         = width * height * 2 * mpp_cfg.fps_in;
    mpp_cfg.qp          = (type == MPP_VIDEO_CodingMJPEG) ? (10) : (24);
    mpp_cfg.gop         = 60;

    mpp_cfg.profile     = 100;
    mpp_cfg.level       = 41;
    mpp_cfg.cabac_en    = 1;

    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
    if (MPP_OK != ret) {
        mpp_err("mpi control enc set sei cfg failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, &mpp_cfg);
    if (MPP_OK != ret) {
        mpp_err("mpi control enc set cfg failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
    if (MPP_OK != ret) {
        mpp_err("mpi control enc get extra info failed\n");
        goto MPP_TEST_OUT;
    }

    /* get and write sps/pps for H.264 */
    if (packet) {
        void *ptr   = mpp_packet_get_pos(packet);
        size_t len  = mpp_packet_get_length(packet);

        if (fp_output)
            fwrite(ptr, 1, len, fp_output);

        packet = NULL;
    }

    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
        mpp_err("mpp_frame_init failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_frame_set_width(frame, width);
    mpp_frame_set_height(frame, height);
    mpp_frame_set_hor_stride(frame, hor_stride);
    mpp_frame_set_ver_stride(frame, ver_stride);

    /* gen and cfg osd plt */
    mpi_enc_gen_osd_plt(&osd_plt, plt_table);
#if MPI_ENC_TEST_SET_OSD
    ret = mpi->control(ctx, MPP_ENC_SET_OSD_PLT_CFG, &osd_plt);
    if (MPP_OK != ret) {
        mpp_err("mpi control enc set osd plt failed\n");
        goto MPP_TEST_OUT;
    }
#endif

    i = 0;
    while (!pkt_eos) {
        MppTask task = NULL;
        RK_S32 index = i++;
        MppBuffer frm_buf_in  = frm_buf[index];
        MppBuffer pkt_buf_out = pkt_buf[index];
        MppBuffer md_info_buf = md_buf[index];
        MppBuffer osd_data_buf = osd_idx_buf[index];
        MppEncOSDData osd_data;
        void *buf = mpp_buffer_get_ptr(frm_buf_in);

        if (i == MPI_ENC_IO_COUNT)
            i = 0;

        if (fp_input) {
            ret = read_yuv_image(buf, &mpp_cfg, fp_input);
            if (ret != MPP_OK  || feof(fp_input)) {
                mpp_log("found last frame. feof %d\n", feof(fp_input));
                frm_eos = 1;
            }
        } else {
            ret = fill_yuv_image(buf, &mpp_cfg, frame_count);
            if (ret)
                goto MPP_TEST_OUT;
        }

        mpp_frame_set_buffer(frame, frm_buf_in);
        mpp_frame_set_eos(frame, frm_eos);

        mpp_packet_init_with_buffer(&packet, pkt_buf_out);

        ret = mpi->poll(ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
        if (ret) {
            mpp_err("mpp task input poll failed ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpi->dequeue(ctx, MPP_PORT_INPUT, &task);
        if (ret || NULL == task) {
            mpp_err("mpp task input dequeue failed ret %d task %p\n", ret, task);
            goto MPP_TEST_OUT;
        }

        if (task) {
            MppFrame frame_out = NULL;

            mpp_task_meta_get_frame(task, KEY_INPUT_FRAME,  &frame_out);
            if (frame_out)
                mpp_assert(frame_out == frame);
        }

        mpp_task_meta_set_frame (task, KEY_INPUT_FRAME,  frame);
        mpp_task_meta_set_packet(task, KEY_OUTPUT_PACKET, packet);
        mpp_task_meta_set_buffer(task, KEY_MOTION_INFO, md_info_buf);

        /* set idr frame */
#if MPI_ENC_TEST_SET_IDR_FRAME
        if (frame_count && frame_count % (mpp_cfg.gop / 4) == 0) {
            ret = mpi->control(ctx, MPP_ENC_SET_IDR_FRAME, NULL);
            if (MPP_OK != ret) {
                mpp_err("mpi control enc set idr frame failed\n");
                goto MPP_TEST_OUT;
            }
        }
#endif

        /* gen and cfg osd plt */
        mpi_enc_gen_osd_data(&osd_data, osd_data_buf, frame_count);
#if MPI_ENC_TEST_SET_OSD
        ret = mpi->control(ctx, MPP_ENC_SET_OSD_DATA_CFG, &osd_data);
        if (MPP_OK != ret) {
            mpp_err("mpi control enc set osd data failed\n");
            goto MPP_TEST_OUT;
        }
#endif

        ret = mpi->enqueue(ctx, MPP_PORT_INPUT, task);
        if (ret) {
            mpp_err("mpp task input enqueue failed\n");
            goto MPP_TEST_OUT;
        }

        ret = mpi->poll(ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
        if (ret) {
            mpp_err("mpp task output poll failed ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        ret = mpi->dequeue(ctx, MPP_PORT_OUTPUT, &task);
        if (ret || NULL == task) {
            mpp_err("mpp task output dequeue failed ret %d task %p\n", ret, task);
            goto MPP_TEST_OUT;
        }

        if (task) {
            MppFrame packet_out = NULL;

            mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, &packet_out);

            mpp_assert(packet_out == packet);
            if (packet) {
                // write packet to file here
                void *ptr   = mpp_packet_get_pos(packet);
                size_t len  = mpp_packet_get_length(packet);

                pkt_eos = mpp_packet_get_eos(packet);

                if (fp_output)
                    fwrite(ptr, 1, len, fp_output);
                mpp_packet_deinit(&packet);

                mpp_log_f("encoded frame %d size %d\n", frame_count, len);
                stream_size += len;

                if (pkt_eos) {
                    mpp_log("found last packet\n");
                    mpp_assert(frm_eos);
                }
            }
            frame_count++;

            ret = mpi->enqueue(ctx, MPP_PORT_OUTPUT, task);
            if (ret) {
                mpp_err("mpp task output enqueue failed\n");
                goto MPP_TEST_OUT;
            }
        }

        if (num_frames && frame_count >= num_frames) {
            mpp_log_f("encode max %d frames", frame_count);
            break;
        }
        if (frm_eos && pkt_eos)
            break;
    }

    ret = mpi->reset(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (frame) {
        mpp_frame_deinit(&frame);
        frame = NULL;
    }

    for (i = 0; i < MPI_ENC_IO_COUNT; i++) {
        if (frm_buf[i]) {
            mpp_buffer_put(frm_buf[i]);
            frm_buf[i] = NULL;
        }

        if (pkt_buf[i]) {
            mpp_buffer_put(pkt_buf[i]);
            pkt_buf[i] = NULL;
        }

        if (md_buf[i]) {
            mpp_buffer_put(md_buf[i]);
            md_buf[i] = NULL;
        }

        if (osd_idx_buf[i]) {
            mpp_buffer_put(osd_idx_buf[i]);
            osd_idx_buf[i] = NULL;
        }
    }

    if (frm_grp) {
        mpp_buffer_group_put(frm_grp);
        frm_grp = NULL;
    }

    if (pkt_grp) {
        mpp_buffer_group_put(pkt_grp);
        pkt_grp = NULL;
    }

    if (fp_output) {
        fclose(fp_output);
        fp_output = NULL;
    }

    if (fp_input) {
        fclose(fp_input);
        fp_input = NULL;
    }

    if (MPP_OK == ret)
        mpp_log("mpi_enc_test success total frame %d bps %lld\n",
                frame_count, (RK_U64)((stream_size * 8 * mpp_cfg.fps_out) / frame_count));
    else
        mpp_err("mpi_enc_test failed ret %d\n", ret);

    return ret;
}


static void mpi_enc_test_help()
{
    mpp_log("usage: mpi_enc_test [options]\n");
    show_options(mpi_enc_cmd);
    mpp_show_support_format();
}

static RK_S32 mpi_enc_test_parse_options(int argc, char **argv, MpiEncTestCmd* cmd)
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
                    strncpy(cmd->file_input, next, MAX_FILE_NAME_LENGTH);
                    cmd->file_input[strlen(next)] = '\0';
                    cmd->have_input = 1;
                } else {
                    mpp_err("input file is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'o':
                if (next) {
                    strncpy(cmd->file_output, next, MAX_FILE_NAME_LENGTH);
                    cmd->file_output[strlen(next)] = '\0';
                    cmd->have_output = 1;
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
            case 'w':
                if (next) {
                    cmd->width = atoi(next);
                } else {
                    mpp_err("invalid input width\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'h':
                if ((*(opt + 1) != '\0') && !strncmp(opt, "help", 4)) {
                    mpi_enc_test_help();
                    err = 1;
                    goto PARSE_OPINIONS_OUT;
                } else if (next) {
                    cmd->height = atoi(next);
                } else {
                    mpp_log("input height is invalid\n");
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'f':
                if (next) {
                    cmd->format = (MppFrameFormat)atoi(next);
                    err = ((cmd->format >= MPP_FMT_YUV_BUTT && cmd->format < MPP_FRAME_FMT_RGB) ||
                           cmd->format >= MPP_FMT_RGB_BUTT);
                }

                if (!next || err) {
                    mpp_err("invalid input format %d\n", cmd->format);
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

static void mpi_enc_test_show_options(MpiEncTestCmd* cmd)
{
    mpp_log("cmd parse result:\n");
    mpp_log("input  file name: %s\n", cmd->file_input);
    mpp_log("output file name: %s\n", cmd->file_output);
    mpp_log("width      : %d\n", cmd->width);
    mpp_log("height     : %d\n", cmd->height);
    mpp_log("type       : %d\n", cmd->type);
    mpp_log("debug flag : %x\n", cmd->debug);
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    MpiEncTestCmd  cmd_ctx;
    MpiEncTestCmd* cmd = &cmd_ctx;

    memset((void*)cmd, 0, sizeof(*cmd));

    // parse the cmd option
    ret = mpi_enc_test_parse_options(argc, argv, cmd);
    if (ret) {
        if (ret < 0) {
            mpp_err("mpi_enc_test_parse_options: input parameter invalid\n");
        }

        mpi_enc_test_help();
        return ret;
    }

    mpi_enc_test_show_options(cmd);

    mpp_env_set_u32("mpi_debug", cmd->debug);

    ret = mpi_enc_test(cmd);

    mpp_env_set_u32("mpi_debug", 0x0);
    return ret;
}

