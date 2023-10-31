/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpi_dec_utils"

#include <string.h>
#include <pthread.h>

#include "rk_mpi.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_lock.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_buffer.h"

#include "mpp_opt.h"
#include "mpi_dec_utils.h"

#define IVF_HEADER_LENGTH           32
#define IVF_FRAME_HEADER_LENGTH     12

#define DEFAULT_PACKET_SIZE         SZ_4K

typedef enum {
    FILE_NORMAL_TYPE,
    FILE_JPEG_TYPE,
    FILE_IVF_TYPE,
    FILE_BUTT,
} FileType;

typedef FileBufSlot *(*ReaderFunc)(FileReader data);

typedef struct FileReader_t {
    FILE            *fp_input;
    size_t          file_size;

    MppCodingType   type;
    FileType        file_type;
    char            *buf;
    size_t          buf_size;
    size_t          stuff_size;
    RK_S32          seek_base;
    ReaderFunc      read_func;

    /* return value for each read */
    size_t          read_total;
    size_t          read_size;
    MppBufferGroup  group;

    pthread_t       thd;
    volatile RK_U32 thd_stop;

    RK_U32          slot_max;
    RK_U32          slot_cnt;
    RK_U32          slot_rd_idx;
    FileBufSlot     **slots;
} FileReaderImpl;

typedef struct DecBufMgrImpl_t {
    MppDecBufMode   buf_mode;
    RK_U32          buf_count;
    RK_U32          buf_size;
    MppBufferGroup  group;
    MppBuffer       *bufs;
} DecBufMgrImpl;

#define READ_ONCE(var) (*((volatile typeof(var) *)(&(var))))

static MPP_RET add_new_slot(FileReaderImpl* impl, FileBufSlot *slot)
{
    mpp_assert(impl);

    slot->index = impl->slot_cnt;
    impl->slots[impl->slot_cnt] = slot;
    impl->slot_cnt++;

    if (impl->slot_cnt >= impl->slot_max) {
        impl->slots = mpp_realloc(impl->slots, FileBufSlot*, impl->slot_max * 2);
        if (!impl->slots)
            return MPP_NOK;

        impl->slot_max *= 2;
    }

    mpp_assert(impl->slots);
    mpp_assert(impl->slot_cnt < impl->slot_max);

    return MPP_OK;
}

static FileBufSlot *read_ivf_file(FileReader data)
{
    FileReaderImpl *impl = (FileReaderImpl*)data;
    RK_U8 ivf_data[IVF_FRAME_HEADER_LENGTH] = {0};
    size_t ivf_data_size = IVF_FRAME_HEADER_LENGTH;
    FILE *fp = impl->fp_input;
    FileBufSlot *slot = NULL;
    size_t data_size = 0;
    size_t read_size = 0;
    RK_U32 eos = 0;

    if (fread(ivf_data, 1, ivf_data_size, fp) != ivf_data_size) {
        /* end of frame queue */
        slot = mpp_calloc(FileBufSlot, 1);
        slot->eos = 1;

        return slot;
    }

    impl->read_total += ivf_data_size;
    data_size = ivf_data[0] | (ivf_data[1] << 8) | (ivf_data[2] << 16) | (ivf_data[3] << 24);
    slot = mpp_malloc_size(FileBufSlot, MPP_ALIGN(sizeof(FileBufSlot) + data_size, SZ_4K));
    slot->data = (char *)(slot + 1);
    read_size = fread(slot->data, 1, data_size, fp);
    impl->read_total += read_size;
    impl->read_size = read_size;

    if (!data_size)
        mpp_err("data_size is zero! ftell size %d\n", ftell(fp));
    /* check reach eos whether or not */
    if (!data_size || read_size != data_size || feof(fp) || impl->read_total >= impl->file_size)
        eos = 1;

    slot->buf = NULL;
    slot->size = read_size;
    slot->eos = eos;

    return slot;
}

static FileBufSlot *read_jpeg_file(FileReader data)
{
    FileReaderImpl *impl = (FileReaderImpl*)data;
    FILE *fp = impl->fp_input;
    size_t read_size = 0;
    size_t buf_size = impl->file_size;
    MppBuffer hw_buf = NULL;
    FileBufSlot *slot = mpp_calloc(FileBufSlot, 1);

    mpp_buffer_get(impl->group, &hw_buf, impl->file_size);
    mpp_assert(hw_buf);

    slot->data = mpp_buffer_get_ptr(hw_buf);
    mpp_assert(slot->data);
    read_size = fread(slot->data, 1, buf_size, fp);

    mpp_assert(read_size == buf_size);

    impl->read_total += read_size;
    impl->read_size = read_size;

    slot->buf   = hw_buf;
    slot->size  = read_size;
    slot->eos   = 1;

    return slot;
}

