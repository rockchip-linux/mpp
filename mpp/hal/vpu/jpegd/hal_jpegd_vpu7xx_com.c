/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#include "hal_jpegd_vpu7xx_com.h"

#include "mpp_debug.h"
#include "mpp_common.h"

#include "jpegd_syntax.h"
#include "hal_jpegd_common.h"

MPP_RET jpegd_vpu7xx_write_qtbl(JpegdHalCtx *ctx, JpegdSyntax *syntax)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;
    JpegdSyntax *s = syntax;
    RK_U16 *base = (RK_U16 *)mpp_buffer_get_ptr(ctx->pTableBase);
    RK_U16 table_tmp[QUANTIZE_TABLE_LENGTH] = {0};
    RK_U32 i, j , idx;

    for (j = 0; j < s->nb_components; j++) {
        idx = s->quant_index[j];

        //turn zig-zag order to raster-scan order
        for (i = 0; i < QUANTIZE_TABLE_LENGTH; i++) {
            table_tmp[zzOrder[i]] = s->quant_matrixes[idx][i];
        }

        memcpy(base + j * QUANTIZE_TABLE_LENGTH, table_tmp, sizeof(RK_U16) * QUANTIZE_TABLE_LENGTH);
    }

    if (jpegd_debug & JPEGD_DBG_HAL_TBL) {
        RK_U8 *data = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase);

        mpp_log("--------------Quant tbl----------------------\n");
        for (i = 0; i < RKD_QUANTIZATION_TBL_SIZE; i += 8) {
            mpp_log("%02x%02x%02x%02x%02x%02x%02x%02x\n",
                    data[i + 7], data[i + 6], data[i + 5], data[i + 4],
                    data[i + 3], data[i + 2], data[i + 1], data[i + 0]);
        }
    }

    jpegd_dbg_func("exit\n");
    return ret;

}

MPP_RET jpegd_vpu7xx_write_htbl(JpegdHalCtx *ctx, JpegdSyntax *jpegd_syntax)
{
    jpegd_dbg_func("enter\n");
    MPP_RET ret = MPP_OK;

    JpegdSyntax *s = jpegd_syntax;
    void * htbl_ptr[6] = {NULL};
    RK_U32 i, j, k = 0;
    RK_U8 *p_htbl_value = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_VALUE_TBL_OFFSET;
    RK_U16 *p_htbl_mincode = (RK_U16 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_MINCODE_TBL_OFFSET  / 2;
    RK_U16 min_code_ac[16] = {0};
    RK_U16 min_code_dc[16] = {0};
    RK_U16 acc_addr_ac[16] = {0};
    RK_U16 acc_addr_dc[16] = {0};
    RK_U8 htbl_value[192] = {0};
    RK_U16 code = 0;
    RK_S32 addr = 0;
    RK_U32 len = 0;
    AcTable *ac_ptr;
    DcTable *dc_ptr;

    htbl_ptr[0] = &s->dc_table[s->dc_index[0]];
    htbl_ptr[1] = &s->ac_table[s->ac_index[0]];

    htbl_ptr[2] = &s->dc_table[s->dc_index[1]];
    htbl_ptr[3] = &s->ac_table[s->ac_index[1]];

    htbl_ptr[4] = htbl_ptr[2];
    htbl_ptr[5] = htbl_ptr[3];

    for (k = 0; k < s->nb_components; k++) {
        dc_ptr = (DcTable *)htbl_ptr[k * 2];
        ac_ptr = (AcTable *)htbl_ptr[k * 2 + 1];

        len = dc_ptr->bits[0];
        code = addr = 0;
        min_code_dc[0] = 0;
        acc_addr_dc[0] = 0;

        for (j = 0; j < 16; j++) {
            len = dc_ptr->bits[j];

            if (len == 0 && j > 0) {
                if (code > (min_code_dc[j - 1] << 1))
                    min_code_dc[j] = code;
                else
                    min_code_dc[j] = min_code_dc[j - 1] << 1;
            } else {
                min_code_dc[j] = code;
            }

            code += len;
            addr += len;
            acc_addr_dc[j] = addr;
            code <<= 1;

        }

        if (dc_ptr->bits[15])
            min_code_dc[0] = min_code_dc[15] + dc_ptr->bits[15] - 1;
        else
            min_code_dc[0] = min_code_dc[15];

        len = ac_ptr->bits[0];
        code = addr = 0;
        min_code_ac[0] = 0;
        acc_addr_ac[0] = 0;
        for (j = 0; j < 16; j++) {
            len = ac_ptr->bits[j];

            if (len == 0 && j > 0) {
                if (code > (min_code_ac[j - 1] << 1))
                    min_code_ac[j] = code;
                else
                    min_code_ac[j] = min_code_ac[j - 1] << 1;
            } else {
                min_code_ac[j] = code;
            }

            code += len;
            addr += len;
            acc_addr_ac[j] = addr;
            code <<= 1;
        }

        if (ac_ptr->bits[15])
            min_code_ac[0] = min_code_ac[15] + ac_ptr->bits[15] - 1;
        else
            min_code_ac[0] = min_code_ac[15];

        for (i = 0; i < 16; i++) {
            *p_htbl_mincode++ = min_code_dc[i];
        }

        for (i = 0; i < 8; i++) {
            *p_htbl_mincode++ = acc_addr_dc[2 * i] | acc_addr_dc[2 * i + 1] << 8;
        }

        for (i = 0; i < 16; i++) {
            *p_htbl_mincode++ = min_code_ac[i];
        }

        for (i = 0; i < 8; i++) {
            *p_htbl_mincode++ = acc_addr_ac[2 * i] | acc_addr_ac[2 * i + 1] << 8;
        }

        for (i = 0; i < MAX_DC_HUFFMAN_TABLE_LENGTH; i++) {
            htbl_value[i] = dc_ptr->vals[i];
        }

        for (i = 0; i < MAX_AC_HUFFMAN_TABLE_LENGTH; i++) {
            htbl_value[i + 16] = ac_ptr->vals[i];
        }

        for (i = 0; i < 12 * 16; i++) {
            *p_htbl_value++ = htbl_value[i];
        }
    }

    if (jpegd_debug & JPEGD_DBG_HAL_TBL) {
        RK_U8 *data = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_VALUE_TBL_OFFSET;

        mpp_log("--------------huffman value tbl----------------------\n");
        for (i = 0; i < RKD_HUFFMAN_VALUE_TBL_SIZE; i += 8) {
            mpp_log("%02x%02x%02x%02x%02x%02x%02x%02x\n",
                    data[i + 7], data[i + 6], data[i + 5], data[i + 4],
                    data[i + 3], data[i + 2], data[i + 1], data[i + 0]);
        }

        data = NULL;
        data = (RK_U8 *)mpp_buffer_get_ptr(ctx->pTableBase) + RKD_HUFFMAN_MINCODE_TBL_OFFSET;

        mpp_log("--------------huffman mincode tbl----------------------\n");
        for (i = 0; i < RKD_HUFFMAN_MINCODE_TBL_SIZE; i += 8) {
            mpp_log("%02x%02x%02x%02x%02x%02x%02x%02x\n",
                    data[i + 7], data[i + 6], data[i + 5], data[i + 4],
                    data[i + 3], data[i + 2], data[i + 1], data[i + 0]);
        }
    }

    jpegd_dbg_func("exit\n");
    return ret;
}