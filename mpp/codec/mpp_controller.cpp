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

#define  MODULE_TAG "mpp_enc"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "h264e_api.h"
#include "mpp_controller.h"

/*
 * all decoder static register here
 */
static const ControlApi *controllers[] = {
    &api_h264e_controller,
};

typedef struct ControllerImpl_t {
    ControllerCfg           cfg;
    const ControlApi     *api;
    void                *ctx;
} ControllerImpl;

MPP_RET controller_encode(Controller ctrller, /*HalEncTask **/EncTask *task)
{
    if (NULL == ctrller || NULL == task) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    ControllerImpl *p = (ControllerImpl *)ctrller;
    if (!p->api->encode)
        return MPP_OK;

    return p->api->encode(p->ctx, (void*)task);
}

MPP_RET controller_init(Controller *ctrl, ControllerCfg *cfg)
{
    if (NULL == ctrl || NULL == cfg) {
        mpp_err_f("found NULL input controller %p config %p\n", ctrl, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *ctrl = NULL;

    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(controllers); i++) {
        const ControlApi *api = controllers[i];
        if (cfg->coding == api->coding) {
            ControllerImpl *p = mpp_calloc(ControllerImpl, 1);
            void *ctx = mpp_calloc_size(void, api->ctx_size);
            if (NULL == ctx || NULL == p) {
                mpp_err_f("failed to alloc parser context\n");
                mpp_free(p);
                mpp_free(ctx);
                return MPP_ERR_MALLOC;
            }

            MPP_RET ret = api->init(ctx, cfg);  // FIXME need to be thinked by lance 2016.05.18
            if (MPP_OK != ret) {
                mpp_err_f("failed to init controller\n");
                mpp_free(p);
                mpp_free(ctx);
                return ret;
            }

            p->cfg  = *cfg;
            p->api  = api;
            p->ctx  = ctx;
            *ctrl = p;
            return MPP_OK;
        }
    }

    return MPP_NOK;
}

MPP_RET controller_deinit(Controller ctrl)
{
    if (NULL == ctrl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    ControllerImpl *p = (ControllerImpl *)ctrl;
    if (p->api->deinit)
        p->api->deinit(p->ctx);

    mpp_free(p->ctx);
    mpp_free(p);
    return MPP_OK;
}

MPP_RET hal_enc_callback(void *ctrller, void *err_info)
{
    if (NULL == ctrller) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    ControllerImpl *p = (ControllerImpl *)ctrller;
    if (!p->api->callback)
        return MPP_OK;
    return p->api->callback(p->ctx, err_info);
}

MPP_RET controller_config(Controller ctrller, RK_S32 cmd, void *para)
{
    if (NULL == ctrller) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    ControllerImpl *p = (ControllerImpl *)ctrller;
    if (!p->api->config)
        return MPP_OK;

    return p->api->config(p->ctx, cmd, para);
}
