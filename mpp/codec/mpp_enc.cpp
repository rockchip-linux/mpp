/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define  MODULE_TAG "mpp_enc"

#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp.h"
#include "mpp_enc.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

void *mpp_enc_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *thd_enc  = mpp->mTheadCodec;
    mpp_list *packets = mpp->mPackets;
    mpp_list *frames  = mpp->mFrames;
    MppFrameImpl frame;
    MppPacket packet;
    size_t size = SZ_1M;
    char *buf = mpp_malloc(char, size);

    while (MPP_THREAD_RUNNING == thd_enc->get_status()) {
        if (frames->list_size()) {
            frames->del_at_head(&frame, sizeof(frame));

            mpp_packet_init(&packet, buf, size);
            packets->add_at_tail(&packet, sizeof(packet));
        }
    }
    mpp_free(buf);
    return NULL;
}

