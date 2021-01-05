/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_cfg"

#include "mpp_log.h"
#include "mpp_cfg.h"

const char *cfg_type_names[] = {
    "RK_S32",
    "RK_U32",
    "RK_S64",
    "RK_U64",
    "void *",
    "struct",
};

static void show_api_type_err(MppCfgApi *api, CfgType type)
{
    mpp_err("cfg %s expect %s input NOT %s\n", api->name,
            cfg_type_names[api->info.type],
            cfg_type_names[type]);
}

MPP_RET check_cfg_api_info(MppCfgApi *api, CfgType type)
{
    CfgType cfg_type = api->info.type;
    RK_S32 cfg_size = api->info.size;
    MPP_RET ret = MPP_OK;

    switch (type) {
    case CFG_FUNC_TYPE_St : {
        if (cfg_type != type) {
            show_api_type_err(api, type);
            ret = MPP_NOK;
        }
        if (cfg_size <= 0) {
            mpp_err("cfg %s found invalid size %d\n", api->name, cfg_size);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_Ptr : {
        if (cfg_type != type) {
            show_api_type_err(api, type);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_S32 :
    case CFG_FUNC_TYPE_U32 : {
        if (cfg_size != sizeof(RK_S32)) {
            show_api_type_err(api, type);
            ret = MPP_NOK;
        }
    } break;
    case CFG_FUNC_TYPE_S64 :
    case CFG_FUNC_TYPE_U64 : {
        if (cfg_size != sizeof(RK_S64)) {
            show_api_type_err(api, type);
            ret = MPP_NOK;
        }
    } break;
    default : {
        mpp_err("cfg %s found invalid cfg type %d\n", api->name, type);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}
