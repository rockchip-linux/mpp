/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_meta"

#include <string.h>
#include <endian.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_lock.h"
#include "mpp_debug.h"
#include "mpp_singleton.h"

#include "mpp_trie.h"
#include "mpp_meta_impl.h"

#include "kmpp_obj.h"
#include "kmpp_meta.h"
#include "rk_venc_cmd.h"

#define META_DBG_FLOW               (0x00000001)
#define META_DBG_KEYS               (0x00000002)

#define meta_dbg(flag, fmt, ...)    mpp_dbg(mpp_meta_debug, flag, fmt, ## __VA_ARGS__)
#define meta_dbg_f(flag, fmt, ...)  mpp_dbg_f(mpp_meta_debug, flag, fmt, ## __VA_ARGS__)

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

#define EXPAND_AS_TRIE(key, _type) \
    do { \
        RK_U64 val = META_KEY_TO_U64(key, _type); \
        KmppEntry e = { .val = 0 }; \
        e.tbl.type = ENTRY_TYPE_LOC_TBL; \
        e.tbl.elem_offset = (rk_u16)(meta_key_count * sizeof(MppMetaVal)); \
        e.tbl.elem_size = sizeof(MppMetaVal); \
        e.tbl.elem_type = ELEM_TYPE_s32; \
        kmpp_objdef_add_entry(mpp_meta_def, 0, (const char *)&val, &e); \
        meta_key_count++; \
    } while (0);

#define EXPAND_AS_COUNT(key, _type) \
    meta_key_count++;

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
    ENTRY(KEY_ENC_MADI_B16,         TYPE_VAL_32) \
    ENTRY(KEY_ENC_MADP_CTU,         TYPE_VAL_32) \
    ENTRY(KEY_INPUT_PSKIP,          TYPE_VAL_32) \
    ENTRY(KEY_OUTPUT_PSKIP,         TYPE_VAL_32) \
    ENTRY(KEY_INPUT_PSKIP_NON_REF,  TYPE_VAL_32) \
    ENTRY(KEY_INPUT_PSKIP_NUM,      TYPE_VAL_32) \
    ENTRY(KEY_ENC_SSE,              TYPE_VAL_64) \
    \
    ENTRY(KEY_ENC_MARK_LTR,         TYPE_VAL_32) \
    ENTRY(KEY_ENC_USE_LTR,          TYPE_VAL_32) \
    ENTRY(KEY_ENC_FRAME_QP,         TYPE_VAL_32) \
    ENTRY(KEY_ENC_BASE_LAYER_PID,   TYPE_VAL_32) \
    \
    /* raw decoder collocated motion-vector buffer */ \
    ENTRY(KEY_DEC_COLMV,             TYPE_SPTR) \
    ENTRY(KEY_DEC_COLMV_FMT,         TYPE_VAL_32) \
    ENTRY(KEY_DEC_COLMV_SIZE,        TYPE_VAL_32) \
    \
    ENTRY(KEY_DEC_TBN_EN,           TYPE_VAL_32) \
    ENTRY(KEY_DEC_TBN_Y_OFFSET,     TYPE_VAL_32) \
    ENTRY(KEY_DEC_TBN_UV_OFFSET,    TYPE_VAL_32)

typedef struct MppMetaSrv_t {
    spinlock_t          lock;
    struct list_head    list_meta;

    RK_U32              meta_id;
    RK_S32              meta_count;
} MppMetaSrv;

static MppMetaSrv *srv_meta = NULL;
static KmppObjDef mpp_meta_def = NULL;
static RK_U32 srv_finalized = 0;
static RK_U32 meta_key_count = 0;
static RK_U32 mpp_meta_debug = 0;
static RK_S32 user_data_index = -1;
static RK_S32 user_datas_index = -1;

RK_S32 meta_hdr_offset_index = -1;
RK_S32 meta_hdr_size_index = -1;

static void put_meta(MppMetaSrv *srv, KmppObj meta);
static void clean_user_data(MppMetaPriv *priv);
static void clean_user_datas(MppMetaPriv *priv);
static inline RK_S32 get_index_of_key(MppMetaKey key, MppMetaType type, const char *caller);
#define get_index_of_key_f(key, type) get_index_of_key(key, type, __FUNCTION__);

