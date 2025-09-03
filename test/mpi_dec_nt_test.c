/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 */

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_dec_nt_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpi_dec_utils.h"

typedef struct {
    MpiDecTestCmd   *cmd;
    MppCtx          ctx;
    MppApi          *mpi;
    RK_U32          quiet;

    /* end of stream flag when set quit the loop */
    RK_U32          loop_end;

    /* input and output */
    DecBufMgr       buf_mgr;
    MppBufferGroup  frm_grp;
    MppPacket       packet;
    MppFrame        frame;

    FILE            *fp_output;
    RK_S32          frame_count;
    RK_S32          frame_num;

    RK_S64          first_pkt;
    RK_S64          first_frm;

    size_t          max_usage;
    float           frame_rate;
    RK_S64          elapsed_time;
    RK_S64          delay;
    FILE            *fp_verify;
    FrmCrc          checkcrc;
} MpiDecLoopData;

static int dec_loop(MpiDecLoopData *data)
{
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos  = 0;
    MPP_RET ret = MPP_OK;
    MpiDecTestCmd *cmd = data->cmd;
    MppCtx ctx  = data->ctx;
    MppApi *mpi = data->mpi;
    MppPacket packet = data->packet;
    FileBufSlot *slot = NULL;
    RK_U32 quiet = data->quiet;
    FrmCrc *checkcrc = &data->checkcrc;

    // when packet size is valid read the input binary file
    ret = reader_read(cmd->reader, &slot);

    mpp_assert(ret == MPP_OK);
    mpp_assert(slot);

    pkt_eos = slot->eos;

    if (pkt_eos) {
        if (data->frame_num < 0 || data->frame_num > data->frame_count) {
            mpp_log_q(quiet, "%p loop again\n", ctx);
            reader_rewind(cmd->reader);
            pkt_eos = 0;
        } else {
            mpp_log_q(quiet, "%p found last packet\n", ctx);
            data->loop_end = 1;
        }
    }

    if (!slot->buf) {
        /* non-jpeg decoding */
        mpp_packet_set_data(packet, slot->data);
        mpp_packet_set_size(packet, slot->size);
        mpp_packet_set_pos(packet, slot->data);
        mpp_packet_set_length(packet, slot->size);
    } else {
        /* jpeg decoding */
        void *buf = mpp_buffer_get_ptr(slot->buf);
        size_t size = mpp_buffer_get_size(slot->buf);

        mpp_packet_set_data(packet, buf);
        mpp_packet_set_size(packet, size);
        mpp_packet_set_pos(packet, buf);
        mpp_packet_set_length(packet, size);
        mpp_packet_set_buffer(packet, slot->buf);
    }

    // setup eos flag
    if (pkt_eos)
        mpp_packet_set_eos(packet);

    do {
        RK_U32 frm_eos = 0;
        RK_S32 get_frm = 0;
        MppFrame frame = NULL;

        // send the packet first if packet is not done
        ret = mpi->decode(ctx, packet, &frame);
        if (ret)
            mpp_err("decode failed ret %d\n", ret);

        // then get all available frame and release
        if (frame) {
            if (mpp_frame_get_info_change(frame)) {
                RK_U32 width = mpp_frame_get_width(frame);
                RK_U32 height = mpp_frame_get_height(frame);
                RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
                RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
                RK_U32 buf_size = mpp_frame_get_buf_size(frame);
                MppBufferGroup grp = NULL;

                mpp_log_q(quiet, "%p decode_get_frame get info changed found\n", ctx);
                mpp_log_q(quiet, "%p decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                          ctx, width, height, hor_stride, ver_stride, buf_size);

                if (MPP_FRAME_FMT_IS_FBC(cmd->format)) {
                    MppFrame frm = NULL;

                    mpp_frame_init(&frm);
                    mpp_frame_set_width(frm, width);
                    mpp_frame_set_height(frm, height);
                    mpp_frame_set_fmt(frm, cmd->format);

                    ret = mpi->control(ctx, MPP_DEC_SET_FRAME_INFO, frm);
                    mpp_frame_deinit(&frm);

                    if (ret) {
                        mpp_err("set fbc frame info failed\n");
                        break;
                    }
                }

                grp = dec_buf_mgr_setup(data->buf_mgr, buf_size, 24, cmd->buf_mode);
                /* Set buffer to mpp decoder */
                ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, grp);
                if (ret) {
                    mpp_err("%p set buffer group failed ret %d\n", ctx, ret);
                    break;
                }

                data->frm_grp = grp;

                /*
                 * All buffer group config done. Set info change ready to let
                 * decoder continue decoding
                 */
                ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                if (ret) {
                    mpp_err("%p info change ready failed ret %d\n", ctx, ret);
                    break;
                }

                mpp_frame_deinit(&frame);
                continue;
            } else {
                char log_buf[256];
                RK_S32 log_size = sizeof(log_buf) - 1;
                RK_S32 log_len = 0;
                RK_U32 err_info = mpp_frame_get_errinfo(frame);
                RK_U32 discard = mpp_frame_get_discard(frame);

                if (!data->first_frm)
                    data->first_frm = mpp_time();

                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    "decode get frame %d", data->frame_count);

                if (mpp_frame_has_meta(frame)) {
                    MppMeta meta = mpp_frame_get_meta(frame);
                    RK_S32 temporal_id = 0;

                    mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id);

                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                        " tid %d", temporal_id);
                }

                if (err_info || discard) {
                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                        " err %x discard %x", err_info, discard);
                }
                mpp_log_q(quiet, "%p %s\n", ctx, log_buf);

                data->frame_count++;
                if (data->fp_output && !err_info)
                    dump_mpp_frame_to_file(frame, data->fp_output);

                if (data->fp_verify) {
                    calc_frm_crc(frame, checkcrc);
                    write_frm_crc(data->fp_verify, checkcrc);
                }

                fps_calc_inc(cmd->fps);
            }
            frm_eos = mpp_frame_get_eos(frame);
            mpp_frame_deinit(&frame);
            get_frm = 1;
        }

        // try get runtime frame memory usage
        if (data->frm_grp) {
            size_t usage = mpp_buffer_group_usage(data->frm_grp);
            if (usage > data->max_usage)
                data->max_usage = usage;
        }

        // when get one output frame check the output frame count limit
        if (get_frm) {
            if (data->frame_num > 0) {
                // when get enough frame quit
                if (data->frame_count >= data->frame_num) {
                    data->loop_end = 1;
                    break;
                }
            } else {
                // when get last frame quit
                if (frm_eos) {
                    mpp_log_q(quiet, "%p found last packet\n", ctx);
                    data->loop_end = 1;
                    break;
                }
            }
        }

        if (packet) {
            if (mpp_packet_get_length(packet)) {
                msleep(1);
                continue;
            }

            if (!data->first_pkt)
                data->first_pkt = mpp_time();

            packet = NULL;
            pkt_done = 1;
        }

        mpp_assert(pkt_done);

        // if last packet is send but last frame is not found continue
        if (pkt_eos && !frm_eos) {
            msleep(1);
            continue;
        }

        if (pkt_done)
            break;

        /*
         * why sleep here:
         * mpi->decode_put_packet will failed when packet in internal queue is
         * full,waiting the package is consumed .Usually hardware decode one
         * frame which resolution is 1080p needs 2 ms,so here we sleep 1ms
         * * is enough.
         */
        msleep(1);
    } while (1);

    return ret;
}

