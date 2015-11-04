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

#define  MODULE_TAG  "hal_regdrv"
#include <stdlib.h>
#include <string.h>
#include "rk_type.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "hal_regdrv.h"



static const RK_U32 reg_mask[33] = { 0x00000000,
                                     0x00000001, 0x00000003, 0x00000007, 0x0000000F,
                                     0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
                                     0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
                                     0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
                                     0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
                                     0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
                                     0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
                                     0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
                                   };


/*!
***********************************************************************
* \brief
*   Print register
***********************************************************************
*/
//MPP_RET hal_deinit_regdrv (HalRegDrvCtx_t *ctx)
//{
//  if (NULL == ctx) {
//      mpp_err_f("found NULL input\n");
//      return MPP_ERR_NULL_PTR;
//  }
//  MPP_FREE(ctx->p_syn);
//  MPP_FREE(ctx->p_reg);
//
//  return MPP_OK;
//}

/*!
***********************************************************************
* \brief
*   Init context
***********************************************************************
*/

//MPP_RET hal_init_regdrv(HalRegDrvCtx_t *ctx)
//{
//  if (NULL == ctx) {
//      mpp_err_f("found NULL input\n");
//      return MPP_ERR_NULL_PTR;
//  }
//
//  if (ctx->reg_size && ctx->syn_size) {
//      ctx->p_syn = mpp_malloc(HalRegDrv_t, ctx->syn_size);
//      ctx->p_reg = mpp_malloc(RK_U32,      ctx->reg_size);
//      if ((NULL == ctx->p_syn) ||(NULL == ctx->p_reg)){
//          mpp_err_f("malloc buffer\n");
//          return MPP_ERR_MALLOC;
//      }
//  }
//
//  return MPP_OK;
//}
/*!
***********************************************************************
* \brief
*   set syntax element to the register position
***********************************************************************
*/
MPP_RET hal_set_regdrv(HalRegDrvCtx_t *ctx, RK_U32 syn_id, RK_U32 val)
{
    RK_U32 reg_id = 0;
    RK_U32 bitpos = 0;
    RK_U32 bitlen = 0;
    RK_U32 valtmp = 0;

    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    mpp_assert(syn_id < ctx->emt_size);
    reg_id = ctx->p_emt[syn_id].reg_id;
    bitpos = ctx->p_emt[syn_id].bitpos;
    bitlen = ctx->p_emt[syn_id].bitlen;

    valtmp  = ctx->p_reg[reg_id];
    valtmp &= ~(reg_mask[bitlen] << bitpos);
    valtmp |= (val & reg_mask[bitlen]) << bitpos;
    ctx->p_reg[reg_id] = valtmp;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   Get register value from the positon
***********************************************************************
*/
MPP_RET hal_get_regdrv(HalRegDrvCtx_t *ctx, RK_U32 syn_id, RK_U32 *pval)
{
    RK_U32 reg_id = 0;
    RK_U32 bitpos = 0;
    RK_U32 bitlen = 0;
    RK_U32 valtmp = 0;

    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    mpp_assert(syn_id < ctx->emt_size);
    reg_id = ctx->p_emt[syn_id].reg_id;
    bitpos = ctx->p_emt[syn_id].bitpos;
    bitlen = ctx->p_emt[syn_id].bitlen;

    valtmp  = ctx->p_reg[reg_id];
    valtmp  = valtmp >> bitpos;
    valtmp &= reg_mask[bitlen];

    *pval = valtmp;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   Print register
***********************************************************************
*/
//MPP_RET hal_print_regdrv(HalRegDrvCtx_t *ctx, RK_U32 syn_id)
//{
//  MPP_RET ret = MPP_ERR_UNKNOW;
//  RK_U32  val = 0;
//
//  if (NULL == ctx) {
//      mpp_err_f("found NULL input\n");
//      return MPP_ERR_NULL_PTR;
//  }
//  if(ret = hal_get_regdrv(ctx, syn_id, &val)) {
//      return ret;
//  }
//  //if (NULL != ctx->fp) {
//  //  fprintf(ctx->fp, "%48s = %10d \n", ctx->p_syn[syn_id].name, val);
//  //}
//  return MPP_OK;
//}

/*!
***********************************************************************
* \brief
*   get regsize
***********************************************************************
*/
RK_U32  hal_get_regsize(HalRegDrvCtx_t *ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    return ctx->reg_size;
}
/*!
***********************************************************************
* \brief
*   get reg data
***********************************************************************
*/
RK_U32 *hal_get_regptr(HalRegDrvCtx_t *ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return NULL;
    }

    return ctx->p_reg;
}