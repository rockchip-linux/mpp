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
#include "mpp_buffer_impl.h"

MPP_RET mpp_buffer_commit(MppBufferGroup group, MppBufferCommit *info)
{
    if (NULL == group || NULL == info) {
        mpp_err("mpp_buffer_commit input null pointer group %p info %p\n",
                group, info);
        return MPP_ERR_NULL_PTR;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    if (p->type != info->type || p->type >= MPP_BUFFER_TYPE_BUTT) {
        mpp_err("mpp_buffer_commit invalid type found group %d info %d\n",
                p->type, info->type);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_create(NULL, p->group_id, info->size, &info->data);
}

MPP_RET mpp_buffer_get_with_tag(const char *tag, MppBufferGroup group, MppBuffer *buffer, size_t size)
{
    if (NULL == group || NULL == buffer || 0 == size) {
        mpp_err("mpp_buffer_get invalid input: group %p buffer %p size %u\n",
                group, buffer, size);
        return MPP_ERR_UNKNOW;
    }

    MppBufferGroupImpl *tmp = (MppBufferGroupImpl *)group;
    // try unused buffer first
    MppBufferImpl *buf = mpp_buffer_get_unused(tmp, size);
    if (NULL == buf) {
        // if failed try init a new buffer
        mpp_buffer_create(tag, tmp->group_id, size, NULL);
        buf = mpp_buffer_get_unused(tmp, size);
    }
    *buffer = buf;
    return (buf) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_buffer_put(MppBuffer *buffer)
{
    if (NULL == buffer) {
        mpp_err("mpp_buffer_put invalid input: buffer %p\n", buffer);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_ref_dec((MppBufferImpl*)*buffer);
}

MPP_RET mpp_buffer_inc_ref(MppBuffer buffer)
{
    if (NULL == buffer) {
        mpp_err("mpp_buffer_put invalid input: buffer %p\n", buffer);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_ref_inc((MppBufferImpl*)buffer);
}


MPP_RET mpp_buffer_group_get(const char *tag, MppBufferMode mode,
                             MppBufferGroup *group, MppBufferType type)
{
    if (NULL == group ||
        mode >= MPP_BUFFER_MODE_BUTT ||
        type >= MPP_BUFFER_TYPE_BUTT) {
        mpp_err("mpp_buffer_group_get input invalid group %p mode %d type %d\n",
                group, mode, type);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_group_init((MppBufferGroupImpl**)group, tag, mode, type);
}

MPP_RET mpp_buffer_group_put(MppBufferGroup *group)
{
    if (NULL == group) {
        mpp_err("mpp_buffer_group_put input invalid group %p\n", group);
        return MPP_NOK;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)*group;
    *group = NULL;

    return mpp_buffer_group_deinit(p);
}

MPP_RET mpp_buffer_group_limit_config(MppBufferGroup group, size_t size, RK_S32 count)
{
    if (NULL == group || 0 == size || count <= 0) {
        mpp_err("mpp_buffer_group_limit_config input invalid group %p size %d count %d\n",
                group, size, count);
        return MPP_NOK;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    mpp_assert(p->mode == MPP_BUFFER_MODE_LIMIT);
    p->limit_size     = size;
    p->limit_count    = count;
    return MPP_OK;
}

