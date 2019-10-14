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

#ifndef __MPP_ENC_IMPL_H__
#define __MPP_ENC_IMPL_H__

#include "enc_impl_api.h"

typedef void* EncImpl;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET enc_impl_init(EncImpl *ctrl, EncImplCfg *cfg);
MPP_RET enc_impl_deinit(EncImpl ctrl);

MPP_RET enc_impl_proc_cfg(EncImpl ctrl, MpiCmd cmd, void *para);
MPP_RET enc_impl_gen_hdr(EncImpl ctrl, MppPacket pkt);

MPP_RET enc_impl_proc_dpb(EncImpl ctrl);
MPP_RET enc_impl_proc_rc(EncImpl ctrl);
MPP_RET enc_impl_proc_hal(EncImpl ctrl, HalEncTask *task);

MPP_RET enc_impl_update_dpb(EncImpl ctrl);
MPP_RET enc_impl_update_hal(EncImpl ctrl, HalEncTask *task);
MPP_RET enc_impl_update_rc(EncImpl ctrl);

MPP_RET hal_enc_callback(void* ctrl, void *err_info);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_IMPL_H__*/
