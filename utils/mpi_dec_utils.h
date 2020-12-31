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

/* For overall configure setup */
typedef struct {
    char            file_input[MAX_FILE_NAME_LENGTH];
    char            file_output[MAX_FILE_NAME_LENGTH];
    char            file_config[MAX_FILE_NAME_LENGTH];
    MppCodingType   type;
    MppFrameFormat  format;
    RK_U32          width;
    RK_U32          height;
    RK_U32          debug;

    RK_U32          have_input;
    RK_U32          have_output;
    RK_U32          have_config;

    RK_U32          simple;
    RK_S32          timeout;
    RK_S32          frame_num;
    size_t          pkt_size;
    /* use for mpi_dec_multi_test */
    RK_S32          nthreads;
    // report information
    size_t          max_usage;
} MpiDecTestCmd;

extern OptionInfo mpi_dec_cmd[];

RK_S32  mpi_dec_test_parse_options(int argc, char **argv, MpiDecTestCmd* cmd);
void    mpi_dec_test_show_options(MpiDecTestCmd* cmd);
void    mpi_dec_test_help();

void    reader_init(FileReader* file_reader, char* file_in, FILE* fp_in);
void    reader_deinit(FileReader *file_reader);
RK_U32  reader_read(FileReader file_reader, char** buf, size_t *size);
void    reader_rewind(FileReader file_reader);

#endif