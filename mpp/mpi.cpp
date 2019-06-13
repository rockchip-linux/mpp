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

#define MODULE_TAG "mpi"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpi_impl.h"
#include "mpp_info.h"
#include "mpp_common.h"
#include "mpp_env.h"

RK_U32 mpi_debug = 0;

typedef struct {
    MppCtxType      type;
    MppCodingType   coding;
    const char      *type_name;
    const char      *coding_name;
} MppCodingTypeInfo;

static MppCodingTypeInfo support_list[] = {
#if HAVE_MPEG2D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingMPEG2,      "dec",  "mpeg2",        },
#endif
#if HAVE_MPEG4D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingMPEG4,      "dec",  "mpeg4",        },
#endif
#if HAVE_H263D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingH263,       "dec",  "h.263",        },
#endif
#if HAVE_H264D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingAVC,        "dec",  "h.264/AVC",    },
#endif
#if HAVE_H265D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingHEVC,       "dec",  "h.265/HEVC",   },
#endif
#if HAVE_VP8D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingVP8,        "dec",  "vp8",          },
#endif
#if HAVE_VP9D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingVP9,        "dec",  "VP9",          },
#endif
#if HAVE_AVSD
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingAVSPLUS,    "dec",  "avs+",         },
#endif
#if HAVE_JPEGD
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingMJPEG,      "dec",  "jpeg",         },
#endif
#if HAVE_H264E
    {   MPP_CTX_ENC,    MPP_VIDEO_CodingAVC,        "enc",  "h.264/AVC",    },
#endif
#if HAVE_JPEGE
    {   MPP_CTX_ENC,    MPP_VIDEO_CodingMJPEG,      "enc",  "jpeg",         },
#endif
#if HAVE_H265E
    {   MPP_CTX_ENC,    MPP_VIDEO_CodingHEVC,       "enc",  "h265",         },
#endif
#if HAVE_VP8E
    {   MPP_CTX_ENC,    MPP_VIDEO_CodingVP8,        "enc",  "vp8",          }
#endif
};

#define check_mpp_ctx(ctx)  _check_mpp_ctx(ctx, __FUNCTION__)

static MPP_RET _check_mpp_ctx(MpiImpl *p, const char *caller)
{
    if (NULL == p || p->check != p || NULL == p->ctx) {
        _mpp_err(MODULE_TAG, "found invalid context %p\n", caller, p);
        return MPP_ERR_UNKNOW;
    }
    return MPP_OK;
}

static MPP_RET mpi_decode(MppCtx ctx, MppPacket packet, MppFrame *frame)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p packet %p frame %p\n", ctx, packet, frame);
    do {
        RK_U32 packet_done = 0;
        Mpp *mpp = p->ctx;
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        if (NULL == frame || NULL == packet) {
            mpp_err_f("found NULL input packet %p frame %p\n", packet, frame);
            ret = MPP_ERR_NULL_PTR;
            break;
        }

        *frame = NULL;

        do {
            /*
             * If there is frame to return get the frame first
             * But if the output mode is block then we need to send packet first
             */
            if (!mpp->mOutputTimeout || packet_done) {
                ret = mpp->get_frame(frame);
                if (ret || *frame)
                    break;
            }

            /* when packet is send do one more get frame here */
            if (packet_done)
                break;

            /*
             * then send input stream with timeout mode
             */
            ret = mpp->put_packet(packet);
            if (MPP_OK == ret)
                packet_done = 1;
        } while (1);
    } while (0);

    mpp_assert(0 == mpp_packet_get_length(packet));

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_decode_put_packet(MppCtx ctx, MppPacket packet)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p packet %p\n", ctx, packet);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        if (NULL == packet) {
            mpp_err_f("found NULL input packet\n");
            ret = MPP_ERR_NULL_PTR;
            break;
        }

        ret = p->ctx->put_packet(packet);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_decode_get_frame(MppCtx ctx, MppFrame *frame)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p frame %p\n", ctx, frame);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        if (NULL == frame) {
            mpp_err_f("found NULL input frame\n");
            ret = MPP_ERR_NULL_PTR;
            break;
        }

        ret = p->ctx->get_frame(frame);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_encode(MppCtx ctx, MppFrame frame, MppPacket *packet)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p frame %p packet %p\n", ctx, frame, packet);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        if (NULL == frame || NULL == packet) {
            mpp_err_f("found NULL input frame %p packet %p\n", frame, packet);
            ret = MPP_ERR_NULL_PTR;
            break;
        }

        // TODO: do encode here
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_encode_put_frame(MppCtx ctx, MppFrame frame)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p frame %p\n", ctx, frame);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        if (NULL == frame) {
            mpp_err_f("found NULL input frame\n");
            ret = MPP_ERR_NULL_PTR;
            break;
        }

        ret = p->ctx->put_frame(frame);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_encode_get_packet(MppCtx ctx, MppPacket *packet)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p packet %p\n", ctx, packet);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        if (NULL == packet) {
            mpp_err_f("found NULL input packet\n");
            ret = MPP_ERR_NULL_PTR;
            break;
        }

        ret = p->ctx->get_packet(packet);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_isp(MppCtx ctx, MppFrame dst, MppFrame src)
{
    MPP_RET ret = MPP_OK;
    mpi_dbg_func("enter ctx %p dst %p src %p\n", ctx, dst, src);

    // TODO: do isp process here

    mpi_dbg_func("leave ret %d\n", ret);
    return MPP_OK;
}

