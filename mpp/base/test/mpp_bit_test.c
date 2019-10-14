/*
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

#define MODULE_TAG "mpp_bit_test"

#include <stdlib.h>

#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_bitwrite.h"

#define BIT_WRITER_BUFFER_SIZE  1024

/*
 * type is for operation type
 * 0 - plain bit put
 * 1 - bit put with detection
 * 2 - write ue
 * 3 - write se
 * 4 - align byte
 */
typedef enum BitOpsType_e {
    BIT_PUT_NO03,
    BIT_PUT,
    BIT_PUT_UE,
    BIT_PUT_SE,
    BIT_ALIGN_BYTE,
} BitOpsType;

typedef struct BitOps_t {
    BitOpsType  type;
    RK_S32      val;
    RK_S32      len;
} BitOps;

static BitOps bit_ops[] = {
    {   BIT_PUT_NO03,       0,      8,  },
    {   BIT_PUT,            0,      3,  },
    {   BIT_PUT,            0,     15,  },
    {   BIT_PUT,            0,     23,  },
    {   BIT_PUT_UE,        17,      0,  },
    {   BIT_PUT_SE,         9,      0,  },
    {   BIT_ALIGN_BYTE,     0,      0,  },
};

void proc_bit_ops(MppWriteCtx *writer, BitOps *ops)
{
    switch (ops->type) {
    case BIT_PUT_NO03 : {
        mpp_writer_put_raw_bits(writer, ops->val, ops->len);
    } break;
    case BIT_PUT : {
        mpp_writer_put_bits(writer, ops->val, ops->len);
    } break;
    case BIT_PUT_UE : {
        mpp_writer_put_ue(writer, ops->val);
    } break;
    case BIT_PUT_SE : {
        mpp_writer_put_ue(writer, ops->val);
    } break;
    case BIT_ALIGN_BYTE : {
        mpp_writer_trailing(writer);
    } break;
    default : {
        mpp_err("invalid ops type %d\n", ops->type);
    } break;
    }
}

int main()
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    void *data = NULL;
    size_t size = BIT_WRITER_BUFFER_SIZE;
    MppWriteCtx writer;
    RK_U32 len_byte;
    RK_U32 i;
    RK_U32 buf_len = 0;
    char buf[BIT_WRITER_BUFFER_SIZE];

    mpp_log("mpp_bit_test start\n");

    data = malloc(size);
    if (NULL == data) {
        mpp_err("mpp_bit_test malloc failed\n");
        goto TEST_FAILED;
    }

    mpp_writer_init(&writer, data, size);

    for (i = 0; i < MPP_ARRAY_ELEMS(bit_ops); i++)
        proc_bit_ops(&writer, &bit_ops[i]);

    len_byte = writer.byte_cnt;

    for (i = 0; i < len_byte; i++) {
        buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len,
                            "%02x ", writer.buffer[i]);
    }

    mpp_log("stream %s\n", buf);

    ret = MPP_OK;
TEST_FAILED:
    if (data)
        free(data);

    if (ret)
        mpp_log("mpp_bit_test failed\n");
    else
        mpp_log("mpp_bit_test success\n");

    return ret;
}

