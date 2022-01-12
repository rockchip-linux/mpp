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

#define MODULE_TAG "mpp_meta"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_lock.h"

#include "mpp_meta_impl.h"

#define META_VAL_INVALID    (0x00000000)
#define META_VAL_VALID      (0x00000001)
#define META_VAL_READY      (0x00000002)

#define WRITE_ONCE(x, val)  ((*(volatile typeof(x) *) &(x)) = (val))
#define READ_ONCE(var)      (*((volatile typeof(var) *)(&(var))))

static MppMetaDef meta_defs[] = {
    /* categorized by type */
    /* data flow type */
    {   KEY_INPUT_FRAME,        TYPE_FRAME,     },
    {   KEY_OUTPUT_FRAME,       TYPE_FRAME,     },
    {   KEY_INPUT_PACKET,       TYPE_PACKET,    },
    {   KEY_OUTPUT_PACKET,      TYPE_PACKET,    },
    /* buffer for motion detection */
    {   KEY_MOTION_INFO,        TYPE_BUFFER,    },
    /* buffer storing the HDR information for current frame*/
    {   KEY_HDR_INFO,           TYPE_BUFFER,    },

    {   KEY_OUTPUT_INTRA,       TYPE_S32,       },
    {   KEY_INPUT_BLOCK,        TYPE_S32,       },
    {   KEY_OUTPUT_BLOCK,       TYPE_S32,       },

    /* extra information for tsvc */
    {   KEY_TEMPORAL_ID,        TYPE_S32,       },
    {   KEY_LONG_REF_IDX,       TYPE_S32,       },
    {   KEY_ENC_AVERAGE_QP,     TYPE_S32,       },

    {   KEY_ROI_DATA,           TYPE_PTR,       },
    {   KEY_ROI_DATA2,          TYPE_PTR,       },
    {   KEY_OSD_DATA,           TYPE_PTR,       },
    {   KEY_OSD_DATA2,          TYPE_PTR,       },
    {   KEY_USER_DATA,          TYPE_PTR,       },
    {   KEY_USER_DATAS,         TYPE_PTR,       },
    {   KEY_QPMAP0,             TYPE_BUFFER,    },
    {   KEY_MV_LIST,            TYPE_PTR,       },

    {   KEY_ENC_MARK_LTR,       TYPE_S32,       },
    {   KEY_ENC_USE_LTR,        TYPE_S32,       },
    {   KEY_ENC_FRAME_QP,       TYPE_S32,       },
    {   KEY_ENC_BASE_LAYER_PID, TYPE_S32,       },
};

class MppMetaService
{
private:
    // avoid any unwanted function
    MppMetaService();
    ~MppMetaService();
    MppMetaService(const MppMetaService &);
    MppMetaService &operator=(const MppMetaService &);

    spinlock_t          mLock;
    struct list_head    mlist_meta;

    RK_U32              meta_id;
    RK_S32              meta_count;
    RK_U32              finished;

public:
    static MppMetaService *get_inst() {
        static MppMetaService instance;
        return &instance;
    }

    /*
     * get_index_of_key does two things:
     * 1. Check the key / type pair is correct or not.
     *    If failed on check return negative value
     * 2. Compare all exsisting meta data defines to find the non-negative index
     */
    RK_S32 get_index_of_key(MppMetaKey key, MppMetaType type);

    MppMetaImpl  *get_meta(const char *tag, const char *caller);
    void          put_meta(MppMetaImpl *meta);
};

MppMetaService::MppMetaService()
    : meta_id(0),
      meta_count(0),
      finished(0)
{
    mpp_spinlock_init(&mLock);
    INIT_LIST_HEAD(&mlist_meta);
}

MppMetaService::~MppMetaService()
{
    if (!list_empty(&mlist_meta)) {
        MppMetaImpl *pos, *n;

        mpp_log_f("cleaning leaked metadata\n");

        list_for_each_entry_safe(pos, n, &mlist_meta, MppMetaImpl, list_meta) {
            put_meta(pos);
        }
    }

    mpp_assert(meta_count == 0);
    finished = 1;
}

RK_S32 MppMetaService::get_index_of_key(MppMetaKey key, MppMetaType type)
{
    RK_S32 i = 0;
    RK_S32 num = MPP_ARRAY_ELEMS(meta_defs);

    for (i = 0; i < num; i++) {
        if ((meta_defs[i].key == key) && (meta_defs[i].type == type))
            break;
    }

    return (i < num) ? (i) : (-1);
}

MppMetaImpl *MppMetaService::get_meta(const char *tag, const char *caller)
{
    MppMetaImpl *impl = mpp_malloc_size(MppMetaImpl, sizeof(MppMetaImpl) +
                                        sizeof(MppMetaVal) * MPP_ARRAY_ELEMS(meta_defs));
    if (impl) {
        const char *tag_src = (tag) ? (tag) : (MODULE_TAG);
        RK_U32 i;

        strncpy(impl->tag, tag_src, sizeof(impl->tag));
        impl->caller = caller;
        impl->meta_id = MPP_FETCH_ADD(&meta_id, 1);
        INIT_LIST_HEAD(&impl->list_meta);
        impl->ref_count = 1;
        impl->node_count = 0;

        for (i = 0; i < MPP_ARRAY_ELEMS(meta_defs); i++)
            impl->vals[i].state = 0;

        mpp_spinlock_lock(&mLock);
        list_add_tail(&impl->list_meta, &mlist_meta);
        mpp_spinlock_unlock(&mLock);
        MPP_FETCH_ADD(&meta_count, 1);
    } else {
        mpp_err_f("failed to malloc meta data\n");
    }
    return impl;
}

