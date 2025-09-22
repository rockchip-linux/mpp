/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_obj_test"

#include "mpp_log.h"
#include "mpp_common.h"

#include "kmpp_obj.h"
#include "kmpp_buffer.h"

#define TEST_DETAIL     1
#define TEST_DEF_DUMP   2
#define TEST_OBJ_UDUMP  4
#define TEST_OBJ_KDUMP  8

#define test_detail(fmt, ...) \
    do { \
        if (flag & TEST_DETAIL) \
            mpp_log(fmt, ##__VA_ARGS__); \
    } while (0)

typedef struct KmppObjTest_t {
    const char *name;
    rk_u32 flag;
    rk_s32 (*func)(const char *name, rk_u32 flag);
} KmppObjTest;

static rk_s32 kmpp_obj_std_test(const char *name, rk_u32 flag)
{
    KmppObjDef def = NULL;
    KmppObj obj = NULL;
    MPP_RET ret = MPP_NOK;

    ret = kmpp_objdef_find(&def, name);
    if (ret) {
        mpp_log("kmpp_objdef_find %s failed\n", name);
        goto done;
    }

    if (flag & TEST_DEF_DUMP)
        kmpp_objdef_dump(def);

    ret = kmpp_obj_get_f(&obj, def);
    if (ret) {
        mpp_log("kmpp_obj_get %s failed ret %d\n", name, ret);
        goto done;
    }

    if (flag & TEST_OBJ_UDUMP)
        kmpp_obj_udump(obj);
    if (flag & TEST_OBJ_KDUMP)
        kmpp_obj_kdump(obj);

    ret = kmpp_obj_put_f(obj);
    if (ret) {
        mpp_log("kmpp_obj_put %s failed\n", name);
    }
    obj = NULL;
    def = NULL;

done:
    if (obj)
        kmpp_obj_put_f(obj);

    return ret;
}

static rk_s32 kmpp_obj_by_name_test(const char *name, rk_u32 flag)
{
    KmppObj obj = NULL;
    MPP_RET ret = MPP_NOK;

    ret = kmpp_obj_get_by_name_f(&obj, name);
    if (ret) {
        mpp_log("kmpp_obj_get_by_name %s failed ret %d\n", name, ret);
        goto done;
    }

    if (flag & TEST_OBJ_UDUMP)
        kmpp_obj_udump(obj);
    if (flag & TEST_OBJ_KDUMP)
        kmpp_obj_kdump(obj);

    ret = kmpp_obj_put_f(obj);
    if (ret) {
        mpp_log("kmpp_obj_put %s failed\n", name);
        goto done;
    }
    obj = NULL;

done:
    if (obj)
        kmpp_obj_put_f(obj);

    return ret;
}

static rk_s32 kmpp_buffer_test(const char *name, rk_u32 flag)
{
    KmppShmPtr sptr;
    KmppObj grp = NULL;
    KmppObj grp_cfg = NULL;
    KmppObj buf = NULL;
    KmppObj buf_cfg = NULL;
    MPP_RET ret = MPP_NOK;
    rk_u32 val = 0;

    ret = kmpp_obj_get_by_name_f(&grp, "KmppBufGrp");
    if (ret) {
        mpp_log("buf grp get obj failed ret %d\n", ret);
        goto done;
    }

    /* KmppBufGrp object ready */
    test_detail("object %s ready\n", kmpp_obj_get_name(grp));

    /* get KmppBufGrpCfg from KmppBufGrp to config */
    grp_cfg = kmpp_buf_grp_to_cfg(grp);
    if (!grp_cfg) {
        mpp_log("buf grp to cfg failed ret %d\n", ret);
        ret = MPP_NOK;
        goto done;
    }

    /* KmppBufGrpCfg object ready */
    test_detail("object %s ready\n", kmpp_obj_get_name(grp_cfg));

    if (flag & TEST_OBJ_UDUMP)
        kmpp_obj_udump(buf_cfg);

    /* write parameters to KmppBufGrpCfg */
    ret = kmpp_obj_set_u32(grp_cfg, "flag", 0);
    if (ret) {
        mpp_log("grp cfg set flag failed ret %d\n", ret);
        goto done;
    }

    ret = kmpp_obj_set_u32(grp_cfg, "count", 10);
    if (ret) {
        mpp_log("grp cfg set count failed ret %d\n", ret);
        goto done;
    }

    ret = kmpp_obj_set_u32(grp_cfg, "size", 4096);
    if (ret) {
        mpp_log("grp cfg set size failed ret %d\n", ret);
        goto done;
    }

    ret = kmpp_obj_set_s32(grp_cfg, "fd", -1);
    if (ret) {
        mpp_log("grp cfg set fd failed ret %d\n", ret);
        goto done;
    }

    /* set buffer group name to test */
    name = "allocator";
    sptr.kaddr = 0;
    sptr.uptr = "rk dma heap";

    ret = kmpp_obj_set_shm(grp_cfg, name, &sptr);
    if (ret) {
        mpp_log("grp cfg set %s failed ret %d\n", name, ret);
        goto done;
    }

    /* set buffer group name to test */
    name = "name";
    sptr.kaddr = 0;
    sptr.uptr = "test";

    ret = kmpp_obj_set_shm(grp_cfg, name, &sptr);
    if (ret) {
        mpp_log("grp cfg set %s failed ret %d\n", name, ret);
        goto done;
    }

    test_detail("object %s write parameters ready\n", kmpp_obj_get_name(grp_cfg));

    /* enable KmppBufGrpCfg by ioctl */
    ret = kmpp_buf_grp_setup(grp);

    test_detail("object %s ioctl ret %d\n", kmpp_obj_get_name(grp), ret);

    /* get KmppBuffer for buffer allocation */
    ret = kmpp_obj_get_by_name_f(&buf, "KmppBuffer");
    if (ret) {
        mpp_log("kmpp_obj_get_by_name failed ret %d\n", ret);
        goto done;
    }

    test_detail("object %s ready\n", kmpp_obj_get_name(buf));

    /* get KmppBufGrpCfg to setup */
    buf_cfg = kmpp_buffer_to_cfg(buf);
    if (!buf_cfg) {
        mpp_log("buf to cfg failed ret %d\n", ret);
        ret = MPP_NOK;
        goto done;
    }

    if (flag & TEST_OBJ_UDUMP)
        kmpp_obj_udump(buf_cfg);

    test_detail("object %s ready\n", kmpp_obj_get_name(buf_cfg));

    /* setup buffer config parameters */
    /* set buffer group */
    ret = kmpp_obj_set_shm_obj(buf_cfg, "group", grp);
    if (ret) {
        mpp_log("buf cfg set group failed ret %d\n", ret);
        goto done;
    }

    /* enable KmppBufferCfg by ioctl */
    ret = kmpp_obj_ioctl_f(buf, 0, buf, NULL);

    test_detail("object %s ioctl ret %d\n", kmpp_obj_get_name(buf), ret);

    kmpp_obj_get_u32(buf_cfg, "size", &val);

    test_detail("object %s size %d\n", kmpp_obj_get_name(buf_cfg), val);

done:
    if (grp)
        kmpp_obj_put_f(grp);

    if (buf)
        kmpp_obj_put_f(buf);

    return ret;
}

static rk_s32 kmpp_shm_test(const char *name, rk_u32 flag)
{
    rk_u32 sizes[] = {512, SZ_4K, SZ_16K, SZ_128K, SZ_256K, SZ_1M, SZ_4M, SZ_16M};
    rk_u32 count = sizeof(sizes) / sizeof(sizes[0]);
    KmppShm shm[count];
    void *ptr;
    rk_s32 ret = rk_ok;
    rk_s32 i;
    (void)name;
    (void)flag;

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(sizes); i++) {
        kmpp_shm_get_f(&shm[i], sizes[i]);
        if (!shm[i]) {
            mpp_log_f("shm get size %d failed\n", sizes[i]);
            ret = rk_nok;
        }

        test_detail("shm get size %d addr %p\n", sizes[i], kmpp_shm_to_entry_f(shm[i]));
    }

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(sizes); i++) {
        if (kmpp_shm_put_f(shm[i])) {
            mpp_log_f("shm put size %d failed\n", sizes[i]);
            ret = rk_nok;
        }
        shm[i] = NULL;
    }

    for (i = (RK_S32)MPP_ARRAY_ELEMS(sizes) - 1; i >= 0; i--) {
        kmpp_shm_get_f(&shm[i], sizes[i]);
        if (!shm[i]) {
            mpp_log_f("shm get size %d failed\n", sizes[i]);
            ret = rk_nok;
        }

        ptr = kmpp_shm_to_entry_f(shm[i]);

        test_detail("shm get size %d addr %p\n", sizes[i], ptr);

        memset(ptr, 0, sizes[i]);

        if (kmpp_shm_put_f(shm[i])) {
            mpp_log_f("shm put size %d failed\n", sizes[i]);
            ret = rk_nok;
        }
    }

    return ret;
}

static KmppObjTest obj_tests[] = {
    {
        "KmppFrame",
        0,
        kmpp_obj_std_test,
    },
    {
        "KmppVencInitCfg",
        0,
        kmpp_obj_by_name_test,
    },
    {
        "KmppBuffer",
        0,
        kmpp_buffer_test,
    },
    {
        "kmpp_shm_test",
        0,
        kmpp_shm_test,
    },
};

int main()
{
    MPP_RET ret = MPP_NOK;
    rk_u32 i;

    mpp_log("start\n");

    for (i = 0; i < MPP_ARRAY_ELEMS(obj_tests); i++) {
        const char *name = obj_tests[i].name;
        rk_u32 flag = obj_tests[i].flag;

        ret = obj_tests[i].func(name, flag);
        if (ret) {
            mpp_log("test %-16s failed ret %d\n", name, ret);
            goto done;
        }
        mpp_log("test %-16s success\n", name);
    }

done:
    mpp_log("done %s \n", ret ? "failed" : "success");

    return ret;
}
