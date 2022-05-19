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

#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "rk_mpi_cmd.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_thread.h"
#include "mpp_common.h"

#include "mpp_impl.h"

#define MAX_FILE_NAME_LEN   512
#define MAX_DUMP_WIDTH      960
#define MAX_DUMP_HEIGHT     540

#define ops_log(fp, fmt, ...)   _ops_log(fp, fmt, ## __VA_ARGS__)

typedef enum MppOpsType_e {
    MPP_DEC_OPS_BASE        = 0,
    MPP_DEC_INIT            = MPP_DEC_OPS_BASE,
    MPP_DEC_SE
} MppOpsType;

/* dump data */
typedef struct MppDumpImpl_t {
    Mutex                   *lock;
    RK_U32                  log_version;
    RK_U32                  debug;
    pid_t                   tid;
    RK_S64                  time_base;

    MppCtxType              type;
    MppCodingType           coding;

    FILE                    *fp_in;    // file for MppPacket
    FILE                    *fp_out;    // file for MppFrame
    FILE                    *fp_ops;    // file for decoder / encoder extra info

    RK_U8                   *fp_buf;    // for resample frame
    RK_U32                  pkt_offset;
    RK_U32                  dump_width;
    RK_U32                  dump_height;
    RK_U32                  dump_size;

    RK_U32                  idx;
} MppDumpImpl;

typedef struct MppOpsInfo_t {
    RK_U32                  idx;
    RK_U32                  id;
    RK_S64                  time;

    union OpsArgs_u {
        struct MppOpsInitArg_t {
            MppCtxType      type;
            MppCodingType   coding;
        };

        struct MppOpsPktArg_t {
            RK_U32          offset;         // offset in packet file
            RK_U32          length;         // pakcet length
            RK_S64          pts;
        };

        struct MppOpsFrmArg_t {
            RK_U32          fd;
            RK_U32          info_change;
            RK_U32          error;
            RK_U32          discard;
            RK_S64          pts;
        };

        struct MppOpCtrlArg_t {
            MpiCmd          cmd;
        };
    };
} MppOpsInfo;

/*
 * decoder file dump:
 * dec_pkt - decoder input byte raw stream file
 * dec_cfg - decoder input stream offset and size info in format [u32 offset|u32 size]
 * dec_frm - decoder output frame file with snapshot method
 */
static const char dec_pkt_path[] = "/data/mpp_dec_in.bin";
static const char dec_ops_path[] = "/data/mpp_dec_ops.bin";
static const char dec_frm_path[] = "/data/mpp_dec_out.bin";

static const char enc_frm_path[] = "/data/mpp_enc_in.bin";
static const char enc_ops_path[] = "/data/mpp_enc_ops.bin";
static const char enc_pkt_path[] = "/data/mpp_enc_out.bin";

static FILE *try_env_file(const char *env, const char *path, pid_t tid)
{
    const char *fname = NULL;
    FILE *fp = NULL;
    char name[MAX_FILE_NAME_LEN];

    mpp_env_get_str(env, &fname, path);
    if (fname == path) {
        snprintf(name, sizeof(name) - 1, "%s-%d", path, tid);
        fname = name;
    }

    fp = fopen(fname, "w+b");
    mpp_log("open %s %p for dump\n", fname, fp);

    return fp;
}

static RK_U8 fetch_data(RK_U32 fmt, RK_U8 *line, RK_U32 num)
{
    RK_U32 offset = 0;
    RK_U32 value = 0;

    if (fmt == MPP_FMT_YUV420SP_10BIT) {
        offset = (num * 2) & 7;
        value = (line[num * 10 / 8] >> offset) |
                (line[num * 10 / 8 + 1] << (8 - offset));

        value = (value & 0x3ff) >> 2;
    } else if (fmt == MPP_FMT_YUV420SP) {
        value = line[num];
    }

    return RK_U8(value);
}

