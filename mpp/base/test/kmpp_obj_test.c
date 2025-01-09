/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_obj_test"

#include "mpp_log.h"
#include "kmpp_obj.h"

int main()
{
    MPP_RET ret = MPP_NOK;
    KmppObjDef def = NULL;
    KmppObj obj = NULL;
    const char *name = NULL;

    mpp_log(MODULE_TAG " start\n");

    ret = kmpp_objdef_get(&def, "KmppFrame");
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_objdef_get failed\n");
        goto done;
    }

    name = kmpp_objdef_get_name(def);
    kmpp_objdef_dump(def);

    ret = kmpp_obj_get(&obj, def);
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_obj_get %s failed ret %d\n", name, ret);
        goto done;
    }

    kmpp_obj_udump(obj);
    kmpp_obj_kdump(obj);

    ret = kmpp_obj_put(obj);
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_obj_put %s failed\n", name);
        goto done;
    }
    obj = NULL;

    ret = kmpp_objdef_put(def);
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_objdef_put %s failed\n", name);
        goto done;
    }
    def = NULL;

    ret = kmpp_objdef_get(&def, "KmppVencInitCfg");
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_objdef_get failed\n");
        goto done;
    }

    name = kmpp_objdef_get_name(def);
    kmpp_objdef_dump(def);

    ret = kmpp_obj_get(&obj, def);
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_obj_get %s failed ret %d\n", name, ret);
        goto done;
    }

    kmpp_obj_udump(obj);
    kmpp_obj_kdump(obj);

    ret = kmpp_obj_put(obj);
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_obj_put %s failed\n", name);
        goto done;
    }
    obj = NULL;

    ret = kmpp_objdef_put(def);
    if (ret) {
        mpp_log(MODULE_TAG " kmpp_objdef_put %s failed\n", name);
        goto done;
    }

done:
    if (ret) {
        if (obj)
            kmpp_obj_put(obj);

        if (def)
            kmpp_objdef_put(def);
    }
    mpp_log(MODULE_TAG " done %s \n", ret ? "failed" : "success");

    return ret;
}
