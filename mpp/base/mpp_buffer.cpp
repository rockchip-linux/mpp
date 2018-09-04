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

#define MODULE_TAG "mpp_buffer"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_buffer_impl.h"

MPP_RET mpp_buffer_import_with_tag(MppBufferGroup group, MppBufferInfo *info, MppBuffer *buffer,
                                   const char *tag, const char *caller)
{
    if (NULL == info) {
        mpp_err("mpp_buffer_commit input null info\n", info);
        return MPP_ERR_NULL_PTR;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;

    if (p) {
        // if group is specified we need to check the parameter
        if ((p->type & MPP_BUFFER_TYPE_MASK) != info->type ||
            (p->type & MPP_BUFFER_TYPE_MASK) >= MPP_BUFFER_TYPE_BUTT ||
            p->mode != MPP_BUFFER_EXTERNAL) {
            mpp_err("mpp_buffer_commit invalid type found group %d info %d group mode %d\n",
                    p->type, info->type, p->mode);
            return MPP_ERR_UNKNOW;
        }
    } else {
        // otherwise use default external group to manage them
        p = mpp_buffer_get_misc_group(MPP_BUFFER_EXTERNAL, info->type);
    }

    mpp_assert(p);

    MPP_RET ret = MPP_OK;
    if (buffer) {
        MppBufferImpl *buf = NULL;
        ret = mpp_buffer_create(tag, caller, p, info, &buf);
        *buffer = buf;
    } else {
        ret = mpp_buffer_create(tag, caller, p, info, NULL);
    }
    return ret;
}

MPP_RET mpp_buffer_get_with_tag(MppBufferGroup group, MppBuffer *buffer, size_t size,
                                const char *tag, const char *caller)
{
    if (NULL == buffer || 0 == size) {
        mpp_err("mpp_buffer_get invalid input: group %p buffer %p size %u\n",
                group, buffer, size);
        return MPP_ERR_UNKNOW;
    }

    if (NULL == group) {
        // deprecated, only for libvpu support
        group = mpp_buffer_get_misc_group(MPP_BUFFER_INTERNAL, MPP_BUFFER_TYPE_ION);
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
            -1,
        };
        // if failed try init a new buffer
        mpp_buffer_create(tag, caller, p, &info, &buf);
    }
    *buffer = buf;
    return (buf) ? (MPP_OK) : (MPP_NOK);
}


