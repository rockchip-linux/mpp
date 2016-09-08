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

#define MODULE_TAG "mpp_task"

#include <string.h>

#include "mpp_task.h"
#include "mpp_task_impl.h"

MPP_RET mpp_task_meta_set_s32(MppTask task, MppMetaKey key, RK_S32 val)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    return mpp_meta_set_s32(impl->meta, key, val);
}

MPP_RET mpp_task_meta_set_s64(MppTask task, MppMetaKey key, RK_S64 val)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    return mpp_meta_set_s64(impl->meta, key, val);
}

MPP_RET mpp_task_meta_set_ptr(MppTask task, MppMetaKey key, void  *val)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    return mpp_meta_set_ptr(impl->meta, key, val);
}

MPP_RET mpp_task_meta_set_frame(MppTask task, MppMetaKey key, MppFrame frame)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    return mpp_meta_set_frame(impl->meta, key, frame);
}

MPP_RET mpp_task_meta_set_packet(MppTask task, MppMetaKey key, MppPacket packet)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    return mpp_meta_set_packet(impl->meta, key, packet);
}

MPP_RET mpp_task_meta_set_buffer(MppTask task, MppMetaKey key, MppBuffer buffer)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    return mpp_meta_set_buffer(impl->meta, key, buffer);
}

MPP_RET mpp_task_meta_get_s32(MppTask task, MppMetaKey key, RK_S32 *val, RK_S32 default_val)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    MPP_RET ret = mpp_meta_get_s32(impl->meta, key, val);
    if (ret)
        *val = default_val;
    return ret;
}

MPP_RET mpp_task_meta_get_s64(MppTask task, MppMetaKey key, RK_S64 *val, RK_S64 default_val)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    MPP_RET ret = mpp_meta_get_s64(impl->meta, key, val);
    if (ret)
        *val = default_val;
    return ret;
}

MPP_RET mpp_task_meta_get_ptr(MppTask task, MppMetaKey key, void  **val, void *default_val)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    MPP_RET ret = mpp_meta_get_ptr(impl->meta, key, val);
    if (ret)
        *val = default_val;
    return ret;
}

MPP_RET mpp_task_meta_get_frame(MppTask task, MppMetaKey key, MppFrame *frame)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    MPP_RET ret = mpp_meta_get_frame(impl->meta, key, frame);
    if (ret)
        *frame = NULL;
    return ret;
}

MPP_RET mpp_task_meta_get_packet(MppTask task, MppMetaKey key, MppPacket *packet)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    MPP_RET ret = mpp_meta_get_packet(impl->meta, key, packet);
    if (ret)
        *packet = NULL;
    return ret;
}

MPP_RET mpp_task_meta_get_buffer(MppTask task, MppMetaKey key, MppBuffer *buffer)
{
    if (check_mpp_task_name(task))
        return MPP_NOK;

    MppTaskImpl *impl = (MppTaskImpl *)task;
    MPP_RET ret = mpp_meta_get_buffer(impl->meta, key, buffer);
    if (ret)
        *buffer = NULL;
    return ret;
}

