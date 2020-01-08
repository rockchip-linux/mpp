#ifndef __HAL_JPEGE_BASE_H
#define __HAL_JPEGE_BASE_H

#include "mpp_device.h"
#include "mpp_hal.h"

#define HAL_JPEGE_DBG_FUNCTION          (0x00000001)
#define HAL_JPEGE_DBG_INPUT             (0x00000010)
#define HAL_JPEGE_DBG_OUTPUT            (0x00000020)

#define hal_jpege_dbg(flag, fmt, ...)   _mpp_dbg(hal_jpege_debug, flag, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_f(flag, fmt, ...) _mpp_dbg_f(hal_jpege_debug, flag, fmt, ## __VA_ARGS__)

#define hal_jpege_dbg_func(fmt, ...)    hal_jpege_dbg_f(HAL_JPEGE_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_input(fmt, ...)   hal_jpege_dbg(HAL_JPEGE_DBG_INPUT, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_output(fmt, ...)  hal_jpege_dbg(HAL_JPEGE_DBG_OUTPUT, fmt, ## __VA_ARGS__)

typedef struct JpegeIocRegInfo_t {
    RK_U32              *regs;
    RegExtraInfo        extra_info;
} JpegeIocRegInfo;

typedef struct hal_jpege_ctx_s {
    IOInterruptCB       int_cb;
    MppDevCtx           dev_ctx;
    JpegeBits           bits;
    JpegeIocRegInfo     ioctl_info;

    MppEncCfgSet        *cfg;
    MppEncCfgSet        *set;
    JpegeSyntax         syntax;

    MppHalApi           hal_api;
} HalJpegeCtx;

extern RK_U32 hal_jpege_debug;

#endif
