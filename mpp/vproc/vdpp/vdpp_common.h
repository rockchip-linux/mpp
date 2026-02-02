/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef VDPP_COMMON_H
#define VDPP_COMMON_H


#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_log.h"

#include "vdpp_api.h"
#include "vdpp_reg.h"
#include "vdpp2_reg.h"

/* marco define */
#define VDPP_TILE_W_MAX                 (120)
#define VDPP_TILE_H_MAX                 (480)

// marcos for common use
#ifndef ROUND
#define CEIL(a)                 (int)( (double)(a) > (int)(a) ? (int)((a)+1) : (int)(a) )
#define FLOOR(a)                (int)( (double)(a) < (int)(a) ? (int)((a)-1) : (int)(a) )
#define ROUND(a)                (int)( (a) > 0 ? ((double) (a) + 0.5) : ((double) (a) - 0.5) )
#endif

// marcos for zme
#define SCALE_FACTOR_DN_FIXPOINT_SHIFT  (12)
#define SCALE_FACTOR_UP_FIXPOINT_SHIFT  (16)
#define GET_SCALE_FACTOR_DN(src, dst)   ((((src) - 1) << SCALE_FACTOR_DN_FIXPOINT_SHIFT) / ((dst) - 1))
#define GET_SCALE_FACTOR_UP(src, dst)   ((((src) - 1) << SCALE_FACTOR_UP_FIXPOINT_SHIFT) / ((dst) - 1))

// marcos for hist
#define VDPP_HIST_PACKED_SIZE           (10240)
#define VDPP_HIST_GLOBAL_OFFSET         (16 * 16 * 16 * 18 / 8)
#define VDPP_HIST_GLOBAL_LENGTH         (256)

/* vdpp log marco */
#define VDPP_LOG_FATAL                  (0x00000001)
#define VDPP_LOG_ERROR                  (0x00000002)
#define VDPP_LOG_WARNING                (0x00000004)
#define VDPP_LOG_INFO                   (0x00000008)
#define VDPP_LOG_DEBUG                  (0x00000010)
#define VDPP_LOG_TRACE                  (0x00000020)

#define vdpp_log(log_func, log_lv, fmt, ...) \
    do {                                     \
        if (vdpp_debug & log_lv)             \
            log_func(fmt, ##__VA_ARGS__);    \
    } while (0)

#define vdpp_logf(fmt, ...) vdpp_log(mpp_logf_f, VDPP_LOG_FATAL, fmt, ##__VA_ARGS__)
#define vdpp_loge(fmt, ...) vdpp_log(mpp_loge_f, VDPP_LOG_ERROR, fmt, ##__VA_ARGS__)
#define vdpp_logw(fmt, ...) vdpp_log(mpp_logw_f, VDPP_LOG_WARNING, fmt, ##__VA_ARGS__)
#define vdpp_logi(fmt, ...) vdpp_log(mpp_logi_f, VDPP_LOG_INFO, fmt, ##__VA_ARGS__)
#define vdpp_logd(fmt, ...) vdpp_log(mpp_logi_f, VDPP_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define vdpp_logt(fmt, ...) vdpp_log(mpp_logi_f, VDPP_LOG_TRACE, fmt, ##__VA_ARGS__)
#define vdpp_logv(fmt, ...) vdpp_log(mpp_logi_f, VDPP_LOG_TRACE, fmt, ##__VA_ARGS__)

#define vdpp_enter()        vdpp_logv("--- enter ---");
#define vdpp_leave()        vdpp_logv("--- leave ---");

#define VDPP_COM_DEBUG_ENV_NAME "vdpp_debug"


typedef struct FdTransInfo_t {
    RK_U32 reg_idx;
    RK_U32 offset;
} VdppRegOffsetInfo;

typedef struct vdpp_addr {
    RK_U32 y;
    RK_U32 cbcr;
    RK_U32 cbcr_offset;
} VdppAddr;


extern RK_U32 vdpp_debug;
extern const char *working_mode_name[];

#ifdef __cplusplus
extern "C" {
#endif

/* common useful functions implemented in vdpp_common.c */
MPP_RET set_addr(struct vdpp_addr *addr, VdppImg *img);
RK_S32 get_avg_luma_from_hist(const RK_U32 *pHist, RK_S32 length);


/* declared common useful functions here, but implemented in vdppX.c */

/* implemented in vdpp1 */
typedef struct dmsr_params DmsrParams; // forward declarations
typedef struct zme_params ZmeParams;
typedef struct scl_info VdppZmeScaleInfo;

void vdpp_set_default_dmsr_param(DmsrParams *dmsr_param);
void vdpp_set_default_zme_param(ZmeParams *zme_param);
void set_dmsr_to_vdpp_reg(const DmsrParams *dmsr_param, RegDmsr *dmsr_reg);
void set_zme_to_vdpp_reg(const ZmeParams *zme_params, RegZme *zme_reg);
void calc_scale_factor(const ZmeParams *zme_params, VdppZmeScaleInfo *scale_info, RK_U8 bypass_en);

/* implemented in vdpp2 */
typedef struct EsParams_t EsParams; // forward declarations
typedef struct ShpParams_t ShpParams;
typedef struct hist_params HistParams;

void vdpp2_set_default_hist_param(HistParams *hist_param);
void vdpp2_set_default_es_param(EsParams *es_param);
void vdpp2_set_default_shp_param(ShpParams *shp_param);
void set_hist_to_vdpp2_reg(HistParams *hist_param, RegHist *hist_reg, RegCom2 *com_reg);
void set_es_to_vdpp2_reg(EsParams *es_param, RegEs *es_reg);
void set_shp_to_vdpp2_reg(ShpParams *shp_param, RegShp *shp_reg);
int update_dci_ctl(HistParams *hist_params);

#ifdef __cplusplus
}
#endif

#endif /* VDPP_COMMON_H */