static FileBufSlot *read_normal_file(FileReader data)
{
    FileReaderImpl *impl = (FileReaderImpl*)data;
    FILE *fp = impl->fp_input;
    size_t read_size = 0;
    size_t buf_size = impl->buf_size;
    size_t size = sizeof(FileBufSlot) + buf_size + impl->stuff_size;
    FileBufSlot *slot = mpp_malloc_size(FileBufSlot, size);
    RK_U32 eos = 0;

    slot->data = (char *)(slot + 1);
    read_size = fread(slot->data, 1, buf_size, fp);
    impl->read_total += read_size;
    impl->read_size = read_size;

    /* check reach eos whether or not */
    if (read_size != buf_size || feof(fp) || impl->read_total >= impl->file_size)
        eos = 1;

    slot->buf = NULL;
    slot->size  = read_size;
    slot->eos   = eos;

    return slot;
}

static void check_file_type(FileReader data, char *file_in, MppCodingType type)
{
    FileReaderImpl *impl = (FileReaderImpl*)data;

    if (strstr(file_in, ".ivf")) {
        impl->file_type     = FILE_IVF_TYPE;
        impl->buf_size      = 0;
        impl->stuff_size    = 0;
        impl->seek_base     = IVF_HEADER_LENGTH;
        impl->read_func     = read_ivf_file;
        impl->slot_max      = 1024;     /* preset 1024 file slots */

        fseek(impl->fp_input, impl->seek_base, SEEK_SET);
        impl->read_total = impl->seek_base;
    } else if (strstr(file_in, ".jpg") ||
               strstr(file_in, ".jpeg") ||
               strstr(file_in, ".mjpeg") ||
               type == MPP_VIDEO_CodingMJPEG) {
        impl->file_type     = FILE_JPEG_TYPE;
        impl->buf_size      = impl->file_size;
        impl->stuff_size    = 0;
        impl->seek_base     = 0;
        impl->read_func     = read_jpeg_file;
        impl->slot_max      = 1;
        mpp_buffer_group_get_internal(&impl->group, MPP_BUFFER_TYPE_ION);
        mpp_assert(impl->group);
    } else {
        RK_U32 buf_size = 0;

        mpp_env_get_u32("reader_buf_size", &buf_size, DEFAULT_PACKET_SIZE);

        buf_size = MPP_MAX(buf_size, SZ_4K);
        buf_size = MPP_ALIGN(buf_size, SZ_4K);

        impl->file_type     = FILE_NORMAL_TYPE;
        impl->buf_size      = buf_size;
        impl->stuff_size    = 256;
        impl->seek_base     = 0;
        impl->read_func     = read_normal_file;
        impl->slot_max      = 1024;     /* preset 1024 file slots */
    }
}

size_t reader_size(FileReader reader)
{
    FileReaderImpl *impl = (FileReaderImpl*)reader;
    size_t size = 0;

    if (impl)
        size = impl->file_size;

    return size;
}

MPP_RET reader_read(FileReader reader, FileBufSlot **buf)
{
    FileReaderImpl *impl = (FileReaderImpl*)reader;
    FileBufSlot *slot = NULL;

    if (NULL == impl || NULL == impl->slots) {
        mpp_log_f("invalid reader %p\n", reader);
        return MPP_NOK;
    }

    if (impl->slot_rd_idx >= impl->slot_max) {
        mpp_log_f("invalid read index % max %d\n", impl->slot_rd_idx, impl->slot_max);
        return MPP_NOK;
    }

    do {
        slot = impl->slots[impl->slot_rd_idx];
        if (slot == NULL || (impl->slot_rd_idx > impl->slot_cnt))
            msleep(1);
    } while (slot == NULL);

    mpp_assert(slot);

    *buf  = slot;
    impl->slot_rd_idx++;

    return MPP_OK;
}

