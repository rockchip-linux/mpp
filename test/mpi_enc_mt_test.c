/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpi_enc_mt_test"

#include <string.h>
#include "rk_mpi.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_list.h"
#include "mpp_lock.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "utils.h"
#include "mpi_enc_utils.h"
#include "camera_source.h"
#include "mpp_enc_roi_utils.h"

#define BUF_COUNT   4

/* For each instance thread return value */
typedef struct {
    float           frame_rate;
    RK_U64          bit_rate;

    RK_S64          time_start;
    RK_S64          time_delay;
    RK_S64          time_total;

    RK_S64          elapsed_time;
    RK_S32          frame_count;
    RK_S64          stream_size;
    RK_S64          delay;
} MpiEncMtCtxRet;

typedef struct MpiEncMtTestPriv_t {
    MppList   *list_buf;
    MppBuffer frm_buf[BUF_COUNT];
    MppBuffer pkt_buf[BUF_COUNT];
} MpiEncMtTestPriv;

typedef struct {
    MppEncTestObjSet    *obj_set;
    const char          *name;
    RK_S32              chn;

    pthread_t           thd_in;     // thread for for frame input
    pthread_t           thd_out;    // thread for for packet output

    struct list_head    frm_list;
    spinlock_t          frm_lock;

    MpiEncTestData      ctx;        // context of encoder
    MpiEncMtTestPriv    priv;       // private data for multi encoder
    MpiEncMtCtxRet      ret;        // return of encoder
} MpiEncMtCtxInfo;

MPP_RET mt_test_res_init(MpiEncMtCtxInfo *info)
{
    MppEncTestObjSet *obj_set = info->obj_set;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MpiEncTestData *p = &info->ctx;
    MpiEncMtTestPriv *priv = &info->priv;
    MppPollType timeout = MPP_POLL_NON_BLOCK;
    RK_U32 quiet = cmd->quiet;
    MPP_RET ret = MPP_OK;
    RK_S32 i;

    mpp_log_q(quiet, "%s start\n", info->name);

    priv->list_buf = mpp_list_create(NULL);
    if (NULL == priv->list_buf) {
        mpp_err_f("failed to get mpp buffer list\n");
        return MPP_ERR_MALLOC;
    }

    ret = mpp_buffer_group_get_internal(&p->buf_grp, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
        return ret;
    }

    for (i = 0; i < BUF_COUNT; i++) {
        ret = mpp_buffer_get(p->buf_grp, &priv->frm_buf[i], p->frame_size + p->header_size);
        if (ret) {
            mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
            return ret;
        }

        ret = mpp_buffer_get(p->buf_grp, &priv->pkt_buf[i], p->frame_size);
        if (ret) {
            mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
            return ret;
        }

        mpp_list_add_at_tail(priv->list_buf, &priv->frm_buf[i], sizeof(priv->frm_buf[i]));
    }

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        return ret;
    }

    mpp_log_q(quiet, "%p encoder test start w %d h %d type %d\n",
              p->ctx, p->width, p->height, p->type);

    ret = p->mpi->control(p->ctx, MPP_SET_INPUT_TIMEOUT, &timeout);
    if (ret) {
        mpp_err("mpi control set input timeout %d ret %d\n", timeout, ret);
        return ret;
    }

    timeout = MPP_POLL_BLOCK;

    ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        return ret;
    }

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        return ret;
    }

    ret = mpp_enc_cfg_init(&p->cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
        return ret;
    }

    ret = p->mpi->control(p->ctx, MPP_ENC_GET_CFG, p->cfg);
    if (ret) {
        mpp_err_f("get enc cfg failed ret %d\n", ret);
        return ret;
    }

    ret = mpi_enc_cfg_setup(p, cmd, obj_set->cfg_obj);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
    }

    return ret;
}