void MppMetaService::put_meta(MppMetaImpl *meta)
{
    if (finished)
        return ;

    RK_S32 ref_count = MPP_SUB_FETCH(&meta->ref_count, 1);

    if (ref_count > 0)
        return;

    if (ref_count < 0) {
        mpp_err_f("invalid negative ref_count %d\n", ref_count);
        return;
    }

    mpp_spinlock_lock(&mLock);
    list_del_init(&meta->list_meta);
    mpp_spinlock_unlock(&mLock);
    MPP_FETCH_SUB(&meta_count, 1);

    mpp_free(meta);
}

MPP_RET mpp_meta_get_with_tag(MppMeta *meta, const char *tag, const char *caller)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaService *service = MppMetaService::get_inst();
    MppMetaImpl *impl = service->get_meta(tag, caller);
    *meta = (MppMeta) impl;
    return (impl) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_meta_put(MppMeta meta)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaService *service = MppMetaService::get_inst();
    MppMetaImpl *impl = (MppMetaImpl *)meta;

    service->put_meta(impl);
    return MPP_OK;
}

MPP_RET mpp_meta_inc_ref(MppMeta meta)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;

    MPP_FETCH_ADD(&impl->ref_count, 1);
    return MPP_OK;
}

RK_S32 mpp_meta_size(MppMeta meta)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return -1;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;

    return MPP_FETCH_ADD(&impl->node_count, 0);
}

MPP_RET mpp_meta_dump(MppMeta meta)
{
    if (NULL == meta) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppMetaImpl *impl = (MppMetaImpl *)meta;
    RK_U32 i;

    mpp_log("dumping meta %d node count %d\n", impl->meta_id, impl->node_count);

    for (i = 0; i < MPP_ARRAY_ELEMS(meta_defs); i++) {
        if (!impl->vals[i].state)
            continue;

        const char *key = (const char *)&meta_defs[i].key;
        const char *type = (const char *)&meta_defs[i].type;

        mpp_log("key %c%c%c%c type %c%c%c%c\n",
                key[3], key[2], key[1], key[0],
                type[3], type[2], type[1], type[0]);
    }

    return MPP_OK;
}

#define MPP_META_ACCESSOR(func_type, arg_type, key_type, key_field)  \
    MPP_RET mpp_meta_set_##func_type(MppMeta meta, MppMetaKey key, arg_type val) \
    { \
        if (NULL == meta) { \
            mpp_err_f("found NULL input\n"); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppMetaService *service = MppMetaService::get_inst(); \
        RK_S32 index = service->get_index_of_key(key, key_type); \
        if (index < 0) \
            return MPP_NOK; \
        MppMetaImpl *impl = (MppMetaImpl *)meta; \
        MppMetaVal *meta_val = &impl->vals[index]; \
        if (MPP_BOOL_CAS(&meta_val->state, META_VAL_INVALID, META_VAL_VALID)) \
            MPP_FETCH_ADD(&impl->node_count, 1); \
        meta_val->key_field = val; \
        MPP_FETCH_OR(&meta_val->state, META_VAL_READY); \
        return MPP_OK; \
    } \
    MPP_RET mpp_meta_get_##func_type(MppMeta meta, MppMetaKey key, arg_type *val) \
    { \
        if (NULL == meta) { \
            mpp_err_f("found NULL input\n"); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppMetaService *service = MppMetaService::get_inst(); \
        RK_S32 index = service->get_index_of_key(key, key_type); \
        if (index < 0) \
            return MPP_NOK; \
        MppMetaImpl *impl = (MppMetaImpl *)meta; \
        MppMetaVal *meta_val = &impl->vals[index]; \
        MPP_RET ret = MPP_NOK; \
        if (MPP_BOOL_CAS(&meta_val->state, META_VAL_VALID | META_VAL_READY, META_VAL_INVALID)) { \
            *val = meta_val->key_field; \
            MPP_FETCH_SUB(&impl->node_count, 1); \
            ret = MPP_OK; \
        } \
        return ret; \
    }

MPP_META_ACCESSOR(s32, RK_S32, TYPE_S32, val_s32)
MPP_META_ACCESSOR(s64, RK_S64, TYPE_S64, val_s64)
MPP_META_ACCESSOR(ptr, void *, TYPE_PTR, val_ptr)
MPP_META_ACCESSOR(frame, MppFrame, TYPE_FRAME, frame)
MPP_META_ACCESSOR(packet, MppPacket, TYPE_PACKET, packet)
MPP_META_ACCESSOR(buffer, MppBuffer, TYPE_BUFFER, buffer)
