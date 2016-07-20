#ifndef __HAL_H264E_API_H__
#define __HAL_H264E_API_H__

#include "mpp_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const MppHalApi hal_api_h264e;

MPP_RET hal_h264e_init    (void *hal, MppHalCfg *cfg);
MPP_RET hal_h264e_deinit  (void *hal);
MPP_RET hal_h264e_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_start   (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_wait    (void *hal, HalTaskInfo *task);
MPP_RET hal_h264e_reset   (void *hal);
MPP_RET hal_h264e_flush   (void *hal);
MPP_RET hal_h264e_control (void *hal, RK_S32 cmd_type, void *param);

#ifdef __cplusplus
}
#endif

#endif