/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpi"

#include <string.h>

#include "rk_mpi.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpi_impl.h"
#include "mpp_info.h"

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
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingAVS,        "dec",  "avs",          },
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingAVSPLUS,    "dec",  "avs+",         },
#endif
#if HAVE_AVS2D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingAVS2,       "dec",  "avs2",         },
#endif
#if HAVE_JPEGD
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingMJPEG,      "dec",  "jpeg",         },
#endif
#if HAVE_AV1D
    {   MPP_CTX_DEC,    MPP_VIDEO_CodingAV1,        "dec",  "av1",          },
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
        ret = check_mpp_ctx(p);
        if (ret)
            break;

        /*
         * NOTE: packet and frame could be NULL
         * If packet is NULL then it is equal to get_frame
         * If frame is NULL then it is equal to put_packet
         */
        if (frame)
            *frame = NULL;

        ret = mpp_decode(p->ctx, packet, frame);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_put_packet(p->ctx, packet);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_get_frame(p->ctx, frame);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_put_frame(p->ctx, frame);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_get_packet(p->ctx, packet);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
    return ret;
}

static MPP_RET mpi_isp(MppCtx ctx, MppFrame dst, MppFrame src)
{
    MPP_RET ret = MPP_OK;
    mpi_dbg_func("enter ctx %p dst %p src %p\n", ctx, dst, src);

    // TODO: do isp process here

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
    return MPP_OK;
}

static MPP_RET mpi_isp_put_frame(MppCtx ctx, MppFrame frame)
{
    MPP_RET ret = MPP_OK;
    mpi_dbg_func("enter ctx %p frame %p\n", ctx, frame);

    // TODO: do isp put frame process here

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_poll(p->ctx, type, timeout);
        if (ret > 0)
            ret = MPP_OK;
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_dequeue(p->ctx, type, task);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_enqueue(p->ctx, type, task);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_reset(p->ctx);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

        ret = mpp_control(p->ctx, cmd, param);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
    return ret;
}

static MppApi mpp_api = {
    .size              = sizeof(mpp_api),
    .version           = 0,
    .decode            = mpi_decode,
    .decode_put_packet = mpi_decode_put_packet,
    .decode_get_frame  = mpi_decode_get_frame,
    .encode            = mpi_encode,
    .encode_put_frame  = mpi_encode_put_frame,
    .encode_get_packet = mpi_encode_get_packet,
    .isp               = mpi_isp,
    .isp_put_frame     = mpi_isp_put_frame,
    .isp_get_frame     = mpi_isp_get_frame,
    .poll              = mpi_poll,
    .dequeue           = mpi_dequeue,
    .enqueue           = mpi_enqueue,
    .reset             = mpi_reset,
    .control           = mpi_control,
    .reserv            = {0},
};

MPP_RET mpp_create(MppCtx *ctx, MppApi **mpi)
{
    mpp_env_get_u32("mpi_debug", &mpi_debug, 0);
    mpp_get_log_level();

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
        ret = mpp_ctx_create(&p->ctx, p);
        if (ret || NULL == p->ctx) {
            mpp_free(p);
            mpp_err_f("failed to create Mpp context ret %d\n", ret);
            ret = MPP_ERR_MALLOC;
            break;
        }

        mpp_api.version = 0;
        p->api      = &mpp_api;
        p->check    = p;
        *ctx = p;
        *mpi = p->api;
    } while (0);

    show_mpp_version();

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

        ret = mpp_ctx_init(p->ctx, type, coding);
        p->type     = type;
        p->coding   = coding;
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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
            mpp_ctx_destroy(p->ctx);

        mpp_free(p);
    } while (0);

    mpi_dbg_func("leave ctx %p ret %d\n", ctx, ret);
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

void mpp_show_support_format(void)
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

typedef struct {
    MppFrameFormat  format;
    const char      *name;
} MppFrameFormatInfo;

static MppFrameFormatInfo color_list[] = {
    { MPP_FMT_YUV420SP,         "YUV420SP,      NV12"   },
    { MPP_FMT_YUV420SP_10BIT,   "YUV420SP-10bit"        },
    { MPP_FMT_YUV422SP,         "YUV422SP,      NV24"   },
    { MPP_FMT_YUV422SP_10BIT,   "YUV422SP-10bit"        },
    { MPP_FMT_YUV420P,          "YUV420P,       I420"   },
    { MPP_FMT_YUV420SP_VU,      "YUV420SP,      NV21"   },
    { MPP_FMT_YUV422P,          "YUV422P,       422P"   },
    { MPP_FMT_YUV422SP_VU,      "YUV422SP,      NV42"   },
    { MPP_FMT_YUV422_YUYV,      "YUV422-YUYV,   YUY2"   },
    { MPP_FMT_YUV422_UYVY,      "YUV422-UYVY,   UYVY"   },
    { MPP_FMT_YUV400,           "YUV400-Y8,     Y800"   },
    { MPP_FMT_YUV444SP,         "YUV444SP"              },
    { MPP_FMT_YUV444P,          "YUV444P"               },

    { MPP_FMT_RGB565,           "RGB565"                },
    { MPP_FMT_BGR565,           "BGR565"                },
    { MPP_FMT_RGB555,           "RGB555"                },
    { MPP_FMT_BGR555,           "BGR555"                },
    { MPP_FMT_RGB888,           "RGB888"                },
    { MPP_FMT_BGR888,           "BGR888"                },

    { MPP_FMT_ARGB8888,         "ARGB8888"              },
    { MPP_FMT_ABGR8888,         "ABGR8888"              },
    { MPP_FMT_BGRA8888,         "BGRA8888"              },
    { MPP_FMT_RGBA8888,         "RGBA8888"              },
};

void mpp_show_color_format(void)
{
    RK_U32 i = 0;

    mpp_log("mpp color support list:");

    for (i = 0; i < MPP_ARRAY_ELEMS(color_list); i++) {
        MppFrameFormatInfo *info = &color_list[i];
        mpp_log("color: id %-5d 0x%05x %s\n",
                info->format, info->format, info->name);
    }
}