static rk_s32 mpp_meta_impl_init(void *entry, KmppObj obj, const char *caller)
{
    MppMetaSrv *srv = srv_meta;
    MppMetaPriv *priv;
    MppMetaVal *vals = (MppMetaVal *)entry;
    RK_U32 i;

    (void)caller;

    if (!srv)
        return rk_nok;

    for (i = 0; i < meta_key_count; i++)
        vals[i].state = 0;

    priv = (MppMetaPriv *)kmpp_obj_to_priv(obj);
    if (priv) {
        priv->obj = obj;
        priv->meta_id = MPP_FETCH_ADD(&srv->meta_id, 1);
        INIT_LIST_HEAD(&priv->list_meta);
        priv->ref_count = 1;
        priv->node_count = 0;

        mpp_spinlock_lock(&srv->lock);
        list_add_tail(&priv->list_meta, &srv->list_meta);
        mpp_spinlock_unlock(&srv->lock);
        MPP_FETCH_ADD(&srv->meta_count, 1);
    }
    return rk_ok;
}

static rk_s32 mpp_meta_impl_deinit(void *entry, KmppObj obj, const char *caller)
{
    (void)entry;
    (void)caller;
    MppMetaSrv *srv = srv_meta;
    MppMetaPriv *priv;

    if (!srv)
        return rk_nok;

    priv = (MppMetaPriv *)kmpp_obj_to_priv(obj);
    if (priv) {
        clean_user_data(priv);
        clean_user_datas(priv);

        mpp_spinlock_lock(&srv->lock);
        list_del_init(&priv->list_meta);
        mpp_spinlock_unlock(&srv->lock);
        MPP_FETCH_SUB(&srv->meta_count, 1);
    }
    return rk_ok;
}

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

    /* Step 1: count meta keys to determine impl_size */
    meta_key_count = 0;
    META_ENTRY_TABLE(EXPAND_AS_COUNT)

    /* Step 2: register local MppMeta objdef with correct size */
    if (!mpp_meta_def) {
        rk_s32 impl_size = sizeof(MppMetaVal) * meta_key_count;

        kmpp_objdef_register(&mpp_meta_def, sizeof(MppMetaPriv),
                             impl_size, "MppMeta", 0);
        if (mpp_meta_def) {
            kmpp_objdef_add_init(mpp_meta_def, mpp_meta_impl_init);
            kmpp_objdef_add_deinit(mpp_meta_def, mpp_meta_impl_deinit);
        }
    }

    /* Step 3: expand key→index mapping into objdef trie */
    meta_key_count = 0;
    META_ENTRY_TABLE(EXPAND_AS_TRIE)
    /* finalize objdef: NULL entry triggers mem_pool creation */
    kmpp_objdef_add_entry(mpp_meta_def, 0, NULL, NULL);

    user_data_index = get_index_of_key_f(KEY_USER_DATA, TYPE_UPTR);
    user_datas_index = get_index_of_key_f(KEY_USER_DATAS, TYPE_UPTR);
    meta_hdr_offset_index = get_index_of_key_f(KEY_HDR_META_OFFSET, TYPE_VAL_32);
    meta_hdr_size_index = get_index_of_key_f(KEY_HDR_META_SIZE, TYPE_VAL_32);

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
        MppMetaPriv *pos, *n;

        mpp_log_f("cleaning leaked metadata\n");

        list_for_each_entry_safe(pos, n, &srv->list_meta, MppMetaPriv, list_meta) {
            list_del_init(&pos->list_meta);
            if (pos->obj)
                kmpp_obj_put_f(pos->obj);
        }
    }

    mpp_assert(srv->meta_count == 0);

    MPP_FREE(srv_meta);

    if (mpp_meta_def) {
        kmpp_objdef_put(mpp_meta_def);
        mpp_meta_def = NULL;
    }

    srv_finalized = 1;

    meta_dbg_flow("meta srv deinited\n");
}

MPP_SINGLETON(MPP_SGLN_META, mpp_meta, mpp_meta_srv_init, mpp_meta_srv_deinit)

