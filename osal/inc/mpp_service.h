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

#ifndef __MPP_SERVICE_H__
#define __MPP_SERVICE_H__

#include "rk_type.h"

/* Use 'v' as magic number */
#define MPP_IOC_MAGIC                       'v'
#define MPP_IOC_CFG_V1                      _IOW(MPP_IOC_MAGIC, 1, unsigned int)
#define MAX_REQ_NUM                         16

#if __SIZEOF_POINTER__ == 4
#define REQ_DATA_PTR(ptr) ((RK_U32)ptr)
#elif __SIZEOF_POINTER__ == 8
#define REQ_DATA_PTR(ptr) ((RK_U64)ptr)
#endif

typedef struct MppReq_t {
    RK_U32 *req;
    RK_U32  size;
} MppReq;

typedef struct mppReqV1_t {
    RK_U32 cmd;
    RK_U32 flag;
    RK_U32 size;
    RK_U32 offset;
    RK_U64 data_ptr;
} MppReqV1;

#endif /* __MPP_SERVICE_H__ */
