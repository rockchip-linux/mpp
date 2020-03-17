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

#ifndef __HAL_TASK_DEFS__
#define __HAL_TASK_DEFS__

#include "rk_type.h"

typedef void* HalTaskHnd;
typedef void* HalTaskGroup;

typedef enum HalTaskStatus_e {
    TASK_IDLE,
    TASK_PROCESSING,
    TASK_PROC_DONE,
    TASK_BUTT,
} HalTaskStatus;

/*
 * modified by parser and encoder
 *
 * number   : the number of the data pointer array element
 * data     : the address of the pointer array, parser will add its data here
 */
typedef struct MppSyntax_t {
    RK_U32              number;
    void                *data;
} MppSyntax;

#endif /* __HAL_TASK_DEFS__ */
