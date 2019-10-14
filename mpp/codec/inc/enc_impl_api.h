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

#ifndef __ENC_IMPL_API_H__
#define __ENC_IMPL_API_H__

#include "rk_mpi_cmd.h"

#include "mpp_platform.h"
#include "hal_task.h"
#include "mpp_enc_cfg.h"

/*
 * the reset wait for extension
 */
typedef struct EncImplCfg_t {
    // input
    MppCodingType   coding;
    MppEncCfgSet    *cfg;
    MppEncCfgSet    *set;

    // output
    RK_S32          task_count;
} EncImplCfg;

/*
 * EncImplApi is the data structure provided from different encoders
 *
 * They will be static register to mpp_enc for scaning
 * name     - encoder name
 * coding   - encoder coding type
 * ctx_size - encoder context size, mpp_dec will use this to malloc memory
 * flag     - reserve
 *
 * init     - encoder initialization function
 * deinit   - encoder de-initialization function
 * encode   - encoder main working function
 * reset    - encoder reset function
 * flush    - encoder output all frames
 * control  - encoder configure function
 */
typedef struct EncImplApi_t {
    char            *name;
    MppCodingType   coding;
    RK_U32          ctx_size;
    RK_U32          flag;

    MPP_RET (*init)(void *ctx, EncImplCfg *ctrlCfg);
    MPP_RET (*deinit)(void *ctx);

    MPP_RET (*proc_cfg)(void *ctx, MpiCmd cmd, void *param);
    MPP_RET (*gen_hdr)(void *ctx, MppPacket pkt);

    MPP_RET (*proc_dpb)(void *ctx);
    MPP_RET (*proc_rc)(void *ctx);
    MPP_RET (*proc_hal)(void *ctx, HalEncTask *task);

    MPP_RET (*update_dpb)(void *ctx);
    MPP_RET (*update_hal)(void *ctx, HalEncTask *task);
    MPP_RET (*update_rc)(void *ctx);

    MPP_RET (*reset)(void *ctx);
    MPP_RET (*flush)(void *ctx);
    MPP_RET (*callback)(void *ctx, void *feedback);
} EncImplApi;

#endif /*__ENC_IMPL_API_H__*/
