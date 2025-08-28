/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_meta"

#include <string.h>
#include <endian.h>

#include "mpp_env.h"
#include "mpp_lock.h"
#include "mpp_debug.h"
#include "mpp_mem_pool.h"
#include "mpp_singleton.h"

#include "mpp_trie.h"
#include "mpp_meta_impl.h"

#define META_DBG_FLOW               (0x00000001)
#define META_DBG_KEYS               (0x00000002)

#define meta_dbg(flag, fmt, ...)    _mpp_dbg(mpp_meta_debug, flag, fmt, ## __VA_ARGS__)
#define meta_dbg_f(flag, fmt, ...)  _mpp_dbg_f(mpp_meta_debug, flag, fmt, ## __VA_ARGS__)

#define meta_dbg_flow(fmt, ...)     meta_dbg(META_DBG_FLOW, fmt, ## __VA_ARGS__)

#define META_VAL_INVALID            (0x00000000)
#define META_VAL_VALID              (0x00000001)
#define META_VAL_READY              (0x00000002)

#define WRITE_ONCE(x, val)          ((*(volatile typeof(x) *) &(x)) = (val))
#define READ_ONCE(var)              (*((volatile typeof(var) *)(&(var))))

#define get_srv_meta(caller) \
    ({ \
        MppMetaSrv *__tmp; \
        if (srv_meta || srv_finalized) { \
            __tmp = srv_meta; \
        } else { \
            mpp_meta_srv_init(); \
            __tmp = srv_meta; \
            if (!__tmp) { \
                mpp_err("mpp meta srv not init at %s : %s\n", __FUNCTION__, caller); \
            } \
        } \
        __tmp; \
    })

#define get_srv_meta_f() \
    ({ \
        MppMetaSrv *__tmp; \
        if (srv_meta || srv_finalized) { \
            __tmp = srv_meta; \
        } else { \
            mpp_meta_srv_init(); \
            __tmp = srv_meta; \
            if (!__tmp) { \
                mpp_err("mpp meta srv not init at %s\n", __FUNCTION__); \
            } \
        } \
        __tmp; \
    })

typedef enum MppMetaDataType_e {
    /* mpp meta data of normal data type */
    TYPE_VAL_32 = '3',
    TYPE_VAL_64 = '6',
    TYPE_KPTR   = 'k',
    TYPE_UPTR   = 'u',
    TYPE_SPTR   = 's',
} MppMetaType;

static inline RK_U64 META_KEY_TO_U64(RK_U32 key, RK_U32 type)
{
    return (RK_U64)((RK_U32)htobe32(key)) | ((RK_U64)type << 32);
}

#define EXPAND_AS_TRIE(key, type) \
    do { \
        RK_U64 val = META_KEY_TO_U64(key, type); \
        mpp_trie_add_info(srv->trie, (const char *)&val, NULL, 0); \
        meta_key_count++; \
    } while (0);

