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

#define MODULE_TAG "vpu_api"

#include <string.h>
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_buffer.h"
#include "vpu_api_legacy.h"
#include "vpu_api.h"
#include "vpu.h"

#ifdef ANDROID
#include <linux/ion.h>
#endif

typedef struct vpu_display_mem_pool_impl {
    vpu_display_mem_pool_FIELDS
    MppBufferGroup group;
    RK_S32 size;
} vpu_display_mem_pool_impl;

static RK_S32 vpu_api_init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size)
{
    mpp_log("vpu_api_init in, extra_size: %d", extra_size);

    if (ctx == NULL) {
        mpp_log("vpu_api_init fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }
    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_init fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->init(ctx, extraData, extra_size);
}

static RK_S32 vpu_api_decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_decode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_decode fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->decode(ctx, pkt, aDecOut);
}
static RK_S32 vpu_api_sendstream(VpuCodecContext *ctx, VideoPacket_t *pkt)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_decode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_sendstream fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->decode_sendstream(pkt);
}

static RK_S32 vpu_api_getframe(VpuCodecContext *ctx, DecoderOut_t *aDecOut)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_decode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_getframe fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->decode_getoutframe(aDecOut);
}

static RK_S32 vpu_api_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_decode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_sendframe fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->encoder_sendframe(ctx, aEncInStrm);
}

static RK_S32 vpu_api_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_decode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_getframe fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->encoder_getstream(ctx, aEncOut);
}



static RK_S32 vpu_api_encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_encode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_encode fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->encode(ctx, aEncInStrm, aEncOut);
}

static RK_S32 vpu_api_flush(VpuCodecContext *ctx)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_encode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_flush fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    return api->flush(ctx);
}

static RK_S32 vpu_api_control(VpuCodecContext *ctx, VPU_API_CMD cmdType, void *param)
{
    if (ctx == NULL) {
        mpp_log("vpu_api_decode fail, input invalid");
        return VPU_API_ERR_UNKNOW;
    }

    VpuApi* api = (VpuApi*)(ctx->vpuApiObj);
    if (api == NULL) {
        mpp_log("vpu_api_decode fail, vpu api invalid");
        return VPU_API_ERR_UNKNOW;
    }

    mpp_log("vpu_api_control in");
    switch (cmdType) {
    case VPU_API_SET_VPUMEM_CONTEXT: {

        mpp_log("vpu_api_control in vpu mem contxt");
        vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)param;

        param = (void*)p_mempool->group;
        break;
    }
    default: {
        break;
    }
    }

    mpp_log("vpu_api_control to mpi");
    return api->control(ctx, cmdType, param);
}

RK_S32 vpu_open_context(VpuCodecContext **ctx)
{
    mpp_log("vpu_open_context in");
    VpuCodecContext *s = *ctx;

    if (!s) {
        s = mpp_malloc(VpuCodecContext, 1);
        if (!s) {
            mpp_err("Input context has not been properly allocated");
            return -1;
        }
        memset(s, 0, sizeof(VpuCodecContext));
        s->enableparsing = 1;

        VpuApi* api = new VpuApi();

        if (api == NULL) {
            mpp_err("Vpu api object has not been properly allocated");
            return -1;
        }

        s->vpuApiObj = (void*)api;
        s->init = vpu_api_init;
        s->decode = vpu_api_decode;
        s->encode = vpu_api_encode;
        s->flush = vpu_api_flush;
        s->control = vpu_api_control;
        s->decode_sendstream = vpu_api_sendstream;
        s->decode_getframe = vpu_api_getframe;
        s->encoder_sendframe = vpu_api_sendframe;
        s->encoder_getstream = vpu_api_getstream;

        *ctx = s;
        return 0;
    }

    if (!s->vpuApiObj) {
        mpp_err("Input context has not been properly allocated and is not NULL either");
        return -1;
    }
    return 0;
}

RK_S32 vpu_close_context(VpuCodecContext **ctx)
{
    mpp_log("vpu_close_context in");
    VpuCodecContext *s = *ctx;

    if (s) {
        VpuApi* api = (VpuApi*)(s->vpuApiObj);
        if (s->vpuApiObj) {
            delete api;
            s->vpuApiObj = NULL;
        }
        mpp_free(s->extradata);
        mpp_free(s->private_data);
        mpp_free(s);
        *ctx = s = NULL;

        mpp_log("vpu_close_context ok");
    }
    return 0;
}

