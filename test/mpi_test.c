/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define MODULE_TAG "mpi_test"

#include <string.h>

#include "rk_mpi.h"

#include "mpp_log.h"
#include "mpp_env.h"

int main()
{
    MPP_RET ret;
    MppCtx ctx  = NULL;
    MppApi *mpi = NULL;

    MppPacket dec_in    = NULL;
    MppFrame  dec_out   = NULL;

    MppFrame  enc_in    = NULL;
    MppPacket enc_out   = NULL;

    MPI_CMD cmd         = MPI_MPP_CMD_BASE;
    MppParam param      = NULL;

    mpp_log("mpi_test start\n");

    mpp_env_set_u32("mpi_debug", 0x1);

    mpp_log("mpi_test decoder test start\n");

    // decoder demo
    ret = mpp_init(&ctx, &mpi);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_FAILED;
    }

    ret = mpi->init(ctx, dec_in);
    if (MPP_OK != ret) {
        mpp_err("mpi->init failed\n");
        goto MPP_TEST_FAILED;
    }

    ret = mpi->control(ctx, cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        goto MPP_TEST_FAILED;
    }

    mpi->decode(ctx, dec_in, &dec_out);
    mpi->decode_put_packet(ctx, dec_in);
    mpi->decode_get_frame(ctx, &dec_out);

    ret = mpi->flush(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->flush failed\n");
        goto MPP_TEST_FAILED;
    }

    mpp_deinit(&ctx);


    mpp_log("mpi_test encoder test start\n");

    // encoder demo
    ret = mpp_init(&ctx, &mpi);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_FAILED;
    }

    ret = mpi->init(ctx, dec_in);
    if (MPP_OK != ret) {
        mpp_err("mpi->init failed\n");
        goto MPP_TEST_FAILED;
    }

    ret = mpi->control(ctx, cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        goto MPP_TEST_FAILED;
    }

    mpi->encode(ctx, enc_in, &enc_out);
    mpi->encode_put_frame(ctx, enc_in);
    mpi->encode_get_packet(ctx, &dec_out);

    ret = mpi->flush(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->flush failed\n");
        goto MPP_TEST_FAILED;
    }

    mpp_deinit(&ctx);

    mpp_log("mpi_test success\n");

    return 0;

MPP_TEST_FAILED:
    if (ctx)
        mpp_deinit(ctx);

    mpp_log("mpi_test failed\n");

    return -1;
}

