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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "h264d_log.h"

#define MODULE_TAG   "h264d_log"

#define LOG_BUF_SIZE 512

RK_U32  g_nalu_cnt = 0;

const LogEnvStr_t logenv_name = {
    "h264d_log_help",
    "h264d_log_show",
    "h264d_log_ctrl",
    "h264d_log_level",
    "h264d_log_outpath",
    "h264d_log_cmppath",
    "h264d_log_decframe",
    "h264d_log_begframe",
    "h264d_log_endframe",
};

const char *loglevel_name[LOG_LEVEL_MAX] = {
    "SILENT  ",
    "FATAL   ",
    "ERROR   ",
    "WARNNING",
    "INFO    ",
    "TRACE   ",
};

const char *logctrl_name[LOG_MAX] = {
    "DEBUG_EN      ",
    "FPGA_MODE     ",
    "LOG_PRINT     ",
    "LOG_WRITE     ",
    "RUN_PAESE     ",
    "RUN_HAL       ",
    "READ_NALU     ",
    "READ_SPS      ",
    "READ_SUBSPS   ",
    "READ_PPS      ",
    "READ_SLICE    ",
    "WRITE_SPSPPS  ",
    "WRITE_RPS     ",
    "WRITE_SCANLIST",
    "WRITE_STEAM   ",
    "WRITE_REG     ",
};

/*!
***********************************************************************
* \brief
*   get log env
***********************************************************************
*/

MPP_RET get_logenv(LogEnv_t *env)
{
    //!< read env
    mpp_env_get_u32(logenv_name.help,     &env->help,     0);
    mpp_env_get_u32(logenv_name.show,     &env->show,     0);
    mpp_env_get_u32(logenv_name.ctrl,     &env->ctrl,     0);
    mpp_env_get_u32(logenv_name.level,    &env->level,    0);
    mpp_env_get_u32(logenv_name.decframe, &env->decframe, 0);
    mpp_env_get_u32(logenv_name.begframe, &env->begframe, 0);
    mpp_env_get_u32(logenv_name.endframe, &env->endframe, 0);
    mpp_env_get_str(logenv_name.outpath,  &env->outpath,  NULL);

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   print env help
***********************************************************************
*/
void print_env_help(LogEnv_t *env)
{
    RK_U8 i = 0;
    (void)env;

    fprintf(stdout, "--------------- h264d help  -------------------- \n");

    fprintf(stdout, "h264d_log_help     :[num] (0/1) show help content \n");
    fprintf(stdout, "h264d_log_show     :[num] (0/1) show current log setting \n");
    fprintf(stdout, "h264d_log_outpath  :[str]  \n");
    fprintf(stdout, "h264d_log_decframe :[num]  \n");
    fprintf(stdout, "h264d_log_begframe :[num]  \n");
    fprintf(stdout, "h264d_log_endframe :[num]  \n");
    fprintf(stdout, "h264d_log_level    :[num]  \n");
    for (i = 0; i < LOG_LEVEL_MAX; i++) {
        fprintf(stdout, "                    (%2d) -- %s  \n", i, loglevel_name[i]);
    }
    fprintf(stdout, "\nh264d_log_ctrl     :[32bit] \n");
    for (i = 0; i < LOG_MAX; i++) {
        fprintf(stdout, "                    (%2d)bit -- %s  \n", i, logctrl_name[i]);
    }
    fprintf(stdout, "------------------------------------------------- \n");
}
/*!
***********************************************************************
* \brief
*   show env flags
***********************************************************************
*/
void show_env_flags(LogEnv_t *env)
{
    RK_U8 i = 0;
    fprintf(stdout, "------------- h264d debug setting   ------------- \n");
    fprintf(stdout, "outputpath    : %s \n", env->outpath);
    fprintf(stdout, "DecodeFrame   : %d \n", env->decframe);
    fprintf(stdout, "LogBeginFrame : %d \n", env->begframe);
    fprintf(stdout, "LogEndFrame   : %d \n", env->endframe);
    fprintf(stdout, "LogLevel      : %s \n", loglevel_name[env->level]);
    for (i = 0; i < LOG_MAX; i++) {
        fprintf(stdout, "%s: %d (%d)\n", logctrl_name[i], GetBitVal(env->ctrl, i), i);
    }
    fprintf(stdout, "------------------------------------------------- \n");
}
/*!
***********************************************************************
* \brief
*   explain log ctrl flag
***********************************************************************
*/
MPP_RET explain_ctrl_flag(RK_U32 ctrl_val, LogFlag_t *pflag)
{
    pflag->print_en = GetBitVal(ctrl_val, LOG_PRINT         );
    pflag->write_en = GetBitVal(ctrl_val, LOG_WRITE         );
    pflag->debug_en = GetBitVal(ctrl_val, LOG_DEBUG      )
                      || GetBitVal(ctrl_val, LOG_READ_NALU     )
                      || GetBitVal(ctrl_val, LOG_READ_SPS      )
                      || GetBitVal(ctrl_val, LOG_READ_SUBSPS   )
                      || GetBitVal(ctrl_val, LOG_READ_PPS      )
                      || GetBitVal(ctrl_val, LOG_READ_SLICE    )
                      || GetBitVal(ctrl_val, LOG_WRITE_SPSPPS  )
                      || GetBitVal(ctrl_val, LOG_WRITE_RPS     )
                      || GetBitVal(ctrl_val, LOG_WRITE_SCANLIST)
                      || GetBitVal(ctrl_val, LOG_WRITE_STEAM   )
                      || GetBitVal(ctrl_val, LOG_WRITE_REG     );

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   set log outpath
***********************************************************************
*/
void set_log_outpath(LogEnv_t *env)
{
    if (NULL == env->outpath) {
        mpp_env_set_str(logenv_name.outpath,  "./h264d_dat" );
        mpp_env_get_str(logenv_name.outpath,  &env->outpath,  NULL);
    }
    if (access(env->outpath, 0)) {
        if (mkdir(env->outpath)) {
            fprintf(stderr, "ERROR: create folder,%s \n", env->outpath);
        }
    }
}
/*!
***********************************************************************
* \brief
*   write log function
***********************************************************************
*/
void writelog(LogCtx_t *ctx, char *filename, RK_U32 line, char *loglevel, const char *msg, ...)
{
#if __DEBUG_EN

    va_list argptr;
    char argbuf[LOG_BUF_SIZE] = { 0 };
    char *pfn = NULL, *pfn0 = NULL, *pfn1 = NULL;

    va_start(argptr, msg);
    vsnprintf(argbuf, sizeof(argbuf), msg, argptr);

    pfn0 = strrchr(filename, '/');
    pfn1 = strrchr(filename, '\\');
    pfn  = pfn0 ? (pfn0 + 1) : (pfn1 ? (pfn1 + 1) : filename);

    if (ctx->flag->print_en) {
        printf("[ TAG = %s ], file: %s, line: %d, [ %s ], %s \n", ctx->tag, pfn, line, loglevel, argbuf);
    }
    if (ctx->fp && ctx->flag->write_en) {
        fprintf(ctx->fp, "%s \n", argbuf);
        //fprintf(ctx->fp, "[ TAG = %s ], file: %s, line: %d, [ %s ], %s \n", ctx->tag, pfn, line, loglevel, argbuf);
        fflush(ctx->fp);
    }
    va_end(argptr);

#endif
}