static inline RK_S32 get_index_of_key(MppMetaKey key, MppMetaType type, const char *caller)
{
    RK_U64 val = META_KEY_TO_U64(key, type);
    KmppEntry *tbl = NULL;

    (void)caller;

    if (kmpp_objdef_get_entry(mpp_meta_def, (const char *)&val, &tbl) == rk_ok && tbl)
        return (RK_S32)(tbl->tbl.elem_offset / sizeof(MppMetaVal));

    return -1;
}

static void *get_meta(const char *tag, const char *caller)
{
    KmppObj obj = NULL;
    rk_s32 ret;

    if (!mpp_meta_def) {
        mpp_err_f("local objdef not registered\n");
        return NULL;
    }

    ret = kmpp_obj_get(&obj, mpp_meta_def, caller);
    if (ret == rk_ok && obj) {
        MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv(obj);
        const char *tag_src = (tag) ? (tag) : (MODULE_TAG);

        snprintf(priv->tag, sizeof(priv->tag), "%s", tag_src);
        priv->caller = caller;
        /* meta_id / list_meta / ref_count / node_count / vals[] set by mpp_meta_impl_init */
    } else {
        mpp_err_f("failed to malloc meta data\n");
    }

    return obj;
}

static void clean_user_data(MppMetaPriv *priv)
{
    MPP_FREE(priv->user_data.pdata);
    priv->user_data.len = 0;
}

static void clean_user_datas(MppMetaPriv *priv)
{
    MPP_FREE(priv->user_data_set.datas);
    priv->user_data_set.count = 0;
    priv->datas_buf_size = 0;
}

static void put_meta(MppMetaSrv *srv, KmppObj obj)
{
    MppMetaPriv *priv;
    RK_S32 ref_count;

    if (!srv || !obj)
        return;

    priv = (MppMetaPriv *)kmpp_obj_to_priv(obj);
    ref_count = MPP_SUB_FETCH(&priv->ref_count, 1);
    if (ref_count > 0)
        return;

    if (ref_count < 0) {
        mpp_err_f("invalid negative ref_count %d\n", ref_count);
        return;
    }

    /* clean_user_data / list_del / meta_count-- done by mpp_meta_impl_deinit */
    kmpp_obj_put_f(obj);
}