MPP_RET reader_index_read(FileReader reader, RK_S32 index, FileBufSlot **buf)
{
    FileReaderImpl *impl = (FileReaderImpl*)reader;
    FileBufSlot *slot = NULL;

    if (NULL == impl || NULL == impl->slots) {
        mpp_log_f("invalid reader %p\n", reader);
        return MPP_NOK;
    }

    if (index >= (RK_S32)impl->slot_max) {
        mpp_log_f("invalid read index % max %d\n", index, impl->slot_max);
        return MPP_NOK;
    }

    do {
        slot = impl->slots[index];
        if (slot == NULL)
            msleep(1);
    } while (slot == NULL);

    mpp_assert(slot);

    *buf  = slot;

    return MPP_OK;
}

void reader_rewind(FileReader reader)
{
    FileReaderImpl *impl = (FileReaderImpl*)reader;

    impl->slot_rd_idx = 0;
}

void reader_init(FileReader* reader, char* file_in, MppCodingType type)
{
    FILE *fp_input = fopen(file_in, "rb");
    FileReaderImpl *impl = NULL;

    if (!fp_input) {
        mpp_err("failed to open input file %s\n", file_in);
        *reader = NULL;
        return;
    }

    impl = mpp_calloc(FileReaderImpl, 1);
    mpp_assert(impl);

    impl->fp_input = fp_input;
    fseek(fp_input, 0L, SEEK_END);
    impl->file_size = ftell(fp_input);
    fseek(fp_input, 0L, SEEK_SET);

    check_file_type(impl, file_in, type);

    impl->slots = mpp_calloc(FileBufSlot*, impl->slot_max);

    reader_start(impl);

    *reader = impl;
}

void reader_deinit(FileReader reader)
{
    FileReaderImpl *impl = (FileReaderImpl*)(reader);
    RK_U32 i;

    mpp_assert(impl);
    reader_stop(impl);

    if (impl->fp_input) {
        fclose(impl->fp_input);
        impl->fp_input = NULL;
    }

    for (i = 0; i < impl->slot_cnt; i++) {
        FileBufSlot *slot = impl->slots[i];
        if (!slot)
            continue;

        if (slot->buf) {
            mpp_buffer_put(slot->buf);
            slot->buf = NULL;
        }
        MPP_FREE(impl->slots[i]);
    }

    if (impl->group) {
        mpp_buffer_group_put(impl->group);
        impl->group = NULL;
    }

    MPP_FREE(impl->slots);
    MPP_FREE(impl);
}

static void* reader_worker(void *param)
{
    FileReaderImpl *impl = (FileReaderImpl*)param;
    RK_U32 eos = 0;

    while (!impl->thd_stop && !eos) {
        FileBufSlot *slot = impl->read_func(impl);

        if (NULL == slot)
            break;

        add_new_slot(impl, slot);
        eos = slot->eos;
    }

    return NULL;
}

void reader_start(FileReader reader)
{
    FileReaderImpl *impl = (FileReaderImpl*)reader;

    impl->thd_stop = 0;
    pthread_create(&impl->thd, NULL, reader_worker, impl);
}

void reader_sync(FileReader reader)
{
    FileReaderImpl *impl = (FileReaderImpl*)reader;

    pthread_join(impl->thd, NULL);
    impl->thd_stop = 1;
}

void reader_stop(FileReader reader)
{
    FileReaderImpl *impl = (FileReaderImpl*)reader;

    if (MPP_BOOL_CAS(&impl->thd_stop, 0, 1))
        pthread_join(impl->thd, NULL);
}

void show_dec_fps(RK_S64 total_time, RK_S64 total_count, RK_S64 last_time, RK_S64 last_count)
{
    float avg_fps = (float)total_count * 1000000 / total_time;
    float ins_fps = (float)last_count * 1000000 / last_time;

    mpp_log("decoded %10lld frame fps avg %7.2f ins %7.2f\n",
            total_count, avg_fps, ins_fps);
}

RK_S32 mpi_dec_opt_i(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        strncpy(cmd->file_input, next, MAX_FILE_NAME_LENGTH - 1);
        cmd->have_input = 1;
        if (!cmd->type)
            name_to_coding_type(cmd->file_input, &cmd->type);
        return 1;
    }

    mpp_err("input file is invalid\n");
    return 0;
}

RK_S32 mpi_dec_opt_o(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        strncpy(cmd->file_output, next, MAX_FILE_NAME_LENGTH - 1);
        cmd->have_output = 1;
        return 1;
    }

    mpp_log("output file is invalid\n");
    return 0;
}

