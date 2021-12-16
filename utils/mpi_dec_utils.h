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

typedef void* FileReader;

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
} MpiDecTestCmd;

extern OptionInfo mpi_dec_cmd[];

RK_S32  mpi_dec_test_cmd_init(MpiDecTestCmd* cmd, int argc, char **argv);
RK_S32  mpi_dec_test_cmd_deinit(MpiDecTestCmd* cmd);
void    mpi_dec_test_cmd_options(MpiDecTestCmd* cmd);

void    reader_init(FileReader* reader, char* file_in);
void    reader_deinit(FileReader reader);

void    reader_start(FileReader reader);
void    reader_sync(FileReader reader);
void    reader_stop(FileReader reader);

size_t  reader_size(FileReader reader);
MPP_RET reader_read(FileReader reader, FileBufSlot **buf);
MPP_RET reader_index_read(FileReader reader, RK_S32 index, FileBufSlot **buf);
void    reader_rewind(FileReader reader);

void show_dec_fps(RK_S64 total_time, RK_S64 total_count, RK_S64 last_time, RK_S64 last_count);

#endif