MPP_RET mt_test_res_deinit(MpiEncMtCtxInfo *info)
{
    MpiEncTestData *p = &info->ctx;
    MpiEncMtTestPriv *priv = &info->priv;
    MPP_RET ret = MPP_OK;
    RK_S32 i;

    p->mpi->reset(p->ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
        return ret;
    }

    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->cfg) {
        mpp_enc_cfg_deinit(p->cfg);
        p->cfg = NULL;
    }

    for (i = 0; i < BUF_COUNT; i++) {
        if (priv->frm_buf[i]) {
            mpp_buffer_put(priv->frm_buf[i]);
            priv->frm_buf[i] = NULL;
        }

        if (priv->pkt_buf[i]) {
            mpp_buffer_put(priv->pkt_buf[i]);
            priv->pkt_buf[i] = NULL;
        }
    }

    if (p->osd_data.buf) {
        mpp_buffer_put(p->osd_data.buf);
        p->osd_data.buf = NULL;
    }

    if (p->buf_grp) {
        mpp_buffer_group_put(p->buf_grp);
        p->buf_grp = NULL;
    }

    if (priv->list_buf) {
        mpp_list_destroy(priv->list_buf);
        priv->list_buf = NULL;
    }

    if (p->roi_ctx) {
        mpp_enc_roi_deinit(p->roi_ctx);
        p->roi_ctx = NULL;
    }

    return MPP_OK;
}

