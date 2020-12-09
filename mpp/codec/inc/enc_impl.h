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

#ifndef __ENC_IMPL_H__
#define __ENC_IMPL_H__

#include "enc_impl_api.h"

typedef void* EncImpl;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET enc_impl_init(EncImpl *impl, EncImplCfg *cfg);
MPP_RET enc_impl_deinit(EncImpl impl);

MPP_RET enc_impl_proc_cfg(EncImpl impl, MpiCmd cmd, void *para);
MPP_RET enc_impl_gen_hdr(EncImpl impl, MppPacket pkt);

MPP_RET enc_impl_start(EncImpl impl, HalEncTask *task);
MPP_RET enc_impl_proc_dpb(EncImpl impl, HalEncTask *task);
MPP_RET enc_impl_proc_hal(EncImpl impl, HalEncTask *task);

MPP_RET enc_impl_add_prefix(EncImpl impl, MppPacket pkt, RK_S32 *length,
                            RK_U8 uuid[16], const void *data, RK_S32 size);

MPP_RET enc_impl_sw_enc(EncImpl impl, HalEncTask *task);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_ENC_IMPL_H__*/
