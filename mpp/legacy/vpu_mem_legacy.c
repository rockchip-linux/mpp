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

#include <string.h>
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"

#include "mpp_buffer.h"

#include "vpu.h"
#include "vpu_mem_legacy.h"

#define VPU_MEM_DBG_FUNCTION            (0x00000001)

static RK_U32 vpu_mem_debug = 0;

#define vpu_mem_dbg(flag, fmt, ...)     _mpp_dbg(vpu_mem_debug, flag, fmt, ## __VA_ARGS__)
#define vpu_mem_dbg_f(flag, fmt, ...)   _mpp_dbg_f(vpu_mem_debug, flag, fmt, ## __VA_ARGS__)

#define vpu_mem_dbg_func(fmt, ...)      vpu_mem_dbg_f(VPU_MEM_DBG_FUNCTION, fmt, ## __VA_ARGS__)

static RK_S32
commit_memory_handle(vpu_display_mem_pool *p, RK_S32 mem_hdl, RK_S32 size)
{
    MppBufferInfo info;
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;

    vpu_mem_dbg_func("in  pool %p hnl %p size %d\n", p, mem_hdl, size);
    memset(&info, 0, sizeof(MppBufferInfo));
    info.type = MPP_BUFFER_TYPE_ION;
    info.fd = mem_hdl;
    info.size = size & 0x07ffffff;
    info.index = (size & 0xf8000000) >> 27;

    p_mempool->size = size;
    p_mempool->buff_size = size;

    mpp_buffer_commit(p_mempool->group, &info);
    vpu_mem_dbg_func("out pool %p fd %d\n", p, info.fd);
    return info.fd;
}

static void* get_free_memory_vpumem(vpu_display_mem_pool *p)
{
    MPP_RET ret = MPP_OK;
    MppBuffer buffer = NULL;
    VPUMemLinear_t *dmabuf = mpp_calloc(VPUMemLinear_t, 1);
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    if (dmabuf == NULL) {
        return NULL;
    }
    vpu_mem_dbg_func("in  pool %p\n", p);
    ret = mpp_buffer_get(p_mempool->group, &buffer, p_mempool->size);
    if (MPP_OK != ret) {
        mpp_free(dmabuf);
        return NULL;
    }
    dmabuf->phy_addr = (RK_U32)mpp_buffer_get_fd(buffer);
    dmabuf->vir_addr = (RK_U32*)mpp_buffer_get_ptr(buffer);
    dmabuf->size = p_mempool->size;
    dmabuf->offset = (RK_U32*)buffer;
    vpu_mem_dbg_func("out pool %p ret %p fd %d size %d buffer %p\n", p, dmabuf,
                     dmabuf->phy_addr, dmabuf->size, buffer);
    return dmabuf;

}

static RK_S32 inc_used_memory_handle_ref(vpu_display_mem_pool *p, void * hdl)
{
    VPUMemLinear_t *dmabuf = (VPUMemLinear_t *)hdl;
    MppBuffer buffer = (MppBuffer)dmabuf->offset;
    vpu_mem_dbg_func("pool %p hnd %p buffer %p\n", p, hdl, buffer);
    if (buffer != NULL) {
        mpp_buffer_inc_ref(buffer);
    }

    (void)p;
    return MPP_OK;
}

static RK_S32 put_used_memory_handle(vpu_display_mem_pool *p, void *hdl)
{
    VPUMemLinear_t *dmabuf = (VPUMemLinear_t *)hdl;
    MppBuffer buf = (MppBuffer)dmabuf->offset;
    vpu_mem_dbg_func("pool %p hnd %p buffer %p\n", p, hdl, buf);
    if (buf != NULL) {
        mpp_buffer_put(buf);
        memset(dmabuf, 0, sizeof(VPUMemLinear_t));
    }
    (void)p;
    return MPP_OK;
}

static RK_S32 get_free_memory_num(vpu_display_mem_pool *p)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    RK_S32 ret = (p_mempool->group) ?
                 (mpp_buffer_group_unused(p_mempool->group)) : (0);

    vpu_mem_dbg_func("pool %p ret %d\n", p, ret);
    return ret;
}

static RK_S32 reset_vpu_mem_pool(vpu_display_mem_pool *p)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    mpp_buffer_group_clear(p_mempool->group);
    return 0;
}


vpu_display_mem_pool* open_vpu_memory_pool()
{
    vpu_display_mem_pool_impl *p_mempool =
        mpp_calloc(vpu_display_mem_pool_impl, 1);

    mpp_env_get_u32("vpu_mem_debug", &vpu_mem_debug, 0);
    vpu_mem_dbg_func("in  pool %p\n", p_mempool);

    if (NULL == p_mempool) {
        return NULL;
    }
    mpp_buffer_group_get_external(&p_mempool->group, MPP_BUFFER_TYPE_ION);
    if (NULL == p_mempool->group) {
        return NULL;
    }
    p_mempool->commit_hdl     = commit_memory_handle;
    p_mempool->get_free       = get_free_memory_vpumem;
    p_mempool->put_used       = put_used_memory_handle;
    p_mempool->inc_used       = inc_used_memory_handle_ref;
    p_mempool->reset          = reset_vpu_mem_pool;
    p_mempool->get_unused_num = get_free_memory_num;
    p_mempool->version        = 1;
    p_mempool->buff_size      = -1;

    vpu_mem_dbg_func("out pool %p group %p\n", p_mempool, p_mempool->group);
    return (vpu_display_mem_pool*)p_mempool;
}