void *enc_test_input(void *arg)
{
    MpiEncMtCtxInfo *info = (MpiEncMtCtxInfo *)arg;
    MppEncTestObjSet *obj_set = info->obj_set;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MpiEncTestData *p = &info->ctx;
    MpiEncMtTestPriv *priv = &info->priv;
    RK_S32 chn = info->chn;
    MppApi *mpi = p->mpi;
    MppCtx ctx = p->ctx;
    MppList *list_buf = priv->list_buf;
    RK_U32 cap_num = 0;
    RK_U32 quiet = cmd->quiet;
    MPP_RET ret = MPP_OK;

    mpp_log_q(quiet, "%s start\n", info->name);

    while (1) {
        MppMeta meta = NULL;
        MppFrame frame = NULL;
        MppBuffer buffer = NULL;
        void *buf = NULL;
        RK_S32 cam_frm_idx = -1;
        MppBuffer cam_buf = NULL;

        mpp_mutex_cond_lock(&list_buf->cond_lock);
        if (!mpp_list_size(list_buf))
            mpp_list_wait(list_buf);

        buffer = NULL;
        mpp_list_del_at_head(list_buf, &buffer, sizeof(buffer));
        if (NULL == buffer) {
            mpp_mutex_cond_unlock(&list_buf->cond_lock);
            continue;
        }

        buf = mpp_buffer_get_ptr(buffer);
        mpp_mutex_cond_unlock(&list_buf->cond_lock);

        if (p->fp_input) {
            ret = read_image((RK_U8 *)buf, p->fp_input, p->width, p->height,
                             p->hor_stride, p->ver_stride, p->fmt);
            if (ret == MPP_NOK || feof(p->fp_input)) {
                p->frm_eos = 1;

                if (p->frame_num < 0 || p->frm_cnt_in < p->frame_num) {
                    clearerr(p->fp_input);
                    rewind(p->fp_input);
                    p->frm_eos = 0;
                    mpp_log_q(quiet, "chn %d loop times %d\n", chn, ++p->loop_times);
                    if (buffer) {
                        mpp_mutex_cond_lock(&list_buf->cond_lock);
                        mpp_list_add_at_tail(list_buf, &buffer, sizeof(buffer));
                        mpp_mutex_cond_unlock(&list_buf->cond_lock);
                    }
                    continue;
                }
                mpp_log_q(quiet, "chn %d found last frame. feof %d\n", chn, feof(p->fp_input));
            } else if (ret == MPP_ERR_VALUE)
                break;
        } else {
            if (p->cam_ctx == NULL) {
                ret = fill_image((RK_U8 *)buf, p->width, p->height, p->hor_stride,
                                 p->ver_stride, p->fmt, p->frm_cnt_in * p->frm_step);
                if (ret)
                    break;
            } else {
                cam_frm_idx = camera_source_get_frame(p->cam_ctx);
                mpp_assert(cam_frm_idx >= 0);

                /* skip unstable frames */
                if (cap_num++ < 50) {
                    camera_source_put_frame(p->cam_ctx, cam_frm_idx);
                    continue;
                }

                cam_buf = camera_frame_to_buf(p->cam_ctx, cam_frm_idx);
                mpp_assert(cam_buf);
            }
        }

        ret = mpp_frame_init(&frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            break;
        }

        mpp_frame_set_width(frame, p->width);
        mpp_frame_set_height(frame, p->height);
        mpp_frame_set_hor_stride(frame, p->hor_stride);
        mpp_frame_set_ver_stride(frame, p->ver_stride);
        mpp_frame_set_fmt(frame, p->fmt);
        mpp_frame_set_eos(frame, p->frm_eos);

        if (p->fp_input && feof(p->fp_input))
            mpp_frame_set_buffer(frame, NULL);
        else if (cam_buf)
            mpp_frame_set_buffer(frame, cam_buf);
        else
            mpp_frame_set_buffer(frame, buffer);

        meta = mpp_frame_get_meta(frame);

        if (p->osd_enable || p->user_data_enable || p->roi_enable) {
            if (p->user_data_enable) {
                MppEncUserData user_data;
                const char *str = "this is user data\n";

                if ((p->frm_cnt_in & 10) == 0) {
                    user_data.pdata = (void *)str;
                    user_data.len = strlen(str) + 1;
                    mpp_meta_set_ptr(meta, KEY_USER_DATA, &user_data);
                }
                static RK_U8 uuid_debug_info[16] = {
                    0x57, 0x68, 0x97, 0x80, 0xe7, 0x0c, 0x4b, 0x65,
                    0xa9, 0x06, 0xae, 0x29, 0x94, 0x11, 0xcd, 0x9a
                };

                MppEncUserDataSet data_group;
                MppEncUserDataFull datas[2];
                const char *str1 = "this is user data 1\n";
                const char *str2 = "this is user data 2\n";
                data_group.count = 2;
                datas[0].len = strlen(str1) + 1;
                datas[0].pdata = (void *)str1;
                datas[0].uuid = uuid_debug_info;

                datas[1].len = strlen(str2) + 1;
                datas[1].pdata = (void *)str2;
                datas[1].uuid = uuid_debug_info;

                data_group.datas = datas;

                mpp_meta_set_ptr(meta, KEY_USER_DATAS, &data_group);
            }

            if (p->osd_enable) {
                /* gen and cfg osd plt */
                mpi_enc_gen_osd_plt(&p->osd_plt, p->frm_cnt_in);

                p->osd_plt_cfg.change = MPP_ENC_OSD_PLT_CFG_CHANGE_ALL;
                p->osd_plt_cfg.type = MPP_ENC_OSD_PLT_TYPE_USERDEF;
                p->osd_plt_cfg.plt = &p->osd_plt;

                ret = mpi->control(ctx, MPP_ENC_SET_OSD_PLT_CFG, &p->osd_plt_cfg);
                if (ret) {
                    mpp_err("mpi control enc set osd plt failed ret %d\n", ret);
                    break;
                }

                /* gen and cfg osd plt */
                mpi_enc_gen_osd_data(&p->osd_data, p->buf_grp, p->width,
                                     p->height, p->frm_cnt_in);
                mpp_meta_set_ptr(meta, KEY_OSD_DATA, (void*)&p->osd_data);
            }

            if (p->roi_enable) {
                RoiRegionCfg *region = &p->roi_region;

                /* calculated in pixels */
                region->x = MPP_ALIGN(p->width / 8, 16);
                region->y = MPP_ALIGN(p->height / 8, 16);
                region->w = 128;
                region->h = 256;
                region->force_intra = 0;
                region->qp_mode = 1;
                region->qp_val = 24;

                mpp_enc_roi_add_region(p->roi_ctx, region);

                region->x = MPP_ALIGN(p->width / 2, 16);
                region->y = MPP_ALIGN(p->height / 4, 16);
                region->w = 256;
                region->h = 128;
                region->force_intra = 1;
                region->qp_mode = 1;
                region->qp_val = 10;

                mpp_enc_roi_add_region(p->roi_ctx, region);

                /* send roi info by metadata */
                mpp_enc_roi_setup_meta(p->roi_ctx, meta);
            }
        }

        if (!p->first_frm)
            p->first_frm = mpp_time();
        /*
         * NOTE: in non-block mode the frame can be resent.
         * The default input timeout mode is block.
         *
         * User should release the input frame to meet the requirements of
         * resource creator must be the resource destroyer.
         */
        p->frm_cnt_in++;
        do {
            ret = mpi->encode_put_frame(ctx, frame);
            if (ret)
                msleep(1);
        } while (ret);

        if (cam_frm_idx >= 0)
            camera_source_put_frame(p->cam_ctx, cam_frm_idx);

        if (p->frame_num > 0 && p->frm_cnt_in >= p->frame_num) {
            p->frm_eos = 1;
            break;
        }

        if (p->loop_end) {
            p->frm_eos = 1;
            break;
        }

        if (p->frm_eos)
            break;
    }

    return NULL;
}