RK_S32 mpi_dec_opt_w(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        cmd->width = atoi(next);
        return 1;
    }

    mpp_err("invalid input width\n");
    return 0;
}

RK_S32 mpi_dec_opt_h(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        cmd->height = atoi(next);
        return 1;
    }

    mpp_err("invalid input height\n");
    return 0;
}

RK_S32 mpi_dec_opt_t(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        MPP_RET ret;

        cmd->type = (MppCodingType)atoi(next);
        ret = mpp_check_support_format(MPP_CTX_DEC, cmd->type);
        if (!ret)
            return 1;
    }

    mpp_err("invalid input coding type\n");
    return 0;
}

RK_S32 mpi_dec_opt_f(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        long number = 0;
        MppFrameFormat format = MPP_FMT_BUTT;

        if (MPP_OK == str_to_frm_fmt(next, &number)) {
            format = (MppFrameFormat)number;

            if (MPP_FRAME_FMT_IS_YUV(format) || MPP_FRAME_FMT_IS_RGB(format)) {
                cmd->format = format;
                return 1;
            }

            mpp_err("invalid input format 0x%x\n", format);
        }
    }

    cmd->format = MPP_FMT_YUV420SP;
    return 0;
}

RK_S32 mpi_dec_opt_n(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        cmd->frame_num = atoi(next);

        if (cmd->frame_num < 0)
            mpp_log("infinite loop decoding mode\n");

        return 1;
    }

    mpp_err("invalid frame number\n");
    return 0;
}

RK_S32 mpi_dec_opt_s(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    cmd->nthreads = -1;
    if (next) {
        cmd->nthreads = atoi(next);
        if (cmd->nthreads >= 1)
            return 1;
    }

    mpp_err("invalid nthreads %d\n", cmd->nthreads);
    cmd->nthreads = 1;
    return 0;
}

RK_S32 mpi_dec_opt_v(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        if (strstr(next, "q"))
            cmd->quiet = 1;
        if (strstr(next, "f"))
            cmd->trace_fps = 1;

        return 1;
    }

    return 0;
}

RK_S32 mpi_dec_opt_slt(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        size_t len = strnlen(next, MAX_FILE_NAME_LENGTH);
        if (len) {
            cmd->file_slt = mpp_calloc(char, len + 1);
            strncpy(cmd->file_slt, next, len);

            return 1;
        }
    }

    mpp_err("input slt verify file is invalid\n");
    return 0;
}

RK_S32 mpi_dec_opt_bufmode(void *ctx, const char *next)
{
    MpiDecTestCmd *cmd = (MpiDecTestCmd *)ctx;

    if (next) {
        if (strstr(next, "hi")) {
            cmd->buf_mode = MPP_DEC_BUF_HALF_INT;
        } else if (strstr(next, "i")) {
            cmd->buf_mode = MPP_DEC_BUF_INTERNAL;
        } else if (strstr(next, "e")) {
            cmd->buf_mode = MPP_DEC_BUF_EXTERNAL;
        } else {
            cmd->buf_mode = MPP_DEC_BUF_HALF_INT;
        }

        return 1;
    }

    mpp_err("invalid ext buf mode value\n");
    return 0;
}


RK_S32 mpi_dec_opt_help(void *ctx, const char *next)
{
    (void)ctx;
    (void)next;
    /* return invalid option to print help */
    return -1;
}

static MppOptInfo dec_opts[] = {
    {"i",       "input_file",   "input bitstream file",             mpi_dec_opt_i},
    {"o",       "output_file",  "output decoded frame file",        mpi_dec_opt_o},
    {"w",       "width",        "the width of input bitstream",     mpi_dec_opt_w},
    {"h",       "height",       "the height of input bitstream",    mpi_dec_opt_h},
    {"t",       "type",         "input stream coding type",         mpi_dec_opt_t},
    {"f",       "format",       "output frame format type",         mpi_dec_opt_f},
    {"n",       "frame_number", "max output frame number",          mpi_dec_opt_n},
    {"s",       "instance_nb",  "number of instances",              mpi_dec_opt_s},
    {"v",       "trace option", "q - quiet f - show fps",           mpi_dec_opt_v},
    {"slt",     "slt file",     "slt verify data file",             mpi_dec_opt_slt},
    {"help",    "help",         "show help",                        mpi_dec_opt_help},
    {"bufmode", "buffer mode",  "hi - half internal (default) i -internal e - external", mpi_dec_opt_bufmode},
};