MPP_RET mpp_meta_get_with_tag(MppMeta *meta, const char *tag, const char *caller)
{
    KmppObj obj;

    if (!meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    obj = (KmppObj)get_meta(tag, caller);
    *meta = (MppMeta)obj;
    return (obj) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_meta_put(MppMeta meta)
{
    if (!meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (kmpp_obj_is_kobj((KmppObj)meta))
        return kmpp_meta_put_f(meta);

    put_meta(get_srv_meta_f(), (KmppObj)meta);
    return MPP_OK;
}

MPP_RET mpp_meta_inc_ref(MppMeta meta)
{
    if (!meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (kmpp_obj_is_kobj((KmppObj)meta))
        return MPP_OK; /* kobj metas use kernel ref mechanism */

    {
        MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta);
        MPP_FETCH_ADD(&priv->ref_count, 1);
    }
    return MPP_OK;
}

RK_S32 mpp_meta_size(MppMeta meta)
{
    if (!meta) {
        mpp_err_f("found NULL input\n");
        return -1;
    }

    if (kmpp_obj_is_kobj((KmppObj)meta))
        return kmpp_meta_size_f(meta);

    {
        MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta);
        return MPP_FETCH_ADD(&priv->node_count, 0);
    }
}

static MPP_RET set_user_data(MppMetaPriv *priv, void *user_data)
{
    MppEncUserData *src = (MppEncUserData *)user_data;

    if (!src) {
        clean_user_data(priv);
        return MPP_OK;
    }

    if (!src->pdata || !src->len) {
        mpp_err_f("invalid user data %p pdata %p len %d\n", user_data, src->pdata, src->len);
        return MPP_ERR_NULL_PTR;
    }

    if (priv->user_data.len < src->len) {
        void *buf_ptr = mpp_realloc(priv->user_data.pdata, RK_U8, src->len);

        if (!buf_ptr) {
            mpp_err_f("failed to realloc user data buf size %d\n", src->len);
            priv->user_data.len = 0;
            return MPP_ERR_MALLOC;
        }
        priv->user_data.pdata = buf_ptr;
    }

    memcpy(priv->user_data.pdata, src->pdata, src->len);
    priv->user_data.len = src->len;

    return MPP_OK;
}

static MPP_RET set_user_datas(MppMetaPriv *priv, void *user_data)
{
    MppEncUserDataSet *src_set = (MppEncUserDataSet *)user_data;
    MppEncUserDataFull *dst_set = NULL;
    void *buf_ptr = NULL;
    RK_U32 data_size = 0;
    RK_U32 struct_size = 0;
    RK_U32 buf_size = 0;
    RK_U32 i = 0;

    if (!src_set) {
        clean_user_datas(priv);
        return MPP_OK;
    }

    if (!src_set->datas || !src_set->count) {
        mpp_err_f("invalid user data %p datas %p count %d\n", src_set, src_set->datas, src_set->count);
        return MPP_ERR_NULL_PTR;
    }

    struct_size = sizeof(MppEncUserDataFull) * src_set->count;
    for (i = 0; i < src_set->count; i++) {
        MppEncUserDataFull *src = &src_set->datas[i];

        if (src->uuid)
            data_size += MPP_ENC_USER_DATA_UUID_LEN;
        data_size += src->len;
    }
    buf_size = struct_size + data_size;

    if (priv->datas_buf_size < buf_size) {
        buf_ptr = mpp_realloc(priv->user_data_set.datas, RK_U8, buf_size);
        if (!buf_ptr) {
            mpp_err_f("failed to realloc user data buf size %d\n", buf_size);
            priv->user_data_set.count = 0;
            priv->datas_buf_size = 0;
            return MPP_ERR_MALLOC;
        }
        priv->user_data_set.datas = (MppEncUserDataFull *)buf_ptr;
    }

    priv->datas_buf_size = buf_size;
    dst_set = priv->user_data_set.datas;
    buf_ptr = (void *)dst_set + struct_size;

    for (i = 0; i < src_set->count; i++) {
        MppEncUserDataFull *src = &src_set->datas[i];
        MppEncUserDataFull *dst = &dst_set[i];

        dst->len = src->len;
        if (src->uuid) {
            dst->uuid = (RK_U8 *)buf_ptr;
            memcpy(buf_ptr, src->uuid, MPP_ENC_USER_DATA_UUID_LEN);
            buf_ptr += MPP_ENC_USER_DATA_UUID_LEN;
        } else {
            dst->uuid = NULL;
        }
        if (src->pdata) {
            dst->pdata = buf_ptr;
            memcpy(buf_ptr, src->pdata, src->len);
            buf_ptr += src->len;
        } else {
            dst->pdata = NULL;
        }
    }
    priv->user_data_set.count = src_set->count;

    return MPP_OK;
}

static MPP_RET get_user_data(MppMetaPriv *priv, void **val)
{
    if (priv->user_data.pdata) {
        *val = &priv->user_data;
        return MPP_OK;
    }

    *val = NULL;
    return MPP_NOK;
}

static MPP_RET get_user_datas(MppMetaPriv *priv, void **val)
{
    if (priv->user_data_set.datas) {
        *val = &priv->user_data_set;
        return MPP_OK;
    }

    *val = NULL;
    return MPP_NOK;
}

MppMeta mpp_meta_dup(MppMeta meta)
{
    if (!meta)
        return NULL;

    if (kmpp_obj_is_kobj((KmppObj)meta))
        return NULL; /* kobj metas: dup not supported, caller must handle NULL */

    {
        MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta);
        MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta);
        KmppObj ret_obj = (KmppObj)get_meta(priv->tag, __FUNCTION__);
        MppMetaPriv *ret_priv;
        MppMetaVal *ret_vals;

        if (!ret_obj)
            return NULL;

        ret_priv = (MppMetaPriv *)kmpp_obj_to_priv(ret_obj);
        ret_vals = (MppMetaVal *)kmpp_obj_to_entry(ret_obj);

        memcpy(ret_vals, vals, meta_key_count * sizeof(MppMetaVal));
        if (priv->user_data.len) {
            memset(&ret_priv->user_data, 0, sizeof(ret_priv->user_data));
            set_user_data(ret_priv, (void *)(intptr_t)&priv->user_data);
        }
        if (priv->user_data_set.count) {
            memset(&ret_priv->user_data_set, 0, sizeof(ret_priv->user_data_set));
            set_user_datas(ret_priv, (void *)(intptr_t)&priv->user_data_set);
        }
        ret_priv->node_count = priv->node_count;

        return (MppMeta)ret_obj;
    }
}