static MPP_RET mpi_isp_put_frame(MppCtx ctx, MppFrame frame)
{
    MPP_RET ret = MPP_OK;
    mpi_dbg_func("enter ctx %p frame %p\n", ctx, frame);

    // TODO: do isp put frame process here

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_isp_get_frame(MppCtx ctx, MppFrame *frame)
{
    MPP_RET ret = MPP_OK;
    mpi_dbg_func("enter ctx %p frame %p\n", ctx, frame);

    // TODO: do isp get frame process here

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_poll(MppCtx ctx, MppPortType type, MppPollType timeout)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p type %d timeout %d\n", ctx, type, timeout);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;;

        if (type >= MPP_PORT_BUTT ||
            timeout < MPP_POLL_BUTT ||
            timeout > MPP_POLL_MAX) {
            mpp_err_f("invalid input type %d timeout %d\n", type, timeout);
            ret = MPP_ERR_UNKNOW;
            break;
        }

        ret = p->ctx->poll(type, timeout);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_dequeue(MppCtx ctx, MppPortType type, MppTask *task)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p type %d task %p\n", ctx, type, task);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;;

        if (type >= MPP_PORT_BUTT || NULL == task) {
            mpp_err_f("invalid input type %d task %p\n", type, task);
            ret = MPP_ERR_UNKNOW;
            break;
        }

        ret = p->ctx->dequeue(type, task);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_enqueue(MppCtx ctx, MppPortType type, MppTask task)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p type %d task %p\n", ctx, type, task);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;;

        if (type >= MPP_PORT_BUTT || NULL == task) {
            mpp_err_f("invalid input type %d task %p\n", type, task);
            ret = MPP_ERR_UNKNOW;
            break;
        }

        ret = p->ctx->enqueue(type, task);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_reset(MppCtx ctx)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p\n", ctx);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;;

        ret = p->ctx->reset();
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MPP_RET mpi_control(MppCtx ctx, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;
    MpiImpl *p = (MpiImpl *)ctx;

    mpi_dbg_func("enter ctx %p cmd %x parm %p\n", ctx, cmd, param);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;;

        ret = p->ctx->control(cmd, param);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

static MppApi mpp_api = {
    sizeof(mpp_api),
    0,
    mpi_decode,
    mpi_decode_put_packet,
    mpi_decode_get_frame,
    mpi_encode,
    mpi_encode_put_frame,
    mpi_encode_get_packet,
    mpi_isp,
    mpi_isp_put_frame,
    mpi_isp_get_frame,
    mpi_poll,
    mpi_dequeue,
    mpi_enqueue,
    mpi_reset,
    mpi_control,
    {0},
};

MPP_RET mpp_create(MppCtx *ctx, MppApi **mpi)
{
    mpp_env_get_u32("mpi_debug", &mpi_debug, 0);

    if (NULL == ctx || NULL == mpi) {
        mpp_err_f("invalid input ctx %p mpi %p\n", ctx, mpi);
        return MPP_ERR_NULL_PTR;
    }

    *ctx = NULL;
    *mpi = NULL;

    MPP_RET ret = MPP_OK;
    mpi_dbg_func("enter ctx %p mpi %p\n", ctx, mpi);
    do {
        MpiImpl *p = mpp_malloc(MpiImpl, 1);
        if (NULL == p) {
            mpp_err_f("failed to allocate context\n");
            ret = MPP_ERR_MALLOC;
            break;
        }

        memset(p, 0, sizeof(*p));
        p->ctx = new Mpp();
        if (NULL == p->ctx) {
            mpp_free(p);
            mpp_err_f("failed to new Mpp\n");
            ret = MPP_ERR_MALLOC;
            break;
        }

        mpp_api.version = mpp_info_get_revision();
        p->api      = &mpp_api;
        p->check    = p;
        *ctx = p;
        *mpi = p->api;
    } while (0);

    mpp_log("mpp version: %s\n", mpp_info_get(INFO_ALL));

    mpi_dbg_func("leave ret %d ctx %p mpi %p\n", ret, *ctx, *mpi);
    return ret;
}

MPP_RET mpp_init(MppCtx ctx, MppCtxType type, MppCodingType coding)
{
    MPP_RET ret = MPP_OK;
    MpiImpl *p = (MpiImpl*)ctx;

    mpi_dbg_func("enter ctx %p type %d coding %d\n", ctx, type, coding);
    do {
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        if (type >= MPP_CTX_BUTT ||
            coding >= MPP_VIDEO_CodingMax) {
            mpp_err_f("invalid input type %d coding %d\n", type, coding);
            ret = MPP_ERR_UNKNOW;
            break;
        }

        ret = p->ctx->init(type, coding);
        p->type     = type;
        p->coding   = coding;
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

MPP_RET mpp_destroy(MppCtx ctx)
{
    mpi_dbg_func("enter ctx %p\n", ctx);

    MPP_RET ret = MPP_OK;
    MpiImpl *p = (MpiImpl*)ctx;

    do {
        ret = check_mpp_ctx(p);
        if (ret)
            return ret;

        if (p->ctx)
            delete p->ctx;

        mpp_free(p);
    } while (0);

    mpi_dbg_func("leave ret %d\n", ret);
    return ret;
}

MPP_RET mpp_check_support_format(MppCtxType type, MppCodingType coding)
{
    MPP_RET ret = MPP_NOK;
    RK_U32 i = 0;

    for (i = 0; i < MPP_ARRAY_ELEMS(support_list); i++) {
        MppCodingTypeInfo *info = &support_list[i];
        if (type    == info->type &&
            coding  == info->coding) {
            ret = MPP_OK;
            break;
        }
    }
    return ret;
}

void mpp_show_support_format()
{
    RK_U32 i = 0;

    mpp_log("mpp coding type support list:");

    for (i = 0; i < MPP_ARRAY_ELEMS(support_list); i++) {
        MppCodingTypeInfo *info = &support_list[i];
        mpp_log("type: %s id %d coding: %-16s id %d\n",
                info->type_name, info->type,
                info->coding_name, info->coding);
    }
}

