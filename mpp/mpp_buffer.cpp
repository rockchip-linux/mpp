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

#define MODULE_TAG "mpp_buffer"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_buffer.h"

MPP_RET _mpp_buffer_commit(const char *tag, MppBufferGroup group, MppBufferCommit *buffer)
{
    return MPP_OK;
}

MPP_RET _mpp_buffer_get(const char *tag, MppBufferGroup group, MppBuffer *buffer, size_t size)
{
    return MPP_OK;
}

MPP_RET _mpp_buffer_put(const char *tag, MppBuffer *buffer)
{
    return MPP_OK;
}

MPP_RET _mpp_buffer_inc_ref(const char *tag, MppBuffer buffer)
{
    return MPP_OK;
}


MPP_RET _mpp_buffer_group_get(const char *tag, MppBufferGroup *group, MppBufferType type)
{
    return MPP_OK;
}

MPP_RET _mpp_buffer_group_put(const char *tag, MppBufferGroup *group)
{
    return MPP_OK;
}