MPP_RET mpp_meta_dump(MppMeta meta)
{
    if (!meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (kmpp_obj_is_kobj((KmppObj)meta))
        return kmpp_obj_udump_f(meta, __FUNCTION__);

    {
        MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta);
        MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta);
        MppTrie trie;
        MppTrieInfo *root;

        mpp_logi("dumping meta %d node count %d\n", priv->meta_id, priv->node_count);

        trie = kmpp_objdef_get_trie(mpp_meta_def);
        if (!trie)
            return MPP_NOK;

        root = mpp_trie_get_info_first(trie);
        if (root) {
            MppTrieInfo *node = root;
            const char *key = NULL;
            char log_str[256];
            RK_S32 pos;

            do {
                KmppEntry *tbl;

                if (mpp_trie_info_is_self(node))
                    continue;

                key = mpp_trie_info_name(node);
                tbl = (KmppEntry *)mpp_trie_info_ctx(node);

                pos = snprintf(log_str, sizeof(log_str) - 1, "key %c%c%c%c - ",
                               key[0], key[1], key[2], key[3]);

                switch (key[4]) {
                case '3' : {
                    snprintf(log_str + pos, sizeof(log_str) - pos - 1, "s32 - %d",
                             vals[tbl->tbl.elem_offset / sizeof(MppMetaVal)].val_s32);
                } break;
                case '6' : {
                    snprintf(log_str + pos, sizeof(log_str) - pos - 1, "s64 - %lld",
                             vals[tbl->tbl.elem_offset / sizeof(MppMetaVal)].val_s64);
                } break;
                case 'k' :
                case 'u' :
                case 's' : {
                    snprintf(log_str + pos, sizeof(log_str) - pos - 1, "ptr - %p",
                             vals[tbl->tbl.elem_offset / sizeof(MppMetaVal)].val_ptr);
                } break;
                default : {
                } break;
                }

                mpp_logi("%s\n", log_str);
            } while ((node = mpp_trie_get_info_next(trie, node)));
        }

        return MPP_OK;
    }
}

/* MPP_META_ACCESSOR — generates set/get/get_d for s32 / s64 / ptr types.
 * frame / packet / buffer are handled separately because their kobj dispatch
 * target is kmpp_meta_set_obj / kmpp_meta_get_obj. */
