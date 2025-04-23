/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_cfg_test"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "mpp_log.h"

#include "mpp_cfg_io.h"

int main(int argc, char *argv[])
{
    MppCfgObj root = NULL;
    MppCfgObj array = NULL;
    MppCfgObj obj = NULL;
    MppCfgVal val;
    rk_s32 array_size = 4;
    rk_s32 ret = rk_nok;
    rk_s32 i;

    mpp_log("start\n");

    if (argc > 1) {
        char *path = argv[1];
        void *buf = NULL;
        rk_s32 fd = -1;
        rk_s32 size = 0;

        fd = open(path, O_RDWR);
        if (fd < 0) {
            mpp_err("open %s failed\n", path);
            goto FILE_DONE;
        }

        mpp_log("open file %s\n", path);

        size = lseek(fd, 0, SEEK_END);
        if (size < 0) {
            mpp_err("lseek failed\n");
            goto FILE_DONE;
        }

        lseek(fd, 0, SEEK_SET);
        mpp_log("get file size %d\n", size);

        buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (!buf) {
            mpp_err("mmap fd %d size %d failed\n", fd, size);
            goto FILE_DONE;
        }

        mpp_log("mmap size %d to %p content:\n", size, buf);
        mpp_log("%s", buf);

        ret = mpp_cfg_from_string(&root, MPP_CFG_STR_FMT_JSON, buf);
        if (ret) {
            mpp_err("mpp_cfg_from_string failed\n");
            goto FILE_DONE;
        }

        mpp_log("get cfg object %p from file\n", root);

        mpp_cfg_dump_f(root);

        mpp_cfg_put_all(root);
        root = NULL;
        ret = rk_ok;

        mpp_log("put cfg object\n");

    FILE_DONE:
        if (buf) {
            munmap(buf, size);
            buf = NULL;
        }
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }

        if (ret)
            return ret;
    }

    ret = mpp_cfg_get_object(&root, NULL, MPP_CFG_TYPE_OBJECT, NULL);
    if (ret) {
        mpp_err("mpp_cfg_get_object failed\n");
        goto DONE;
    }

    mpp_log("get root object\n");

    ret = mpp_cfg_get_array(&array, NULL, array_size);
    if (ret) {
        mpp_err("mpp_cfg_get_array failed\n");
        goto DONE;
    }

    mpp_log("get array\n");

    for (i = 0; i < array_size; i++) {
        obj = NULL;
        val.s32 = i;
        ret = mpp_cfg_get_object(&obj, NULL, MPP_CFG_TYPE_S32, &val);
        if (ret) {
            mpp_err("mpp_cfg_get_object array element failed\n");
            goto DONE;
        }

        ret = mpp_cfg_add(array, obj);
        if (ret) {
            mpp_err("mpp_cfg_add array element failed\n");
            goto DONE;
        }
    }

    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_err("mpp_cfg_add failed\n");
        goto DONE;
    }

    mpp_log("add array to root\n");

    obj = NULL;
    val.s32 = 1920;
    ret = mpp_cfg_get_object(&obj, "width", MPP_CFG_TYPE_S32, &val);
    if (ret) {
        mpp_err("mpp_cfg_get s32 failed\n");
        goto DONE;
    }

    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_err("mpp_cfg_add s32 failed\n");
        goto DONE;
    }

    mpp_log("add s32 to root\n");

    obj = NULL;
    val.u32 = 1080;
    ret = mpp_cfg_get_object(&obj, "height", MPP_CFG_TYPE_U32, &val);
    if (ret) {
        mpp_err("mpp_cfg_get u32 failed\n");
        goto DONE;
    }

    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_err("mpp_cfg_add u32 failed\n");
        goto DONE;
    }

    mpp_log("set u32 to root\n");

    obj = NULL;
    val.str = "hello world";
    ret = mpp_cfg_get_object(&obj, "test", MPP_CFG_TYPE_STRING, &val);
    if (ret) {
        mpp_err("mpp_cfg_get string failed\n");
        goto DONE;
    }

    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_err("mpp_cfg_add string failed\n");
        goto DONE;
    }

    mpp_log("set string to root\n");

    mpp_cfg_dump_f(root);

    ret = mpp_cfg_del(array);
    if (ret) {
        mpp_err("mpp_cfg_del failed\n");
        goto DONE;
    }

    mpp_log("del array from root\n");

DONE:
    mpp_cfg_put_all(root);

    mpp_log("done %s\n", ret ? "failed" : "success");

    return ret;
}
