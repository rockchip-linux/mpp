/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_enc_test"

#include <string.h>
#include <math.h>
#include "rk_mpi.h"
#include "rk_venc_kcfg.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_soc.h"

#include "utils.h"
#include "mpi_enc_utils.h"
#include "camera_source.h"
#include "mpp_enc_roi_utils.h"
#include "mpp_rc_api.h"
#include "osd3_test.h"
#include "kmpp_obj.h"

/* For each instance thread return value */
typedef struct {
    float           frame_rate;
    RK_U64          bit_rate;
    RK_S64          elapsed_time;
    RK_S32          frame_count;
    RK_S64          stream_size;
    RK_S64          delay;
} MpiEncMultiCtxRet;

typedef struct MpiEncTestPriv_t {
    MppBuffer frm_buf;
    MppBuffer pkt_buf;
    MppBuffer md_info;
} MpiEncTestPriv;

typedef struct {
    MppEncTestObjSet    *obj_set;
    const char          *name;
    RK_S32              chn;

    pthread_t           thd;        // thread for for each instance
    MpiEncTestData      ctx;        // context of encoder
    MpiEncTestPriv      priv;       // private data for encoder
    MpiEncMultiCtxRet   ret;        // return of encoder
} MpiEncMultiCtxInfo;

static RK_S32 get_mdinfo_size(MpiEncTestData *p, MppCodingType type)
{
    RockchipSocType soc_type = mpp_get_soc_type();
    RK_S32 md_size;
    RK_U32 w = p->hor_stride, h = p->ver_stride;

    if (soc_type == ROCKCHIP_SOC_RV1126B) {
        md_size = (MPP_VIDEO_CodingHEVC == type) ?
                  (MPP_ALIGN(w, 32) >> 5) * (MPP_ALIGN(h, 32) >> 5) * 20 :
                  (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 16) >> 4) * 16;
    } else if (soc_type == ROCKCHIP_SOC_RK3588) {
        md_size = (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 64) >> 6) * 32;
    } else {
        md_size = (MPP_VIDEO_CodingHEVC == type) ?
                  (MPP_ALIGN(w, 32) >> 5) * (MPP_ALIGN(h, 32) >> 5) * 16 :
                  (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 16) >> 4) * 16;
    }

    return md_size;
}

static MPP_RET kmpp_cfg_init(MpiEncMultiCtxInfo *info)
{
    MppVencKcfg init_kcfg = NULL;
    MpiEncTestData *p = &info->ctx;
    MpiEncTestArgs *cmd = info->obj_set->cmd;
    MPP_RET ret = MPP_NOK;

    mpp_venc_kcfg_init(&init_kcfg, MPP_VENC_KCFG_TYPE_INIT);
    if (!init_kcfg) {
        mpp_err_f("kmpp_venc_init_cfg_init failed\n");
        return ret;
    }

    p->init_kcfg = init_kcfg;

    mpp_venc_kcfg_set_u32(init_kcfg, "type", MPP_CTX_ENC);
    mpp_venc_kcfg_set_u32(init_kcfg, "coding", p->type);
    mpp_venc_kcfg_set_s32(init_kcfg, "chan_id", 0);
    mpp_venc_kcfg_set_s32(init_kcfg, "online", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "buf_size", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_strm_cnt", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "shared_buf_en", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "smart_en", cmd->rc_mode == MPP_ENC_RC_MODE_SMTRC);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_width", p->width);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_height", p->height);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_lt_cnt", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "qpmap_en", cmd->deblur_en);
    mpp_venc_kcfg_set_u32(init_kcfg, "chan_dup", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "tmvp_enable", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "only_smartp", 0);
    /* set notify mode to zero to disable rockit ko call back */
    mpp_venc_kcfg_set_u32(init_kcfg, "ntfy_mode", 0);
    /* set input timeout to block mode to insure put_frame ioctl return while encoding finished */
    mpp_venc_kcfg_set_s32(init_kcfg, "input_timeout", MPP_POLL_BLOCK);

    ret = p->mpi->control(p->ctx, MPP_SET_VENC_INIT_KCFG, init_kcfg);
    if (ret)
        mpp_err_f("mpi control set kmpp enc cfg failed ret %d\n", ret);

    return ret;
}