void *enc_test_output(void *arg)
{
    MpiEncMtCtxInfo *info = (MpiEncMtCtxInfo *)arg;
    MppEncTestObjSet *obj_set = info->obj_set;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MpiEncTestData *p = &info->ctx;
    MpiEncMtTestPriv *priv = &info->priv;
    MpiEncMtCtxRet *enc_ret = &info->ret;
    MppList *list_buf = priv->list_buf;
    RK_S32 chn = info->chn;
    MppApi *mpi = p->mpi;
    MppCtx ctx = p->ctx;
    RK_U32 quiet = cmd->quiet;
    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    RK_U32 eoi = 1;

    void *ptr;
    size_t len;
    char log_buf[256];
    RK_S32 log_size = sizeof(log_buf) - 1;
    RK_S32 log_len = 0;

    while (1) {
        ret = mpi->encode_get_packet(ctx, &packet);
        if (ret || NULL == packet) {
            msleep(1);
            continue;
        }

        p->last_pkt = mpp_time();

        // write packet to file here
        ptr = mpp_packet_get_pos(packet);
        len = mpp_packet_get_length(packet);
        log_size = sizeof(log_buf) - 1;
        log_len = 0;

        if (!p->first_pkt)
            p->first_pkt = mpp_time();

        p->pkt_eos = mpp_packet_get_eos(packet);

        if (p->fp_output[chn])
            fwrite(ptr, 1, len, p->fp_output[chn]);

        log_len += snprintf(log_buf + log_len, log_size - log_len,
                            "encoded frame %-4d", p->frm_cnt_out);

        /* for low delay partition encoding */
        if (mpp_packet_is_partition(packet)) {
            eoi = mpp_packet_is_eoi(packet);

            log_len += snprintf(log_buf + log_len, log_size - log_len,
                                " pkt %d", p->frm_pkt_cnt);
            p->frm_pkt_cnt = (eoi) ? (0) : (p->frm_pkt_cnt + 1);
        }

        log_len += snprintf(log_buf + log_len, log_size - log_len,
                            " size %-7zu", len);

        if (mpp_packet_has_meta(packet)) {
            MppMeta meta = mpp_packet_get_meta(packet);
            MppFrame frm = NULL;
            RK_S32 temporal_id = 0;
            RK_S32 lt_idx = -1;
            RK_S32 avg_qp = -1;

            if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    " tid %d", temporal_id);

            if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    " lt %d", lt_idx);

            if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    " qp %d", avg_qp);

            if (MPP_OK == mpp_meta_get_frame(meta, KEY_INPUT_FRAME, &frm)) {
                MppBuffer frm_buf = NULL;

                mpp_assert(frm);
                frm_buf = mpp_frame_get_buffer(frm);

                if (frm_buf) {
                    mpp_mutex_cond_lock(&list_buf->cond_lock);
                    mpp_list_add_at_tail(list_buf, &frm_buf, sizeof(frm_buf));
                    mpp_list_signal(list_buf);
                    mpp_mutex_cond_unlock(&list_buf->cond_lock);
                }

                mpp_frame_deinit(&frm);
            }
        }

        mpp_log_q(quiet, "chn %d %s\n", chn, log_buf);

        mpp_packet_deinit(&packet);
        fps_calc_inc(cmd->fps);

        p->stream_size += len;
        p->frm_cnt_out += eoi;

        if (p->frm_cnt_out != p->frm_cnt_in)
            continue;

        if (p->frame_num > 0 && p->frm_cnt_out >= p->frame_num) {
            p->pkt_eos = 1;
            break;
        }

        if (p->frm_eos) {
            p->pkt_eos = 1;
            break;
        }

        if (p->pkt_eos) {
            mpp_log_q(quiet, "chn %d found last packet\n", chn);
            mpp_assert(p->frm_eos);
            break;
        }
    } while (!eoi);

    enc_ret->elapsed_time = p->last_pkt - p->first_frm;
    enc_ret->frame_count = p->frm_cnt_out;
    enc_ret->stream_size = p->stream_size;
    enc_ret->frame_rate = (float)p->frm_cnt_out * 1000000 / enc_ret->elapsed_time;
    enc_ret->bit_rate = (p->stream_size * 8 * (p->fps_out_num / p->fps_out_den)) / p->frm_cnt_out;
    enc_ret->delay = p->first_pkt - p->first_frm;

    return NULL;
}

