/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_cfg_test"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_debug.h"

#include "mpp_cfg_io.h"

static const char *str_fmt[] = {
    "log",
    "json",
    "toml",
    "invalid"
};

static rk_s32 test_to_from(MppCfgObj obj, MppCfgStrFmt fmt)
{
    MppCfgObj out = NULL;
    char *std = NULL;
    char *str = NULL;
    rk_s32 ret = rk_nok;

    ret = mpp_cfg_to_string(obj, fmt, &std);
    if (ret) {
        mpp_loge("mpp_cfg obj to %s string failed\n", str_fmt[fmt]);
        goto DONE;
    }

    ret = mpp_cfg_from_string(&out, fmt, std);
    if (ret) {
        mpp_loge("mpp_cfg out from %s string failed\n", str_fmt[fmt]);
        goto DONE;
    }

    ret = mpp_cfg_to_string(out, fmt, &str);
    if (ret) {
        mpp_loge("mpp_cfg out to %s string failed\n", str_fmt[fmt]);
        goto DONE;
    }

    if (strcmp(std, str)) {
        mpp_loge("mpp_cfg mismatch on from / to %s string\n", str_fmt[fmt]);
        mpp_logi("string std:\n");
        mpp_cfg_print_string(std);
        mpp_logi("string out:\n");
        mpp_cfg_print_string(out);
        ret = rk_nok;
    }

DONE:
    MPP_FREE(std);
    MPP_FREE(str);
    mpp_cfg_put_all(out);

    return ret;
}

int main(int argc, char *argv[])
{
    MppCfgObj root = NULL;
    MppCfgObj array = NULL;
    MppCfgObj obj = NULL;
    MppCfgVal val;
    rk_s32 array_size = 4;
    rk_s32 ret = rk_nok;
    rk_s32 i;

    mpp_logi("start\n");

    if (argc > 1) {
        char *path = argv[1];
        void *buf = NULL;
        rk_s32 fd = -1;
        rk_s32 size = 0;

        fd = open(path, O_RDWR);
        if (fd < 0) {
            mpp_loge("open %s failed\n", path);
            goto FILE_DONE;
        }

        mpp_logi("open file %s\n", path);

        size = lseek(fd, 0, SEEK_END);
        if (size < 0) {
            mpp_loge("lseek failed\n");
            goto FILE_DONE;
        }

        lseek(fd, 0, SEEK_SET);
        mpp_logi("get file size %d\n", size);

        buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        if (!buf) {
            mpp_loge("mmap fd %d size %d failed\n", fd, size);
            goto FILE_DONE;
        }

        mpp_logi("mmap size %d to %p content:\n", size, buf);
        mpp_logi("%s", buf);

        ret = mpp_cfg_from_string(&root, MPP_CFG_STR_FMT_JSON, buf);
        if (ret) {
            mpp_loge("mpp_cfg_from_string failed\n");
            goto FILE_DONE;
        }

        mpp_logi("get cfg object %p from file\n", root);

        mpp_cfg_dump_f(root);

        ret = test_to_from(root, MPP_CFG_STR_FMT_LOG);
        mpp_logi("test to / from log string %s\n", ret ? "failed" : "success");
        if (ret)
            goto FILE_DONE;

        ret = test_to_from(root, MPP_CFG_STR_FMT_JSON);
        mpp_logi("test to / from json string %s\n", ret ? "failed" : "success");
        if (ret)
            goto FILE_DONE;

    FILE_DONE:
        if (buf) {
            munmap(buf, size);
            buf = NULL;
        }
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }

        mpp_cfg_put_all(root);
        root = NULL;

        if (ret)
            return ret;
    }

    ret = mpp_cfg_get_object(&root, NULL, MPP_CFG_TYPE_OBJECT, NULL);
    if (ret) {
        mpp_loge("mpp_cfg_get_object failed\n");
        goto DONE;
    }

    mpp_logi("get root object\n");

    ret = mpp_cfg_get_array(&array, NULL, array_size);
    if (ret) {
        mpp_loge("mpp_cfg_get_array failed\n");
        goto DONE;
    }

    mpp_logi("get array\n");

    for (i = 0; i < array_size; i++) {
        obj = NULL;
        val.s32 = i;
        ret = mpp_cfg_get_object(&obj, NULL, MPP_CFG_TYPE_S32, &val);
        if (ret) {
            mpp_loge("mpp_cfg_get_object array element failed\n");
            goto DONE;
        }

        ret = mpp_cfg_add(array, obj);
        if (ret) {
            mpp_loge("mpp_cfg_add array element failed\n");
            goto DONE;
        }
    }

    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add failed\n");
        goto DONE;
    }

    mpp_logi("add array to root\n");

    obj = NULL;
    val.s32 = 1920;
    ret = mpp_cfg_get_object(&obj, "width", MPP_CFG_TYPE_S32, &val);
    if (ret) {
        mpp_loge("mpp_cfg_get s32 failed\n");
        goto DONE;
    }

    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_loge("mpp_cfg_add s32 failed\n");
        goto DONE;
    }

    mpp_logi("add s32 to root\n");

    obj = NULL;
    val.u32 = 1080;
    ret = mpp_cfg_get_object(&obj, "height", MPP_CFG_TYPE_U32, &val);
    if (ret) {
        mpp_loge("mpp_cfg_get u32 failed\n");
        goto DONE;
    }

    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_loge("mpp_cfg_add u32 failed\n");
        goto DONE;
    }

    mpp_logi("set u32 to root\n");

    obj = NULL;
    val.str = "hello world";
    ret = mpp_cfg_get_object(&obj, "test", MPP_CFG_TYPE_STRING, &val);
    if (ret) {
        mpp_loge("mpp_cfg_get string failed\n");
        goto DONE;
    }

    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_loge("mpp_cfg_add string failed\n");
        goto DONE;
    }

    mpp_logi("set string to root\n");

    mpp_cfg_dump_f(root);

    ret = test_to_from(root, MPP_CFG_STR_FMT_LOG);
    mpp_logi("test to / from log string %s\n", ret ? "failed" : "success");
    if (ret)
        goto DONE;

    ret = test_to_from(root, MPP_CFG_STR_FMT_JSON);
    mpp_logi("test to / from json string %s\n", ret ? "failed" : "success");
    if (ret)
        goto DONE;

    ret = mpp_cfg_del(array);
    if (ret) {
        mpp_loge("mpp_cfg_del failed\n");
        goto DONE;
    }

    mpp_logi("del array from root\n");

DONE:
    if (root) {
        mpp_cfg_put_all(root);
        root = NULL;
    }
    if (array) {
        mpp_cfg_put_all(array);
        array = NULL;
    }

    mpp_logi("done %s\n", ret ? "failed" : "success");

    return ret;
}
