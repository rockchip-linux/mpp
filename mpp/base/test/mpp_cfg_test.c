/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_cfg_test"

#include <stdio.h>
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

static rk_s32 add_array_element(MppCfgObj array, MppCfgType type, MppCfgVal *val)
{
    MppCfgObj obj = NULL;
    rk_s32 ret;

    ret = mpp_cfg_get_object(&obj, NULL, type, val);
    if (ret) {
        mpp_loge("mpp_cfg_get_object array element type %d failed\n", type);
        return ret;
    }

    ret = mpp_cfg_add(array, obj);
    if (ret) {
        mpp_loge("mpp_cfg_add array element failed\n");
        mpp_cfg_put_all(obj);
        return ret;
    }

    return rk_ok;
}

static rk_s32 test_typed_arrays(MppCfgObj root)
{
    MppCfgObj array = NULL;
    MppCfgVal val;
    rk_s32 ret = rk_nok;
    rk_s32 i;

    mpp_logi("test typed arrays\n");

    ret = mpp_cfg_get_array(&array, "s8_array", 4);
    if (ret) {
        mpp_loge("mpp_cfg_get_array s8 failed\n");
        goto DONE;
    }
    for (i = 0; i < 4; i++) {
        val.s8 = (rk_s8)(-128 + i * 10);
        ret = add_array_element(array, MPP_CFG_TYPE_s8, &val);
        if (ret)
            goto DONE;
    }
    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add s8_array failed\n");
        goto DONE;
    }
    array = NULL;

    ret = mpp_cfg_get_array(&array, "u8_array", 4);
    if (ret) {
        mpp_loge("mpp_cfg_get_array u8 failed\n");
        goto DONE;
    }
    for (i = 0; i < 4; i++) {
        val.u8 = (rk_u8)(i * 50);
        ret = add_array_element(array, MPP_CFG_TYPE_u8, &val);
        if (ret)
            goto DONE;
    }
    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add u8_array failed\n");
        goto DONE;
    }
    array = NULL;

    ret = mpp_cfg_get_array(&array, "s16_array", 4);
    if (ret) {
        mpp_loge("mpp_cfg_get_array s16 failed\n");
        goto DONE;
    }
    for (i = 0; i < 4; i++) {
        val.s16 = (rk_s16)(-1000 + i * 100);
        ret = add_array_element(array, MPP_CFG_TYPE_s16, &val);
        if (ret)
            goto DONE;
    }
    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add s16_array failed\n");
        goto DONE;
    }
    array = NULL;

    ret = mpp_cfg_get_array(&array, "u16_array", 4);
    if (ret) {
        mpp_loge("mpp_cfg_get_array u16 failed\n");
        goto DONE;
    }
    for (i = 0; i < 4; i++) {
        val.u16 = (rk_u16)(i * 1000);
        ret = add_array_element(array, MPP_CFG_TYPE_u16, &val);
        if (ret)
            goto DONE;
    }
    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add u16_array failed\n");
        goto DONE;
    }
    array = NULL;

    ret = mpp_cfg_get_array(&array, "s64_array", 4);
    if (ret) {
        mpp_loge("mpp_cfg_get_array s64 failed\n");
        goto DONE;
    }
    for (i = 0; i < 4; i++) {
        val.s64 = (rk_s64)(-1000000LL + i * 100000LL);
        ret = add_array_element(array, MPP_CFG_TYPE_s64, &val);
        if (ret)
            goto DONE;
    }
    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add s64_array failed\n");
        goto DONE;
    }
    array = NULL;

    ret = mpp_cfg_get_array(&array, "u64_array", 4);
    if (ret) {
        mpp_loge("mpp_cfg_get_array u64 failed\n");
        goto DONE;
    }
    for (i = 0; i < 4; i++) {
        val.u64 = (rk_u64)(i * 1000000ULL);
        ret = add_array_element(array, MPP_CFG_TYPE_u64, &val);
        if (ret)
            goto DONE;
    }
    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add u64_array failed\n");
        goto DONE;
    }
    array = NULL;

    ret = mpp_cfg_get_array(&array, "bool_array", 4);
    if (ret) {
        mpp_loge("mpp_cfg_get_array bool failed\n");
        goto DONE;
    }
    for (i = 0; i < 4; i++) {
        val.b1 = (i % 2 == 0) ? (rk_bool)RK_TRUE : (rk_bool)RK_FALSE;
        ret = add_array_element(array, MPP_CFG_TYPE_BOOL, &val);
        if (ret)
            goto DONE;
    }
    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add bool_array failed\n");
        goto DONE;
    }
    array = NULL;

    {
        const char *str_values[] = {"str0", "str1", "str2", "str3"};
        ret = mpp_cfg_get_array(&array, "string_array", 4);
        if (ret) {
            mpp_loge("mpp_cfg_get_array string failed\n");
            goto DONE;
        }
        for (i = 0; i < 4; i++) {
            val.str = (char *)str_values[i];
            ret = add_array_element(array, MPP_CFG_TYPE_STRING, &val);
            if (ret)
                goto DONE;
        }
        ret = mpp_cfg_add(root, array);
        if (ret) {
            mpp_loge("mpp_cfg_add string_array failed\n");
            goto DONE;
        }
        array = NULL;
    }

    ret = rk_ok;