MPP_RET mpp_buffer_put_with_caller(MppBuffer buffer, const char *caller)
{
    if (NULL == buffer) {
        mpp_err("mpp_buffer_put invalid input: buffer %p\n", buffer);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_ref_dec((MppBufferImpl*)buffer, caller);
}

MPP_RET mpp_buffer_inc_ref_with_caller(MppBuffer buffer, const char *caller)
{
    if (NULL == buffer) {
        mpp_err("mpp_buffer_inc_ref invalid input: buffer %p\n", buffer);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_ref_inc((MppBufferImpl*)buffer, caller);
}

MPP_RET mpp_buffer_read_with_caller(MppBuffer buffer, size_t offset, void *data, size_t size, const char *caller)
{
    if (NULL == buffer || NULL == data) {
        mpp_err_f("invalid input: buffer %p data %p\n", buffer, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    if (NULL == p->info.ptr)
        mpp_buffer_mmap(p, caller);

    void *src = p->info.ptr;
    mpp_assert(src != NULL);
    if (src)
        memcpy(data, (char*)src + offset, size);

    return MPP_OK;
}

MPP_RET mpp_buffer_write_with_caller(MppBuffer buffer, size_t offset, void *data, size_t size, const char *caller)
{
    if (NULL == buffer || NULL == data) {
        mpp_err_f("invalid input: buffer %p data %p\n", buffer, data);
        return MPP_ERR_UNKNOW;
    }

    if (0 == size)
        return MPP_OK;

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    if (offset + size > p->info.size)
        return MPP_ERR_VALUE;
    if (NULL == p->info.ptr)
        mpp_buffer_mmap(p, caller);

    void *dst = p->info.ptr;
    mpp_assert(dst != NULL);
    if (dst)
        memcpy((char*)dst + offset, data, size);

    return MPP_OK;
}

void *mpp_buffer_get_ptr_with_caller(MppBuffer buffer, const char *caller)
{
    if (NULL == buffer) {
        mpp_err_f("invalid NULL input\n");
        return NULL;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    if (NULL == p->info.ptr)
        mpp_buffer_mmap(p, caller);

    mpp_assert(p->info.ptr != NULL);
    if (NULL == p->info.ptr)
        mpp_err("mpp_buffer_get_ptr buffer %p ret NULL caller\n", buffer, caller);

    return p->info.ptr;
}

int mpp_buffer_get_fd_with_caller(MppBuffer buffer, const char *caller)
{
    if (NULL == buffer) {
        mpp_err_f("invalid NULL input\n");
        return -1;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    int fd = p->info.fd;
    mpp_assert(fd >= 0);
    if (fd < 0)
        mpp_err("mpp_buffer_get_fd buffer %p fd %d caller %s\n", buffer, fd, caller);

    return fd;
}

size_t mpp_buffer_get_size_with_caller(MppBuffer buffer, const char *caller)
{
    if (NULL == buffer) {
        mpp_err_f("invalid NULL input\n");
        return 0;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    if (p->info.size == 0)
        mpp_err("mpp_buffer_get_size buffer %p ret zero size caller %s\n", buffer, caller);

    return p->info.size;
}

int mpp_buffer_get_index_with_caller(MppBuffer buffer, const char *caller)
{
    if (NULL == buffer) {
        mpp_err_f("invalid NULL input\n");
        return -1;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    (void)caller;
    return p->info.index;
}

MPP_RET mpp_buffer_set_index_with_caller(MppBuffer buffer, int index,
                                         const char *caller)
{
    if (NULL == buffer) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_UNKNOW;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    p->info.index = index;
    (void)caller;
    return MPP_OK;
}


MPP_RET mpp_buffer_info_get_with_caller(MppBuffer buffer, MppBufferInfo *info, const char *caller)
{
    if (NULL == buffer || NULL == info) {
        mpp_err_f("invalid input: buffer %p info %p\n", buffer, info);
        return MPP_ERR_UNKNOW;
    }

    MppBufferImpl *p = (MppBufferImpl*)buffer;
    if (NULL == p->info.ptr)
        mpp_buffer_mmap(p, caller);

    *info = p->info;
    (void)caller;
    return MPP_OK;
}

MPP_RET mpp_buffer_group_get(MppBufferGroup *group, MppBufferType type, MppBufferMode mode,
                             const char *tag, const char *caller)
{
    if (NULL == group ||
        mode >= MPP_BUFFER_MODE_BUTT ||
        (type & MPP_BUFFER_TYPE_MASK) >= MPP_BUFFER_TYPE_BUTT) {
        mpp_err_f("input invalid group %p mode %d type %d\n",
                  group, mode, type);
        return MPP_ERR_UNKNOW;
    }

    return mpp_buffer_group_init((MppBufferGroupImpl**)group, tag, caller, mode, type);
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
    RK_S32 unused = 0;

    if (p->mode == MPP_BUFFER_INTERNAL) {
        if (p->limit_count)
            unused = p->limit_count - p->count_used;
        else
            unused = 3; /* NOTE: 3 for 1 decoding 2 deinterlace buffer */
    } else
        unused = p->count_unused;

    return unused;
}

size_t mpp_buffer_group_usage(MppBufferGroup group)
{
    if (NULL == group) {
        mpp_err_f("input invalid group %p\n", group);
        return MPP_BUFFER_MODE_BUTT;
    }

    MppBufferGroupImpl *p = (MppBufferGroupImpl *)group;
    return p->usage;
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

