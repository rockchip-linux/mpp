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

MPP_RET mpp_buffer_commit(MppBufferGroup group, MppBufferInfo *info)
{
    if (NULL == group || NULL == info) {
        mpp_err("mpp_buffer_commit input null pointer group %p info %p\n",
                group, info);
        return MPP_ERR_NULL_PTR;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    if (p->type != info->type || p->type >= MPP_BUFFER_TYPE_BUTT ||
        p->mode != MPP_BUFFER_EXTERNAL) {
        mpp_err("mpp_buffer_commit invalid type found group %d info %d group mode %d\n",
                p->type, info->type, p->mode);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_create(NULL, p->group_id, info);
}

MPP_RET mpp_buffer_get_with_tag(const char *tag, MppBufferGroup group, MppBuffer *buffer, size_t size)
{
    if (NULL == buffer || 0 == size) {
        mpp_err("mpp_buffer_get invalid input: group %p buffer %p size %u\n",
                group, buffer, size);
        return MPP_ERR_UNKNOW;
    }

    if (NULL == group) {
        // deprecated, only for libvpu support
        group = mpp_buffer_legacy_group();
    }

    mpp_assert(group);

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    // try unused buffer first
    MppBufferImpl *buf = mpp_buffer_get_unused(p, size);
    if (NULL == buf && MPP_BUFFER_INTERNAL == p->mode) {
        MppBufferInfo info = {
            p->type,
            size,
            NULL,
            NULL,
            -1,
            NULL,
        };
        // if failed try init a new buffer
        mpp_buffer_create(tag, p->group_id, &info);
        buf = mpp_buffer_get_unused(p, size);
    }
    *buffer = buf;
    return (buf) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_buffer_put(MppBuffer buffer)
{
    if (NULL == buffer) {
        mpp_err("mpp_buffer_put invalid input: buffer %p\n", buffer);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_ref_dec((MppBufferImpl*)buffer);
}

MPP_RET mpp_buffer_inc_ref(MppBuffer buffer)
{
    if (NULL == buffer) {
        mpp_err("mpp_buffer_inc_ref invalid input: buffer %p\n", buffer);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_ref_inc((MppBufferImpl*)buffer);
}

MPP_RET mpp_buffer_read(MppBuffer buffer, size_t offset, void *data, size_t size)
{
    if (NULL == buffer || NULL == data) {
        mpp_err_f("invalid input: buffer %p data %p\n", buffer, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    void *src = p->info.ptr;
    mpp_assert(src != NULL);
    memcpy(data, (char*)src + offset, size);
    return MPP_OK;
}

MPP_RET mpp_buffer_write(MppBuffer buffer, size_t offset, void *data, size_t size)
{
    if (NULL == buffer || NULL == data) {
        mpp_err_f("invalid input: buffer %p data %p\n", buffer, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    void *dst = p->info.ptr;
    mpp_assert(dst != NULL);
    memcpy((char*)dst + offset, data, size);
    return MPP_OK;
}

void *mpp_buffer_get_ptr(MppBuffer buffer)
{
    if (NULL == buffer) {
        mpp_err_f("invalid input: buffer %p\n", buffer);
        return NULL;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    void *ptr = p->info.ptr;
    mpp_assert(ptr != NULL);
    return ptr;
}

int mpp_buffer_get_fd(MppBuffer buffer)
{
    if (NULL == buffer) {
        mpp_err_f("invalid input: buffer %p\n", buffer);
        return -1;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    int fd = p->info.fd;
    mpp_assert(fd >= 0);
    return fd;
}

MPP_RET mpp_buffer_info_get(MppBuffer buffer, MppBufferInfo *info)
{
    if (NULL == buffer || NULL == info) {
        mpp_err_f("invalid input: buffer %p info %p\n", buffer, info);
        return MPP_ERR_UNKNOW;
    }

    *info = ((MppBufferImpl*)buffer)->info;
    return MPP_OK;
}

MPP_RET mpp_buffer_group_get(const char *tag, MppBufferMode mode,
                             MppBufferGroup *group, MppBufferType type)
{
    if (NULL == group ||
        mode >= MPP_BUFFER_MODE_BUTT ||
        type >= MPP_BUFFER_TYPE_BUTT) {
        mpp_err_f("input invalid group %p mode %d type %d\n",
                  group, mode, type);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_group_init((MppBufferGroupImpl**)group, tag, mode, type);
}

MPP_RET mpp_buffer_group_put(MppBufferGroup group)
{
    if (NULL == group) {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_NOK;
    }

    return mpp_buffer_group_deinit((MppBufferGroupImpl *)group);
}

MPP_RET mpp_buffer_group_clear(MppBufferGroup group)
{
    if (NULL == group) {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_NOK;
    }

    return mpp_buffer_group_reset((MppBufferGroupImpl *)group);
}

RK_S32  mpp_buffer_group_unused(MppBufferGroup group)
{
    if (NULL == group) {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_NOK;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    return (p->mode == MPP_BUFFER_INTERNAL ? 1 : p->count_unused);
}

MppBufferMode mpp_buffer_group_mode(MppBufferGroup group)
{
    if (NULL == group) {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_BUFFER_MODE_BUTT;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    return p->mode;
}

MppBufferType mpp_buffer_group_type(MppBufferGroup group)
{
    if (NULL == group) {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_BUFFER_TYPE_BUTT;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    return p->type;
}

MPP_RET mpp_buffer_group_limit_config(MppBufferGroup group, size_t size, RK_S32 count)
{
    if (NULL == group) {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_NOK;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    mpp_assert(p->mode == MPP_BUFFER_INTERNAL);
    p->limit_size     = size;
    p->limit_count    = count;
    return MPP_OK;
}

