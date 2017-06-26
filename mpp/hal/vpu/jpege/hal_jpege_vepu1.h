#ifndef __HAL_JPEGE_VEPU1_H
#define __HAL_JPEGE_VEPU1_H
#include "rk_type.h"

MPP_RET hal_jpege_vepu1_init(void *hal, MppHalCfg *cfg);
MPP_RET hal_jpege_vepu1_deinit(void *hal);
MPP_RET hal_jpege_vepu1_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET hal_jpege_vepu1_start(void *hal, HalTaskInfo *task);
MPP_RET hal_jpege_vepu1_wait(void *hal, HalTaskInfo *task);
MPP_RET hal_jpege_vepu1_reset(void *hal);
MPP_RET hal_jpege_vepu1_flush(void *hal);
MPP_RET hal_jpege_vepu1_control(void *hal, RK_S32 cmd, void *param);

#endif