void *thread_decode(void *arg)
{
    MpiDecLoopData *data = (MpiDecLoopData *)arg;
    RK_S64 t_s, t_e;

    memset(&data->checkcrc, 0, sizeof(data->checkcrc));
    data->checkcrc.luma.sum = mpp_malloc(RK_ULONG, 512);
    data->checkcrc.chroma.sum = mpp_malloc(RK_ULONG, 512);

    t_s = mpp_time();

    while (!data->loop_end)
        dec_loop(data);

    t_e = mpp_time();
    data->elapsed_time = t_e - t_s;
    data->frame_count = data->frame_count;
    data->frame_rate = (float)data->frame_count * 1000000 / data->elapsed_time;
    data->delay = data->first_frm - data->first_pkt;

    mpp_log("decode %d frames time %lld ms delay %3d ms fps %3.2f\n",
            data->frame_count, (RK_S64)(data->elapsed_time / 1000),
            (RK_S32)(data->delay / 1000), data->frame_rate);

    MPP_FREE(data->checkcrc.luma.sum);
    MPP_FREE(data->checkcrc.chroma.sum);

    return NULL;
}

int dec_nt_decode(MpiDecTestCmd *cmd)
{
    // base flow context
    MppCtx ctx          = NULL;
    MppApi *mpi         = NULL;

    // input / output
    MppPacket packet    = NULL;
    MppFrame  frame     = NULL;

    // paramter for resource malloc
    RK_U32 width        = cmd->width;
    RK_U32 height       = cmd->height;
    MppCodingType type  = cmd->type;

    // config for runtime mode
    MppDecCfg cfg       = NULL;
    RK_U32 need_split   = 1;

    // resources
    MppBuffer frm_buf   = NULL;
    pthread_t thd;
    pthread_attr_t attr;
    MpiDecLoopData data;
    MPP_RET ret = MPP_OK;

    mpp_log("mpi_dec_test start\n");
    memset(&data, 0, sizeof(data));
    pthread_attr_init(&attr);

    cmd->simple = (cmd->type != MPP_VIDEO_CodingMJPEG) ? (1) : (0);

    if (cmd->have_output) {
        data.fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == data.fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            goto MPP_TEST_OUT;
        }
    }

    if (cmd->file_slt) {
        data.fp_verify = fopen(cmd->file_slt, "wt");
        if (!data.fp_verify)
            mpp_err("failed to open verify file %s\n", cmd->file_slt);
    }

    ret = dec_buf_mgr_init(&data.buf_mgr);
    if (ret) {
        mpp_err("dec_buf_mgr_init failed\n");
        goto MPP_TEST_OUT;
    }

    ret = mpp_packet_init(&packet, NULL, 0);
    mpp_err_f("mpp_packet_init get %p\n", packet);
    if (ret) {
        mpp_err("mpp_packet_init failed\n");
        goto MPP_TEST_OUT;
    }

    // decoder demo
    ret = mpp_create(&ctx, &mpi);
    if (ret) {
        mpp_err("mpp_create failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_log("%p mpi_dec_test decoder test start w %d h %d type %d\n",
            ctx, width, height, type);

    ret = mpi->control(ctx, MPP_SET_DISABLE_THREAD, NULL);

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (ret) {
        mpp_err("%p mpp_init failed\n", ctx);
        goto MPP_TEST_OUT;
    }

    mpp_dec_cfg_init(&cfg);

    /* get default config from decoder context */
    ret = mpi->control(ctx, MPP_DEC_GET_CFG, cfg);
    if (ret) {
        mpp_err("%p failed to get decoder cfg ret %d\n", ctx, ret);
        goto MPP_TEST_OUT;
    }

    /*
     * split_parse is to enable mpp internal frame spliter when the input
     * packet is not aplited into frames.
     */
    ret = mpp_dec_cfg_set_u32(cfg, "base:split_parse", need_split);
    if (ret) {
        mpp_err("%p failed to set split_parse ret %d\n", ctx, ret);
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx, MPP_DEC_SET_CFG, cfg);
    if (ret) {
        mpp_err("%p failed to set cfg %p ret %d\n", ctx, cfg, ret);
        goto MPP_TEST_OUT;
    }

    data.cmd            = cmd;
    data.ctx            = ctx;
    data.mpi            = mpi;
    data.loop_end       = 0;
    data.packet         = packet;
    data.frame          = frame;
    data.frame_count    = 0;
    data.frame_num      = cmd->frame_num;
    data.quiet          = cmd->quiet;

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&thd, &attr, thread_decode, &data);
    if (ret) {
        mpp_err("failed to create thread for input ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    if (cmd->frame_num < 0) {
        // wait for input then quit decoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop decoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        data.loop_end = 1;
    }

    pthread_join(thd, NULL);

    cmd->max_usage = data.max_usage;

    ret = mpi->reset(ctx);
    if (ret) {
        mpp_err("%p mpi->reset failed\n", ctx);
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    if (data.packet) {
        mpp_packet_deinit(&data.packet);
        data.packet = NULL;
    }

    if (frame) {
        mpp_frame_deinit(&frame);
        frame = NULL;
    }

    if (ctx) {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (!cmd->simple) {
        if (frm_buf) {
            mpp_buffer_put(frm_buf);
            frm_buf = NULL;
        }
    }

    data.frm_grp = NULL;
    if (data.buf_mgr) {
        dec_buf_mgr_deinit(data.buf_mgr);
        data.buf_mgr = NULL;
    }

    if (data.fp_output) {
        fclose(data.fp_output);
        data.fp_output = NULL;
    }

    if (data.fp_verify) {
        fclose(data.fp_verify);
        data.fp_verify = NULL;
    }

    if (cfg) {
        mpp_dec_cfg_deinit(cfg);
        cfg = NULL;
    }

    pthread_attr_destroy(&attr);

    return ret;
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    MpiDecTestCmd  cmd_ctx;
    MpiDecTestCmd* cmd = &cmd_ctx;

    memset((void*)cmd, 0, sizeof(*cmd));
    cmd->format = MPP_FMT_BUTT;
    cmd->pkt_size = MPI_DEC_STREAM_SIZE;

    // parse the cmd option
    ret = mpi_dec_test_cmd_init(cmd, argc, argv);
    if (ret)
        goto RET;

    mpi_dec_test_cmd_options(cmd);

    ret = dec_nt_decode(cmd);
    if (MPP_OK == ret)
        mpp_log("test success max memory %.2f MB\n", cmd->max_usage / (float)(1 << 20));
    else
        mpp_err("test failed ret %d\n", ret);

RET:
    mpi_dec_test_cmd_deinit(cmd);

    return ret;
}

