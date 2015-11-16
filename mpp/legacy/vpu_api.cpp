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
#ifdef ANDROID
#include <dlfcn.h>
#endif

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"

#include "vpu_api_legacy.h"
#include "vpu_mem_legacy.h"
#include "vpu_api.h"

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

#ifdef ANDROID
RK_S32 open_orign_vpu(VpuCodecContext **ctx)
{
    void *rkapi_hdl = NULL;
    RK_S32 (*rkvpu_open_cxt)(VpuCodecContext **ctx);
    rkapi_hdl = dlopen("/system/lib/librk_vpuapi.so", RTLD_LAZY);
    if (rkapi_hdl == NULL) {
        mpp_log("dlopen librk_vpuapi library fail\n");
        rkapi_hdl = dlopen("/system/lib/librk_on2.so", RTLD_LAZY);
        if(rkapi_hdl == NULL){
        return -1;
        }
    }
    rkvpu_open_cxt = (RK_S32 (*)(VpuCodecContext **ctx))dlsym(rkapi_hdl, "vpu_open_context");
    if (rkvpu_open_cxt == NULL) {
        mpp_err("dlsym rkvpu_open_cxt fail ");
        dlclose(rkapi_hdl);
        return -1;
    } else {
        (*rkvpu_open_cxt)(ctx);
        return MPP_OK;
    }
    dlclose(rkapi_hdl);
}

RK_S32 close_orign_vpu(VpuCodecContext **ctx)
{
    void *rkapi_hdl = NULL;
    RK_S32 (*rkvpu_close_cxt)(VpuCodecContext **ctx);
    rkapi_hdl = dlopen("/system/lib/librk_vpuapi.so", RTLD_LAZY);
    if (rkapi_hdl == NULL) {
        mpp_log("dlopen librk_vpuapi library fail\n");
        rkapi_hdl = dlopen("/system/lib/librk_on2.so", RTLD_LAZY);
        if(rkapi_hdl == NULL){
        return -1;
        }
    }
    rkvpu_close_cxt = (RK_S32 (*)(VpuCodecContext **ctx))dlsym(rkapi_hdl, "vpu_close_context");
    if (rkvpu_close_cxt == NULL) {
        mpp_err_f("dlsym rkvpu_close_cxt fail");
        dlclose(rkapi_hdl);
        return -1;
    } else {
        (*rkvpu_close_cxt)(ctx);
        return MPP_OK;
    }
    dlclose(rkapi_hdl);
}
#endif
RK_S32 vpu_open_context(VpuCodecContext **ctx)
{
    VpuCodecContext *s = *ctx;
    RK_U32 value;
    mpp_env_get_u32("chg_orig", &value, 0);
#ifdef ANDROID
    if (value || !s) {
        if (s) {
            free(s);
            s = NULL;
        }
        open_orign_vpu(&s);
        s->extra_cfg.reserved[0] = 1;
        *ctx = s;
        return MPP_OK;
    }
#endif
    if (s != NULL) {
        mpp_log("s->videoCoding = %d", s->videoCoding);
        if (s->videoCoding == OMX_RK_VIDEO_CodingHEVC
            ||(s->videoCoding == OMX_RK_VIDEO_CodingAVC &&
            s->codecType == CODEC_DECODER)) {
            free(s);
            s = NULL;
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
        } else {
#ifdef ANDROID
            free(s);
            s = NULL;
            open_orign_vpu(&s);
            s->extra_cfg.reserved[0] = 1;
            *ctx = s;
            return MPP_OK;
#endif
        }
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
    RK_U32 value;
    mpp_env_get_u32("chg_orig", &value, 0);
#ifdef ANDROID
    if (value || s->extra_cfg.reserved[0]) {
        close_orign_vpu(ctx);
        mpp_log("org vpu_close_context ok");
        return MPP_OK;
    }
#endif
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