static void dump_frame(FILE *fp, MppFrame frame, RK_U8 *tmp, RK_U32 w, RK_U32 h)
{
    RK_U32 i = 0, j = 0;
    RK_U32 fmt = mpp_frame_get_fmt(frame);
    RK_U32 width = mpp_frame_get_width(frame);
    RK_U32 height = mpp_frame_get_height(frame);
    RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
    RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
    RK_U8 *p_buf = (RK_U8 *) mpp_buffer_get_ptr(mpp_frame_get_buffer(frame));

    RK_U8 *psrc = p_buf;
    RK_U8 *pdes = tmp;
    RK_U32 size = 0;

    if (pdes) {
        if (hor_stride > w || ver_stride > h) {
            RK_U32 step = MPP_MAX((hor_stride + w - 1) / w,
                                  (ver_stride + h - 1) / h);
            RK_U32 img_w = width / step;
            RK_U32 img_h = height / step;

            img_w -= img_w & 0x1;
            img_h -= img_h & 0x1;
            for (i = 0; i < img_h; i++) {
                for (j = 0; j < img_w; j++)
                    pdes[j] = fetch_data(fmt, psrc, j * step);
                pdes += img_w;
                psrc += step * hor_stride;
            }
            psrc = p_buf + hor_stride * ver_stride;
            pdes = tmp + img_w * img_h;
            for (i = 0; i < (img_h / 2); i++) {
                for (j = 0; j < (img_w / 2); j++) {
                    pdes[2 * j + 0] = fetch_data(fmt, psrc, 2 * j * step + 0);
                    pdes[2 * j + 1] = fetch_data(fmt, psrc, 2 * j * step + 1);
                }
                pdes += img_w;
                psrc += step * hor_stride;
            }
            width = img_w;
            height = img_h;
        } else if (fmt == MPP_FMT_YUV420SP_10BIT) {
            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++)
                    pdes[j] = fetch_data(fmt, psrc, j);
                pdes += width;
                psrc += hor_stride;
            }
            psrc = p_buf + hor_stride * ver_stride;
            pdes = tmp + width * height;
            for (i = 0; i < height / 2; i++) {
                for (j = 0; j < width; j++)
                    pdes[j] = fetch_data(fmt, psrc, j);
                pdes += width;
                psrc += hor_stride;
            }
        }
        size = width * height * 1.5;
    } else {
        tmp = p_buf;
        switch (fmt) {
        case MPP_FMT_YUV420SP :
        case MPP_FMT_YUV420P : {
            size = hor_stride * ver_stride * 3 / 2;
        } break;
        case MPP_FMT_YUV422SP : {
            size = hor_stride * ver_stride * 2;
        } break;
        case MPP_FMT_YUV444SP : {
            size = hor_stride * ver_stride * 3;
        } break;
        default : break;
        }
    }
    mpp_log("dump_yuv: w:h [%d:%d] stride [%d:%d] pts %lld\n",
            width, height, hor_stride, ver_stride, mpp_frame_get_pts(frame));
    fwrite(tmp, 1, size, fp);
    fflush(fp);
}

void _ops_log(FILE *fp, const char *fmt, ...)
{
    struct tm tm_buf;
    struct tm* ptm;
    struct timespec tp;
    char logs[64];
    size_t len = 0;
    va_list args;
    va_start(args, fmt);

    clock_gettime(CLOCK_REALTIME_COARSE, &tp);
    ptm = localtime_r(&tp.tv_sec, &tm_buf);
    len = strftime(logs, sizeof(logs), "%m-%d %H:%M:%S", ptm);
    mpp_assert(len < sizeof(logs));
    fprintf(fp, "%s.%03ld,", logs, tp.tv_nsec / 1000000);
    vfprintf(fp, fmt, args);
    fflush(fp);

    va_end(args);
}

MPP_RET mpp_dump_init(MppDump *info)
{
    if (!(mpp_debug & (MPP_DBG_DUMP_IN | MPP_DBG_DUMP_OUT | MPP_DBG_DUMP_CFG))) {
        *info = NULL;
        return MPP_OK;
    }

    MppDumpImpl *p = mpp_calloc(MppDumpImpl, 1);

    mpp_env_get_u32("mpp_dump_width", &p->dump_width, 0);
    mpp_env_get_u32("mpp_dump_height", &p->dump_height, 0);
    p->dump_size = p->dump_width * p->dump_height * 3 / 2;

    p->lock = new Mutex();
    p->debug = mpp_debug;
    p->tid = syscall(SYS_gettid);
    p->log_version = 0;
    p->time_base = mpp_time();

    *info = p;

    return MPP_OK;
}

MPP_RET mpp_dump_deinit(MppDump *info)
{
    if (info && *info) {
        MppDumpImpl *p = (MppDumpImpl *)*info;

        MPP_FCLOSE(p->fp_in);
        MPP_FCLOSE(p->fp_out);
        MPP_FCLOSE(p->fp_ops);
        MPP_FREE(p->fp_buf);

        if (p->lock) {
            delete p->lock;
            p->lock = NULL;
        }
    }

    return MPP_OK;
}

MPP_RET mpp_ops_init(MppDump info, MppCtxType type, MppCodingType coding)
{
    if (NULL == info)
        return MPP_OK;

    MppDumpImpl *p = (MppDumpImpl *)info;
    AutoMutex auto_lock(p->lock);

    p->type = type;
    p->coding = coding;

    if (type == MPP_CTX_DEC) {
        if (p->debug & MPP_DBG_DUMP_IN)
            p->fp_in = try_env_file("mpp_dump_in", dec_pkt_path, p->tid);

        if (p->debug & MPP_DBG_DUMP_OUT) {
            p->fp_out = try_env_file("mpp_dump_out", dec_frm_path, p->tid);
            if (p->dump_size)
                p->fp_buf = mpp_malloc(RK_U8, p->dump_size);
        }

        if (p->debug & MPP_DBG_DUMP_CFG)
            p->fp_ops = try_env_file("mpp_dump_ops", dec_ops_path, p->tid);
    } else {
        if (p->debug & MPP_DBG_DUMP_IN) {
            p->fp_in = try_env_file("mpp_dump_in", enc_frm_path, p->tid);
            if (p->dump_size)
                p->fp_buf = mpp_malloc(RK_U8, p->dump_size);
        }

        if (p->debug & MPP_DBG_DUMP_OUT)
            p->fp_out = try_env_file("mpp_dump_out", enc_pkt_path, p->tid);

        if (p->debug & MPP_DBG_DUMP_CFG)
            p->fp_ops = try_env_file("mpp_dump_ops", enc_ops_path, p->tid);
    }

    if (p->fp_ops)
        ops_log(p->fp_ops, "%d,%s,%d,%d\n", p->idx++, "init", type, coding);

    return MPP_OK;
}

