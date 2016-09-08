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

#ifndef __ENCODER_CONTROLLER_API_H__
#define __ENCODER_CONTROLLER_API_H__

#include "encoder_codec_api.h"

typedef void* Controller;

typedef struct EncTask_t {
    HalTaskHnd      hnd;

    RK_S32          hal_frm_idx_in;
    RK_S32          hal_pkt_idx_out;

    MppBuffer       ctrl_frm_buf_in;
    MppBuffer       ctrl_pkt_buf_out;

    h264e_syntax    syntax_data;

    HalTaskInfo     info;
} EncTask;


#ifdef __cplusplus
extern "C" {
#endif

MPP_RET controller_init(Controller *ctrl, ControllerCfg *cfg);
MPP_RET controller_deinit(Controller ctrl);
MPP_RET controller_encode(Controller ctrl, HalEncTask *task);
MPP_RET controller_config(Controller ctrl, RK_S32 cmd, void *para);
MPP_RET hal_enc_callback(void* ctrl, void *err_info);

#ifdef __cplusplus
}
#endif

#endif /*__ENCODER_CONTROLLER_API_H__*/