int enc_test_mt(MppEncTestObjSet *obj_set, const char *name)
{
    MpiEncMtCtxInfo *ctxs = NULL;
    MpiEncTestArgs *cmd = obj_set->cmd;
    float total_rate = 0.0;
    RK_S32 ret = MPP_NOK;
    RK_S32 i = 0;

    ctxs = mpp_calloc(MpiEncMtCtxInfo, cmd->nthreads);
    if (NULL == ctxs) {
        mpp_err("failed to alloc context for instances\n");
        return -1;
    }

    for (i = 0; i < cmd->nthreads; i++) {
        ctxs[i].obj_set = obj_set;
        ctxs[i].name = name;
        ctxs[i].chn = i;

        ret = mpi_enc_ctx_init(&ctxs[i].ctx, cmd, i);
        if (ret) {
            mpp_err_f("test ctx init failed ret %d\n", ret);
            return ret;
        }

        ret = mt_test_res_init(&ctxs[i]);
        if (ret) {
            mpp_err_f("test resource deinit failed ret %d\n", ret);
            return ret;
        }

        ret = pthread_create(&ctxs[i].thd_out, NULL, enc_test_output, &ctxs[i]);
        if (ret) {
            mpp_err("failed to create thread %d\n", i);
            return ret;
        }

        ret = pthread_create(&ctxs[i].thd_in, NULL, enc_test_input, &ctxs[i]);
        if (ret) {
            mpp_err("failed to create thread %d\n", i);
            return ret;
        }
    }


    if (cmd->frame_num < 0) {
        // wait for input then quit encoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop encoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        mpp_log_f("loop_end start");
        for (i = 0; i < cmd->nthreads; i++)
            ctxs[i].ctx.loop_end = 1;
    }

    for (i = 0; i < cmd->nthreads; i++) {
        pthread_join(ctxs[i].thd_in, NULL);
        pthread_join(ctxs[i].thd_out, NULL);

        ret = mt_test_res_deinit(&ctxs[i]);
        if (ret) {
            mpp_err_f("test resource deinit failed ret %d\n", ret);
            return ret;
        }

        ret = mpi_enc_ctx_deinit(&ctxs[i].ctx);
        if (ret) {
            mpp_err_f("test ctx deinit failed ret %d\n", ret);
            return ret;
        }
    }

    for (i = 0; i < cmd->nthreads; i++) {
        MpiEncMtCtxRet *enc_ret = &ctxs[i].ret;

        mpp_log("chn %d encode %d frames time %lld ms delay %3d ms fps %3.2f bps %lld\n",
                i, enc_ret->frame_count, (RK_S64)(enc_ret->elapsed_time / 1000),
                (RK_S32)(enc_ret->delay / 1000), enc_ret->frame_rate, enc_ret->bit_rate);

        total_rate += enc_ret->frame_rate;
    }

    MPP_FREE(ctxs);

    total_rate /= cmd->nthreads;
    mpp_log("%s average frame rate %.2f\n", name, total_rate);

    return ret;
}

int main(int argc, char **argv)
{
    MppEncTestObjSet obj_set;
    RK_S32 ret = MPP_NOK;

    memset(&obj_set, 0, sizeof(obj_set));
    // parse the cmd option
    ret = mpi_enc_test_objset_update_by_args(&obj_set, argc, argv);
    if (ret)
        goto DONE;

    mpp_enc_args_dump(obj_set.cmd_obj, MODULE_TAG);

    ret = enc_test_mt(&obj_set, argv[0]);

DONE:
    if (obj_set.cmd_obj)
        mpi_enc_test_cmd_put(&obj_set);
    if (obj_set.cfg_obj)
        mpp_enc_cfg_deinit(obj_set.cfg_obj);

    return ret;
}