MPP_RET test_mpp_run(MpiEncMultiCtxInfo *info)
{
    MppEncTestObjSet *obj_set = info->obj_set;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MpiEncTestData *p = &info->ctx;
    MpiEncTestPriv *priv = &info->priv;
    MppApi *mpi = p->mpi;
    MppCtx ctx = p->ctx;
    RK_U32 quiet = cmd->quiet;
    RK_S32 chn = info->chn;
    RK_U32 cap_num = 0;
    DataCrc checkcrc;
    MPP_RET ret = MPP_OK;
    RK_FLOAT psnr_const = 0;
    RK_U32 sse_unit_in_pixel = 0;
    RK_U32 soc_type;

    memset(&checkcrc, 0, sizeof(checkcrc));
    checkcrc.sum = mpp_malloc(RK_ULONG, 512);
    soc_type = mpp_get_soc_type();

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        MppPacket packet = NULL;

        /*
         * Can use packet with normal malloc buffer as input not pkt_buf.
         * Please refer to vpu_api_legacy.cpp for normal buffer case.
         * Using pkt_buf buffer here is just for simplifing demo.
         */
        mpp_packet_init_with_buffer(&packet, priv->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);

        ret = mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
            goto RET;
        } else {
            /* get and write sps/pps for H.264 */

            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);

            if (p->fp_output[chn])
                fwrite(ptr, 1, len, p->fp_output[chn]);
        }

        mpp_packet_deinit(&packet);

        sse_unit_in_pixel = p->type == MPP_VIDEO_CodingAVC ? 16 : 8;
        psnr_const = (16 + log2(MPP_ALIGN(p->width, sse_unit_in_pixel) *
                                MPP_ALIGN(p->height, sse_unit_in_pixel)));
    }
    while (!p->pkt_eos) {
        MppMeta meta = NULL;
        MppFrame frame = NULL;
        MppPacket packet = NULL;
        void *buf = mpp_buffer_get_ptr(priv->frm_buf);
        RK_S32 cam_frm_idx = -1;
        MppBuffer cam_buf = NULL;
        RK_U32 eoi = 1;

        if (p->fp_input) {
            mpp_buffer_sync_begin(priv->frm_buf);
            ret = read_image(buf, p->fp_input, p->width, p->height,
                             p->hor_stride, p->ver_stride, p->fmt);
            if (ret == MPP_NOK || feof(p->fp_input)) {
                p->frm_eos = 1;

                if (p->frame_num < 0) {
                    clearerr(p->fp_input);
                    rewind(p->fp_input);
                    p->frm_eos = 0;
                    mpp_log_q(quiet, "chn %d loop times %d\n", chn, ++p->loop_times);
                    continue;
                }
                mpp_log_q(quiet, "chn %d found last frame. feof %d\n", chn, feof(p->fp_input));
            } else if (ret == MPP_ERR_VALUE)
                goto RET;
            mpp_buffer_sync_end(priv->frm_buf);
        } else {
            if (p->cam_ctx == NULL) {
                mpp_buffer_sync_begin(priv->frm_buf);
                ret = fill_image(buf, p->width, p->height, p->hor_stride,
                                 p->ver_stride, p->fmt, p->frm_cnt_out * p->frm_step);
                if (ret)
                    goto RET;
                mpp_buffer_sync_end(priv->frm_buf);
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
            goto RET;
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
            mpp_frame_set_buffer(frame, priv->frm_buf);

        meta = mpp_frame_get_meta(frame);
        mpp_packet_init_with_buffer(&packet, priv->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);
        mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
        mpp_meta_set_buffer(meta, KEY_MOTION_INFO, priv->md_info);

        if (cmd->osd_enable || cmd->user_data_enable || cmd->roi_enable || cmd->roi_jpeg_enable) {
            if (cmd->user_data_enable) {
                MppEncUserData user_data;
                char *str = "this is user data\n";

                if ((p->frm_cnt_out & 10) == 0) {
                    user_data.pdata = str;
                    user_data.len = strlen(str) + 1;
                    mpp_meta_set_ptr(meta, KEY_USER_DATA, &user_data);
                }
                static RK_U8 uuid_debug_info[16] = {
                    0x57, 0x68, 0x97, 0x80, 0xe7, 0x0c, 0x4b, 0x65,
                    0xa9, 0x06, 0xae, 0x29, 0x94, 0x11, 0xcd, 0x9a
                };

                MppEncUserDataSet data_group;
                MppEncUserDataFull datas[2];
                char *str1 = "this is user data 1\n";
                char *str2 = "this is user data 2\n";
                data_group.count = 2;
                datas[0].len = strlen(str1) + 1;
                datas[0].pdata = str1;
                datas[0].uuid = uuid_debug_info;

                datas[1].len = strlen(str2) + 1;
                datas[1].pdata = str2;
                datas[1].uuid = uuid_debug_info;

                data_group.datas = datas;

                mpp_meta_set_ptr(meta, KEY_USER_DATAS, &data_group);
            }

            if (cmd->osd_enable) {
                if (soc_type == ROCKCHIP_SOC_RV1126B) {
                    /* osd for RV1126B use struct MppEncOSDData3 */
                    RK_S32 osd_case;
                    if (!p->osd_pattern) {
                        osd3_gen_smpte_bar_argb(&p->osd_pattern);
                    }

                    if (p->type == MPP_VIDEO_CodingMJPEG) {
                        osd_case = cmd->jpeg_osd_case;
                    } else {
                        osd_case = p->frm_cnt_out;
                    }

                    osd3_get_test_case(&p->osd_data3, p->osd_pattern,
                                       (RK_U32)osd_case % OSD_CASE_BUTT, &p->osd_buffer);

                    mpp_meta_set_ptr(meta, KEY_OSD_DATA3, (void*)&p->osd_data3);
                } else {
                    /* gen and cfg osd plt */
                    mpi_enc_gen_osd_plt(&p->osd_plt, p->frm_cnt_out);

                    p->osd_plt_cfg.change = MPP_ENC_OSD_PLT_CFG_CHANGE_ALL;
                    p->osd_plt_cfg.type = MPP_ENC_OSD_PLT_TYPE_USERDEF;
                    p->osd_plt_cfg.plt = &p->osd_plt;

                    ret = mpi->control(ctx, MPP_ENC_SET_OSD_PLT_CFG, &p->osd_plt_cfg);
                    if (ret) {
                        mpp_err("mpi control enc set osd plt failed ret %d\n", ret);
                        goto RET;
                    }

                    /* gen and cfg osd plt */
                    mpi_enc_gen_osd_data(&p->osd_data, p->buf_grp, p->width,
                                         p->height, p->frm_cnt_out);
                    mpp_meta_set_ptr(meta, KEY_OSD_DATA, (void*)&p->osd_data);
                }
            }

            if (cmd->roi_enable) {
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

            if (cmd->roi_jpeg_enable) {
                RK_U32 index;
                RK_U32 width = 128;
                RK_U32 height = 128;
                RK_U32 start_x = 0;
                RK_U32 start_y = 0;

                p->roi_jpeg_cfg.change = 1;
                p->roi_jpeg_cfg.non_roi_en = 1;
                p->roi_jpeg_cfg.non_roi_level = 0;

                for (index = 0; index < 16; index++) {
                    if ((start_x + width) > p->width || (start_y + height) > p->height)
                        break;
                    p->roi_jpeg_cfg.regions[index].roi_en = 1;
                    p->roi_jpeg_cfg.regions[index].x = start_x;
                    p->roi_jpeg_cfg.regions[index].y = start_y;
                    p->roi_jpeg_cfg.regions[index].w = width;
                    p->roi_jpeg_cfg.regions[index].h = height;
                    p->roi_jpeg_cfg.regions[index].level = 63;

                    start_x += width;
                    start_y += height;
                }

                if (cmd->kmpp_en)
                    ret = mpi->control(ctx, MPP_ENC_SET_JPEG_ROI_CFG, &p->roi_jpeg_cfg);
                else
                    mpp_meta_set_ptr(meta, KEY_JPEG_ROI_DATA, (void*)&p->roi_jpeg_cfg);
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
        ret = mpi->encode_put_frame(ctx, frame);
        if (ret) {
            mpp_err("chn %d encode put frame failed\n", chn);
            mpp_frame_deinit(&frame);
            goto RET;
        }

        mpp_frame_deinit(&frame);

        do {
            ret = mpi->encode_get_packet(ctx, &packet);
            if (ret) {
                mpp_err("chn %d encode get packet failed\n", chn);
                goto RET;
            }

            mpp_assert(packet);

            if (packet) {
                // write packet to file here
                void *ptr   = mpp_packet_get_pos(packet);
                size_t len  = mpp_packet_get_length(packet);
                char log_buf[256];
                RK_S32 log_size = sizeof(log_buf) - 1;
                RK_S32 log_len = 0;

                if (!p->first_pkt)
                    p->first_pkt = mpp_time();

                p->pkt_eos = mpp_packet_get_eos(packet);

                if (p->fp_output[chn])
                    fwrite(ptr, 1, len, p->fp_output[chn]);

                if (p->fp_verify && !p->pkt_eos) {
                    calc_data_crc((RK_U8 *)ptr, (RK_U32)len, &checkcrc);
                    mpp_log("p->frm_cnt_out=%d, len=%d\n", p->frm_cnt_out, len);
                    write_data_crc(p->fp_verify, &checkcrc);
                }

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
                    meta = mpp_packet_get_meta(packet);
                    RK_S32 temporal_id = 0;
                    RK_S32 lt_idx = -1;
                    RK_S32 avg_qp = -1, bps_rt = -1;
                    RK_S32 use_lt_idx = -1;
                    RK_S64 sse = 0;
                    RK_FLOAT psnr = 0;

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " tid %d", temporal_id);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " lt %d", lt_idx);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " qp %2d", avg_qp);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_BPS_RT, &bps_rt))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " bps_rt %d", bps_rt);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_USE_LTR, &use_lt_idx))
                        log_len += snprintf(log_buf + log_len, log_size - log_len, " vi");

                    if (MPP_OK == mpp_meta_get_s64(meta, KEY_ENC_SSE, &sse)) {
                        psnr = 3.01029996 * (psnr_const - log2(sse));
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " psnr %.4f", psnr);
                    }
                }

                mpp_log_q(quiet, "chn %d %s\n", chn, log_buf);

                mpp_packet_deinit(&packet);
                fps_calc_inc(cmd->fps);

                p->stream_size += len;
                p->frm_cnt_out += eoi;

                if (p->pkt_eos) {
                    mpp_log_q(quiet, "chn %d found last packet\n", chn);
                    mpp_assert(p->frm_eos);
                }
            }
        } while (!eoi);

        if (cam_frm_idx >= 0)
            camera_source_put_frame(p->cam_ctx, cam_frm_idx);

        if (p->frame_num > 0 && p->frm_cnt_out >= p->frame_num)
            break;

        if (p->loop_end)
            break;

        if (p->frm_eos && p->pkt_eos)
            break;
    }
