/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef __RC_IMPL_H__
#define __RC_IMPL_H__

#include "mpp_list.h"
#include "mpp_rc_api.h"

#define MAX_RC_API_COUNT        32

#ifdef __cplusplus

class RcImplApiService
{
private:
    // avoid any unwanted function
    RcImplApiService();
    ~RcImplApiService();
    RcImplApiService(const RcImplApiService &);
    RcImplApiService &operator=(const RcImplApiService &);

    RK_U32              mApiCount;

    struct list_head    mApis;

public:
    static RcImplApiService *get_instance() {
        static RcImplApiService instance;
        AutoMutex auto_lock(get_lock());
        return &instance;
    }
    static Mutex *get_lock() {
        static Mutex lock;
        return &lock;
    }

    MPP_RET     api_add(const RcImplApi *api);
    RcImplApi   *api_get(MppCodingType type, const char *name);

    MPP_RET     api_get_all(RcApiBrief *brief, RK_S32 *count, RK_S32 max_count);
    MPP_RET     api_get_by_type(RcApiBrief *brief, RK_S32 *count,
                                RK_S32 max_count, MppCodingType type);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RC_IMPL_H__ */