#define EXPAND_AS_LOG(key, type) \
    do { \
        RK_U32 key_val = htobe32(key); \
        char *str = (char *)&key_val; \
        mpp_logi("%2d - %-24s (%c%c%c%c) : %-12s\n", \
                 i++, #key, str[0], str[1], str[2], str[3], #type); \
    } while (0);

#define META_ENTRY_TABLE(ENTRY) \
    /* categorized by type */ \
    /* data flow type */ \
    ENTRY(KEY_INPUT_FRAME,          TYPE_SPTR) \
    ENTRY(KEY_OUTPUT_FRAME,         TYPE_SPTR) \
    ENTRY(KEY_INPUT_PACKET,         TYPE_SPTR) \
    ENTRY(KEY_OUTPUT_PACKET,        TYPE_SPTR) \
    /* buffer for motion detection */ \
    ENTRY(KEY_MOTION_INFO,          TYPE_SPTR) \
    /* buffer storing the HDR information for current frame*/ \
    ENTRY(KEY_HDR_INFO,             TYPE_SPTR) \
    /* the offset of HDR meta data in frame buffer */ \
    ENTRY(KEY_HDR_META_OFFSET,      TYPE_VAL_32) \
    ENTRY(KEY_HDR_META_SIZE,        TYPE_VAL_32) \
    \
    ENTRY(KEY_OUTPUT_INTRA,         TYPE_VAL_32) \
    ENTRY(KEY_INPUT_BLOCK,          TYPE_VAL_32) \
    ENTRY(KEY_OUTPUT_BLOCK,         TYPE_VAL_32) \
    ENTRY(KEY_INPUT_IDR_REQ,        TYPE_VAL_32) \
    \
    /* extra information for tsvc */ \
    ENTRY(KEY_TEMPORAL_ID,          TYPE_VAL_32) \
    ENTRY(KEY_LONG_REF_IDX,         TYPE_VAL_32) \
    ENTRY(KEY_ENC_AVERAGE_QP,       TYPE_VAL_32) \
    ENTRY(KEY_ENC_START_QP,         TYPE_VAL_32) \
    ENTRY(KEY_ENC_BPS_RT,           TYPE_VAL_32) \
    \
    ENTRY(KEY_ROI_DATA,             TYPE_UPTR) \
    ENTRY(KEY_ROI_DATA2,            TYPE_UPTR) \
    ENTRY(KEY_JPEG_ROI_DATA,        TYPE_UPTR) \
    ENTRY(KEY_OSD_DATA,             TYPE_UPTR) \
    ENTRY(KEY_OSD_DATA2,            TYPE_UPTR) \
    ENTRY(KEY_OSD_DATA3,            TYPE_UPTR) \
    ENTRY(KEY_USER_DATA,            TYPE_UPTR) \
    ENTRY(KEY_USER_DATAS,           TYPE_UPTR) \
    ENTRY(KEY_QPMAP0,               TYPE_SPTR) \
    /* buffer for super encode v3 */ \
    ENTRY(KEY_NPU_SOBJ_FLAG,        TYPE_SPTR) \
    ENTRY(KEY_NPU_UOBJ_FLAG,        TYPE_UPTR) \
    ENTRY(KEY_BUFFER_UPSCALE,       TYPE_SPTR) \
    ENTRY(KEY_BUFFER_DOWNSCALE,     TYPE_SPTR) \
    \
    ENTRY(KEY_LVL64_INTER_NUM,      TYPE_VAL_32) \
    ENTRY(KEY_LVL32_INTER_NUM,      TYPE_VAL_32) \
    ENTRY(KEY_LVL16_INTER_NUM,      TYPE_VAL_32) \
    ENTRY(KEY_LVL8_INTER_NUM,       TYPE_VAL_32) \
    ENTRY(KEY_LVL32_INTRA_NUM,      TYPE_VAL_32) \
    ENTRY(KEY_LVL16_INTRA_NUM,      TYPE_VAL_32) \
    ENTRY(KEY_LVL8_INTRA_NUM,       TYPE_VAL_32) \
    ENTRY(KEY_LVL4_INTRA_NUM,       TYPE_VAL_32) \
    ENTRY(KEY_INPUT_PSKIP,          TYPE_VAL_32) \
    ENTRY(KEY_OUTPUT_PSKIP,         TYPE_VAL_32) \
    ENTRY(KEY_INPUT_PSKIP_NON_REF,  TYPE_VAL_32) \
    ENTRY(KEY_ENC_SSE,              TYPE_VAL_64) \
    \
    ENTRY(KEY_ENC_MARK_LTR,         TYPE_VAL_32) \
    ENTRY(KEY_ENC_USE_LTR,          TYPE_VAL_32) \
    ENTRY(KEY_ENC_FRAME_QP,         TYPE_VAL_32) \
    ENTRY(KEY_ENC_BASE_LAYER_PID,   TYPE_VAL_32) \
    \
    ENTRY(KEY_DEC_TBN_EN,           TYPE_VAL_32) \
    ENTRY(KEY_DEC_TBN_Y_OFFSET,     TYPE_VAL_32) \
    ENTRY(KEY_DEC_TBN_UV_OFFSET,    TYPE_VAL_32)

typedef struct MppMetaSrv_t {
    spinlock_t          lock;
    struct list_head    list_meta;
    MppTrie             trie;

    RK_U32              meta_id;
    RK_S32              meta_count;
} MppMetaSrv;

static MppMetaSrv *srv_meta = NULL;
static MppMemPool pool_meta = NULL;
static RK_U32 srv_finalized = 0;
static RK_U32 meta_key_count = 0;
static RK_U32 mpp_meta_debug = 0;

static void put_meta(MppMetaSrv *srv, MppMetaImpl *meta);

static void mpp_meta_srv_init()
{
    MppMetaSrv *srv = srv_meta;

    mpp_env_get_u32("mpp_meta_debug", &mpp_meta_debug, 0);

    if (srv)
        return;

    srv = mpp_calloc(MppMetaSrv, 1);
    if (!srv) {
        mpp_err_f("failed to malloc meta service\n");
        return;
    }

    srv_meta = srv;

    mpp_spinlock_init(&srv->lock);
    INIT_LIST_HEAD(&srv->list_meta);

    mpp_trie_init(&srv->trie, "MppMetaDef");
    if (srv->trie) {
        meta_key_count = 0;
        META_ENTRY_TABLE(EXPAND_AS_TRIE)
        mpp_trie_add_info(srv->trie, NULL, NULL, 0);
    }

    pool_meta = mpp_mem_pool_init_f("MppMeta", sizeof(MppMetaImpl) +
                                    sizeof(MppMetaVal) * meta_key_count);

    meta_dbg_flow("meta key count %d\n", meta_key_count);
    if (mpp_meta_debug & META_DBG_KEYS) {
        RK_S32 i = 0;

        META_ENTRY_TABLE(EXPAND_AS_LOG)
    }
}

static void mpp_meta_srv_deinit()
{
    MppMetaSrv *srv = srv_meta;

    if (!srv)
        return;

    if (!list_empty(&srv->list_meta)) {
        MppMetaImpl *pos, *n;

        mpp_log_f("cleaning leaked metadata\n");

        list_for_each_entry_safe(pos, n, &srv->list_meta, MppMetaImpl, list_meta) {
            put_meta(srv, pos);
        }
    }

    mpp_assert(srv->meta_count == 0);

    if (srv->trie) {
        mpp_trie_deinit(srv->trie);
        srv->trie = NULL;
    }

    MPP_FREE(srv_meta);

    if (pool_meta) {
        mpp_mem_pool_deinit_f(pool_meta);
        pool_meta = NULL;
    }

    srv_finalized = 1;

    meta_dbg_flow("meta srv deinited\n");
}

MPP_SINGLETON(MPP_SGLN_META, mpp_meta, mpp_meta_srv_init, mpp_meta_srv_deinit)

static inline RK_S32 get_index_of_key(MppMetaKey key, MppMetaType type, const char *caller)
{
    MppMetaSrv *srv = get_srv_meta(caller);
    MppTrieInfo *info = NULL;

    if (srv) {
        RK_U64 val = META_KEY_TO_U64(key, type);

        info = mpp_trie_get_info(srv->trie, (const char *)&val);
    }

    return info ? info->index : -1;
}

#define get_index_of_key_f(key, type) get_index_of_key(key, type, __FUNCTION__)

static MppMetaImpl *get_meta(MppMetaSrv *srv, const char *tag, const char *caller)
{
    MppMetaImpl *impl = (MppMetaImpl *)mpp_mem_pool_get(pool_meta, caller);

    if (impl) {
        const char *tag_src = (tag) ? (tag) : (MODULE_TAG);
        RK_U32 i;

        strncpy(impl->tag, tag_src, sizeof(impl->tag) - 1);
        impl->caller = caller;
        impl->meta_id = MPP_FETCH_ADD(&srv->meta_id, 1);
        INIT_LIST_HEAD(&impl->list_meta);
        impl->ref_count = 1;
        impl->node_count = 0;

        for (i = 0; i < meta_key_count; i++)
            impl->vals[i].state = 0;

        mpp_spinlock_lock(&srv->lock);
        list_add_tail(&impl->list_meta, &srv->list_meta);
        mpp_spinlock_unlock(&srv->lock);
        MPP_FETCH_ADD(&srv->meta_count, 1);
    } else {
        mpp_err_f("failed to malloc meta data\n");
    }

    return impl;
}

static void put_meta(MppMetaSrv *srv, MppMetaImpl *meta)
{
    RK_S32 ref_count;

    if (!srv)
        return;

    ref_count = MPP_SUB_FETCH(&meta->ref_count, 1);
    if (ref_count > 0)
        return;

    if (ref_count < 0) {
        mpp_err_f("invalid negative ref_count %d\n", ref_count);
        return;
    }

    mpp_spinlock_lock(&srv->lock);
    list_del_init(&meta->list_meta);
    mpp_spinlock_unlock(&srv->lock);
    MPP_FETCH_SUB(&srv->meta_count, 1);

    if (pool_meta)
        mpp_mem_pool_put_f(pool_meta, meta);
}

MPP_RET mpp_meta_get_with_tag(MppMeta *meta, const char *tag, const char *caller)
{
    MppMetaSrv *srv = get_srv_meta(caller);
    MppMetaImpl *impl;

    if (!srv)
        return MPP_NOK;

    if (!meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    impl = get_meta(srv, tag, caller);
    *meta = (MppMeta) impl;
    return (impl) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_meta_put(MppMeta meta)
{
    MppMetaImpl *impl = (MppMetaImpl *)meta;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    put_meta(get_srv_meta_f(), impl);
    return MPP_OK;
}

MPP_RET mpp_meta_inc_ref(MppMeta meta)
{
    MppMetaImpl *impl = (MppMetaImpl *)meta;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_FETCH_ADD(&impl->ref_count, 1);
    return MPP_OK;
}

RK_S32 mpp_meta_size(MppMeta meta)
{
    MppMetaImpl *impl = (MppMetaImpl *)meta;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return -1;
    }

    return MPP_FETCH_ADD(&impl->node_count, 0);
}

MPP_RET mpp_meta_dump(MppMeta meta)
{
    MppMetaSrv *srv = get_srv_meta_f();
    MppMetaImpl *impl = (MppMetaImpl *)meta;
    MppTrieInfo *root;

    if (!impl) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_logi("dumping meta %d node count %d\n", impl->meta_id, impl->node_count);

    if (!srv || !srv->trie)
        return MPP_NOK;

    root = mpp_trie_get_info_first(srv->trie);
    if (root) {
        MppTrieInfo *node = root;
        const char *key = NULL;
        char log_str[256];
        RK_S32 pos;

        do {
            if (mpp_trie_info_is_self(node))
                continue;

            key = mpp_trie_info_name(node);

            pos = snprintf(log_str, sizeof(log_str) - 1, "key %c%c%c%c - ",
                           key[0], key[1], key[2], key[3]);

            switch (key[4]) {
            case '3' : {
                snprintf(log_str + pos, sizeof(log_str) - pos - 1, "s32 - %d",
                         impl->vals[node->index].val_s32);
            } break;
            case '6' : {
                snprintf(log_str + pos, sizeof(log_str) - pos - 1, "s64 - %lld",
                         impl->vals[node->index].val_s64);
            } break;
            case 'k' :
            case 'u' :
            case 's' : {
                snprintf(log_str + pos, sizeof(log_str) - pos - 1, "ptr - %p",
                         impl->vals[node->index].val_ptr);
            } break;
            default : {
            } break;
            }

            mpp_logi("%s\n", log_str);
        } while ((node = mpp_trie_get_info_next(srv->trie, node)));
    }

    return MPP_OK;
}

#define MPP_META_ACCESSOR(func_type, arg_type, key_type, key_field)  \
    MPP_RET mpp_meta_set_##func_type(MppMeta meta, MppMetaKey key, arg_type val) \
    { \
        MppMetaImpl *impl = (MppMetaImpl *)meta; \
        MppMetaVal *meta_val; \
        RK_S32 index; \
        if (!impl) { \
            mpp_err_f("found NULL input\n"); \
            return MPP_ERR_NULL_PTR; \
        } \
        index = get_index_of_key_f(key, key_type); \
        if (index < 0) \
            return MPP_NOK; \
        meta_val = &impl->vals[index]; \
        if (MPP_BOOL_CAS(&meta_val->state, META_VAL_INVALID, META_VAL_VALID)) \
            MPP_FETCH_ADD(&impl->node_count, 1); \
        meta_val->key_field = val; \
        MPP_FETCH_OR(&meta_val->state, META_VAL_READY); \
        return MPP_OK; \
    } \
    MPP_RET mpp_meta_get_##func_type(MppMeta meta, MppMetaKey key, arg_type *val) \
    { \
        MppMetaImpl *impl = (MppMetaImpl *)meta; \
        MppMetaVal *meta_val; \
        RK_S32 index; \
        MPP_RET ret = MPP_NOK; \
        if (!impl) { \
            mpp_err_f("found NULL input\n"); \
            return MPP_ERR_NULL_PTR; \
        } \
        index = get_index_of_key_f(key, key_type); \
        if (index < 0) \
            return MPP_NOK; \
        meta_val = &impl->vals[index]; \
        if (MPP_BOOL_CAS(&meta_val->state, META_VAL_VALID | META_VAL_READY, META_VAL_INVALID)) { \
            *val = meta_val->key_field; \
            MPP_FETCH_SUB(&impl->node_count, 1); \
            ret = MPP_OK; \
        } \
        return ret; \
    } \
    MPP_RET mpp_meta_get_##func_type##_d(MppMeta meta, MppMetaKey key, arg_type *val, arg_type def) \
    { \
        MppMetaImpl *impl = (MppMetaImpl *)meta; \
        MppMetaVal *meta_val; \
        RK_S32 index; \
        MPP_RET ret = MPP_NOK; \
        if (!impl) { \
            mpp_err_f("found NULL input\n"); \
            return MPP_ERR_NULL_PTR; \
        } \
        index = get_index_of_key_f(key, key_type); \
        if (index < 0) \
            return MPP_NOK; \
        meta_val = &impl->vals[index]; \
        if (MPP_BOOL_CAS(&meta_val->state, META_VAL_VALID | META_VAL_READY, META_VAL_INVALID)) { \
            *val = meta_val->key_field; \
            MPP_FETCH_SUB(&impl->node_count, 1); \
            ret = MPP_OK; \
        } else { \
            *val = def; \
        } \
        return ret; \
    }

MPP_META_ACCESSOR(s32, RK_S32, TYPE_VAL_32, val_s32)
MPP_META_ACCESSOR(s64, RK_S64, TYPE_VAL_64, val_s64)
MPP_META_ACCESSOR(ptr, void *, TYPE_UPTR, val_ptr)
MPP_META_ACCESSOR(frame, MppFrame, TYPE_SPTR, frame)
MPP_META_ACCESSOR(packet, MppPacket, TYPE_SPTR, packet)
MPP_META_ACCESSOR(buffer, MppBuffer, TYPE_SPTR, buffer)