#define MPP_META_ACCESSOR(func_type, arg_type, key_type, key_field)  \
    MPP_RET mpp_meta_set_##func_type(MppMeta meta, MppMetaKey key, arg_type val) \
    { \
        if (kmpp_obj_is_kobj((KmppObj)meta)) \
            return kmpp_meta_set_##func_type((KmppMeta)meta, key, val); \
        { \
            MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta); \
            MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta); \
            MppMetaVal *meta_val; \
            RK_S32 index; \
            if (!priv) { \
                mpp_err_f("found NULL input\n"); \
                return MPP_ERR_NULL_PTR; \
            } \
            index = get_index_of_key_f(key, key_type); \
            if (index < 0) \
                return MPP_NOK; \
            meta_val = &vals[index]; \
            if (MPP_BOOL_CAS(&meta_val->state, META_VAL_INVALID, META_VAL_VALID)) \
                MPP_FETCH_ADD(&priv->node_count, 1); \
            if (index == user_data_index) { \
                set_user_data(priv, (void *)(intptr_t)val); \
            } else if (index == user_datas_index) { \
                set_user_datas(priv, (void *)(intptr_t)val); \
            } else { \
                meta_val->key_field = val; \
            } \
            MPP_FETCH_OR(&meta_val->state, META_VAL_READY); \
            return MPP_OK; \
        } \
    } \
    MPP_RET mpp_meta_get_##func_type(MppMeta meta, MppMetaKey key, arg_type *val) \
    { \
        if (kmpp_obj_is_kobj((KmppObj)meta)) \
            return kmpp_meta_get_##func_type((KmppMeta)meta, key, val); \
        { \
            MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta); \
            MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta); \
            MppMetaVal *meta_val; \
            RK_S32 index; \
            MPP_RET ret = MPP_NOK; \
            if (!priv) { \
                mpp_err_f("found NULL input\n"); \
                return MPP_ERR_NULL_PTR; \
            } \
            index = get_index_of_key_f(key, key_type); \
            if (index < 0) \
                return MPP_NOK; \
            meta_val = &vals[index]; \
            if (MPP_BOOL_CAS(&meta_val->state, META_VAL_VALID | META_VAL_READY, META_VAL_INVALID)) { \
                if (index == user_data_index) \
                    get_user_data(priv, (void**)val); \
                else if (index == user_datas_index) \
                    get_user_datas(priv, (void**)val); \
                else \
                    *val = meta_val->key_field; \
                MPP_FETCH_SUB(&priv->node_count, 1); \
                ret = MPP_OK; \
            } \
            return ret; \
        } \
    } \
    MPP_RET mpp_meta_get_##func_type##_d(MppMeta meta, MppMetaKey key, arg_type *val, arg_type def) \
    { \
        if (kmpp_obj_is_kobj((KmppObj)meta)) \
            return kmpp_meta_get_##func_type##_d((KmppMeta)meta, key, val, def); \
        { \
            MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta); \
            MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta); \
            MppMetaVal *meta_val; \
            RK_S32 index; \
            MPP_RET ret = MPP_NOK; \
            if (!priv) { \
                mpp_err_f("found NULL input\n"); \
                return MPP_ERR_NULL_PTR; \
            } \
            index = get_index_of_key_f(key, key_type); \
            if (index < 0) \
                return MPP_NOK; \
            meta_val = &vals[index]; \
            if (MPP_BOOL_CAS(&meta_val->state, META_VAL_VALID | META_VAL_READY, META_VAL_INVALID)) { \
                if (index == user_data_index) \
                    get_user_data(priv, (void**)val); \
                else if (index == user_datas_index) \
                    get_user_datas(priv, (void**)val); \
                else \
                    *val = meta_val->key_field; \
                MPP_FETCH_SUB(&priv->node_count, 1); \
                ret = MPP_OK; \
            } else { \
                *val = def; \
            } \
            return ret; \
        } \
    }

MPP_META_ACCESSOR(s32, RK_S32, TYPE_VAL_32, val_s32)
MPP_META_ACCESSOR(s64, RK_S64, TYPE_VAL_64, val_s64)
MPP_META_ACCESSOR(ptr, void *, TYPE_UPTR, val_ptr)

/*
 * frame / packet / buffer accessors — dispatch to kmpp_meta_set_obj / get_obj
 * for kobj metas, and use the same local path (vals + priv) for local metas.
 */