static RK_U32 dec_opt_cnt = MPP_ARRAY_ELEMS(dec_opts);

RK_S32 mpi_dec_show_help(const char *name)
{
    RK_U32 max_name = 1;
    RK_U32 max_full_name = 1;
    RK_U32 max_help = 1;
    char logs[256];
    RK_U32 len;
    RK_U32 i;

    mpp_log("usage: %s [options]\n", name);

    for (i = 0; i < dec_opt_cnt; i++) {
        MppOptInfo *opt = &dec_opts[i];

        if (opt->name) {
            len = strlen(opt->name);
            if (len > max_name)
                max_name = len;
        }

        if (opt->full_name) {
            len = strlen(opt->full_name);
            if (len > max_full_name)
                max_full_name = len;
        }

        if (opt->help) {
            len = strlen(opt->help);
            if (len > max_help)
                max_help = len;
        }
    }

    snprintf(logs, sizeof(logs) - 1, "-%%-%ds %%-%ds %%-%ds\n", max_name, max_full_name, max_help);

    for (i = 0; i < dec_opt_cnt; i++) {
        MppOptInfo *opt = &dec_opts[i];

        mpp_log(logs, opt->name, opt->full_name, opt->help);
    }
    mpp_show_support_format();

    return -1;
}

RK_S32 mpi_dec_test_cmd_init(MpiDecTestCmd* cmd, int argc, char **argv)
{
    MppOpt opts = NULL;
    RK_S32 ret = -1;
    RK_U32 i;

    if ((argc < 2) || (cmd == NULL))
        goto done;

    mpp_opt_init(&opts);
    /* should change node count when option increases */
    mpp_opt_setup(opts, cmd, 35, dec_opt_cnt);

    for (i = 0; i < dec_opt_cnt; i++)
        mpp_opt_add(opts, &dec_opts[i]);

    /* mark option end */
    mpp_opt_add(opts, NULL);

    ret = mpp_opt_parse(opts, argc, argv);

    if (cmd->have_input) {
        reader_init(&cmd->reader, cmd->file_input, cmd->type);
        if (cmd->reader)
            mpp_log("input file %s size %ld\n", cmd->file_input, reader_size(cmd->reader));
    }
    if (cmd->trace_fps) {
        fps_calc_init(&cmd->fps);
        mpp_assert(cmd->fps);
        fps_calc_set_cb(cmd->fps, show_dec_fps);
    }

done:
    if (opts) {
        mpp_opt_deinit(opts);
        opts = NULL;
    }

    if (ret)
        mpi_dec_show_help(argv[0]);

    return ret;
}

RK_S32 mpi_dec_test_cmd_deinit(MpiDecTestCmd* cmd)
{
    if (!cmd)
        return 0;

    if (cmd->reader) {
        reader_deinit(cmd->reader);
        cmd->reader = NULL;
    }

    MPP_FREE(cmd->file_slt);

    if (cmd->fps) {
        fps_calc_deinit(cmd->fps);
        cmd->fps = NULL;
    }

    return 0;
}

void mpi_dec_test_cmd_options(MpiDecTestCmd* cmd)
{
    if (cmd->quiet)
        return;

    mpp_log("cmd parse result:\n");
    mpp_log("input  file name: %s\n", cmd->file_input);
    mpp_log("output file name: %s\n", cmd->file_output);
    mpp_log("width      : %4d\n", cmd->width);
    mpp_log("height     : %4d\n", cmd->height);
    mpp_log("type       : %4d\n", cmd->type);
    mpp_log("max frames : %4d\n", cmd->frame_num);
    if (cmd->file_slt)
        mpp_log("verify     : %s\n", cmd->file_slt);
}

MPP_RET dec_buf_mgr_init(DecBufMgr *mgr)
{
    DecBufMgrImpl *impl = NULL;
    MPP_RET ret = MPP_NOK;

    if (mgr) {
        impl = mpp_calloc(DecBufMgrImpl, 1);
        if (impl) {
            ret = MPP_OK;
        } else {
            mpp_err_f("failed to create decoder buffer manager\n");
        }

        *mgr = impl;
    }

    return ret;
}