static RK_S32 commit_memory_handle(vpu_display_mem_pool *p, RK_S32 mem_hdl, RK_S32 size)
{
    MppBufferInfo info;
    MPP_RET ret = MPP_OK;

    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    memset(&info, 0, sizeof(MppBufferInfo));
    info.type = MPP_BUFFER_TYPE_ION;
    info.fd = mem_hdl;
    info.size = size;
    p_mempool->size = size;
    ret = mpp_buffer_commit(p_mempool->group, &info);
    return mem_hdl;
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
    ret = mpp_buffer_get(p_mempool->group, &buffer, p_mempool->size);
    if (MPP_OK != ret) {
        mpp_free(dmabuf);
        return NULL;
    }
    dmabuf->phy_addr = (RK_U32)mpp_buffer_get_fd(buffer);
    dmabuf->vir_addr = (RK_U32*)mpp_buffer_get_ptr(buffer);
    dmabuf->size = p_mempool->size;
    dmabuf->offset = (RK_U32*)buffer;
    return NULL;

}

static RK_S32 inc_used_memory_handle_ref(vpu_display_mem_pool *p, void * hdl)
{
    (void)p;
    VPUMemLinear_t *dmabuf = (VPUMemLinear_t *)hdl;
    MppBuffer buffer = (MppBuffer)dmabuf->offset;
    if (buffer != NULL) {
        mpp_buffer_inc_ref(buffer);
    }
    return MPP_OK;

}

static RK_S32 put_used_memory_handle(vpu_display_mem_pool *p, void *hdl)
{
    (void)p;
    VPUMemLinear_t *dmabuf = (VPUMemLinear_t *)hdl;
    MppBuffer buf = (MppBuffer)dmabuf->offset;
    if (buf != NULL) {
        mpp_buffer_put(buf);
    }
    return MPP_OK;
}

static RK_S32 get_free_memory_num(vpu_display_mem_pool *p)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    if (p_mempool->group != NULL) {
        return mpp_buffer_group_unused(p_mempool->group);
    }
    return 0;
}

static RK_S32 reset_vpu_mem_pool(vpu_display_mem_pool *p)
{
    (void)p;
    //  vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    return 0;

}


vpu_display_mem_pool* open_vpu_memory_pool()
{
    mpp_err("open_vpu_memory_pool in\n");
    vpu_display_mem_pool_impl *p_mempool = mpp_calloc(vpu_display_mem_pool_impl, 1);

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
    return (vpu_display_mem_pool*)p_mempool;
}

void close_vpu_memory_pool(vpu_display_mem_pool *p)
{

    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    mpp_buffer_group_put(p_mempool->group);
    return;
}

int create_vpu_memory_pool_allocator(vpu_display_mem_pool **ipool, int num, int size)
{
    (void)ipool;
    (void)num;
    (void)size;
    return 0;
}

void release_vpu_memory_pool_allocator(vpu_display_mem_pool *ipool)
{
    (void)ipool;
}

RK_S32 VPUMemJudgeIommu()
{
    int ret = 0;
#ifdef ANDROID
    if (VPUClientGetIOMMUStatus() > 0) {
        //mpp_err("media.used.iommu");
        ret = 1;
    }
#endif
    return ret;
}


RK_S32 VPUMallocLinear(VPUMemLinear_t *p, RK_U32 size)
{
    (void)p;
    (void)size;
    return 0;
}

RK_S32 VPUFreeLinear(VPUMemLinear_t *p)
{
    put_used_memory_handle(NULL, p);
    return 0;
}


RK_S32 VPUMemDuplicate(VPUMemLinear_t *dst, VPUMemLinear_t *src)
{
    (void)dst;
    (void)src;
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
    //mpp_err("fd = 0x%x",fd);
    return fd;

}

RK_S32 vpu_mem_judge_used_heaps_type()
{
    // TODO, use property_get
#if 0 //def ANDROID
    if (!VPUClientGetIOMMUStatus() > 0) {
        return ION_HEAP(ION_CMA_HEAP_ID);
    } else {
        ALOGV("USE ION_SYSTEM_HEAP");
        return ION_HEAP(ION_VMALLOC_HEAP_ID);
    }
#endif

    return 0;
}



