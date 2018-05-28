/*
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

#define MODULE_TAG "mpp_rt"

#include <fcntl.h>
#include <unistd.h>

#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_runtime.h"

#define MAX_DTS_PATH_LEN        256

static const char *mpp_dts_base = "/proc/device-tree/";

static const char *mpp_vpu_names[] = {
    "vpu_service",
    "vpu-service",
    "vpu",
    //"hevc_service",
    //"hevc-service",
    //"rkvdec",
    //"rkvenc",
    //"vpu_combo",
};

static const char *mpp_vpu_address[] = {
    "",                 /* old kernel   */
    "@10108000",        /* rk3036       */
    "@20020000",        /* rk322x       */
    "@30000000",        /* rv1108       */
    "@ff9a0000",        /* rk3288/3366  */
    "@ff350000",        /* rk322xh/3328 */
    "@ff650000",        /* rk3399       */
};

class MppRuntimeService
{
private:
    // avoid any unwanted function
    MppRuntimeService();
    ~MppRuntimeService() {};
    MppRuntimeService(const MppRuntimeService &);
    MppRuntimeService &operator=(const MppRuntimeService &);

    RK_U32  allocator_valid[MPP_BUFFER_TYPE_BUTT];

public:
    static MppRuntimeService *get_instance() {
        static MppRuntimeService instance;
        return &instance;
    }

    RK_U32 get_allocator_valid(MppBufferType type);
};

RK_U32 MppRuntimeService::get_allocator_valid(MppBufferType type)
{
    MppBufferType buffer_type = (MppBufferType)(type & MPP_BUFFER_TYPE_MASK);
    return (buffer_type < MPP_BUFFER_TYPE_BUTT) ? allocator_valid[buffer_type] : (0);
};

MppRuntimeService::MppRuntimeService()
{
    allocator_valid[MPP_BUFFER_TYPE_NORMAL] = 1;

    if (access("/dev/ion", F_OK | R_OK | W_OK)) {
        allocator_valid[MPP_BUFFER_TYPE_ION] = 0;
        mpp_log("NOT found ion allocator\n");
    } else {
        allocator_valid[MPP_BUFFER_TYPE_ION] = 1;
        mpp_log("found ion allocator\n");
    }

    if (access("/dev/dri/card0", F_OK | R_OK | W_OK)) {
        allocator_valid[MPP_BUFFER_TYPE_DRM] = 0;
        mpp_log("NOT found drm allocator\n");
    } else {
        allocator_valid[MPP_BUFFER_TYPE_DRM] = 1;
        mpp_log("found drm allocator\n");
    }

    // If both ion and drm is enabled detect allocator in dts to choose one
    // TODO: When unify dma fd kernel is completed this part will be removed.
    if (allocator_valid[MPP_BUFFER_TYPE_ION] &&
        allocator_valid[MPP_BUFFER_TYPE_DRM]) {
        /* Detect hardware buffer type is ion or drm */
        RK_U32 i, j;
        char path[MAX_DTS_PATH_LEN];
        RK_U32 path_len = MAX_DTS_PATH_LEN;
        RK_U32 dts_path_len = snprintf(path, path_len, "%s", mpp_dts_base);
        char *p = path + dts_path_len;
        RK_U32 allocator_found = 0;

        path_len -= dts_path_len;

        for (i = 0; i < MPP_ARRAY_ELEMS(mpp_vpu_names); i++) {
            for (j = 0; j < MPP_ARRAY_ELEMS(mpp_vpu_address); j++) {
                RK_U32 dev_path_len = snprintf(p, path_len, "%s%s",
                                               mpp_vpu_names[i], mpp_vpu_address[j]);
                int f_ok = access(path, F_OK);
                if (f_ok == 0) {
                    snprintf(p + dev_path_len, path_len - dev_path_len, "/%s", "allocator");
                    f_ok = access(path, F_OK);
                    if (f_ok == 0) {
                        RK_S32 val = 0;
                        FILE *fp = fopen(path, "rb");
                        if (fp) {
                            size_t len = fread(&val, 1, 4, fp);
                            // zero for ion non-zero for drm ->
                            // zero     - disable drm
                            // non-zero - disable ion
                            if (len != 4) {
                                mpp_err("failed to read dts allocator value default 0\n");
                                val = 0;
                            }

                            if (val == 0) {
                                allocator_valid[MPP_BUFFER_TYPE_DRM] = 0;
                                mpp_log("found ion allocator in dts\n");
                            } else {
                                allocator_valid[MPP_BUFFER_TYPE_ION] = 0;
                                mpp_log("found drm allocator in dts\n");
                            }
                            allocator_found = 1;
                        }
                    }
                }
                if (allocator_found)
                    break;
            }
            if (allocator_found)
                break;
        }

        if (!allocator_found)
            mpp_log("Can NOT found allocator in dts, enable both ion and drm\n");
    }
}

RK_U32 mpp_rt_allcator_is_valid(MppBufferType type)
{
    return MppRuntimeService::get_instance()->get_allocator_valid(type);
}