void dec_buf_mgr_deinit(DecBufMgr mgr)
{
    DecBufMgrImpl *impl = (DecBufMgrImpl *)mgr;

    if (NULL == impl)
        return;

    /* release buffer group for half internal and external mode */
    if (impl->group) {
        mpp_buffer_group_put(impl->group);
        impl->group = NULL;
    }

    /* release the buffers used in external mode */
    if (impl->buf_count && impl->bufs) {
        RK_U32 i;

        for (i = 0; i < impl->buf_count; i++) {
            if (impl->bufs[i]) {
                mpp_buffer_put(impl->bufs[i]);
                impl->bufs[i] = NULL;
            }
        }

        MPP_FREE(impl->bufs);
    }

    MPP_FREE(impl);
}

MppBufferGroup dec_buf_mgr_setup(DecBufMgr mgr, RK_U32 size, RK_U32 count, MppDecBufMode mode)
{
    DecBufMgrImpl *impl = (DecBufMgrImpl *)mgr;
    MPP_RET ret = MPP_NOK;

    if (!impl)
        return NULL;

    /* cleanup old buffers if previous buffer group exists */
    if (impl->group) {
        if (mode != impl->buf_mode) {
            /* switch to different buffer mode just release old buffer group */
            mpp_buffer_group_put(impl->group);
            impl->group = NULL;
        } else {
            /* otherwise just cleanup old buffers */
            mpp_buffer_group_clear(impl->group);
        }

        /* if there are external mode old buffers do cleanup */
        if (impl->bufs) {
            RK_U32 i;

            for (i = 0; i < impl->buf_count; i++) {
                if (impl->bufs[i]) {
                    mpp_buffer_put(impl->bufs[i]);
                    impl->bufs[i] = NULL;
                }
            }

            MPP_FREE(impl->bufs);
        }
    }

    switch (mode) {
    case MPP_DEC_BUF_HALF_INT : {
        /* reuse previous half internal buffer group and just reconfig limit */
        if (NULL == impl->group) {
            ret = mpp_buffer_group_get_internal(&impl->group, MPP_BUFFER_TYPE_ION);
            if (ret) {
                mpp_err_f("get mpp internal buffer group failed ret %d\n", ret);
                break;
            }
        }
        /* Use limit config to limit buffer count and buffer size */
        ret = mpp_buffer_group_limit_config(impl->group, size, count);
        if (ret) {
            mpp_err_f("limit buffer group failed ret %d\n", ret);
        }
    } break;
    case MPP_DEC_BUF_INTERNAL : {
        /* do nothing juse keep buffer group empty */
        mpp_assert(NULL == impl->group);
        ret = MPP_OK;
    } break;
    case MPP_DEC_BUF_EXTERNAL : {
        RK_U32 i;
        MppBufferInfo commit;

        impl->bufs = mpp_calloc(MppBuffer, count);
        if (!impl->bufs) {
            mpp_err_f("create %d external buffer record failed\n", count);
            break;
        }

        /* reuse previous external buffer group */
        if (NULL == impl->group) {
            ret = mpp_buffer_group_get_external(&impl->group, MPP_BUFFER_TYPE_ION);
            if (ret) {
                mpp_err_f("get mpp external buffer group failed ret %d\n", ret);
                break;
            }
        }

        /*
         * NOTE: Use default misc allocater here as external allocator for demo.
         * But in practical case the external buffer could be GraphicBuffer or gst dmabuf.
         * The misc allocator will cause the print at the end like:
         * ~MppBufferService cleaning misc group
         */
        commit.type = MPP_BUFFER_TYPE_ION;
        commit.size = size;

        for (i = 0; i < count; i++) {
            ret = mpp_buffer_get(NULL, &impl->bufs[i], size);
            if (ret || NULL == impl->bufs[i]) {
                mpp_err_f("get misc buffer failed ret %d\n", ret);
                break;
            }

            commit.index = i;
            commit.ptr = mpp_buffer_get_ptr(impl->bufs[i]);
            commit.fd = mpp_buffer_get_fd(impl->bufs[i]);

            ret = mpp_buffer_commit(impl->group, &commit);
            if (ret) {
                mpp_err_f("external buffer commit failed ret %d\n", ret);
                break;
            }
        }
    } break;
    default : {
        mpp_err_f("unsupport buffer mode %d\n", mode);
    } break;
    }

    if (ret) {
        dec_buf_mgr_deinit(impl);
        impl = NULL;
    } else {
        impl->buf_count = count;
        impl->buf_size = size;
        impl->buf_mode = mode;
    }

    return impl ? impl->group : NULL;
}