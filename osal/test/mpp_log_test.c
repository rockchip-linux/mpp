/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_log_test"

#include "mpp_log.h"

int main()
{
    mpp_logi("mpp log test start\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nset log level to MPP_LOG_VERBOSE\n");
    mpp_set_log_level(MPP_LOG_VERBOSE);

    mpp_logf("test mpp_logf\n");
    mpp_loge("test mpp_loge\n");
    mpp_logw("test mpp_logw\n");
    mpp_logi("test mpp_logi\n");
    mpp_logd("test mpp_logd\n");
    mpp_logv("test mpp_logv\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nset log level to MPP_LOG_DEBUG\n");
    mpp_set_log_level(MPP_LOG_DEBUG);

    mpp_logf("test mpp_logf\n");
    mpp_loge("test mpp_loge\n");
    mpp_logw("test mpp_logw\n");
    mpp_logi("test mpp_logi\n");
    mpp_logd("test mpp_logd\n");
    mpp_logv("test mpp_logv\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nset log level to MPP_LOG_INFO\n");
    mpp_set_log_level(MPP_LOG_INFO);

    mpp_logf("test mpp_logf\n");
    mpp_loge("test mpp_loge\n");
    mpp_logw("test mpp_logw\n");
    mpp_logi("test mpp_logi\n");
    mpp_logd("test mpp_logd\n");
    mpp_logv("test mpp_logv\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nset log level to MPP_LOG_WARN\n");
    mpp_set_log_level(MPP_LOG_WARN);

    mpp_logf("test mpp_logf\n");
    mpp_loge("test mpp_loge\n");
    mpp_logw("test mpp_logw\n");
    mpp_logi("test mpp_logi\n");
    mpp_logd("test mpp_logd\n");
    mpp_logv("test mpp_logv\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nset log level to MPP_LOG_ERROR\n");
    mpp_set_log_level(MPP_LOG_ERROR);

    mpp_logf("test mpp_logf\n");
    mpp_loge("test mpp_loge\n");
    mpp_logw("test mpp_logw\n");
    mpp_logi("test mpp_logi\n");
    mpp_logd("test mpp_logd\n");
    mpp_logv("test mpp_logv\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nset log level to MPP_LOG_FATAL\n");
    mpp_set_log_level(MPP_LOG_FATAL);

    mpp_logf("test mpp_logf\n");
    mpp_loge("test mpp_loge\n");
    mpp_logw("test mpp_logw\n");
    mpp_logi("test mpp_logi\n");
    mpp_logd("test mpp_logd\n");
    mpp_logv("test mpp_logv\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nset log level to MPP_LOG_SILENT\n");
    mpp_set_log_level(MPP_LOG_SILENT);

    mpp_logf("test mpp_logf\n");
    mpp_loge("test mpp_loge\n");
    mpp_logw("test mpp_logw\n");
    mpp_logi("test mpp_logi\n");
    mpp_logd("test mpp_logd\n");
    mpp_logv("test mpp_logv\n");

    mpp_set_log_level(MPP_LOG_INFO);
    mpp_logi("\nchange MODULE_TAG to log_test\n");

#undef  MODULE_TAG
#define MODULE_TAG "log_test"

    mpp_logi("\nlog with tag changed to log_test\n");

#undef  MODULE_TAG
#define MODULE_TAG "mpp_log_test"

    mpp_logi("\ndeprecated function test\n");

    mpp_log("call mpp_log -- info log\n");
    mpp_log_f("call mpp_log_f -- info log with function name\n");

    mpp_log("\n");

    mpp_err("call mpp_err -- error log\n");
    mpp_err_f("call mpp_err_f -- error log with function name\n");

    mpp_logi("\ndeprecated function log\n");

    _mpp_log(MODULE_TAG, "log test", "_mpp_log");
    _mpp_err(MODULE_TAG, "log test", "_mpp_err");

    mpp_log("\nmpp log test done\n");

    return 0;
}