#define MPP_META_ACCESSOR_OBJ(func_type, arg_type, key_type)  \
    MPP_RET mpp_meta_set_##func_type(MppMeta meta, MppMetaKey key, arg_type val) \
    { \
        if (kmpp_obj_is_kobj((KmppObj)meta)) \
            return kmpp_meta_set_obj((KmppMeta)meta, key, (KmppObj)val); \
        { \
            MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta); \
            MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta); \
            MppMetaVal *meta_val; \
            RK_S32 index; \
            if (!priv) { \
                mpp_err_f("found NULL input\n"); \
                return MPP_ERR_NULL_PTR; \
            } \
            index = get_index_of_key_f(key, key_type); \
            if (index < 0) \
                return MPP_NOK; \
            meta_val = &vals[index]; \
            if (MPP_BOOL_CAS(&meta_val->state, META_VAL_INVALID, META_VAL_VALID)) \
                MPP_FETCH_ADD(&priv->node_count, 1); \
            meta_val->val_ptr = val; \
            MPP_FETCH_OR(&meta_val->state, META_VAL_READY); \
            return MPP_OK; \
        } \
    } \
    MPP_RET mpp_meta_get_##func_type(MppMeta meta, MppMetaKey key, arg_type *val) \
    { \
        if (kmpp_obj_is_kobj((KmppObj)meta)) { \
            KmppObj obj = NULL; \
            rk_s32 ret = kmpp_meta_get_obj((KmppMeta)meta, key, &obj); \
            *val = (arg_type)obj; \
            return (ret == rk_ok) ? MPP_OK : MPP_NOK; \
        } \
        { \
            MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta); \
            MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta); \
            MppMetaVal *meta_val; \
            RK_S32 index; \
            MPP_RET ret = MPP_NOK; \
            if (!priv) { \
                mpp_err_f("found NULL input\n"); \
                return MPP_ERR_NULL_PTR; \
            } \
            index = get_index_of_key_f(key, key_type); \
            if (index < 0) \
                return MPP_NOK; \
            meta_val = &vals[index]; \
            if (MPP_BOOL_CAS(&meta_val->state, META_VAL_VALID | META_VAL_READY, META_VAL_INVALID)) { \
                *val = meta_val->val_ptr; \
                MPP_FETCH_SUB(&priv->node_count, 1); \
                ret = MPP_OK; \
            } \
            return ret; \
        } \
    } \
    MPP_RET mpp_meta_get_##func_type##_d(MppMeta meta, MppMetaKey key, arg_type *val, arg_type def) \
    { \
        if (kmpp_obj_is_kobj((KmppObj)meta)) { \
            KmppObj obj = NULL; \
            rk_s32 ret = kmpp_meta_get_obj_d((KmppMeta)meta, key, &obj, (KmppObj)def); \
            *val = (arg_type)obj; \
            return (ret == rk_ok) ? MPP_OK : MPP_NOK; \
        } \
        { \
            MppMetaPriv *priv = (MppMetaPriv *)kmpp_obj_to_priv((KmppObj)meta); \
            MppMetaVal *vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta); \
            MppMetaVal *meta_val; \
            RK_S32 index; \
            MPP_RET ret = MPP_NOK; \
            if (!priv) { \
                mpp_err_f("found NULL input\n"); \
                return MPP_ERR_NULL_PTR; \
            } \
            index = get_index_of_key_f(key, key_type); \
            if (index < 0) \
                return MPP_NOK; \
            meta_val = &vals[index]; \
            if (MPP_BOOL_CAS(&meta_val->state, META_VAL_VALID | META_VAL_READY, META_VAL_INVALID)) { \
                *val = meta_val->val_ptr; \
                MPP_FETCH_SUB(&priv->node_count, 1); \
                ret = MPP_OK; \
            } else { \
                *val = def; \
            } \
            return ret; \
        } \
    }

MPP_META_ACCESSOR_OBJ(frame, MppFrame, TYPE_SPTR)
MPP_META_ACCESSOR_OBJ(packet, MppPacket, TYPE_SPTR)
MPP_META_ACCESSOR_OBJ(buffer, MppBuffer, TYPE_SPTR)

RK_S32 mpp_meta_s32_read(MppMeta meta, RK_S32 index, RK_S32 *val)
{
    MppMetaVal *vals;
    MppMetaVal *meta_val;
    MPP_RET ret = MPP_NOK;

    if (!meta || index < 0 || (RK_U32)index >= meta_key_count) {
        mpp_err_f("found NULL input meta %p index %d\n", meta, index);
        return MPP_ERR_NULL_PTR;
    }

    if (kmpp_obj_is_kobj((KmppObj)meta))
        return MPP_NOK;

    vals = (MppMetaVal *)kmpp_obj_to_entry((KmppObj)meta);
    meta_val = &vals[index];
    if (meta_val->state == (META_VAL_VALID | META_VAL_READY)) {
        *val = meta_val->val_s32;
        ret = MPP_OK;
    }

    return ret;
}