DONE:
    return ret;
}

static rk_s32 test_object_array(MppCfgObj root)
{
    MppCfgObj array = NULL;
    MppCfgObj obj = NULL;
    MppCfgObj inner_obj = NULL;
    MppCfgVal val;
    rk_s32 ret = rk_nok;
    rk_s32 i;
    const char *obj_names[] = {"obj_a", "obj_b", "obj_c"};

    mpp_logi("test object array\n");

    ret = mpp_cfg_get_array(&array, "object_array", 3);
    if (ret) {
        mpp_loge("mpp_cfg_get_array object failed\n");
        goto DONE;
    }

    for (i = 0; i < 3; i++) {
        ret = mpp_cfg_get_object(&obj, NULL, MPP_CFG_TYPE_OBJECT, NULL);
        if (ret) {
            mpp_loge("mpp_cfg_get_object for inner object failed\n");
            goto DONE;
        }

        val.s32 = i + 100;
        ret = mpp_cfg_get_object(&inner_obj, "id", MPP_CFG_TYPE_s32, &val);
        if (ret) {
            mpp_loge("mpp_cfg_get_object id failed\n");
            goto DONE;
        }
        ret = mpp_cfg_add(obj, inner_obj);
        if (ret) {
            mpp_loge("mpp_cfg_add id failed\n");
            goto DONE;
        }
        inner_obj = NULL;

        val.str = (char *)obj_names[i];
        ret = mpp_cfg_get_object(&inner_obj, "name", MPP_CFG_TYPE_STRING, &val);
        if (ret) {
            mpp_loge("mpp_cfg_get_object name failed\n");
            goto DONE;
        }
        ret = mpp_cfg_add(obj, inner_obj);
        if (ret) {
            mpp_loge("mpp_cfg_add name failed\n");
            goto DONE;
        }
        inner_obj = NULL;

        val.b1 = (i == 0) ? (rk_bool)RK_TRUE : (rk_bool)RK_FALSE;
        ret = mpp_cfg_get_object(&inner_obj, "active", MPP_CFG_TYPE_BOOL, &val);
        if (ret) {
            mpp_loge("mpp_cfg_get_object active failed\n");
            goto DONE;
        }
        ret = mpp_cfg_add(obj, inner_obj);
        if (ret) {
            mpp_loge("mpp_cfg_add active failed\n");
            goto DONE;
        }
        inner_obj = NULL;

        ret = mpp_cfg_add(array, obj);
        if (ret) {
            mpp_loge("mpp_cfg_add object to array failed\n");
            goto DONE;
        }
        obj = NULL;
    }

    ret = mpp_cfg_add(root, array);
    if (ret) {
        mpp_loge("mpp_cfg_add object_array failed\n");
        goto DONE;
    }

    ret = rk_ok;
DONE:
    if (inner_obj)
        mpp_cfg_put_all(inner_obj);
    if (obj)
        mpp_cfg_put_all(obj);
    return ret;
}

