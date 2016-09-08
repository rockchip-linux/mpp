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
#ifndef __H264D_LOG_H__
#define __H264D_LOG_H__

#include <stdio.h>
#include <assert.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_bitread.h"


#define H264D_DBG_ERROR             (0x00000001)
#define H264D_DBG_ASSERT            (0x00000002)
#define H264D_DBG_WARNNING          (0x00000004)
#define H264D_DBG_LOG               (0x00000008)

#define H264D_DBG_INPUT             (0x00000010)   //!< input packet
#define H264D_DBG_PPS_SPS           (0x00000020)
#define H264D_DBG_LOOP_STATE        (0x00000040)
#define H264D_DBG_PARSE_NALU        (0x00000080)

#define H264D_DBG_DPB_INFO          (0x00000100)   //!< size, 
#define H264D_DBG_DPB_MALLIC        (0x00000200)   //!< malloc


#define H264D_DBG_DPB_REF_ERR       (0x00001000)
#define H264D_DBG_SLOT_FLUSH        (0x00002000)   //!< dpb buffer slot remain
#define H264D_DBG_SEI               (0x00004000)
#define H264D_DBG_CALLBACK          (0x00008000)

#define H264D_DBG_WRITE_ES_EN       (0x00010000)   //!< write input ts stream
#define H264D_DBG_FIELD_PAIRED      (0x00020000)
#define H264D_DBG_DISCONTINUOUS     (0x00040000)
#define H264D_DBG_ERR_DUMP          (0x00080000)
//!< hal marco
#define H264D_DBG_GEN_REGS          (0x01000000)
#define H264D_DBG_RET_REGS          (0x02000000)
#define H264D_DBG_DECOUT_INFO       (0x04000000)

extern RK_U32 rkv_h264d_parse_debug;

extern RK_U32 rkv_h264d_hal_debug;


#define H264D_DBG(level, fmt, ...)\
do {\
    if (level & rkv_h264d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__);}\
} while (0)


