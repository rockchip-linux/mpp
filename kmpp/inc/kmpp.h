/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_H__
#define __KMPP_H__

#include <unistd.h>

#include "mpp_impl.h"
#include "mpp_dec_cfg.h"
#include "mpp_enc_cfg.h"
#include "mpp_mem_pool.h"
#include "kmpp_obj.h"

typedef struct Kmpp_t Kmpp;
typedef struct KmppOps_t {
    MPP_RET (*open_client)(Kmpp *ctx);
    MPP_RET (*init)(Kmpp *ctx, MppCtxType type, MppCodingType coding);

    MPP_RET (*start)(Kmpp *ctx);
    MPP_RET (*stop)(Kmpp *ctx);

    MPP_RET (*pause)(Kmpp *ctx);
    MPP_RET (*resume)(Kmpp *ctx);

    MPP_RET (*put_packet)(Kmpp *ctx, MppPacket packet);
    MPP_RET (*get_frame)(Kmpp *ctx, MppFrame *frame);

    MPP_RET (*put_frame)(Kmpp *ctx, MppFrame frame);
    MPP_RET (*get_packet)(Kmpp *ctx, MppPacket *packet);
    MPP_RET (*release_packet)(Kmpp *ctx, MppPacket *packet);

    MPP_RET (*poll)(Kmpp *ctx, MppPortType type, MppPollType timeout);
    MPP_RET (*dequeue)(Kmpp *ctx, MppPortType type, MppTask *task);
    MPP_RET (*enqueue)(Kmpp *ctx, MppPortType type, MppTask task);

    MPP_RET (*reset)(Kmpp *ctx);
    MPP_RET (*control)(Kmpp *ctx, MpiCmd cmd, MppParam param);

    MPP_RET (*notify_flag)(Kmpp *ctx, RK_U32 flag);
    MPP_RET (*notify)(Kmpp *ctx, MppBufferGroup group);

    MPP_RET (*get_fd)(Kmpp *ctx, RK_S32 *fd);
    MPP_RET (*close_fd)(Kmpp *ctx, RK_S32 fd);
    void    (*clear)(Kmpp *ctx);
} KmppOps;

struct Kmpp_t {
    MppPollType         mInputTimeout;
    MppPollType         mOutputTimeout;

    MppCtx              mCtx;
    RK_S32              mChanId;
    RK_U32              mEncVersion;

    MppCtxType          mType;
    MppCodingType       mCoding;
    RK_U32              mChanDup;

    RK_U32              mInitDone;

    RK_S32              mClientFd;
    struct timeval      mTimeout;

    MppBufferGroup      mPacketGroup;
    MppPacket           mPacket;

    KmppOps             *mApi;
    KmppObj             mVencInitKcfg;
    MppMemPool          mVencPacketPool;
};

#ifdef __cplusplus
extern "C" {
#endif

void mpp_get_api(Kmpp *ctx);

#ifdef __cplusplus
}
#endif
#endif /*__MPP_H__*/