RET:
    MPP_FREE(checkcrc.sum);

    return ret;
}

void *enc_test(void *arg)
{
    MpiEncMultiCtxInfo *info = (MpiEncMultiCtxInfo *)arg;
    MppEncTestObjSet *obj_set = info->obj_set;
    MpiEncTestArgs *cmd = obj_set->cmd;
    MpiEncTestData *p = &info->ctx;
    MpiEncTestPriv *priv = &info->priv;
    MpiEncMultiCtxRet *enc_ret = &info->ret;
    MppPollType timeout = MPP_POLL_BLOCK;
    RK_U32 quiet = cmd->quiet;
    MPP_RET ret = MPP_OK;
    RK_S64 t_s = 0;
    RK_S64 t_e = 0;
    size_t mdinfo_size;

    mpp_log_q(quiet, "%s start\n", info->name);

    ret = mpi_enc_ctx_init(p, cmd, info->chn);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_group_get_internal(&p->buf_grp, MPP_BUFFER_TYPE_DRM | MPP_BUFFER_FLAGS_CACHABLE);
    if (ret) {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &priv->frm_buf, p->frame_size + p->header_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &priv->pkt_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mdinfo_size = get_mdinfo_size(p, cmd->type);
    ret = mpp_buffer_get(p->buf_grp, &priv->md_info, mdinfo_size);
    if (ret) {
        mpp_err_f("failed to get buffer for motion info output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mpp_log_q(quiet, "%p encoder test start w %d h %d type %d\n",
              p->ctx, p->width, p->height, p->type);

    ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        goto MPP_TEST_OUT;
    }

    if (cmd->kmpp_en)
        kmpp_cfg_init(info);

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    if (cmd->kmpp_en)
        ret = mpp_enc_cfg_init_k(&p->cfg);
    else
        ret = mpp_enc_cfg_init(&p->cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->control(p->ctx, MPP_ENC_GET_CFG, p->cfg);
    if (ret) {
        mpp_err_f("get enc cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpi_enc_cfg_setup(p, cmd, obj_set->cfg_obj);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    t_s = mpp_time();
    ret = test_mpp_run(info);
    t_e = mpp_time();
    if (ret) {
        mpp_err_f("test mpp run failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->reset(p->ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

    enc_ret->elapsed_time = t_e - t_s;
    enc_ret->frame_count = p->frm_cnt_out;
    enc_ret->stream_size = p->stream_size;
    enc_ret->frame_rate = (float)p->frm_cnt_out * 1000000 / enc_ret->elapsed_time;
    enc_ret->bit_rate = (p->stream_size * 8 * (cmd->fps_out_num / cmd->fps_out_den)) / p->frm_cnt_out;
    enc_ret->delay = p->first_pkt - p->first_frm;

MPP_TEST_OUT:
    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->cfg) {
        mpp_enc_cfg_deinit(p->cfg);
        p->cfg = NULL;
    }

    if (priv->frm_buf) {
        mpp_buffer_put(priv->frm_buf);
        priv->frm_buf = NULL;
    }

    if (priv->pkt_buf) {
        mpp_buffer_put(priv->pkt_buf);
        priv->pkt_buf = NULL;
    }

    if (priv->md_info) {
        mpp_buffer_put(priv->md_info);
        priv->md_info = NULL;
    }

    if (p->osd_data.buf) {
        mpp_buffer_put(p->osd_data.buf);
        p->osd_data.buf = NULL;
    }

    if (p->osd_buffer) {
        kmpp_obj_put_f(p->osd_buffer);
        p->osd_buffer = NULL;
    }

    if (p->buf_grp) {
        mpp_buffer_group_put(p->buf_grp);
        p->buf_grp = NULL;
    }

    if (p->roi_ctx) {
        mpp_enc_roi_deinit(p->roi_ctx);
        p->roi_ctx = NULL;
    }
    if (p->init_kcfg)
        mpp_venc_kcfg_deinit(p->init_kcfg);

    if (p->osd_pattern) {
        free(p->osd_pattern);
        p->osd_pattern = NULL;
    }

    mpi_enc_ctx_deinit(p);

    return NULL;
}

int enc_test_multi(MppEncTestObjSet* obj_set, const char *name)
{
    MpiEncMultiCtxInfo *ctxs = NULL;
    MpiEncTestArgs *cmd = obj_set->cmd;
    float total_rate = 0.0;
    RK_S32 ret = MPP_NOK;
    RK_S32 i = 0;

    ctxs = mpp_calloc(MpiEncMultiCtxInfo, cmd->nthreads);
    if (NULL == ctxs) {
        mpp_err("failed to alloc context for instances\n");
        return -1;
    }

    for (i = 0; i < cmd->nthreads; i++) {
        ctxs[i].obj_set = obj_set;
        ctxs[i].name = name;
        ctxs[i].chn = i;

        ret = pthread_create(&ctxs[i].thd, NULL, enc_test, &ctxs[i]);
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
        for (i = 0; i < cmd->nthreads; i++)
            ctxs[i].ctx.loop_end = 1;
    }

    for (i = 0; i < cmd->nthreads; i++)
        pthread_join(ctxs[i].thd, NULL);

    for (i = 0; i < cmd->nthreads; i++) {
        MpiEncMultiCtxRet *enc_ret = &ctxs[i].ret;

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
    MppEncTestObjSet *obj_set = NULL;
    RK_S32 ret;

    ret = mpi_enc_test_objset_get(&obj_set);
    if (ret)
        goto DONE;

    // parse the cmd option
    ret = mpi_enc_test_objset_update_by_args(obj_set, argc, argv, MODULE_TAG);
    if (ret)
        goto DONE;

    ret = enc_test_multi(obj_set, argv[0]);

DONE:
    mpi_enc_test_objset_put(obj_set);

    return ret;
}
