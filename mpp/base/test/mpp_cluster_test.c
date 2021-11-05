/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_cluster_test"

#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpp_cluster.h"

typedef struct MppTestNode_t {
    MppNode         node;
} MppTestNode;

MppTestNode test_node;

static RK_S32 mpp_cluster_test_worker(void *param)
{
    RK_S32 ret = MPP_NOK;
    (void) param;

    mpp_log_f("worker run start\n");
    mpp_log_f("worker run ret %d\n", ret);

    return ret;
}

int main()
{
    MPP_RET ret = MPP_OK;
    MppNode node = test_node.node;
    RK_U32 total_run = 2;

    mpp_log("mpp_cluster_test start\n");

    ret = mpp_node_init(&node);
    if (ret) {
        mpp_err("mpp_node_init failed ret %d\n", ret);
        goto DONE;
    }

    mpp_log("mpp_cluster_test init node done\n");

    /* setup node info */
    mpp_node_set_func(node, mpp_cluster_test_worker, &test_node);

    mpp_log("mpp_cluster_test attach node start\n");
    ret = mpp_node_attach(node, VPU_CLIENT_RKVDEC);
    if (ret) {
        mpp_err("mpp_node_attach failed ret %d\n", ret);
        goto DONE;
    }

    mpp_log("mpp_cluster_test trigger start\n");

    do {
        ret = mpp_node_trigger(node, 1);
        if (ret) {
            mpp_err("mpp_node_trigger failed ret %d\n", ret);
            goto DONE;
        }

        mpp_log("mpp_cluster_test trigger %d left\n", total_run);

        msleep(5);
    } while (--total_run);

    mpp_log("mpp_cluster_test detach start\n");

    ret = mpp_node_detach(node);
    if (ret) {
        mpp_err("mpp_node_detach failed ret %d\n", ret);
        goto DONE;
    }

    mpp_log("mpp_cluster_test deinit start\n");

    ret = mpp_node_deinit(node);
    if (ret) {
        mpp_err("mpp_node_deinit failed ret %d\n", ret);
        goto DONE;
    }

    mpp_log("mpp_cluster_test deinit done\n");

DONE:
    mpp_log("mpp_cluster_test done %s\n", ret ? "failed" : "success");
    return ret;
}