void close_vpu_memory_pool(vpu_display_mem_pool *p)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;

    vpu_mem_dbg_func("pool %p group %p\n", p_mempool, p_mempool->group);
    mpp_buffer_group_put(p_mempool->group);
    mpp_free(p_mempool);
    return;
}

int create_vpu_memory_pool_allocator(vpu_display_mem_pool **ipool,
                                     int num, int size)
{
    vpu_display_mem_pool_impl *p_mempool =
        mpp_calloc(vpu_display_mem_pool_impl, 1);

    mpp_env_get_u32("vpu_mem_debug", &vpu_mem_debug, 0);
    vpu_mem_dbg_func("in  pool %p num %d size %d\n", p_mempool, num, size);

    if (NULL == p_mempool)
        return -1;

    mpp_buffer_group_get_internal(&p_mempool->group, MPP_BUFFER_TYPE_ION);
    mpp_buffer_group_limit_config(p_mempool->group, 0, num + 4);
    p_mempool->commit_hdl     = commit_memory_handle;
    p_mempool->get_free       = get_free_memory_vpumem;
    p_mempool->put_used       = put_used_memory_handle;
    p_mempool->inc_used       = inc_used_memory_handle_ref;
    p_mempool->reset          = reset_vpu_mem_pool;
    p_mempool->get_unused_num = get_free_memory_num;
    p_mempool->version        = 0;
    p_mempool->buff_size      = size;
    p_mempool->size           = size;
    *ipool = (vpu_display_mem_pool*)p_mempool;

    vpu_mem_dbg_func("out pool %p group %p\n", p_mempool, p_mempool->group);
    return 0;
}

void release_vpu_memory_pool_allocator(vpu_display_mem_pool *ipool)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)ipool;
    if (p_mempool == NULL)
        return;

    vpu_mem_dbg_func("pool %p group %p\n", p_mempool, p_mempool->group);

    if (p_mempool->group) {
        mpp_buffer_group_put(p_mempool->group);
        p_mempool->group = NULL;
    }

    vpu_mem_dbg_func("free %p\n", p_mempool);
    mpp_free(p_mempool);
    return;
}

RK_S32 VPUMemJudgeIommu()
{
    int ret = 0;

    if (VPUClientGetIOMMUStatus() > 0) {
        //mpp_err("media.used.iommu");
        ret = 1;
    }

    return ret;
}


RK_S32 VPUMallocLinear(VPUMemLinear_t *p, RK_U32 size)
{
    int ret = 0;
    MppBuffer buffer = NULL;
    ret = mpp_buffer_get(NULL, &buffer, size);
    if (ret != MPP_OK) {
        return -1;
    }
    p->phy_addr = (RK_U32)mpp_buffer_get_fd(buffer);
    p->vir_addr = (RK_U32*)mpp_buffer_get_ptr(buffer);
    p->size = size;
    p->offset = (RK_U32*)buffer;
    return 0;
}

RK_S32 VPUMallocLinearFromRender(VPUMemLinear_t *p, RK_U32 size, void *ctx)
{
    VPUMemLinear_t *dma_buf = NULL;
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)ctx;
    if (ctx == NULL) {
        return VPUMallocLinear(p, size);
    }
    dma_buf = (VPUMemLinear_t *)
              p_mempool->get_free((vpu_display_mem_pool *)ctx);
    memset(p, 0, sizeof(VPUMemLinear_t));
    if (dma_buf != NULL) {
        if (dma_buf->size < size) {
            mpp_free(dma_buf);
            return -1;
        }
        memcpy(p, dma_buf, sizeof(VPUMemLinear_t));
        mpp_free(dma_buf);
        return 0;
    }
    return -1;
}

RK_S32 VPUFreeLinear(VPUMemLinear_t *p)
{
    if (p->offset != NULL) {
        put_used_memory_handle(NULL, p);
    }
    return 0;
}

RK_S32 VPUMemDuplicate(VPUMemLinear_t *dst, VPUMemLinear_t *src)
{
    MppBuffer buffer = (MppBuffer)src->offset;
    if (buffer != NULL) {
        mpp_buffer_inc_ref(buffer);
    }
    memcpy(dst, src, sizeof(VPUMemLinear_t));
    return 0;
}

RK_S32 VPUMemLink(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}

RK_S32 VPUMemFlush(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}

RK_S32 VPUMemClean(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}


RK_S32 VPUMemInvalidate(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}

RK_S32 VPUMemGetFD(VPUMemLinear_t *p)
{
    RK_S32 fd = 0;
    MppBuffer buffer = (MppBuffer)p->offset;
    fd = mpp_buffer_get_fd(buffer);
    return fd;
}

