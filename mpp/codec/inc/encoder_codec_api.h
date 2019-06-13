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

#ifndef __CONTROL_API_H__
#define __CONTROL_API_H__

#include "mpp_buf_slot.h"
#include "hal_task.h"

// config cmd
typedef enum EncCfgCmd_t {
    CHK_ENC_CFG,
    SET_ENC_CFG,
    SET_ENC_RC_CFG,
    SET_IDR_FRAME,
} EncCfgCmd;

/*
 * the reset wait for extension
 */
typedef struct EncControllerInitCfg_t {
    // input
    MppCodingType   coding;
    MppEncCfgSet    *cfg;
    MppEncCfgSet    *set;

    // output
    RK_S32          task_count;
} ControllerCfg;

/*
 * ControlApi is the data structure provided from different encoders
 *
 * They will be static register to mpp_enc for scaning
 * name     - encoder name
 * coding   - encoder coding type
 * ctx_size - encoder context size, mpp_dec will use this to malloc memory
 * flag     - reserve
 *
 * init     - encoder initialization function
 * deinit   - encoder de-initialization function
 * encoder    - encoder main working function, mpp_dec will input packet and get output syntax
 * reset    - encoder reset function
 * flush    - encoder output all frames
 * control  - encoder configure function
 */
typedef struct ControlApi_t {
    char            *name;
    MppCodingType   coding;
    RK_U32          ctx_size;
    RK_U32          flag;

    MPP_RET (*init)(void *ctx, ControllerCfg *ctrlCfg);
    MPP_RET (*deinit)(void *ctx);

    MPP_RET (*encode)(void *ctx, HalEncTask *task);

    MPP_RET (*reset)(void *ctx);
    MPP_RET (*flush)(void *ctx);
    MPP_RET (*config)(void *ctx, RK_S32 cmd, void *param);
    MPP_RET (*callback)(void *ctx, void *feedback);
} ControlApi;

#endif /*__CONTROL_API_H__*/