#define H264D_ERR(fmt, ...)\
do {\
    if (H264D_DBG_ERROR & rkv_h264d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define ASSERT(val)\
do {\
    if (H264D_DBG_ASSERT & rkv_h264d_parse_debug)\
        { mpp_assert(val); }\
} while (0)

#define H264D_WARNNING(fmt, ...)\
do {\
    if (H264D_DBG_WARNNING & rkv_h264d_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#define H264D_LOG(fmt, ...)\
do {\
    if (H264D_DBG_LOG & rkv_h264d_parse_debug)\
        {  mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)



#define   __DEBUG_EN   1


//!< get bit value
#define GetBitVal(val, pos)   ( ( (val)>>(pos) ) & 0x1 & (val) )


typedef enum {
    LOG_LEVEL_SILENT   = 0,
    LOG_LEVEL_FATAL       ,
    LOG_LEVEL_ERROR       ,
    LOG_LEVEL_WARNNING    ,
    LOG_LEVEL_INFO        ,
    LOG_LEVEL_TRACE       ,
    LOG_LEVEL_MAX         ,
} LogLevel_type;

typedef enum {
    LOG_DEBUG         = 0,
    LOG_FPGA             ,
    LOG_PRINT            ,
    LOG_WRITE            ,
    RUN_PARSE            ,
    RUN_HAL              ,
    LOG_READ_NALU        ,
    LOG_READ_SPS         ,
    LOG_READ_SUBSPS      ,
    LOG_READ_PPS         ,
    LOG_READ_SLICE       ,
    LOG_WRITE_SPSPPS     ,
    LOG_WRITE_RPS        ,
    LOG_WRITE_SCANLIST   ,
    LOG_WRITE_STEAM      ,
    LOG_WRITE_REG        ,
    LOG_MAX              ,
} LogCtrl_type;

typedef struct log_flag_t {
    RK_U8        debug_en;
    RK_U8        print_en;
    RK_U8        write_en;
    RK_U32       level;
} LogFlag_t;

typedef struct log_ctx_t {
    const char   *tag;
    LogFlag_t    *flag;
    FILE         *fp;
} LogCtx_t;

typedef struct log_env_str_t {
    char *help;
    char *show;
    char *ctrl;
    char *level;
    char *outpath;
    char *cmppath;
    char *decframe;
    char *begframe;
    char *endframe;
} LogEnvStr_t;


typedef struct log_env_ctx_t {
    RK_U32 help;
    RK_U32 show;
    RK_U32 ctrl;
    RK_U32 level;
    RK_U32 decframe;
    RK_U32 begframe;
    RK_U32 endframe;
    char   *outpath;
    //!< files
    FILE *fp_driver;
    FILE *fp_syn_parse;
    FILE *fp_syn_hal;
    FILE *fp_run_parse;
    FILE *fp_run_hal;

} LogEnv_t;

typedef struct h264d_logctx_t {
    LogEnv_t  env;
    LogFlag_t log_flag;
    LogCtx_t  *parr[LOG_MAX];
} H264dLogCtx_t;

//!< write log
#define LogEnable(ctx, loglevel)  ( ctx && ((LogCtx_t*)ctx)->flag->debug_en && (((LogCtx_t*)ctx)->flag->level & loglevel) )

#define LogTrace(ctx, ...)\
        do{ if(LogEnable(ctx, LOG_LEVEL_TRACE)) {\
                 writelog(ctx, "TRACE", __FILE__, __LINE__, ##__VA_ARGS__);\
                 } }while (0)
#define LogInfo(ctx, ...)\
        do{ if(LogEnable(ctx, LOG_LEVEL_INFO)) {\
                 writelog(ctx, "INFO",__FILE__, __LINE__, ##__VA_ARGS__);\
                 } }while (0)

#define LogWarnning(ctx, ...)\
        do{ if(LogEnable(ctx, LOG_LEVEL_WARNNING)) {\
                 writelog(ctx, "WARNNING",__FILE__, __LINE__, ##__VA_ARGS__);\
                 } }while (0)

#define LogError(ctx, ...)\
        do{ if(LogEnable(ctx, LOG_LEVEL_ERROR)) {\
                 writelog(ctx, "ERROR",__FILE__, __LINE__, ##__VA_ARGS__);\
                 ASSERT(0);\
                 } }while (0)
#define LogFatal(ctx, ...)\
        do{ if(LogEnable(ctx, LOG_LEVEL_ERROR)) {\
                 writelog(ctx, "FATAL", __FILE__, __LINE__, ##__VA_ARGS__);\
                 ASSERT(0);\
                 } }while (0)

#define FunctionIn(ctx)\
        do{ if(LogEnable(ctx, LOG_LEVEL_TRACE)) {\
                 writelog(ctx, "FunIn",__FILE__, __LINE__, __FUNCTION__);\
                 } } while (0)

#define FunctionOut(ctx)\
        do{if(LogEnable(ctx, LOG_LEVEL_TRACE)) {\
                 writelog(ctx, "FunOut", __FILE__, __LINE__, __FUNCTION__);\
                 } } while (0)

//!< vaule check
#define VAL_CHECK(ret, val, ...)\
    do{ if(!(val)){\
    ret = MPP_ERR_VALUE;\
    H264D_WARNNING("value error(%d).\n", __LINE__);\
    goto __FAILED;\
    } } while (0)
//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
    do{ if(!(val)) {\
    ret = MPP_ERR_MALLOC;\
    H264D_ERR("malloc buffer error(%d).\n", __LINE__);\
    ASSERT(0); goto __FAILED;\
    } } while (0)
//!< file check
#define FLE_CHECK(ret, val, ...)\
    do{ if(!(val)) {\
    ret = MPP_ERR_OPEN_FILE;\
    H264D_WARNNING("open file error(%d).\n", __LINE__);\
    ASSERT(0); goto __FAILED;\
    } } while (0)

//!< input check
#define INP_CHECK(ret, val, ...)\
    do{ if((val)) {\
    ret = MPP_ERR_INIT;\
    H264D_WARNNING("input empty(%d).\n", __LINE__);\
    goto __RETURN;\
    } } while (0)
//!< function return check
#define FUN_CHECK(val)\
    do{ if((val) < 0) {\
    H264D_WARNNING("Function error(%d).\n", __LINE__); \
    goto __FAILED;\
    } } while (0)

#ifdef ANDROID
#define  FPRINT(fp, ...)  //{ { mpp_log(__VA_ARGS__); } if (fp) { fprintf(fp, ##__VA_ARGS__); fflush(fp);}  }
#else
#define  FPRINT(fp, ...)  { if (fp) { fprintf(fp, ##__VA_ARGS__); fflush(fp);}  }
#endif

#define  FCLOSE(fp)    do{ if(fp) fclose(fp); fp = NULL; } while (0)

extern RK_U32  g_nalu_cnt0;
extern RK_U32  g_nalu_cnt1;
extern RK_S32  g_max_bytes;
extern RK_U32  g_max_slice_data;
extern FILE   *g_debug_file0;
extern FILE   *g_debug_file1;
#ifdef __cplusplus
extern "C" {
#endif

extern const LogEnvStr_t logenv_name;
extern const char *logctrl_name[LOG_MAX];
extern const char *loglevel_name[LOG_LEVEL_MAX];

void    print_env_help(LogEnv_t *env);
void    show_env_flags(LogEnv_t *env);

MPP_RET get_logenv(LogEnv_t *env);
MPP_RET explain_ctrl_flag(RK_U32 ctrl_val, LogFlag_t *pflag);

void    set_log_outpath(LogEnv_t *env);
void    set_bitread_logctx(BitReadCtx_t *bitctx, LogCtx_t *p_ctx);
void    writelog(void *ctx, ...);

#ifdef __cplusplus
}
#endif

#endif /* __H264D_LOG_H__ */