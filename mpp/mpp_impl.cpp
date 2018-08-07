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

#define MODULE_TAG "mpp_impl"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"

#include "mpp_impl.h"

#define MAX_DUMP_WIDTH      960
#define MAX_DUMP_HEIGHT     540

static const char dec_in_path[] = "/data/mpp_dec_in.bin";
static const char enc_in_path[] = "/data/enc_dec_in.bin";
static const char dec_out_path[] = "/data/mpp_dec_out.bin";
static const char enc_out_path[] = "/data/enc_dec_out.bin";

MPP_RET mpp_dump_init(MppDumpInfo *info, MppCtxType type)
{
    const char *fname = NULL;
    const char *path = NULL;
    RK_U32 dump_frame_size;

    memset(info, 0, sizeof(*info));
    info->type = type;

    mpp_env_get_u32("mpp_dump_width", &info->dump_width, MAX_DUMP_WIDTH);
    mpp_env_get_u32("mpp_dump_height", &info->dump_height, MAX_DUMP_HEIGHT);

    dump_frame_size = info->dump_width * info->dump_height * 3 / 2;

    if (mpp_debug & MPP_DBG_DUMP_IN) {
        if (type == MPP_CTX_DEC) {
            path = dec_in_path;
        } else {
            path = enc_in_path;
            info->fp_buf = mpp_malloc(RK_U8, dump_frame_size);
        }

        mpp_env_get_str("mpp_dump_in", &fname, path);
        info->fp_in = fopen(fname, "w+b");
        mpp_log("open %s %p for input dump\n", fname, info->fp_in);
    }

    if (mpp_debug & MPP_DBG_DUMP_OUT) {
        if (type == MPP_CTX_DEC) {
            path = dec_out_path;
            info->fp_buf = mpp_malloc(RK_U8, dump_frame_size);
        } else {
            path = enc_out_path;
        }

        mpp_env_get_str("mpp_dump_out", &fname, path);
        info->fp_out = fopen(fname, "w+b");
        mpp_log("open %s %p for output dump\n", fname, info->fp_out);
    }

    return MPP_OK;
}

MPP_RET mpp_dump_deinit(MppDumpInfo *info)
{
    MPP_FCLOSE(info->fp_in);
    MPP_FCLOSE(info->fp_out);
    MPP_FREE(info->fp_buf);

    return MPP_OK;
}

MPP_RET mpp_dump_packet(MppDumpInfo *info, MppPacket pkt)
{
    FILE *fp = (info->type == MPP_CTX_DEC) ? info->fp_in : info->fp_out;

    if (fp && pkt) {
        fwrite(mpp_packet_get_data(pkt), 1,
               mpp_packet_get_length(pkt), fp);
        fflush(fp);
    }

    return MPP_OK;
}

MPP_RET mpp_dump_frame(MppDumpInfo *info, MppFrame frame)
{
    RK_U32 dump_width = info->dump_width;
    RK_U32 dump_height = info->dump_height;
    RK_U8 *fp_buf = info->fp_buf;
    FILE *fp = (info->type == MPP_CTX_DEC) ? info->fp_out : info->fp_in;

    if (NULL == fp || NULL == fp_buf || NULL == frame)
        return MPP_OK;

    MppBuffer buf = mpp_frame_get_buffer(frame);
    if (NULL == buf)
        return MPP_OK;

    RK_U32 width = mpp_frame_get_hor_stride(frame);
    RK_U32 height = mpp_frame_get_ver_stride(frame);
    RK_U8 *ptr = (RK_U8 *) mpp_buffer_get_ptr(buf);

    if (width > dump_width || height > dump_height) {
        RK_U32 i = 0, j = 0, step = 0;
        RK_U32 img_w = 0, img_h = 0;
        RK_U8 *pdes = NULL, *psrc = NULL;

        step = MPP_MAX((width + dump_width - 1) / dump_width,
                       (height + dump_height - 1) / dump_height);
        img_w = width / step;
        img_h = height / step;
        pdes = fp_buf;
        psrc = ptr;
        for (i = 0; i < img_h; i++) {
            for (j = 0; j < img_w; j++) {
                pdes[j] = psrc[j * step];
            }
            pdes += img_w;
            psrc += step * width;
        }
        pdes = fp_buf + img_w * img_h;
        psrc = (RK_U8 *)ptr + width * height;
        for (i = 0; i < (img_h / 2); i++) {
            for (j = 0; j < (img_w / 2); j++) {
                pdes[2 * j + 0] = psrc[2 * j * step + 0];
                pdes[2 * j + 1] = psrc[2 * j * step + 1];
            }
            pdes += img_w;
            psrc += step * width;
        }

        fwrite(fp_buf, 1, img_w * img_h * 3 / 2, fp);

        width = img_w;
        height = img_h;
    } else {
        fwrite(ptr, 1, width * height * 3 / 2, fp);
    }

    fflush(fp);

    if (mpp_debug & MPP_DBG_DUMP_LOG) {
        RK_S64 pts = mpp_frame_get_pts(frame);

        mpp_log("dump_yuv: [%d:%d] pts %lld", width, height, pts);
    }

    return MPP_OK;
}