MPP_RET mpp_ops_dec_put_pkt(MppDump info, MppPacket pkt)
{
    MppDumpImpl *p = (MppDumpImpl *)info;
    if (NULL == p || NULL == pkt || NULL == p->fp_in)
        return MPP_OK;

    RK_U32 length = mpp_packet_get_length(pkt);
    AutoMutex auto_lock(p->lock);

    if (p->fp_in) {
        fwrite(mpp_packet_get_data(pkt), 1, length, p->fp_in);
        fflush(p->fp_in);
    }

    if (p->fp_ops) {
        ops_log(p->fp_ops, "%d,%s,%d,%d\n", p->idx++, "pkt", p->pkt_offset, length);

        p->pkt_offset += length;
    }

    return MPP_OK;
}

MPP_RET mpp_ops_dec_get_frm(MppDump info, MppFrame frame)
{
    MppDumpImpl *p = (MppDumpImpl *)info;
    if (NULL == p || NULL == frame || NULL == p->fp_out)
        return MPP_OK;

    AutoMutex auto_lock(p->lock);

    MppBuffer buf = mpp_frame_get_buffer(frame);
    RK_S32 fd = (buf) ? mpp_buffer_get_fd(buf) : (-1);
    RK_U32 info_change = mpp_frame_get_info_change(frame);
    RK_U32 error = mpp_frame_get_errinfo(frame);
    RK_U32 discard = mpp_frame_get_discard(frame);

    if (p->fp_ops) {
        ops_log(p->fp_ops, "%d,%s,%d,%d,%d,%d,%lld\n", p->idx, "frm", fd,
                info_change, error, discard, mpp_frame_get_pts(frame));
    }

    if (NULL == buf || fd < 0) {
        mpp_err("failed to dump frame\n");
        return MPP_NOK;
    }

    dump_frame(p->fp_out, frame, p->fp_buf, p->dump_width, p->dump_height);

    if (p->debug & MPP_DBG_DUMP_LOG) {
        RK_S64 pts = mpp_frame_get_pts(frame);
        RK_U32 width = mpp_frame_get_hor_stride(frame);
        RK_U32 height = mpp_frame_get_ver_stride(frame);

        mpp_log("yuv_info: [%d:%d] pts %lld", width, height, pts);
    }

    return MPP_OK;
}

MPP_RET mpp_ops_enc_put_frm(MppDump info, MppFrame frame)
{
    MppDumpImpl *p = (MppDumpImpl *)info;
    if (NULL == p || NULL == frame || NULL == p->fp_in)
        return MPP_OK;

    AutoMutex auto_lock(p->lock);

    dump_frame(p->fp_in, frame, p->fp_buf, p->dump_width, p->dump_height);

    if (p->debug & MPP_DBG_DUMP_LOG) {
        RK_S64 pts = mpp_frame_get_pts(frame);
        RK_U32 width = mpp_frame_get_hor_stride(frame);
        RK_U32 height = mpp_frame_get_ver_stride(frame);

        mpp_log("yuv_info: [%d:%d] pts %lld", width, height, pts);
    }

    return MPP_OK;
}

MPP_RET mpp_ops_enc_get_pkt(MppDump info, MppPacket pkt)
{
    MppDumpImpl *p = (MppDumpImpl *)info;
    if (NULL == p || NULL == pkt)
        return MPP_OK;

    RK_U32 length = mpp_packet_get_length(pkt);
    AutoMutex auto_lock(p->lock);

    if (p->fp_out) {
        fwrite(mpp_packet_get_data(pkt), 1, length, p->fp_out);
        fflush(p->fp_out);
    }

    return MPP_OK;
}

MPP_RET mpp_ops_ctrl(MppDump info, MpiCmd cmd)
{
    MppDumpImpl *p = (MppDumpImpl *)info;
    if (NULL == p)
        return MPP_OK;

    AutoMutex auto_lock(p->lock);

    if (p->fp_ops)
        ops_log(p->fp_ops, "%d,%s,%d\n", p->idx, "ctrl", cmd);

    return MPP_OK;
}

MPP_RET mpp_ops_reset(MppDump info)
{
    MppDumpImpl *p = (MppDumpImpl *)info;
    if (NULL == p)
        return MPP_OK;

    AutoMutex auto_lock(p->lock);

    if (p->fp_ops)
        ops_log(p->fp_ops, "%d,%s\n", p->idx, "rst");

    return MPP_OK;
}
