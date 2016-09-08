/*
*
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
#ifndef __HAL_REGDRV_H__
#define __HAL_REGDRV_H__

#include <stdio.h>
#include "rk_type.h"
#include "mpp_err.h"
#include "rk_mpi.h"



typedef struct hal_regdrv_t {
    RK_U32 syn_id;
    RK_U32 reg_id;
    RK_U32 bitlen;
    RK_U32 bitpos;
    char   *name;
} HalRegDrv_t;



typedef struct hal_regctx_t {
    MppCtxType      type;
    MppCodingType   coding;
    RK_U32          reg_size;  //!< hard regs count
    RK_U32          *p_reg;
    RK_U32          emt_size;  //!< last reg syntax
    HalRegDrv_t     *p_emt;
    void            *log;       //!< for debug
} HalRegDrvCtx_t;



#ifdef  __cplusplus
extern "C" {
#endif

//MPP_RET hal_init_regdrv   (HalRegDrvCtx_t *ctx);
MPP_RET hal_set_regdrv    (HalRegDrvCtx_t *ctx, RK_U32 syn_id, RK_U32  val);
MPP_RET hal_get_regdrv    (HalRegDrvCtx_t *ctx, RK_U32 syn_id, RK_U32 *pval);
//MPP_RET hal_print_regdrv  (HalRegDrvCtx_t *ctx, RK_U32 syn_id);
//MPP_RET hal_deinit_regdrv (HalRegDrvCtx_t *ctx);

RK_U32  hal_get_regsize   (HalRegDrvCtx_t *ctx);
RK_U32 *hal_get_regptr    (HalRegDrvCtx_t *ctx);

#ifdef  __cplusplus
}
#endif


#endif /* __HAL_REGDRV_H__ */
