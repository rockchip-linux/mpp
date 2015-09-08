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
#include "rk_type.h"
#include "mpp_err.h"


//!< undefine tag
#ifdef MODULE_TAG
#undef MODULE_TAG
#endif
//!< debug
#define   __DEBUG_EN   1
//!< return
#define   RET_TURE     1
#define   RET_FALSE    0
//!< marco
#define   ASSERT	   assert
#define  FCLOSE(fp)	   do{ if(fp) fclose(fp); fp = NULL; } while (0)

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
	LOG_DEBUG_EN      = 0,
	LOG_FPGA             ,
	LOG_PRINT            ,
	RUN_PARSE = LOG_PRINT,
	LOG_WRITE            ,
	RUN_HAL   = LOG_WRITE,
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

typedef struct log_flag_t
{
	RK_U8        debug_en;
	RK_U8        print_en;
	RK_U8        write_en;
	RK_U32	     level;
}LogFlag_t;

typedef struct log_ctx_t
{
	const char   *tag;
	LogFlag_t    *flag;
	FILE         *fp;	
}LogCtx_t;

typedef struct log_env_str_t{
	char *help;
	char *show;
	char *ctrl;
	char *level;
	char *outpath;
	char *decframe;
	char *begframe;
	char *endframe;
}LogEnvStr_t;


typedef struct log_env_ctx_t{
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

}LogEnv_t;
typedef struct h264d_logctx_t
{
	LogEnv_t  env; 
	LogFlag_t log_flag;
	LogCtx_t  *buf;
	LogCtx_t  *parr[LOG_MAX];
}H264dLogCtx_t;


//!< write log
#define LogEnable(ctx, loglevel)      ( ctx && ctx->flag->debug_en && (ctx->flag->level & loglevel) )

#define LogTrace(ctx, ...)      do{ if(LogEnable(ctx, LOG_LEVEL_TRACE))\
	writelog(ctx, __FILE__, __LINE__, "TRACE", __VA_ARGS__);      }while (0)
#define LogInfo(ctx, ...)       do{ if(LogEnable(ctx, LOG_LEVEL_INFO))\
	writelog(ctx, __FILE__, __LINE__,  "INFO", __VA_ARGS__);      }while (0)
#define LogWarnning(ctx, ...)   do{ if(LogEnable(ctx, LOG_LEVEL_WARNNING))\
	writelog(ctx, __FILE__, __LINE__,  "WARNNING", __VA_ARGS__);  }while (0)
#define LogError(ctx, ...)      do{ if(LogEnable(ctx, LOG_LEVEL_ERROR))\
	writelog(ctx, __FILE__, __LINE__,  "ERROR", __VA_ARGS__); ASSERT(0); }while (0)
#define LogFatal(ctx, ...)      do{ if(LogEnable(ctx, LOG_LEVEL_ERROR))\
	writelog(ctx, __FILE__, __LINE__,  "FATAL", __VA_ARGS__); ASSERT(0); }while (0)
#define FunctionIn(ctx)         do{ if(LogEnable(ctx, LOG_LEVEL_TRACE))\
	writelog(ctx, __FILE__, __LINE__,  "FunIn",  __FUNCTION__); } while (0)
#define FunctionOut(ctx)        do{if(LogEnable(ctx, LOG_LEVEL_TRACE))\
	writelog(ctx, __FILE__, __LINE__,  "FunOut", __FUNCTION__);  } while (0)


#define   __RETURN     __Ret
#define   __FAILED     __failed


#define  VAL_CHECK(val)         do{ if((val))  goto __FAILED; } while (0)
#define  FUN_CHECK(val)         do{ if((val))  goto __FAILED; } while (0)
#define  MEM_CHECK(val)         do{ if(!(val)) goto __FAILED; } while (0)
#define  FLE_CHECK(val)         do{ if(!(val)) goto __FAILED; } while (0)
#define  RET_CHECK(val)         do{ if((val))  goto __RETURN; } while (0)

#ifdef __cplusplus
extern "C" {
#endif
	extern const LogEnvStr_t logenv_name;

	MPP_RET h264d_log_deinit(H264dLogCtx_t *logctx);
	MPP_RET h264d_log_init  (H264dLogCtx_t *logctx);

	void writelog(LogCtx_t *ctx, char *fname, RK_U32 line, char *loglevel, const char *msg, ...);


#ifdef __cplusplus
}
#endif

#endif /* __H264D_LOG_H__ */