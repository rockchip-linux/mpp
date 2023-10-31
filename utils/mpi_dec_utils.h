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

#ifndef __MPI_DEC_UTILS_H__
#define __MPI_DEC_UTILS_H__

#include <stdio.h>
#include "utils.h"

#define MAX_FILE_NAME_LENGTH        256
#define MPI_DEC_STREAM_SIZE         (SZ_4K)
#define MPI_DEC_LOOP_COUNT          4

/*
 * NOTE: We can choose decoder's buffer mode here.
 * There are three mode that decoder can support:
 *
 * Mode 1: Pure internal mode
 * In the mode user will NOT call MPP_DEC_SET_EXT_BUF_GROUP
 * control to decoder. Only call MPP_DEC_SET_INFO_CHANGE_READY
 * to let decoder go on. Then decoder will use create buffer
 * internally and user need to release each frame they get.
 *
 * Advantage:
 * Easy to use and get a demo quickly
 * Disadvantage:
 * 1. The buffer from decoder may not be return before
 * decoder is close. So memroy leak or crash may happen.
 * 2. The decoder memory usage can not be control. Decoder
 * is on a free-to-run status and consume all memory it can
 * get.
 * 3. Difficult to implement zero-copy display path.
 *
 * Mode 2: Half internal mode
 * This is the mode current test code using. User need to
 * create MppBufferGroup according to the returned info
 * change MppFrame. User can use mpp_buffer_group_limit_config
 * function to limit decoder memory usage.
 *
 * Advantage:
 * 1. Easy to use
 * 2. User can release MppBufferGroup after decoder is closed.
 *    So memory can stay longer safely.
 * 3. Can limit the memory usage by mpp_buffer_group_limit_config
 * Disadvantage:
 * 1. The buffer limitation is still not accurate. Memory usage
 * is 100% fixed.
 * 2. Also difficult to implement zero-copy display path.
 *
 * Mode 3: Pure external mode
 * In this mode use need to create empty MppBufferGroup and
 * import memory from external allocator by file handle.
 * On Android surfaceflinger will create buffer. Then
 * mediaserver get the file handle from surfaceflinger and
 * commit to decoder's MppBufferGroup.
 *
 * Advantage:
 * 1. Most efficient way for zero-copy display
 * Disadvantage:
 * 1. Difficult to learn and use.
 * 2. Player work flow may limit this usage.
 * 3. May need a external parser to get the correct buffer
 * size for the external allocator.
 *
 * The required buffer size caculation:
 * hor_stride * ver_stride * 3 / 2 for pixel data
 * hor_stride * ver_stride / 2 for extra info
 * Total hor_stride * ver_stride * 2 will be enough.
 *
 * For H.264/H.265 20+ buffers will be enough.
 * For other codec 10 buffers will be enough.
 */
typedef enum MppDecBufMode_e {
    MPP_DEC_BUF_HALF_INT,
    MPP_DEC_BUF_INTERNAL,
    MPP_DEC_BUF_EXTERNAL,
    MPP_DEC_BUF_MODE_BUTT,
} MppDecBufMode;

typedef void* FileReader;
typedef void* DecBufMgr;

typedef struct FileBufSlot_t {
    RK_S32          index;
    MppBuffer       buf;
    size_t          size;
    RK_U32          eos;
    char            *data;
} FileBufSlot;

/* For overall configure setup */
typedef struct MpiDecTestCmd_t {
    char            file_input[MAX_FILE_NAME_LENGTH];
    char            file_output[MAX_FILE_NAME_LENGTH];

    MppCodingType   type;
    MppFrameFormat  format;
    RK_U32          width;
    RK_U32          height;

    RK_U32          have_input;
    RK_U32          have_output;

    RK_U32          simple;
    RK_S32          timeout;
    RK_S32          frame_num;
    size_t          pkt_size;
    MppDecBufMode   buf_mode;

    /* use for mpi_dec_multi_test */
    RK_S32          nthreads;
    // report information
    size_t          max_usage;

    /* data for share */
    FileReader      reader;
    FpsCalc         fps;

    /* runtime log flag */
    RK_U32          quiet;
    RK_U32          trace_fps;
    char            *file_slt;
} MpiDecTestCmd;

RK_S32  mpi_dec_test_cmd_init(MpiDecTestCmd* cmd, int argc, char **argv);
RK_S32  mpi_dec_test_cmd_deinit(MpiDecTestCmd* cmd);
void    mpi_dec_test_cmd_options(MpiDecTestCmd* cmd);

void    reader_init(FileReader* reader, char* file_in, MppCodingType type);
void    reader_deinit(FileReader reader);

void    reader_start(FileReader reader);
void    reader_sync(FileReader reader);
void    reader_stop(FileReader reader);

size_t  reader_size(FileReader reader);
MPP_RET reader_read(FileReader reader, FileBufSlot **buf);
MPP_RET reader_index_read(FileReader reader, RK_S32 index, FileBufSlot **buf);
void    reader_rewind(FileReader reader);

MPP_RET dec_buf_mgr_init(DecBufMgr *mgr);
void dec_buf_mgr_deinit(DecBufMgr mgr);
MppBufferGroup dec_buf_mgr_setup(DecBufMgr mgr, RK_U32 size, RK_U32 count, MppDecBufMode mode);

void show_dec_fps(RK_S64 total_time, RK_S64 total_count, RK_S64 last_time, RK_S64 last_count);

#endif