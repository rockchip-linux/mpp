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

#define MODULE_TAG "mpp_opt"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_trie.h"
#include "mpp_common.h"

#include "mpp_opt.h"

typedef struct MppOptImpl_t {
    void        *ctx;
    MppTrie     trie;
    RK_S32      node_cnt;
    RK_S32      info_cnt;
} MppOptImpl;

MPP_RET mpp_opt_init(MppOpt *opt)
{
    MppOptImpl *impl = mpp_calloc(MppOptImpl, 1);

    *opt = impl;

    return (impl) ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_opt_deinit(MppOpt opt)
{
    MppOptImpl *impl = (MppOptImpl *)opt;

    if (NULL == impl)
        return MPP_NOK;

    if (impl->trie) {
        mpp_trie_deinit(impl->trie);
        impl->trie = NULL;
    }
    MPP_FREE(impl);

    return MPP_OK;
}

MPP_RET mpp_opt_setup(MppOpt opt, void *ctx)
{
    MppOptImpl *impl = (MppOptImpl *)opt;

    if (NULL == impl)
        return MPP_NOK;

    mpp_trie_init(&impl->trie, "mpp_opt");
    if (impl->trie) {
        impl->ctx = ctx;
        return MPP_OK;
    }

    return MPP_NOK;
}

MPP_RET mpp_opt_add(MppOpt opt, MppOptInfo *info)
{
    MppOptImpl *impl = (MppOptImpl *)opt;

    if (NULL == impl || NULL == impl->trie)
        return MPP_NOK;

    if (NULL == info)
        return mpp_trie_add_info(impl->trie, NULL, NULL, 0);

    return mpp_trie_add_info(impl->trie, info->name, info, sizeof(*info));
}

MPP_RET mpp_opt_parse(MppOpt opt, int argc, char **argv)
{
    MppOptImpl *impl = (MppOptImpl *)opt;
    char **cfg_argv = NULL;
    RK_S32 cfg_argc_idx = 0;
    MPP_RET ret = MPP_NOK;
    RK_S32 opt_idx = 0;
    RK_S32 i;

    if (NULL == impl || NULL == impl->trie || argc < 2 || NULL == argv)
        return ret;

    cfg_argv = mpp_calloc(char*, argc);
    if (!cfg_argv) {
        mpp_err("malloc cfg_argv failed\n");
        return ret;
    }

    /* 1. put the program name at the first position of the new array */
    cfg_argv[cfg_argc_idx++] = argv[0];

    /* 2. then put -cfg and its argument */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-cfg") == 0) {
            cfg_argv[cfg_argc_idx++] = argv[i++];
            if (i < argc)
                cfg_argv[cfg_argc_idx++] = argv[i];
        }
    }

    /* 3. finally put other command arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-cfg") == 0)
            i++;
        else
            cfg_argv[cfg_argc_idx++] = argv[i];
    }

    ret = MPP_OK;

    /* use cfg_argv to parse options */
    while (opt_idx <= cfg_argc_idx) {
        RK_S32 opt_next = opt_idx + 1;
        char *opts = cfg_argv[opt_idx++];
        char *next = (opt_next >= cfg_argc_idx) ? NULL : cfg_argv[opt_next];

        if (NULL == opts)
            break;

        if (opts[0] == '-' && opts[1] != '\0') {
            MppOptInfo *info = NULL;
            MppTrieInfo *node = mpp_trie_get_info(impl->trie, opts + 1);
            RK_S32 step = 0;

            if (NULL == node) {
                mpp_err("invalid option %s\n", opts + 1);
                continue;
            }

            info = mpp_trie_info_ctx(node);
            if (info->proc)
                step = info->proc(impl->ctx, next);

            /* option failure or help */
            if (step < 0) {
                ret = step;
                break;
            }

            opt_idx += step;
        }
    }

    MPP_FREE(cfg_argv);

    return ret;
}