static rk_s32 test_nested_array(MppCfgObj root)
{
    MppCfgObj outer_array = NULL;
    MppCfgObj inner_array = NULL;
    MppCfgVal val;
    rk_s32 ret = rk_nok;
    rk_s32 i, j;

    mpp_logi("test nested array\n");

    ret = mpp_cfg_get_array(&outer_array, "nested_array", 3);
    if (ret) {
        mpp_loge("mpp_cfg_get_array outer failed\n");
        goto DONE;
    }

    for (i = 0; i < 3; i++) {
        ret = mpp_cfg_get_array(&inner_array, NULL, 3);
        if (ret) {
            mpp_loge("mpp_cfg_get_array inner failed\n");
            goto DONE;
        }

        for (j = 0; j < 3; j++) {
            val.s32 = i * 10 + j;
            ret = add_array_element(inner_array, MPP_CFG_TYPE_s32, &val);
            if (ret)
                goto DONE;
        }

        ret = mpp_cfg_add(outer_array, inner_array);
        if (ret) {
            mpp_loge("mpp_cfg_add inner array failed\n");
            goto DONE;
        }
        inner_array = NULL;
    }

    ret = mpp_cfg_add(root, outer_array);
    if (ret) {
        mpp_loge("mpp_cfg_add nested_array failed\n");
        goto DONE;
    }

    ret = rk_ok;
DONE:
    if (inner_array)
        mpp_cfg_put_all(inner_array);
    return ret;
}

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
    if (!std || !std[0]) {
        mpp_loge("mpp_cfg obj to %s string returned empty\n", str_fmt[fmt]);
        ret = rk_nok;
        goto DONE;
    }

    ret = mpp_cfg_from_string(&out, fmt, std);
    if (ret) {
        mpp_loge("mpp_cfg out from %s string failed ret %d\n", str_fmt[fmt], ret);
        goto DONE;
    }
    if (!out) {
        mpp_loge("mpp_cfg out from %s string returned NULL object\n", str_fmt[fmt]);
        ret = rk_nok;
        goto DONE;
    }

    ret = mpp_cfg_to_string(out, fmt, &str);
    if (ret) {
        mpp_loge("mpp_cfg out to %s string failed ret %d\n", str_fmt[fmt], ret);
        goto DONE;
    }
    if (!str || !str[0]) {
        mpp_loge("mpp_cfg out to %s string returned empty\n", str_fmt[fmt]);
        ret = rk_nok;
        goto DONE;
    }

    if (strcmp(std, str)) {
        mpp_loge("mpp_cfg mismatch on from / to %s string\n", str_fmt[fmt]);
        mpp_logi("string std:\n");
        mpp_cfg_print_string(std);
        mpp_logi("string out:\n");
        mpp_cfg_print_string(str);
        ret = rk_nok;
    } else {
        ret = rk_ok;
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
        MppCfgStrFmt file_fmt = MPP_CFG_STR_FMT_JSON;
        char *ext = strrchr(path, '.');

        if (ext) {
            if (!strcmp(ext, ".toml"))
                file_fmt = MPP_CFG_STR_FMT_TOML;
            else if (!strcmp(ext, ".json"))
                file_fmt = MPP_CFG_STR_FMT_JSON;
        }
        mpp_logi("file %s format %s\n", path, str_fmt[file_fmt]);

        fd = open(path, O_RDWR);
        if (fd < 0) {
            mpp_loge("open %s failed\n", path);
            goto FILE_DONE;
        }

        size = lseek(fd, 0, SEEK_END);
        if (size < 0) {
            mpp_loge("lseek failed\n");
            goto FILE_DONE;
        }
        lseek(fd, 0, SEEK_SET);

        buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        if (!buf) {
            mpp_loge("mmap fd %d size %d failed\n", fd, size);
            goto FILE_DONE;
        }

        ret = mpp_cfg_from_string(&root, file_fmt, buf);
        if (ret) {
            mpp_loge("mpp_cfg_from_string failed\n");
            goto FILE_DONE;
        }

        mpp_logi("cfg object %p from file\n", root);
        mpp_cfg_dump_f(root);

        ret = test_to_from(root, file_fmt);
        mpp_logi("test to / from %s string %s\n", str_fmt[file_fmt], ret ? "failed" : "success");

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

    mpp_logi("test basic s32 array\n");
    ret = mpp_cfg_get_array(&array, NULL, array_size);
    if (ret) {
        mpp_loge("mpp_cfg_get_array failed\n");
        goto DONE;
    }

    for (i = 0; i < array_size; i++) {
        obj = NULL;
        val.s32 = i;
        ret = mpp_cfg_get_object(&obj, NULL, MPP_CFG_TYPE_s32, &val);
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

    obj = NULL;
    val.s32 = 1920;
    ret = mpp_cfg_get_object(&obj, "width", MPP_CFG_TYPE_s32, &val);
    if (ret) {
        mpp_loge("mpp_cfg_get s32 failed\n");
        goto DONE;
    }
    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_loge("mpp_cfg_add s32 failed\n");
        goto DONE;
    }

    obj = NULL;
    val.u32 = 1080;
    ret = mpp_cfg_get_object(&obj, "height", MPP_CFG_TYPE_u32, &val);
    if (ret) {
        mpp_loge("mpp_cfg_get u32 failed\n");
        goto DONE;
    }
    ret = mpp_cfg_add(root, obj);
    if (ret) {
        mpp_loge("mpp_cfg_add u32 failed\n");
        goto DONE;
    }

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

    ret = test_typed_arrays(root);
    if (ret) {
        mpp_loge("test_typed_arrays failed\n");
        goto DONE;
    }
    array = NULL;

    ret = test_object_array(root);
    if (ret) {
        mpp_loge("test_object_array failed\n");
        goto DONE;
    }

    ret = test_nested_array(root);
    if (ret) {
        mpp_loge("test_nested_array failed\n");
        goto DONE;
    }

    mpp_cfg_dump_f(root);

    {
        MppCfgObj simple_root = NULL;
        MppCfgObj simple_array = NULL;
        rk_s32 ser_ret;

        mpp_logi("test serialization\n");

        ret = mpp_cfg_get_object(&simple_root, NULL, MPP_CFG_TYPE_OBJECT, NULL);
        if (ret)
            goto DONE;

        val.s32 = 1920;
        ret = mpp_cfg_get_object(&obj, "width", MPP_CFG_TYPE_s32, &val);
        if (ret)
            goto DONE;
        ret = mpp_cfg_add(simple_root, obj);
        if (ret)
            goto DONE;
        obj = NULL;

        ret = mpp_cfg_get_array(&simple_array, "values", 4);
        if (ret)
            goto DONE;
        for (i = 0; i < 4; i++) {
            val.s32 = i * 10;
            ret = add_array_element(simple_array, MPP_CFG_TYPE_s32, &val);
            if (ret)
                goto DONE;
        }
        ret = mpp_cfg_add(simple_root, simple_array);
        if (ret)
            goto DONE;
        simple_array = NULL;

        ser_ret = test_to_from(simple_root, MPP_CFG_STR_FMT_LOG);
        mpp_logi("to/from log %s\n", ser_ret ? "failed" : "success");

        ser_ret = test_to_from(simple_root, MPP_CFG_STR_FMT_JSON);
        mpp_logi("to/from json %s\n", ser_ret ? "failed" : "success");

        mpp_cfg_put_all(simple_root);
    }

    ret = rk_ok;

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
